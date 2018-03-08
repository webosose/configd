// Copyright (c) 2013-2018 LG Electronics, Inc.
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

#ifndef _LIBCONFIGDCACHE_H_
#define _LIBCONFIGDCACHE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pbnjson.h>
#include <pbnjson/c/jtypes.h>
#include <luna-service2/lunaservice.h>
#include <stdbool.h>

typedef enum {
    CONFIG_NO_ERROR = 0,
    CONFIG_INVALID_ARGUMENTS,
    CONFIG_MEMORY_ALLOCATION_ERROR,
    CONFIG_MODULE_NOT_INITIALIZED,
    CONFIG_LSCALL_FAILURE,
    CONFIG_INVALID_LSHANDLER,
    CONFIG_VALUE_NOT_FOUND,
    CONFIG_DATA_TYPE_MISMATCH,
    CONFIG_FILE_PARSE_ERROR
} libconfigd_error_t;

#define CONFIG_WILDCARD '*'

/**
 * Prefetch the requested configs
 *
 * config_loadConfigs must be called only once. It's design intention to maintain config
 * list carefully. Calling more than once will not be supported.
 *
 * If called twice, the process will be terminated abnormally by abort();
 *
 * @param lsHandle A active LSHandle
 * @param configNames null terminated list of config name.
 * @param errorCode
 */
bool config_loadConfigs(LSHandle *lsHandle, char *configNames[], int32_t *errorCode);
bool config_loadConfigsFromFile(char *path, char *fileName, int32_t *errorCode);

/**
 * Set configs with new key-value pairs.
 *
 * Update configCacheObj first before actuall data store
 * It's to guarantee that providing new value immediately.
 * Using stored LSHandle which is set by config_loadConfigs. Thus config_loadConfigs is called
 * before config_setConfigs
 *
 * @param configsObj new key value pair object
 * @param errorCode
 */
bool config_setConfigs(jvalue_ref configsObj, int32_t *errorCode);

/**
 * Get configs for requested configs
 *
 * Using stored LSHandle which is set by config_loadConfigs. Thus config_loadConfigs is called
 * before config_setConfigs.
 * Call setConfigs with configs and call function with its result in LSCall callback function.
 * When call callback function, passing json object of "configs" key and cb function has ownership
 * of that object.
 *
 * @param configNames null terminated list of config name.
 * @param cbFunc Callback function to pass values. Function should have same prototype with "configdNotify".
 * @param errorCode
 */
bool config_getConfigs(char *configNames[], void *cbFunc, int32_t *errorCode);

/**
 * Invoke reconfigure process. Reconfigure fully execute configure sequence with pre and post process.
 *
 * @param timeout
 * @param errorCode
 */
bool config_reconfigs(uint32_t timeout, int32_t *errorCode);

/**
 * Callback function for config_addWatch
 *
 * changedObjects contains the 'configs' object of reply from getConfigs call which contains change set
 * of latest reconfigure.
 *
 * Because the ownership of watch_context is not maintained by libconfigd, the user
 * have responsibility to validate the watch_context before use.
 *
 * @param changedObjects jobject created from jobject_create() which contains json key/value pairs
 *                       the object could be NULL, JNULL, JINVALID or jobject which have no child object.
 *                       the caller have to validate using jis_valid(changedObjects) before use.
 *                       the ownership is not transfered.
 * @param watch_context user context supplied as second parameter of config_addWatch
 *                      the ownership is not transfered.
 */
typedef void (*configdNotify)(jvalue_ref changedObjects, void *watch_context);

/**
 * Notify when configd cache is changed.
 *
 * The first param func is unique key for watcher management. If func is called more than once,
 * only last one will be remain valid.
 *
 * Because ownership of second param watch_context is not transfered, the caller have duty to
 * maintaining the lifecycle of the watch_context.
 *
 * @param func pointer to callback function
 * @param watch_context void pointer to userdata. The ownership is not transfered.
 */
void config_addWatch(configdNotify func, void *watch_context);

/**
 * Remove from the watcher list
 *
 * @param func pointer to function
 * @return true if func is found and removed from wather list
 */
bool config_removeWatch(configdNotify func);

/**
 * Return a duplicated array of requested config names.
 *
 * The list is same as second argument of config_loadConfigs();
 * The caller have responsibility to free returned value using j_relase();
 * if config_loadConfigs() is never called or returns false, the process will be
 * terminated by abort()
 *
 * @return jarray, which is created using jarray_create();
 */
jvalue_ref config_getReqConfigNamesArray();

/**
 * Return all list of cached configs (key/value pairs)
 *
 * It is same as "configs" field from configd reply.
 * The returned jobject is duplicated by jvalue_duplicate() which means the caller
 * have responsibility to free resource by j_release(&jobj);
 *
 * @return jobject, which is created using jobject_create();
 *         jnull(), if there are no config pair received from configd
 */
jvalue_ref config_getAllConfigs();

/**
 * Return list of missingConfigs
 *
 * The missingConfigs are one field from configd reply. It means there are at least one
 * config key requested is not exist in configd db (at least at the time requested).
 *
 * The actual value is the copy of missingConfigs field from last (subscription) reply from configd.
 *
 * If config_loadConfigs() was never called before, the calling process will be terminated
 * by abort()
 * If config_loadConfigs() was called successfully but never get reply yet, it will block until
 * first reply is received.
 *
 * @return jarray, which is created using jarray_create() if there are missing configs
 *         jnull() if there are no missingConfigs
 */
jvalue_ref config_getMissingConfigs();

/**
 * Block wait until config_getMissingConfigs() returns jnull() or timeoutMilli is passed.
 *
 * This function must be called from different thread from which LSHandle of config_loadConfigs
 * is attached.
 *
 * @param timeoutMilli waiting timeout in milliseconds. 0 or less is not allowed
 * @return true if all requested configs are received.
 *         false if timeout expired.
 */
bool config_waitConfigs(int64_t timeoutMilli);

/**
 * Block wait until recieved reloadDone luna signal or timeout reached.
 *
 * This function must be called after config_loadConfigs
 *
 * @param timeoutMilli waiting timeout in milliseconds. 0 or less is not allowed
 * @return true if reload is finished in timeout
 *         false if timeout expired.
 */
bool config_waitReLoadConfigs(int64_t timeoutMilli);

/**
 * Get config value as boolean from local cached db.
 *
 * If a config_loadConfigs is called and first reply is not received, it will block wait
 * until first reply is received.
 *
 * The type of second argument is int32_t. It's for portability because there are various
 * non-standard boolean types which size is different.
 * For example, bool(stdbool.h) is size 1 in MacOS and Linux PC, gboolean(glib.h) is same as
 * gint which means the size is 4 in most 32bit system.
 *
 * @param configNameBuf
 * @param pData pointer to int32_t. It's true if return non-zero, false if return zero.
 * @param errorCode libconfigd_error_t will be returned if not null
 * @return true if the configNameBuf is exist in cached db and type is boolean.
 *         false if the configNameBuf is not found in cached db or not a boolean.
 *
 * @example
 *  OPTIMIZED GET USING C-STRING LITERAL.
 *   This method is faster by using sizeof() instead of strlen().
 *      #define CONFIGD_EXAMPLE_MY_FEATURE "com.webos.app.example.enableMyFeature"
 *      bool isMyFeatureEnabled = false;
 *      int32_t tmpEnabled = 0;
 *      if (config_getBoolean(J_CSTR_TO_BUF(CONFIGD_EXAMPLE_MY_FEATURE), &tmpEnabled, NULL)) {
 *          isMyFeatureEnabled = tmpEnabled;
 *      } else {
 *          #error "feature not found"
 *      }
 *
 *  UNOPTIMIZED GENERAL GET USING ARBITARY STRING
 *      char *myExampleFeature = g_strdup("com.webos.app.example.enableMyFeature");
 *      bool isMyFeatureEnabled = false;
 *      int32_t tmpEnabled = 0;
 *      if (config_getBoolean(j_str_to_buffer(myExampleFeature), &tmpEnabled, NULL)) {
 *          isMyFeatureEnabled = tmpEnabled;
 *      } else {
 *          #error "feature not found"
 *      }
 */
bool config_getBoolean(raw_buffer configNameBuf, int32_t *pData, int32_t *errorCode);

/**
 * Get config value as 32bit signed integer from local cached db.
 *
 * If a config_loadConfigs is called and first reply is not received, it will block wait
 * until first reply is received.
 *
 * @param configNameBuf
 * @param pData pointer to int32_t. The value will be returned if succeed.
 * @param errorCode libconfigd_error_t will be returned if not null
 * @return true if the configNameBuf is exist in cached db and integer number.
 *         false if the configNameBuf is not found in cached db or it's not in 32bit integer range
 *
 * @example
 *  OPTIMIZED GET USING C-STRING LITERAL.
 *   This method is faster by using sizeof() instead of strlen().
 *      #define CONFIGD_EXAMPLE_MY_FEATURE "com.webos.app.example.enableMyFeature"
 *      int32_t myValue = 0;
 *      if (config_getInteger(J_CSTR_TO_BUF(CONFIGD_EXAMPLE_MY_FEATURE), &myValue, NULL)) {
 *          return myValue;
 *      } else {
 *          #error "feature not found"
 *      }
 *
 *  UNOPTIMIZED GENERAL GET USING ARBITARY STRING
 *      char *myExampleFeature = g_strdup("com.webos.app.example.enableMyFeature");
 *      int32_t myValue = 0;
 *      if (config_getInteger(j_str_to_buffer(myExampleFeature), &myValue, NULL)) {
 *          return myValue
 *      } else {
 *          #error "feature not found"
 *      }
 */
bool config_getInteger(raw_buffer configNameBuf, int32_t *pData, int32_t *errorCode);

/**
 * Get config value as string from local cached db.
 *
 * If a config_loadConfigs is called and first reply is not received, it will block wait
 * until first reply is received.
 *
 * If return true, pData must be freed using g_free(pData.m_str)
 *
 * @param configNameBuf
 * @param pData pointer to raw_buffer. The new allocated and null-terminated string will be
 *              returned if succeed.
 * @param errorCode libconfigd_error_t will be returned if not null
 * @return true if the configNameBuf is exist in cached db and it's type is string
 *         false if the configNameBuf is not found in cached db or it's not an string or null
 * @example
 *  OPTIMIZED GET USING C-STRING LITERAL.
 *   This method is faster by using sizeof() instead of strlen().
 *      #define CONFIGD_EXAMPLE_MY_FEATURE "com.webos.app.example.enableMyFeature"
 *      raw_buffer myValue;
 *      if (config_getBoolean(J_CSTR_TO_BUF(CONFIGD_EXAMPLE_MY_FEATURE), &myValue, NULL)) {
 *          char *dupBuf = g_strndup(myValue.m_str, myVlaue.m_len);
 *          g_free(myValue.m_str);
 *      } else {
 *          #error "feature not found"
 *      }
 *
 *  UNOPTIMIZED GENERAL GET USING ARBITARY STRING
 *      char *myExampleFeature = g_strdup("com.webos.app.example.enableMyFeature");
 *      raw_buffer myValue;
 *      if (config_getBoolean(J_CSTR_TO_BUF(myExampleFeature), &myValue, NULL)) {
 *          char *dupBuf = g_strndup(myValue.m_str, myVlaue.m_len);
 *          g_free(myValue.m_str);
 *      } else {
 *          #error "feature not found"
 *      }
 */
bool config_getStringDup(raw_buffer configNameBuf, raw_buffer *pData, int32_t *errorCode);

/**
 * Get config value as json object from local cached db.
 *
 * Similar to confih_getBoolean()
 *
 * If return true, pData must be freed using j_release(&pData)
 *
 * @example
 *      #define CONFIGD_EXAMPLE_MY_FEATURE "com.webos.app.example.enableMyFeature"
 *      jvalue_ref myValue;
 *      if (config_getBoolean(J_CSTR_TO_BUF(CONFIGD_EXAMPLE_MY_FEATURE), &myValue, NULL)) {
 *          use myValue;
 *          j_release(&myValue);   <== BE CAREFUL ABOUT '&'.
 *      } else {
 *          #error "feature not found"
 *      }
 *
 *  UNOPTIMIZED GENERAL GET USING ARBITARY STRING
 *      char *myExampleFeature = g_strdup("com.webos.app.example.enableMyFeature");
 *      jvalue_ref myValue;
 *      if (config_getBoolean(J_CSTR_TO_BUF(myExampleFeature), &myValue, NULL)) {
 *          use myValue;
 *          j_release(&myValue);   <== BE CAREFUL ABOUT '&'.
 *      } else {
 *          #error "feature not found"
 *      }
 */
bool config_getJsonObject(raw_buffer configNameBuf, jvalue_ref *pData, int32_t *errorCode);

inline bool config_getBooleanSimple(raw_buffer configNameBuf, bool defaultValue)
{
    bool retVal;
    int32_t error;
    int32_t configVal;

    retVal = config_getBoolean(configNameBuf, &configVal, &error);

    if (retVal) {
        return (configVal == 1) ? true : false;
    }

    return defaultValue;
}

inline int32_t config_getIntegerSimple(raw_buffer configNameBuf, int32_t defaultValue)
{
    bool retVal = false;
    int32_t error;
    int32_t configVal;

    retVal = config_getInteger(configNameBuf, &configVal, &error);

    if (retVal) {
        return configVal;
    }

    return defaultValue;
}

inline char *config_getStringDupSimple(raw_buffer configNameBuf, char *defaultValue)
{
    bool retVal = false;
    char *retCharVal = NULL;
    int32_t error;
    raw_buffer configValBuf;

    retVal = config_getStringDup(configNameBuf, &configValBuf, &error);

    if (retVal) {
        retCharVal = g_strdup(configValBuf.m_str);
        free((void *) configValBuf.m_str);
        return retCharVal;
    }

    return g_strdup(defaultValue);
}

inline jvalue_ref config_getJsonObjectSimple(raw_buffer configNameBuf, jvalue_ref defaultValue)
{
    bool retVal = false;
    int32_t error;
    jvalue_ref configValObj;

    retVal = config_getJsonObject(configNameBuf, &configValObj, &error);

    if (retVal) {
        return configValObj;
    }

    return jvalue_duplicate(defaultValue);
}

#ifdef __cplusplus
}
#endif

#endif //_LIBCONFIGDCACHE_H_

