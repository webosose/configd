// Copyright (c) 2014-2023 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <semaphore.h>

#include "libconfigd.h"
#include "Logging.h"

#define GETCONFIGS_METHOD "palm://com.webos.service.config/getConfigs"
#define SETCONFIGS_METHOD "palm://com.webos.service.config/setConfigs"
#define RECONFIGS_METHOD "palm://com.webos.service.config/reconfigure"

static bool config_setLSHandle(LSHandle *lsHandle);
static LSHandle *config_getLSHandle();

static bool config_cbWatchConfigs(LSHandle *lsHandle, LSMessage *message, void *userData);
static bool config_cbSetConfigs(LSHandle *lsHandle, LSMessage *message, void *userData);
static bool config_cbReconfigs(LSHandle *lsHandle, LSMessage *message, void *userData);
static bool configd_getConfigsUnsafe(LSHandle *lsHandle, jvalue_ref queryObject, int32_t *errorCode);
static bool config_cbReLoadConfigs(LSHandle *lsHandle, LSMessage *message, void *userData);
static bool config_cbResultConfigs(LSHandle *lsHandle, LSMessage *message, void *userData);
static void config_evaluateMissingConfigsSafe();

/**
 * usage example
 *  {
 *      CONFIG_COND_WAIT(&gcond, &gmutex)
 *      do something
 *      g_mutex_unlock(&gmutex);
 *  }
 */
#define CONFIG_COND_WAIT(cond, lock)                     \
    g_mutex_lock(lock);                                  \
    while (configUpdateWaitingLsCount)                   \
    {                                                    \
        g_cond_wait(cond, lock);                         \
    }

/* __J_ARRAY is a valid jvalue_ref object of type array*/
#define C_ARRAY_TO_J_ARRAY(__J_ARRAY, __C_ARRAY)                        \
    {                                                                   \
        int32_t __i = 0;                                                \
        while(__C_ARRAY[__i] && strcmp(__C_ARRAY[__i], ""))             \
        {                                                               \
            jarray_append(__J_ARRAY, jstring_create(__C_ARRAY[__i]));   \
            ++__i;                                                      \
        }                                                               \
    }

PmLogContext confdLibContext; // For Logging.h keep variable name as is
static jvalue_ref configQueryNames = NULL;
static jvalue_ref configCacheObj = NULL;
static jvalue_ref configMissingConfigs = NULL;
static GMutex configCacheLock;
static GCond configCacheCond;
static int32_t configUpdateWaitingLsCount = 0;
static bool configReLoadDone = false;
static GHashTable *configWatchers = NULL;
static LSHandle *configLSHandle = NULL;

static bool config_setLSHandle(LSHandle *lsHandle)
{
    // Use internally store LSHandle
    if (NULL != lsHandle)
        configLSHandle = lsHandle;

    return (NULL != configLSHandle);
}

static LSHandle *config_getLSHandle()
{
    return configLSHandle;
}

static bool config_cbWatchConfigs(LSHandle *lsHandle, LSMessage *message, void *userData)
{
    JSchemaInfo schemaInfo;
    jvalue_ref replyObj = NULL;
    jvalue_ref configsObj = NULL; // No ownership
    jvalue_ref missingConfigsObj = NULL; // No ownership
    jvalue_ref queryObject = (jvalue_ref) userData;

    g_mutex_lock(&configCacheLock);

    const char *method = LSMessageGetMethod(message);

    if (!g_strcmp0(method, LUNABUS_ERROR_SERVICE_DOWN))
    {
        LOG_LIB_INFO_PAIRS(MSGID_LIBCONFIGD, 0, "Service Disconnected.");

        if (configd_getConfigsUnsafe(lsHandle, queryObject, NULL))
        {
            LOG_LIB_INFO_PAIRS(MSGID_LIBCONFIGD, 0, "Service Re-connected.");
        }

        g_mutex_unlock(&configCacheLock);
        return true;
    }

    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    replyObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);


    if (jobject_get_exists(replyObj, J_CSTR_TO_BUF("configs"), &configsObj))
    {
        if (NULL == configCacheObj)
        {
            configCacheObj = jvalue_duplicate(configsObj);
        }
        else
        {
            //update the current cache with the new updated values
            // adding one by one for subscription reply and multiple Config_loadConfigs support.
            jobject_iter it;
            jobject_key_value keyValPair;
            jobject_iter_init(&it, configsObj);

            while (jobject_iter_next(&it, &keyValPair))
            {
                jobject_put(configCacheObj, jvalue_copy(keyValPair.key), jvalue_copy(keyValPair.value));
            }
        }
    }

    // remove from cache if any config is been deleted.
    if ((NULL != configCacheObj) && jobject_get_exists(replyObj, J_CSTR_TO_BUF("missingConfigs"), &missingConfigsObj))
    {
        raw_buffer missingConfigBuf;
        int32_t index = 0;

        for (index = 0; index < jarray_size(missingConfigsObj); index++)
        {
            missingConfigBuf = jstring_get(jarray_get(missingConfigsObj, index));

            if (missingConfigBuf.m_str)
            {
                jobject_remove(configCacheObj, missingConfigBuf);
                jstring_free_buffer(missingConfigBuf);
            }
        }
    }

    config_evaluateMissingConfigsSafe();

    if (configUpdateWaitingLsCount)
    {
        --configUpdateWaitingLsCount;
    }

    // wake up all pending clients
    g_cond_broadcast(&configCacheCond);
    g_mutex_unlock(&configCacheLock);

    // notify to all watchers
    // To let callback functions to call config_getXXX(), it's important to unlock first.
    if (configWatchers)
    {
        GHashTableIter iter;
        gpointer key, value;
        g_hash_table_iter_init(&iter, configWatchers);
        while (g_hash_table_iter_next(&iter, &key, &value))
        {
            if (key)
            {
                configdNotify  notify = (configdNotify)key;
                notify(configsObj, value);
            }
        }
    }

    j_release(&replyObj);
    return true;
}

static bool config_cbSetConfigs(LSHandle *lsHandle, LSMessage *message, void *userData)
{
    JSchemaInfo schemaInfo;
    jvalue_ref replyObj = NULL;
    jvalue_ref resultObj = NULL;
    bool result = false;
    jvalue_ref backupConfigObj = (jvalue_ref) userData;

    g_mutex_lock(&configCacheLock);

    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    replyObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);

    if (jobject_get_exists(replyObj, J_CSTR_TO_BUF("returnValue"), &resultObj))
    {
        if (CONV_OK == jboolean_get(resultObj, &result))
        {
            result = true;
        }

    }

    if (result)
    {
        LOG_LIB_INFO_PAIRS(MSGID_LIBCONFIGD, 0, "setConfigs is requested successfully");
        j_release(&backupConfigObj);
    }
    else
    {
        LOG_LIB_INFO_PAIRS(MSGID_LIBCONFIGD, 0, "setConfigs request is failed");

        if (!jis_null(backupConfigObj))
        {
            j_release(&configCacheObj);
            configCacheObj = backupConfigObj;
        }
    }

    j_release(&replyObj);
    g_mutex_unlock(&configCacheLock);

    return result;
}

static bool config_cbReconfigs(LSHandle *lsHandle, LSMessage *message, void *userData)
{
    JSchemaInfo schemaInfo;
    jvalue_ref replyObj = NULL;
    jvalue_ref resultObj = NULL;
    bool result = false;

    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    replyObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);

    if (jobject_get_exists(replyObj, J_CSTR_TO_BUF("returnValue"), &resultObj))
    {
        if (CONV_OK == jboolean_get(resultObj, &result))
        {
            result = true;
        }

    }

    if (result)
    {
        LOG_LIB_INFO_PAIRS(MSGID_LIBCONFIGD, 0, "setConfigs is requested successfully");
    }
    else
    {
        LOG_LIB_INFO_PAIRS(MSGID_LIBCONFIGD, 0, "setConfigs request is failed");
    }

    j_release(&replyObj);

    return result;
}

//Prefetch the requested configs
bool config_loadConfigs(LSHandle *lsHandle, char *configNames[], int32_t *errorCode)
{
    jvalue_ref jArrayConfigNames = NULL;
    jvalue_ref queryObject = NULL;
    bool retVal = false;

    PmLogGetContext("libconfigd", &confdLibContext);

    // sanity check for input arguments
    if (!config_setLSHandle(lsHandle) || (NULL == configNames))
    {
        if (NULL != errorCode)
        {
            *errorCode = CONFIG_INVALID_ARGUMENTS;
        }

        return false;
    }

    g_mutex_lock(&configCacheLock);
    assert(!configQueryNames);

    queryObject = jobject_create();

    if (jis_null(queryObject))
    {
        if (NULL != errorCode)
        {
            *errorCode = CONFIG_MEMORY_ALLOCATION_ERROR;
        }

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    jArrayConfigNames = jarray_create(NULL);

    if (jis_null(jArrayConfigNames))
    {
        if (NULL != errorCode)
        {
            *errorCode = CONFIG_MEMORY_ALLOCATION_ERROR;
        }

        j_release(&jArrayConfigNames);
        g_mutex_unlock(&configCacheLock);
        return false;
    }

    C_ARRAY_TO_J_ARRAY(jArrayConfigNames, configNames);
    jobject_put(queryObject, jstring_create("configNames"), jArrayConfigNames);
    jobject_put(queryObject, jstring_create("subscribe"), jboolean_create(true));

    if ((retVal = configd_getConfigsUnsafe(config_getLSHandle(), jvalue_duplicate(queryObject), errorCode)))
    {
        configQueryNames = jvalue_duplicate(jArrayConfigNames);
    }

    j_release(&queryObject);
    g_mutex_unlock(&configCacheLock);

    return retVal;
}

static bool configd_getConfigsUnsafe(LSHandle *lsHandle, jvalue_ref queryObject, int32_t *errorCode)
{
    if (LSCall(lsHandle, GETCONFIGS_METHOD, jvalue_tostring_simple(queryObject), config_cbWatchConfigs, queryObject, NULL,
               NULL))
    {
        configUpdateWaitingLsCount++;

        if (!LSCall(config_getLSHandle(), "palm://com.palm.bus/signal/addmatch",
                "{\"category\":\"/com/webos/config\", \"method\":\"reloadDone\"}",
                config_cbReLoadConfigs, NULL, NULL, NULL))
        {
            LOG_LIB_INFO_PAIRS(MSGID_LIBCONFIGD, 0, "Failed to Register signal callback");
        }

        if (NULL != errorCode)
        {
            *errorCode = CONFIG_NO_ERROR;
        }

        return true;
    }

    if (NULL != errorCode)
    {
        *errorCode = CONFIG_LSCALL_FAILURE;
    }

    j_release(&queryObject);

    return false;
}

static bool config_cbReLoadConfigs(LSHandle *lsHandle, LSMessage *message, void *userData)
{
    JSchemaInfo schemaInfo;
    jvalue_ref replyObj = NULL;
    jvalue_ref resultObj = NULL;
    bool result = true;

    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    replyObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);

    // Ignore basic reponse from bus system.
    // luna-bus return with payload {"returnValue":true} for first registeration
    if (jobject_get_exists(replyObj, J_CSTR_TO_BUF("returnValue"), &resultObj))
    {
        result = false;
    }
    j_release(&replyObj);

    if (!result) return result;

    g_mutex_lock(&configCacheLock);

    LOG_LIB_INFO_PAIRS(MSGID_LIBCONFIGD, 0, "Re-Load Configs Done");
    configReLoadDone = true;

    // wake up all pending clients
    g_cond_broadcast(&configCacheCond);
    g_mutex_unlock(&configCacheLock);

    return result;
}

static bool config_cbResultConfigs(LSHandle *lsHandle, LSMessage *message, void *userData)
{
    JSchemaInfo schemaInfo;
    jvalue_ref replyObj = NULL;
    jvalue_ref configsObj = NULL; // No ownership

    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    replyObj = jdom_parse(j_cstr_to_buffer(LSMessageGetPayload(message)), DOMOPT_NOOPT, &schemaInfo);

    if (jobject_get_exists(replyObj, J_CSTR_TO_BUF("configs"), &configsObj))
    {
        configdNotify cbFunc = (configdNotify)userData;
        cbFunc(jvalue_duplicate(configsObj), NULL);
    }

    j_release(&replyObj);

    return true;
}

void config_addWatch(configdNotify func, void *watch_context)
{
    g_mutex_lock(&configCacheLock);

    if (!configWatchers)
    {
        configWatchers = g_hash_table_new(g_direct_hash, g_direct_equal);
    }

    g_hash_table_insert(configWatchers, func, watch_context);

    g_mutex_unlock(&configCacheLock);
}

bool config_removeWatch(configdNotify func)
{
    bool retVal = false;
    g_mutex_lock(&configCacheLock);

    if (configWatchers)
    {
        retVal = g_hash_table_remove(configWatchers, func);
    }

    g_mutex_unlock(&configCacheLock);
    return retVal;
}

jvalue_ref config_getReqConfigNamesArray()
{
    jvalue_ref namesArray = jnull();

    g_mutex_lock(&configCacheLock);

    if (configQueryNames && jis_valid(configQueryNames) && jis_array(configQueryNames))
    {
        namesArray = jvalue_duplicate(configQueryNames);
    }

    g_mutex_unlock(&configCacheLock);

    return namesArray;
}

jvalue_ref config_getAllConfigs()
{
    jvalue_ref allConfigs = jnull();

    CONFIG_COND_WAIT(&configCacheCond, &configCacheLock);

    if (configCacheObj && jis_valid(configCacheObj) && jis_object(configCacheObj))
    {
        allConfigs = jvalue_duplicate(configCacheObj);
    }

    g_mutex_unlock(&configCacheLock);

    return allConfigs;
}

jvalue_ref config_getMissingConfigs()
{
    jvalue_ref missingConfigs = jnull();

    CONFIG_COND_WAIT(&configCacheCond, &configCacheLock);

    if (jis_valid(configMissingConfigs) && jis_array(configMissingConfigs))
    {
        missingConfigs = jvalue_duplicate(configMissingConfigs);
    }

    g_mutex_unlock(&configCacheLock);

    return missingConfigs;
}

static void config_evaluateMissingConfigsSafe()
{
    if (!jis_null(configQueryNames))
    {
        if (!jis_null(configMissingConfigs))
        {
            j_release(&configMissingConfigs);
            configMissingConfigs = jnull();
        }

        if (!jis_null(configCacheObj))
        {
            jvalue_ref missingFromRequested = jarray_create(NULL);
            for (int index = 0; index < jarray_size(configQueryNames); index++)
            {
                jvalue_ref configName = jarray_get(configQueryNames, index);
                raw_buffer configNameBuf = jstring_get_fast(configName);
                if (jis_string(configName)
                    && configNameBuf.m_str
                    && configNameBuf.m_str[configNameBuf.m_len - 1] != CONFIG_WILDCARD)
                {
                    if (!jobject_containskey(configCacheObj, configNameBuf))
                    {
                        jarray_append(missingFromRequested, jvalue_copy(configName));
                    }
                }
            }

            if (jarray_size(missingFromRequested) > 0)
            {
                configMissingConfigs = missingFromRequested;
            }
            else
            {
                j_release(&missingFromRequested);
            }
        }
        else
        {   // first getConfigs reply is not received
            configMissingConfigs = jvalue_duplicate(configQueryNames);
        }
    }
}

bool config_waitConfigs(int64_t timeoutMilli)
{
    assert(timeoutMilli>0);
    assert(jarray_size(configQueryNames)>0);

    gint64 tsBegin = g_get_monotonic_time();
    gint64 tsEnd = tsBegin + timeoutMilli * G_TIME_SPAN_MILLISECOND;
    bool retVal = true;

    g_mutex_lock(&configCacheLock);

    config_evaluateMissingConfigsSafe();

    while (configUpdateWaitingLsCount
           || ( !jis_null(configMissingConfigs) && jarray_size(configMissingConfigs) > 0))
    {
        if (!g_cond_wait_until(&configCacheCond, &configCacheLock, tsEnd))
        {   // timeout expired
            retVal = false;
            break;
        }
    }

    g_mutex_unlock(&configCacheLock);

    return retVal;
}

bool config_waitReLoadConfigs(int64_t timeoutMilli)
{
    assert(timeoutMilli>0);
    assert(jarray_size(configQueryNames)>0);

    gint64 tsBegin = g_get_monotonic_time();
    gint64 tsEnd = tsBegin + timeoutMilli * G_TIME_SPAN_MILLISECOND;
    bool retVal = true;

    g_mutex_lock(&configCacheLock);

    while (!configReLoadDone)
    {
        if (!g_cond_wait_until(&configCacheCond, &configCacheLock, tsEnd))
        {   // timeout expired
            retVal = false;
            break;
        }
    }

    g_mutex_unlock(&configCacheLock);

    return retVal;
}

bool config_getBoolean(raw_buffer configNameBuf, int32_t *pData, int32_t *errorCode)
{
    bool bConfigVal = false;
    jvalue_ref valObject = NULL;

    if (!configNameBuf.m_str || !pData)
    {
        if (errorCode)
        {
            *errorCode = CONFIG_INVALID_ARGUMENTS;
        }

        return false;
    }

    CONFIG_COND_WAIT(&configCacheCond, &configCacheLock);

    if (!configCacheObj)
    {
        if (errorCode)
        {
            //Config_loadConfig not called
            *errorCode = CONFIG_MODULE_NOT_INITIALIZED;
        }

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    if (!(jobject_get_exists(configCacheObj, configNameBuf, &valObject)))
    {
        if (errorCode)
        {
            *errorCode = CONFIG_VALUE_NOT_FOUND;
        }

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    if (CONV_NOT_A_BOOLEAN == jboolean_get(valObject, &bConfigVal))
    {
        if (errorCode)
        {
            *errorCode = CONFIG_DATA_TYPE_MISMATCH;
        }

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    g_mutex_unlock(&configCacheLock);

    //Assuming jboolen_get will return actual config value
    *pData = bConfigVal;

    if (errorCode)
    {
        *errorCode = CONFIG_NO_ERROR;
    }

    return true;
}

bool config_getInteger(raw_buffer configNameBuf, int32_t *pData, int32_t *errorCode)
{
    jvalue_ref valObject = NULL;

    if (!configNameBuf.m_str || !pData)
    {
        if (errorCode)
        {
            *errorCode = CONFIG_INVALID_ARGUMENTS;
        }

        return false;
    }

    CONFIG_COND_WAIT(&configCacheCond, &configCacheLock);

    if (!configCacheObj)
    {
        if (errorCode)
        {
            *errorCode = CONFIG_MODULE_NOT_INITIALIZED;
        }

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    if (!(jobject_get_exists(configCacheObj, configNameBuf, &valObject)))
    {
        if (errorCode)
        {
            *errorCode = CONFIG_VALUE_NOT_FOUND;
        }

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    if (CONV_OK != jnumber_get_i32(valObject, pData))
    {
        if (errorCode)
        {
            *errorCode = CONFIG_DATA_TYPE_MISMATCH;
        }

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    g_mutex_unlock(&configCacheLock);

    if (errorCode)
    {
        *errorCode = CONFIG_NO_ERROR;
    }

    return true;
}

bool config_getStringDup(raw_buffer configNameBuf, raw_buffer *pData, int32_t *errorCode)
{
    jvalue_ref valObject = NULL;

    if (!configNameBuf.m_str || !pData)
    {
        if (errorCode)
        {
            *errorCode = CONFIG_INVALID_ARGUMENTS;
        }

        return false;
    }


    CONFIG_COND_WAIT(&configCacheCond, &configCacheLock);

    if (!configCacheObj)
    {
        if (errorCode)
        {
            *errorCode = CONFIG_MODULE_NOT_INITIALIZED;
        }

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    if (!(jobject_get_exists(configCacheObj, configNameBuf, &valObject)))
    {
        if (errorCode)
        {
            *errorCode = CONFIG_VALUE_NOT_FOUND;
        }

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    *pData = jstring_get(valObject);
    g_mutex_unlock(&configCacheLock);

    if (NULL == pData->m_str)
    {
        if (errorCode)
        {
            *errorCode = CONFIG_VALUE_NOT_FOUND;
        }

        return false;
    }

    if (errorCode)
    {
        *errorCode = CONFIG_NO_ERROR;
    }

    return true;
}

bool config_getJsonObject(raw_buffer configNameBuf, jvalue_ref *pData, int32_t *errorCode)
{
    jvalue_ref valObject = NULL;

    if (!configNameBuf.m_str || !pData)
    {
        if (errorCode)
        {
            *errorCode = CONFIG_INVALID_ARGUMENTS;
        }

        return false;
    }

    CONFIG_COND_WAIT(&configCacheCond, &configCacheLock);

    if (!configCacheObj)
    {
        if (errorCode)
        {
            *errorCode = CONFIG_MODULE_NOT_INITIALIZED;
        }

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    if (!(jobject_get_exists(configCacheObj, configNameBuf, &valObject)))
    {
        if (errorCode)
        {
            *errorCode = CONFIG_VALUE_NOT_FOUND;
        }

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    *pData = jvalue_duplicate(valObject);
    g_mutex_unlock(&configCacheLock);

    if (errorCode)
    {
        *errorCode = CONFIG_NO_ERROR;
    }

    return true;
}

bool config_setConfigs(jvalue_ref configsObj, int32_t *errorCode)
{
    LSHandle *lsHandle = config_getLSHandle();
    if ((NULL == lsHandle) || !jis_valid(configsObj))
    {
        *errorCode = CONFIG_INVALID_LSHANDLER;
        return false;
    }

    if (NULL != errorCode)
        *errorCode = CONFIG_NO_ERROR;

    // update configs cache to provide new value immediately
    jobject_iter it;
    jobject_key_value keyValPair;
    jvalue_ref configsValObj = NULL;

    if (jobject_get_exists(configsObj, J_CSTR_TO_BUF("configs"), &configsValObj))
    {
        if (jobject_size(configsValObj)) {
           jobject_iter_init(&it, configsValObj);
        } else {
            LOG_LIB_INFO_PAIRS(MSGID_LIBCONFIGD, 0, "%s: request with empty configs", __FUNCTION__);
            return true;
        }
    } else {
        if (NULL != errorCode)
            *errorCode = CONFIG_INVALID_ARGUMENTS;
        return false;
    }

    CONFIG_COND_WAIT(&configCacheCond, &configCacheLock);

    // Restore old configCacheObj in case error
    // free it on callback or error case
    jvalue_ref backupConfigObj = NULL;

    if (!jis_null(configCacheObj))
    {
        LOG_LIB_INFO_PAIRS(MSGID_LIBCONFIGD, 0, "%s: Copy configCacheObj for backup", __FUNCTION__);
        backupConfigObj = jvalue_duplicate(configCacheObj);
    }

    while (jobject_iter_next(&it, &keyValPair))
    {
        if (jis_null(keyValPair.value)) continue;

        if (jobject_containskey2(configCacheObj, keyValPair.key))
        {
            raw_buffer jKeyBuf = jstring_get(keyValPair.key);

            jobject_remove(configCacheObj, jKeyBuf);
        }

        jobject_put(configCacheObj, jvalue_copy(keyValPair.key), jvalue_copy(keyValPair.value));
    }

    if (!LSCallOneReply(lsHandle, SETCONFIGS_METHOD, jvalue_tostring_simple(configsObj),
            config_cbSetConfigs, backupConfigObj, NULL, NULL))
    {
        if (NULL != errorCode)
            *errorCode = CONFIG_LSCALL_FAILURE;

        j_release(&configCacheObj);
        configCacheObj = backupConfigObj;

        g_mutex_unlock(&configCacheLock);
        return false;
    }

    g_mutex_unlock(&configCacheLock);
    return true;
}

bool config_getConfigs(char *configNames[], void *cbFunc, int32_t *errorCode)
{
    jvalue_ref jArrayConfigNames = NULL;
    jvalue_ref queryObject = NULL;
    bool retVal = false;

    LSHandle *lsHandle = config_getLSHandle();
    if (NULL == lsHandle)
    {
        if (NULL != errorCode)
        {
            *errorCode = CONFIG_INVALID_LSHANDLER;
        }
        return false;
    }

    if (NULL == configNames)
    {
        if (NULL != errorCode)
        {
            *errorCode = CONFIG_INVALID_ARGUMENTS;
        }

        return false;
    }

    if (NULL != errorCode)
        *errorCode = CONFIG_NO_ERROR;


    CONFIG_COND_WAIT(&configCacheCond, &configCacheLock);

    queryObject = jobject_create();
    jArrayConfigNames = jarray_create(NULL);

    if (jis_null(queryObject) || jis_null(jArrayConfigNames))
    {
        if (NULL != errorCode)
        {
            *errorCode = CONFIG_MEMORY_ALLOCATION_ERROR;
        }

        g_mutex_unlock(&configCacheLock);
        j_release(&queryObject);
        j_release(&jArrayConfigNames);

        return false;
    }

    C_ARRAY_TO_J_ARRAY(jArrayConfigNames, configNames);
    jobject_put(queryObject, jstring_create("configNames"), jArrayConfigNames);

    retVal = LSCallOneReply(lsHandle, GETCONFIGS_METHOD, jvalue_tostring_simple(queryObject),
                            config_cbResultConfigs, cbFunc, NULL, NULL);
    if (!retVal)
    {
        if (NULL != errorCode)
        {
            *errorCode = CONFIG_LSCALL_FAILURE;
        }
    }

    j_release(&queryObject);
    j_release(&jArrayConfigNames);
    g_mutex_unlock(&configCacheLock);
    return retVal;
}

bool config_reconfigs(uint32_t timeout, int32_t *errorCode)
{
    LSHandle *lsHandle = config_getLSHandle();
    if (NULL == lsHandle)
    {
        *errorCode = CONFIG_INVALID_LSHANDLER;
        return false;
    }

    if (NULL != errorCode)
        *errorCode = CONFIG_NO_ERROR;

    jvalue_ref optObject = jobject_create();
    jobject_put(optObject, jstring_create("timeout"), jnumber_create_i32(timeout));

    if (!LSCallOneReply(lsHandle, RECONFIGS_METHOD, jvalue_tostring_simple(optObject),
                        config_cbReconfigs, NULL, NULL, NULL))
    {
        if (NULL != errorCode)
            *errorCode = CONFIG_LSCALL_FAILURE;

        j_release(&optObject);
        return false;
    }

    j_release(&optObject);
    return true;
}

bool config_loadConfigsFromFile(char *path, char *fileName, int32_t *errorCode)
{
    JSchemaInfo schemaInfo;
    jvalue_ref parseObj           = NULL;
    char *category = NULL;
    int categoryLength = 0;

    if (!path || !fileName)
    {
        if (NULL != errorCode)
            *errorCode = CONFIG_INVALID_ARGUMENTS;

        return false;
    }

    char *fullPath = g_strconcat(path, "/", fileName, NULL);
    if (!fullPath) {
        // CID 9026655, 9158810 - handle null pointer dereference
        LOG_LIB_ERROR_PAIRS(MSGID_LIBCONFIGD, 0, "%s: failed to concatenate strings", __FUNCTION__);

        if (NULL != errorCode)
            *errorCode = CONFIG_INVALID_ARGUMENTS;

        return false;
    }

    if (!g_file_test(fullPath, G_FILE_TEST_EXISTS))
    {
        if (NULL != errorCode)
            *errorCode = CONFIG_INVALID_ARGUMENTS;

        g_free(fullPath);
        return false;
    }

    // Initialize JSON Schema
    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);

    // Fetch the json object from the *.json file
    parseObj = jdom_parse_file(fullPath, &schemaInfo, JFileOptMMap);

    if (!jis_valid(parseObj))
    {
        LOG_LIB_ERROR_PAIRS(MSGID_JSON_PARSE_FILE_ERR, 0, "%s fail Invalid JSON Format in File: %s",
                __FUNCTION__, fullPath);

        if (NULL != errorCode)
            *errorCode = CONFIG_FILE_PARSE_ERROR;

        g_free(fullPath);
        return false;
    }

    categoryLength = strlen(fileName);
    category = (char *)malloc((categoryLength + 1) * sizeof(char));
    if (!category) {
        // CID 9026696, 9158814 - handle null pointer dereference
        LOG_LIB_ERROR_PAIRS(MSGID_LIBCONFIGD, 0, "%s: failed to allocate memory for category", __FUNCTION__);
        
        if (NULL != errorCode)
            *errorCode = CONFIG_FILE_PARSE_ERROR;

        g_free(fullPath);
        return false;
    }

    strcpy(category, fileName);

    g_free(fullPath);
    while ((category[categoryLength] != '.') && (categoryLength > 0))
    {
        categoryLength--;
    }

    if (categoryLength == 0)
    {
        free(category);
        if (NULL != errorCode)
            *errorCode = CONFIG_INVALID_ARGUMENTS;
        return false;
    }

    category[categoryLength] = '\0';

    //update the current cache with the new updated values
    // adding one by one for subscription reply and multiple Config_loadConfigs support.
    jobject_iter it;
    jobject_key_value keyValPair;
    jobject_iter_init(&it, parseObj);

    CONFIG_COND_WAIT(&configCacheCond, &configCacheLock);

    while (jobject_iter_next(&it, &keyValPair))
    {
        raw_buffer tempBuf = jstring_get(keyValPair.key);
        char *strKey = NULL;
        jvalue_ref jvalKey = NULL;

        strKey = g_strconcat(category, ".", tempBuf.m_str, NULL);
        jvalKey = jstring_create(strKey);

        if (jobject_containskey2(configCacheObj, jvalKey))
        {
            raw_buffer jKeyBuf = jstring_get(jvalKey);
            jobject_remove(configCacheObj, jKeyBuf);
            jstring_free_buffer(jKeyBuf);
        }

        jobject_put(configCacheObj, jvalKey, jvalue_copy(keyValPair.value));

        jstring_free_buffer(tempBuf);
        g_free(strKey);
    }

    g_mutex_unlock(&configCacheLock);
    free(category);

    // notify to all watchers
    if (configWatchers)
    {
        GHashTableIter iter;
        gpointer key, value;
        g_hash_table_iter_init(&iter, configWatchers);
        while (g_hash_table_iter_next(&iter, &key, &value))
        {
            if (key)
            {
                configdNotify  notify = (configdNotify)key;
                notify(parseObj, value);
            }
        }
    }

    j_release(&parseObj);
    if (NULL != errorCode)
        *errorCode = CONFIG_NO_ERROR;

    LOG_LIB_INFO_PAIRS(MSGID_LIBCONFIGD, 0, "Successfully updated data from %s/%s", path, fileName);
    return true;
}
