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

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <sys/time.h>

#include <PmLogLib.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    GET_CONFIG,
    GET_CONFIGS,
    RE_CONFIG,
    SET_CONFIGS,
    SET_TEMP_CONFIG,
    CONFIGD_SERVICE_MAIN,
    CREATE_DB,
    DB_INSERTRECORD,
    DB_LOAD,
    DB_DELETE,
    DB_FETCHRECORD,
    DB_FETCHCONFIGRECORDS,
    DB_FINALIZEDB,
    POST_PROCESS_RECONFIG,
    NO_OF_BENCHMARK_BLOCKS
};

extern PmLogContext confdServiceLogContext;
extern PmLogContext confdLibContext;
extern void ConfigdLazyDbgPrint(const char *format, ...);

#define LOG_INFO_PAIRS(...)                     PmLogInfo(confdServiceLogContext, ##__VA_ARGS__)
#define LOG_MESSAGE_PAIRS(...)                  PmLogMsg(confdServiceLogContext, ##__VA_ARGS__)
#define LOG_DEBUG(...)                          PmLogDebug(confdServiceLogContext, ##__VA_ARGS__)
#define LOG_WARNING_PAIRS(...)                  PmLogWarning(confdServiceLogContext, ##__VA_ARGS__)
#define LOG_ERROR_PAIRS(...)                    PmLogError(confdServiceLogContext, ##__VA_ARGS__)
#define LOG_CRITICAL_PAIRS(...)                 PmLogCritical(confdServiceLogContext, ##__VA_ARGS__)

#define LOG_LIB_INFO_PAIRS(...)                 PmLogInfo(confdLibContext, ##__VA_ARGS__)
#define LOG_LIB_ERROR_PAIRS(...)                PmLogError(confdLibContext, ##__VA_ARGS__)

#define LAZY_LOG_PRINT(...) do {ConfigdLazyDbgPrint(__VA_ARGS__);} while(0)

/* Define MSGID's */
#define MSGID_SRVC_REGISTER_FAILED              "SRVC_REGISTER_FAILED" /* Failed to register service on luna bus */
#define MSGID_SRVC_PREPARE_FAILED               "SRVC_PREPARE_FAILED" /* Failed to register service on luna bus */
#define MSGID_SRVC_ATTACH_FAIL                  "SRVC_ATTACH_FAIL" /* Failed to attach service handle to mainloop */
#define MSGID_CATEGORY_REGISTER_FAILED          "CATEGORY_REGISTER_FAILED" /* Failed to register category */
#define MSGID_JSON_PARSE_ERR                    "JSON_PARSE_ERR" /* Json parse error */
#define MSGID_JSON_PARSE_FILE_ERR               "JSON_PARSE_FILE_ERR" /* Json parse file error */
#define MSGID_SELECTION_FAIL                    "Will not watch layer selection change" /* Selection fail */
#define MSGID_DEPRECATED_API                    "DEPRECATED_API" /* Call of a deprecated api */

/* Configure.c */
#define MSGID_CONFIGURE                         "CONFIGURE" /* Configure file Log Key */

/* ConfigdService.c */
#define MSGID_CONFIGDSERVICE                    "CONFIGDSERVICE" /* ConfigdService file Log Key */

/* ConfigureData.c */
#define MSGID_CONFIGUREDATA                     "CONFIGUREDATA" /* ConfigureData file Log Key */

/* dbInterface.c */
#define MSGID_DBINTERFACE                       "DBINTERFACE" /* dbInterface file Log Key */

/* libconfigd.c */
#define MSGID_LIBCONFIGD                        "LIBCONFIGD" /* libconfigd file Log Key */

#ifdef __cplusplus
}
#endif

#endif // _LOGGER_H_

