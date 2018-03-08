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

#include <lunaservice.h>
#include <glib.h>
#include <pbnjson.h>

#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>


#define GETCONFIGS_METHOD "luna://com.webos.service.config/getConfigs"
#define GETCONFIGS_TIMEOUT 2000 // milliseconds
#define DEBUG_PRINT(...) // g_print(__VA_ARGS__);
// ////////////////////////////////////////////////////////////
//

typedef struct
{
    GCond cond;
    GMutex lock;
    LSMessageToken lsToken;
    char *requestedConfig;
    bool gotReply;
    jvalue_ref replyValue;
} ConfigdRequestedToken;

LSHandle *configd_lsClientHandle = NULL;
GMainLoop *configd_lsMainLoop = NULL;

// Note: Put to configd_lsRequestedTokens only if you can lock configd_lsRequestAccessLock
// and configd_lsRequestWriteLock in order.
GHashTable *configd_lsRequestedTokens = NULL;  //KEY: token, VALUE: GetConfigtoken*
GRWLock configd_lsRequestAccessLock;
GMutex configd_lsRequestWriteLock;

JSchemaInfo configd_lsRequestSchemaInfo;

ConfigdRequestedToken *allocConfigdRequestedToken()
{
    ConfigdRequestedToken *token = g_malloc0(sizeof(ConfigdRequestedToken));

    g_cond_init(&token->cond);
    g_mutex_init(&token->lock);
    token->lsToken = LSMESSAGE_TOKEN_INVALID;
    token->replyValue = jinvalid();
    token->gotReply = false;

    return token;
}

// destroy ConfigdRequestedToken : (GDestroyNotify)
void freeConfigdRequestedToken(gpointer data)
{
    if (data)
    {
        ConfigdRequestedToken *token = (ConfigdRequestedToken *)data;
        g_mutex_clear(&token->lock);
        g_cond_clear(&token->cond);
        g_free(token->requestedConfig);
        j_release(&token->replyValue);
        g_free(data);
    }
}

// ////////////////////////////////////////////////////////////
// Call callback

bool configd_cbGetConfig(LSHandle *sh, LSMessage *reply, void *ctx)
{
    const LSMessageToken lsToken = LSMessageGetResponseToken(reply);
    const char *payload = LSMessageGetPayload(reply);
    ConfigdRequestedToken *reqToken = NULL;

    g_rw_lock_writer_lock(&configd_lsRequestAccessLock);
    g_mutex_lock(&configd_lsRequestWriteLock);

    if ((reqToken = g_hash_table_lookup(configd_lsRequestedTokens, &lsToken)))
    {
        // notify to getter regardless of returnValue
        g_mutex_lock(&reqToken->lock);

        jvalue_ref replyObj = jdom_parse(j_cstr_to_buffer(payload), DOMOPT_INPUT_OUTLIVES_WITH_NOCHANGE,
                                         &configd_lsRequestSchemaInfo);

        if (jis_valid(replyObj))
        {
            jvalue_ref configs = jobject_get(replyObj, J_CSTR_TO_BUF("configs"));
            reqToken->replyValue = jvalue_copy(jobject_get(configs, j_cstr_to_buffer(reqToken->requestedConfig)));

            j_release(&replyObj);
        }

        DEBUG_PRINT("%lld:%s:Reply received. SIGNAL tid=%d: Token %ld ,req %s payload %s\n", g_get_monotonic_time(),
                    __FUNCTION__, (int)syscall(SYS_gettid), (long)lsToken, reqToken->requestedConfig, payload);

        reqToken->gotReply = true;
        g_cond_signal(&reqToken->cond);
        g_mutex_unlock(&reqToken->lock);
    }
    else
    {
        g_print("%lld:%s:Reply received after TIMEOUT : Token %ld , payload %s\n", g_get_monotonic_time(), __FUNCTION__,
                (long)lsToken, payload);
    }

    g_mutex_unlock(&configd_lsRequestWriteLock);
    g_rw_lock_writer_unlock(&configd_lsRequestAccessLock);

    return true;
}


// ////////////////////////////////////////////////////////////
// Client APIs
//

/**
 * @return new allocated jobject of response value or jinvalid().
 *         caller have responsibility to release with j_release().
 */
jvalue_ref configd_getConfig(char *key)
{
    assert(key);

    if (!configd_lsClientHandle)
    {
        // not initialized or shutting down
        return jinvalid();
    }

    jvalue_ref retVal = jinvalid();

    // Notes:
    // If lock after return from LSCallOneReply, there are small chance
    // for the reply is received before Mutex is locked which will cause
    // the reply is trashed without paired-request.
    // To avoid this problem, Mutex is locked before call LSCallOneReply.
    // Because CallMap is locked during send as well as right after
    // reply callback is called, the losing time is quite small.
    g_rw_lock_reader_lock(&configd_lsRequestAccessLock);

    // Request from this thread
    LSMessageToken lsToken;
    char *argument = g_strdup_printf("{\"configNames\":[\"%s\"]}", key);
    LSError lsError;
    LSErrorInit(&lsError);

    DEBUG_PRINT("%lld.%s: call \n", g_get_monotonic_time(), __FUNCTION__);

    if (!LSCallOneReply(configd_lsClientHandle, GETCONFIGS_METHOD, argument, configd_cbGetConfig, configd_lsMainLoop,
                        &lsToken, &lsError))
    {
        g_error("%lld.%s: reply with error \n", g_get_monotonic_time(), __FUNCTION__);
        LSErrorPrint(&lsError, stderr);
        LSErrorFree(&lsError);
        g_rw_lock_reader_unlock(&configd_lsRequestAccessLock);
        return jinvalid();
    }

    // put to the request queue
    ConfigdRequestedToken *token = allocConfigdRequestedToken();
    token->lsToken = lsToken;
    token->requestedConfig = g_strdup(key);
    g_mutex_lock(&token->lock);


    g_mutex_lock(&configd_lsRequestWriteLock);
    assert(NULL == g_hash_table_lookup(configd_lsRequestedTokens, &token->lsToken));
    g_hash_table_insert(configd_lsRequestedTokens, &token->lsToken, token);
    g_mutex_unlock(&configd_lsRequestWriteLock);
    g_rw_lock_reader_unlock(&configd_lsRequestAccessLock);

    // Waiting for the response. for other thread.
    gint64 end_time = g_get_monotonic_time() + GETCONFIGS_TIMEOUT * G_TIME_SPAN_MILLISECOND;

    DEBUG_PRINT("%lld.%s: wait :tid=%d\n", g_get_monotonic_time(), __FUNCTION__, (int)syscall(SYS_gettid));

    while (g_cond_wait_until(&token->cond, &token->lock, end_time)
            && !token->gotReply)
    {
        DEBUG_PRINT("%lld.%s break \n", g_get_monotonic_time(), __FUNCTION__);
        fflush(stdout);
        ; // g_cond_wait_until() returned because of interrupt. wait more.
    }

    DEBUG_PRINT("%lld.%s ret after  wait \n", g_get_monotonic_time(), __FUNCTION__);

    if (jis_valid(token->replyValue))
    {
        retVal = jvalue_copy(token->replyValue);
    }
    else
    {
        // Timeout or error reply. I Don't care the reason
    }

    g_mutex_unlock(&token->lock);

    g_rw_lock_reader_lock(&configd_lsRequestAccessLock);
    g_mutex_lock(&configd_lsRequestWriteLock);
    assert(NULL == g_hash_table_lookup(configd_lsRequestedTokens, &token->lsToken));
    g_hash_table_remove(configd_lsRequestedTokens, &token->lsToken);
    g_mutex_unlock(&configd_lsRequestWriteLock);
    g_rw_lock_reader_unlock(&configd_lsRequestAccessLock);

    return retVal;
}

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

gpointer ClientThread(gpointer data)
{
    int queryCount = *(int *)data;

    for (int i = 0; i < queryCount; ++i)
    {
        // for all input configs
        const char *configName = client_nextConfigName();
        gint64 time_begin = g_get_monotonic_time();
        jvalue_ref configValue = configd_getConfig((char *)configName);
        gint64 time_end = g_get_monotonic_time();
        client_log(configName, configValue, time_begin, time_end);
        j_release(&configValue);
    }

    return NULL;
}

// ////////////////////////////////////////////////////////////
// Command line arguments
static int cmd_querycount = 0;
static int cmd_threadcount = 1;
static char **cmd_filenames = NULL;

static GOptionEntry cmd_entries[] =
{
    { "queryCount", 'q', 0, G_OPTION_ARG_INT , &cmd_querycount, "Total number of properties to query", "nQ"},
    { "threadCount", 't', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT , &cmd_threadcount, "Number of threads", "nT"},
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

    gint64 time_begin = g_get_monotonic_time();

    {
        // Initialize Request list
        g_rw_lock_init(&configd_lsRequestAccessLock);
        g_mutex_init(&configd_lsRequestWriteLock);
        configd_lsRequestedTokens = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, freeConfigdRequestedToken);

        jschema_info_init(&configd_lsRequestSchemaInfo, jschema_all(), NULL, NULL);
    }

    {
        for (int i = 0; i < 1024; ++i)
        {
            if (dup(1) >= cmd_querycount )
            {
                break;
            }
        }
    }

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

    // shutting down worker thread
    g_main_loop_quit(configd_lsMainLoop);
    g_thread_join(lsThread);

    // clean up
    g_rw_lock_clear(&configd_lsRequestAccessLock);
    g_mutex_clear(&configd_lsRequestWriteLock);

    // more cleanup?
    return 0;
}
