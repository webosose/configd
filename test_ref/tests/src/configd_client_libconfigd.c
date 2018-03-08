// Copyright (c) 2014-2018 LG Electronics, Inc.
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


#include <unistd.h>

#include "libconfigd.h"
#include <lunaservice.h>
#include <glib.h>
#include <pbnjson.h>

#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>


#define SETCONFIGS_METHOD "palm://com.webos.service.config/setConfigs"
#define SETCONFIGS_DELAY   2000 // milliseconds
#define SETCONFIGS_TIMEOUT 2000 // milliseconds
#define DEBUG_PRINT(...) // g_print(__VA_ARGS__);

LSHandle *configd_lsClientHandle = NULL;
GMainLoop *configd_lsMainLoop = NULL;

#define CLIENT_KEY_INT0     "system.nonExistingInt"
#define CLIENT_KEY_BOOL0    "system.nonExistingBoolean"
#define CLIENT_KEY_STR0     "system.nonExistingString"
#define CLIENT_KEY_INT1     "system.nonExistingInt1"
#define CLIENT_KEY_BOOL1    "system.nonExistingBoolean1"
#define CLIENT_KEY_STR1     "system.nonExistingString1"

#define CLIENT_VALUE_INT0   345
#define CLIENT_VALUE_BOOL0  true
#define CLIENT_VALUE_STR0   "bean bird"
#define CLIENT_VALUE_INT1   123
#define CLIENT_VALUE_BOOL1  false
#define CLIENT_VALUE_STR1   "red bird"

#define BOOL_STR(x)     (x)?"true":"false"

// ////////////////////////////////////////////////////////////
// LS Thread
gpointer LS2InputThread(gpointer data)
{
    GMainLoop *mainloop = (GMainLoop *)data;

    g_main_loop_run(mainloop);

    return NULL;
}

// ////////////////////////////////////////////////////////////
// For Test Client Threads
static guint client_configsIndex = 0;
static GPtrArray *client_configs = NULL;
static GMutex client_configsLock;

static GArray *client_logs = NULL;
typedef struct {
    gint64 time_requested;
    gint64 time_replied;
    const char *configName;
    jvalue_ref configValue;
} client_log_t;

void client_log(const char *configName, jvalue_ref configValue, gint64 time_req, gint64 time_rep)
{
    g_mutex_lock(&client_configsLock);

    if (!client_logs)
    {
        client_logs = g_array_new(FALSE, FALSE, sizeof(client_log_t));
    }
    client_log_t newlog;
    newlog.time_requested = time_req;
    newlog.time_replied = time_rep;
    newlog.configName = g_strdup(configName); // no ownership
    newlog.configValue = jvalue_copy(configValue); // has ownership ; -_-;;;

    g_array_append_val(client_logs, newlog);
    g_mutex_unlock(&client_configsLock);
}

int client_countConfigs()
{
    return client_configs ? client_configs->len : 0 ;
}

void client_addConfigName(char *configName)
{
    g_mutex_lock(&client_configsLock);

    if (!client_configs)
    {
        client_configs = g_ptr_array_new();
    }

    g_ptr_array_add(client_configs, g_strdup(configName));
    g_mutex_unlock(&client_configsLock);
}

const char *client_nextConfigName()
{
    assert(client_configs);
    char *retConfigName = NULL;

    g_mutex_lock(&client_configsLock);
    retConfigName = (char *)g_ptr_array_index(client_configs, client_configsIndex);
    ++client_configsIndex;
    client_configsIndex = (client_configsIndex < client_configs->len) ? client_configsIndex : 0;
    g_mutex_unlock(&client_configsLock);

    return retConfigName;
}

// must be freed using g_strfreev
char** client_getConfigNames()
{
    g_mutex_lock(&client_configsLock);

    char **strArray = g_malloc(sizeof(char*) * (client_configs->len + 1));

    for (int i = 0; i < client_configs->len; ++i)
    {
        strArray[i] = g_strdup(g_ptr_array_index(client_configs, i));
    }
    strArray[client_configs->len] = NULL;

    g_mutex_unlock(&client_configsLock);

    return strArray;
}

gpointer ClientThread(gpointer data)
{
    int queryCount = *(int *)data;

    for (int i = 0; i < queryCount; ++i)
    {
        // for all input configs
        const char *configName = client_nextConfigName();
        gint64 time_begin = g_get_monotonic_time();
        jvalue_ref configValue = jinvalid();

        int32_t errCode = 0;
        if (!config_getJsonObject(j_cstr_to_buffer(configName), &configValue, &errCode))
        {
            g_print("Failed to getJsonObject : %s : errorCode=%d\n", configName, errCode);
        }

        gint64 time_end = g_get_monotonic_time();
        client_log(configName, configValue, time_begin, time_end);
        j_release(&configValue);
    }

    return NULL;
}

bool configd_cbSetConfig(LSHandle *sh, LSMessage *reply, void *ctx)
{
    g_print("setConfigs callback replied : %s \n", LSMessageGetPayload(reply));
    return true;
}

gpointer DelayedSetConfigsThread(gpointer data)
{
    g_usleep(SETCONFIGS_DELAY*1000); // doesn't care about interrupt

    jvalue_ref setConfigs_configs = (jvalue_ref)data;
    jvalue_ref setConfigsObj = jobject_create();
    jobject_put(setConfigsObj, jstring_create("configs"), setConfigs_configs);

    LSError lsError;
    LSErrorInit(&lsError);
    if (!LSCallOneReply(configd_lsClientHandle, SETCONFIGS_METHOD, jvalue_tostring_simple(setConfigsObj),
                        NULL, configd_lsMainLoop, NULL, &lsError)) {
        LSErrorPrint(&lsError, stderr);
        LSErrorFree(&lsError);
    }

    return NULL;
}

void client_callbackTest(jvalue_ref changedObjects, void *watch_context)
{
    gint64 timestamp = g_get_monotonic_time();
    g_print("%lld: libconfigd: Change Notified : %s \n", timestamp, jvalue_tostring_simple(changedObjects));
}

// ////////////////////////////////////////////////////////////
// Command line arguments
static int cmd_querycount = 0;
static int cmd_threadcount = 1;
static char **cmd_filenames = NULL;

static GOptionEntry cmd_entries[] =
{
    { "queryCount", 'q', 0, G_OPTION_ARG_INT , &cmd_querycount, "Total number of properties to query", "nQ"},
    { "threadCount", 't', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT , &cmd_threadcount, "Number of th reads", "nT"},
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &cmd_filenames, "List of filenames of config.json", "config.json ..."},
    { NULL }
};

int main(int argc, char **argv)
{
    GError *error = NULL;

    {
        // parse command line options
        GOptionContext *optionContext = g_option_context_new("- Test Client for configd-service. Individual queries");
        g_option_context_add_main_entries(optionContext, cmd_entries, NULL);

        if (!g_option_context_parse(optionContext, &argc, &argv, &error))
        {
            g_print("option parsing failed: %s\n", error->message);
            return -1;
        }

        if (!cmd_filenames)
        {
            // filename must be supplied
            gchar *help;

            help = g_option_context_get_help(optionContext, TRUE, NULL);
            g_print("%s", help);
            g_free(help);
            return -2;
        }
    }

    {
        // parse config files & getting configsToQuery
        JSchemaInfo schemaInfo;
        jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);

        for (int i = 0; cmd_filenames && cmd_filenames[i] ; ++i)
        {
            g_print("cmd_filenames[%d] = %s\n", i, cmd_filenames[i]);
            jvalue_ref configFileObj = jdom_parse_file(cmd_filenames[i], &schemaInfo, JFileOptMMap);

            if (jis_valid(configFileObj))
            {
                // get package name from filename
                char *filepath = g_strdup(cmd_filenames[i]);
                char *filename = basename(filepath);
                char *fileext = strrchr(filename, '.');

                if (fileext)
                {
                    *fileext = '\0';
                }

                char *packagename = g_strdup(filename);
                g_free(filepath);

                // get list of keys
                jobject_iter iter;

                if (jobject_iter_init(&iter, configFileObj))
                {
                    jobject_key_value keyval;

                    while (jobject_iter_next(&iter, &keyval))
                    {
                        if (jis_string(keyval.key))
                        {
                            raw_buffer strbuf = jstring_get(keyval.key);
                            char *configName = g_strjoin(".", packagename, strbuf.m_str, NULL);

                            DEBUG_PRINT("config : %s\n", configName);
                            client_addConfigName(configName);

                            g_free(configName);
                            jstring_free_buffer(strbuf);
                        }
                    }
                }

                g_free(packagename);
                j_release(&configFileObj);
            }
        }

        if (client_countConfigs() == 0)
        {
            g_error("No Configs in config files\n");
            return -3;
        }
    }

    { // Add non existing configs to request
        client_addConfigName(CLIENT_KEY_INT0);
        client_addConfigName(CLIENT_KEY_BOOL0);
        client_addConfigName(CLIENT_KEY_STR0);
    }

    gint64 time_begin = g_get_monotonic_time();

    {
        // Initialze LS2
        // This should be done before create any client.
        // TODO : Check TVService status; PROBLEM #1
        LSError lsError;
        LSErrorInit(&lsError);

        if (!LSRegisterPubPriv(NULL, &configd_lsClientHandle, false, &lsError))
        {
            LSErrorPrint(&lsError, stderr);
            LSErrorFree(&lsError);
            return -2;
        }
    }

    // LS2 Thread
    configd_lsMainLoop = g_main_loop_new(NULL, FALSE);
    LSGmainAttach(configd_lsClientHandle, configd_lsMainLoop, NULL);
    GThread *lsThread = g_thread_new("LS2handle", LS2InputThread, configd_lsMainLoop);

    // Load Configs
    char **configNames = client_getConfigNames();
    config_loadConfigs(configd_lsClientHandle, configNames, NULL);

    // Client Threads
    int queryCount = (cmd_querycount > 0) ? cmd_querycount : client_countConfigs();
    int threadCount = (cmd_threadcount > 0) ? cmd_threadcount : 1;

    GThread **clientThreads = g_new0(GThread *, threadCount);

    for (int clientIdx = 0; clientIdx < threadCount; ++clientIdx)
    {
        clientThreads[clientIdx] = g_thread_new("dummyClient", ClientThread, &queryCount);
    }

    // waiting all client work completed
    for (int clientIdx = 0; clientIdx < threadCount; ++clientIdx)
    {
        g_thread_join(clientThreads[clientIdx]);
    }

    g_free(clientThreads);

    gint64 time_end = g_get_monotonic_time();

    {
        int logCount = client_logs->len;
        for (int i = 0; i < logCount; ++i)
        {
            client_log_t *alog = &g_array_index(client_logs, client_log_t, i);
            g_print("%lld-%lld: %lld us  %s : %s \n", alog->time_requested, alog->time_replied, (alog->time_replied - alog->time_requested), alog->configName, jvalue_tostring_simple(alog->configValue));
        }
    }

    g_print("%lld-%lld: Total %lld ms for %d x %d = %d configs\n", time_begin, time_end, (time_end - time_begin) / G_TIME_SPAN_MILLISECOND, threadCount, queryCount, threadCount * queryCount);

    {   // Test waiting and callbacks
        jvalue_ref missingConfigs = config_getMissingConfigs();
        g_print("WAITING for missing configs : %s\n", jvalue_tostring_simple(missingConfigs));
        j_release(&missingConfigs);

        // Prepare setConfigs list
        jvalue_ref setConfigs_configs = jobject_create();
        jobject_put(setConfigs_configs, jstring_create(CLIENT_KEY_INT0), jnumber_create_i32(CLIENT_VALUE_INT0));
        jobject_put(setConfigs_configs, jstring_create(CLIENT_KEY_BOOL0), jboolean_create(CLIENT_VALUE_BOOL0));
        jobject_put(setConfigs_configs, jstring_create(CLIENT_KEY_STR0), jstring_create(CLIENT_VALUE_STR0));

        // Test wait and callback
        config_addWatch(client_callbackTest, setConfigs_configs);

        // call setConfigs with 2s delay
        GThread *setConfigsThread = g_thread_new("setConfigsThread", DelayedSetConfigsThread, setConfigs_configs);

        // wait for missingConfigs for 3s
        time_begin = g_get_monotonic_time();
        bool retWait = config_waitConfigs(3000);
        time_end = g_get_monotonic_time();
        g_print("%lld-%lld: waited %lldms including 2000ms delay : %s\n", time_begin, time_end, (time_end - time_begin) / G_TIME_SPAN_MILLISECOND, retWait?"REPLY RECEIVED":"TIMEOUT");
        { // TEST VALID
            g_print(" Int0 : key = %s value = %d / expected value = %d\n", CLIENT_KEY_INT0, config_getIntegerSimple(J_CSTR_TO_BUF(CLIENT_KEY_INT0), CLIENT_VALUE_INT1), CLIENT_VALUE_INT0);
            g_print(" Bool0 : key = %s value = %s / expected value = %s\n", CLIENT_KEY_BOOL0, BOOL_STR(config_getBooleanSimple(J_CSTR_TO_BUF(CLIENT_KEY_BOOL0), CLIENT_VALUE_BOOL1)), BOOL_STR(CLIENT_VALUE_BOOL0));
            gchar *strval = config_getStringDupSimple(J_CSTR_TO_BUF(CLIENT_KEY_STR0), CLIENT_VALUE_STR1);
            g_print(" Str0 : key = %s value = %s / expected value = %s \n", CLIENT_KEY_STR0, strval, CLIENT_VALUE_STR0);
            g_free(strval);
        }

        { // TEST INVALID
            g_print(" Int1 : key = %s value = %d / expected value = %d\n", CLIENT_KEY_INT1, config_getIntegerSimple(J_CSTR_TO_BUF(CLIENT_KEY_INT1), CLIENT_VALUE_INT1), CLIENT_VALUE_INT1);
            g_print(" Bool1 : key = %s value = %s / expected value = %s\n", CLIENT_KEY_BOOL1, BOOL_STR(config_getBooleanSimple(J_CSTR_TO_BUF(CLIENT_KEY_BOOL1), CLIENT_VALUE_BOOL1)), BOOL_STR(CLIENT_VALUE_BOOL1));
            gchar *strval = config_getStringDupSimple(J_CSTR_TO_BUF(CLIENT_KEY_STR1), CLIENT_VALUE_STR1);
            g_print(" Str1 : key = %s value = %s / expected value = %s \n", CLIENT_KEY_STR1, strval, CLIENT_VALUE_STR1);
            g_free(strval);
        }

        { // DUMP ALL

            jvalue_ref reqNames = config_getReqConfigNamesArray();
            jvalue_ref allConfigs = config_getAllConfigs();

            g_print("ConfigNames : %s\n", jvalue_tostring_simple(reqNames));
            g_print("allConfigs : %s\n", jvalue_tostring_simple(allConfigs));

            j_release(&reqNames);
            j_release(&allConfigs);
        }
        g_thread_join(setConfigsThread);
    }

    {   // clearing configs
        jvalue_ref setConfigs_configs = jobject_create();
        jobject_put(setConfigs_configs, jstring_create(CLIENT_KEY_INT0), jnull());
        jobject_put(setConfigs_configs, jstring_create(CLIENT_KEY_BOOL0), jnull());
        jobject_put(setConfigs_configs, jstring_create(CLIENT_KEY_STR0), jnull());

        GThread *setConfigsThread = g_thread_new("setConfigsThread2", DelayedSetConfigsThread, setConfigs_configs);
        g_thread_join(setConfigsThread);
    }

    // shutting down worker thread
    g_main_loop_quit(configd_lsMainLoop);
    g_thread_join(lsThread);

    return 0;
}
