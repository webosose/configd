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

#include <glib.h>
#include <string.h>

#include "Logging.h"

PmLogContext confdServiceLogContext;
PmLogContext confdLibContext;

#define LAZY_DBGPRINT_WAIT          300         /* ms */
#define LAZY_DBGPRINT_FORGET        (30*1000)   /* ms */
#define LAZY_DBGPRINT_DELAY_MAX     (10*1000)   /* ms */
#define LAZY_DBGPRINT_COUNT_TH_MIN  10

static GSList *lazyMsgs = NULL;
static guint lazyMsgsTimerId = 0;
static GMutex lazyMsgsLock;

static gboolean lazyDbgPrintHandle(gpointer userData);
static void lazyDbgPrintPost(char *msg);

typedef struct {
    gint64 tsFirst;
    gint64 tsLast;
    gint32 count;
    gint32 flushThreshold;
    char *msg;
} lazyMsg_t;

//Benchmark related globals
struct timeval _astStartTime[NO_OF_BENCHMARK_BLOCKS];
struct timeval _astEndTime[NO_OF_BENCHMARK_BLOCKS];

char *Msg1[] = {
    "[Start]",
    "[End]"
};

char *Msg2[NO_OF_BENCHMARK_BLOCKS] = {
    "[GET_CONFIG]",
    "[GET_CONFIGS]",
    "[RE_CONFIG]",
    "[SET_CONFIG]",
    "[SET_TEMP_CONFIG]",
    "[CONFIGD_SERVICE_MAIN]",
    "[CREATE_DB]",
    "[DB_INSERTRECORD]",
    "[DB_LOAD]",
    "[DB_DELETE]",
    "[DB_FETCHRECORD]",
    "[DB_FETCHCONFIGRECORDS]",
    "[DB_FINALIZEDB]",
    "[POST_PROCESS_RECONFIG]"
};

static gboolean lazyDbgPrintHandle(gpointer userData)
{
    gboolean returnVal = true;
    gint64 tsNow = g_get_monotonic_time();

    g_mutex_lock(&lazyMsgsLock);

    for (GSList *iter = lazyMsgs; iter;) {
        lazyMsg_t *lazyMsgTmp = (lazyMsg_t*) iter->data;
        gint64 tsLDiff = tsNow - lazyMsgTmp->tsFirst;
        gint64 tsSDiff = tsNow - lazyMsgTmp->tsLast;
        gboolean timeOff = false;
        gboolean countOff = false;

        if (lazyMsgTmp->count > 0 && (tsSDiff > (LAZY_DBGPRINT_WAIT * G_TIME_SPAN_MILLISECOND) || tsLDiff > (LAZY_DBGPRINT_DELAY_MAX * G_TIME_SPAN_MILLISECOND))) {
            timeOff = true;
        }
        if (lazyMsgTmp->count >= lazyMsgTmp->flushThreshold) {
            countOff = true;
            lazyMsgTmp->flushThreshold *= 2;
        }

        if (timeOff || countOff) {
            // condition to flush
            LOG_INFO_PAIRS(MSGID_CONFIGDSERVICE, 2, PMLOGKFV("uptime", "%lld", lazyMsgTmp->tsFirst), PMLOGKFV("count", "%d", lazyMsgTmp->count), "%s", lazyMsgTmp->msg);

            // RESET counter
            lazyMsgTmp->count = 0;
        }

        GSList *prev = iter;
        iter = g_slist_next(iter);

        if (tsSDiff > (LAZY_DBGPRINT_FORGET * G_TIME_SPAN_MILLISECOND)) {
            // let's forget this node;
            lazyMsgs = g_slist_remove_link(lazyMsgs, prev);
            lazyMsg_t *lazyMsgTmp = (lazyMsg_t*) prev->data;
            g_free(lazyMsgTmp->msg);
            g_slist_free_1(prev);
        }
    }
    if (!lazyMsgs) {
        // Ok no more item in queue. Let's stop
        lazyMsgsTimerId = 0;
        returnVal = false;
    }

    g_mutex_unlock(&lazyMsgsLock);

    return returnVal;
}

static void lazyDbgPrintPost(char *msg)
{
    lazyMsg_t *lazyMsg = NULL;
    gint64 tsNow = g_get_monotonic_time();

    if (!msg)
        return;

    g_mutex_lock(&lazyMsgsLock);

    for (GSList *iter = lazyMsgs; iter; iter = g_slist_next(iter)) {
        lazyMsg_t *lazyMsgTmp = (lazyMsg_t*) iter->data;
        if (!strcmp(lazyMsgTmp->msg, msg)) {
            lazyMsg = lazyMsgTmp;
            break;
        }
    }

    if (!lazyMsg) {
        lazyMsg = g_new0(lazyMsg_t, 1);
        lazyMsg->tsFirst = tsNow;
        lazyMsg->tsLast = tsNow;
        lazyMsg->count = 1;
        lazyMsg->flushThreshold = LAZY_DBGPRINT_COUNT_TH_MIN;

        lazyMsg->msg = g_strdup(msg);
        lazyMsgs = g_slist_append(lazyMsgs, lazyMsg);
    } else {
        if (!lazyMsg->count) {
            lazyMsg->tsFirst = tsNow;
        }
        ++lazyMsg->count;
        lazyMsg->tsLast = tsNow;
    }

    if (!lazyMsgsTimerId) {
        // NOTE: Do not adjust timer. No need that accuracy.
        lazyMsgsTimerId = g_timeout_add(LAZY_DBGPRINT_WAIT, lazyDbgPrintHandle, NULL);
    }

    g_mutex_unlock(&lazyMsgsLock);
}

void ConfigdLazyDbgPrint(const char *format, ...)
{
    char *msg = NULL;
    va_list ap;

    va_start(ap, format);
    msg = g_strdup_vprintf(format, ap);
    va_end(ap);

    lazyDbgPrintPost(msg);
    g_free(msg);
}

