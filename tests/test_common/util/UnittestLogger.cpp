// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#include <gtest/gtest.h>
#include <pbnjson.hpp>
#include <iostream>
#include <fstream>
#include <string.h>

#include "Environment.h"
#include "util/Logger.hpp"
#include "util/Platform.h"

using namespace pbnjson;
using namespace std;

#define MSGID_TEST "CONFIGURE" /* Configure file Log Key */

class UnittestLogger : public testing::Test {
protected:
    UnittestLogger()
        : m_logger(Logger::getInstance())
    {
        m_logger->clear();
    }

    virtual ~UnittestLogger()
    {
        Platform::deleteFile(LOG_FILE_PATH);
    }

    void givenSetLogType(LogType type)
    {
        if (LogType_File == type) {
            EXPECT_TRUE(m_logger->setLogType(type, LOG_FILE_PATH));
        } else {
            EXPECT_TRUE(m_logger->setLogType(type));
        }

    }

    void whenWriteLog(const char* level)
    {
        if (strcmp(LOG_VERBOSE, level) == 0) {
            EXPECT_TRUE(m_logger->verbose(MSG_SINGLE_LINE));
        } else if (strcmp(LOG_DEBUG, level) == 0) {
            EXPECT_TRUE(m_logger->debug(MSG_SINGLE_LINE));
        } else if (strcmp(LOG_INFO, level) == 0) {
            EXPECT_TRUE(m_logger->info(MSGID_TEST, MSG_SINGLE_LINE));
        } else if (strcmp(LOG_WARNING,level) == 0) {
            EXPECT_TRUE(m_logger->warning(MSGID_TEST, MSG_SINGLE_LINE));
        } else if (strcmp(LOG_ERROR, level) == 0) {
            EXPECT_TRUE(m_logger->error(MSGID_TEST, MSG_SINGLE_LINE));
        }
    }

    void whenWriteLogWithFormat(const char* level)
    {
        if (strcmp(LOG_VERBOSE, level) == 0) {
            EXPECT_TRUE(m_logger->verbose(MSG_SINGLE_LINE_FORMAT, "line number", 1));
        } else if (strcmp(LOG_DEBUG, level) == 0) {
            EXPECT_TRUE(m_logger->debug(MSG_SINGLE_LINE_FORMAT, "line number", 1));
        } else if (strcmp(LOG_INFO, level) == 0) {
            EXPECT_TRUE(m_logger->info(MSGID_TEST, MSG_SINGLE_LINE_FORMAT, "line number", 1));
        } else if (strcmp(LOG_WARNING, level) == 0) {
            EXPECT_TRUE(m_logger->warning(MSGID_TEST, MSG_SINGLE_LINE_FORMAT, "line number", 1));
        } else if (strcmp(LOG_ERROR, level) == 0) {
            EXPECT_TRUE(m_logger->error(MSGID_TEST, MSG_SINGLE_LINE_FORMAT, "line number", 1));
        }
    }

    void whenWriteMultipleLineLog(const char* level, int count)
    {
        for (int i = 1 ; i <= count ; ++i) {
            if (strcmp(LOG_VERBOSE, level) == 0) {
                EXPECT_TRUE(m_logger->verbose(MSG_SINGLE_LINE_FORMAT, "line number", i));
            } else if (strcmp(LOG_DEBUG, level) == 0) {
                EXPECT_TRUE(m_logger->debug(MSG_SINGLE_LINE_FORMAT, "line number", i));
            } else if (strcmp(LOG_INFO, level) == 0) {
                EXPECT_TRUE(m_logger->info(MSGID_TEST, MSG_SINGLE_LINE_FORMAT, "line number", i));
            } else if (strcmp(LOG_WARNING, level) == 0) {
                EXPECT_TRUE(m_logger->warning(MSGID_TEST, MSG_SINGLE_LINE_FORMAT, "line number", i));
            } else if (strcmp(LOG_ERROR, level) == 0) {
                EXPECT_TRUE(m_logger->error(MSGID_TEST, MSG_SINGLE_LINE_FORMAT, "line number", i));
            }
        }
    }

    void thenExistLog(const char* level, const char* target)
    {
        EXPECT_TRUE(m_logger->isExistLog(level, target));
    }

    void thenIsNotExistLog(const char* level, const char* target)
    {
        EXPECT_FALSE(m_logger->isExistLog(level, target));
    }

    Logger* m_logger;

    const char* MSG_SINGLE_LINE_FORMAT = "formatted message - str : %s, int : %d";
    const char* MSG_SINGLE_LINE = "single line message";
    const char* MSG_MULTI_LINE = "Line number : ";
    const char* LOG_FILE_PATH = PATH_OUTPUT "/log";
    const char* LOG_VERBOSE = "verbose";
    const char* LOG_DEBUG = "debug";
    const char* LOG_INFO = "info";
    const char* LOG_WARNING = "warning";
    const char* LOG_ERROR = "error";
};

TEST_F(UnittestLogger, singletonMethodTest)
{
    EXPECT_NE(nullptr, Logger::getInstance());
    EXPECT_EQ(Logger::getInstance(), Logger::getInstance());
}

TEST_F(UnittestLogger, checkInitialization)
{
    EXPECT_EQ(LogType_PmLog, m_logger->getLogType());
    EXPECT_STREQ("", m_logger->getLogFilePath().c_str());
    EXPECT_STREQ("", m_logger->getLogFromMemory().c_str());
}

TEST_F(UnittestLogger, setTypeVerification)
{
    m_logger->setLogType(LogType_Console);
    EXPECT_EQ(LogType_Console, m_logger->getLogType());
}

TEST_F(UnittestLogger, writeMemoryTypeDebugLog)
{
    givenSetLogType(LogType_Memory);
    whenWriteLog(LOG_DEBUG);
    thenExistLog(LOG_DEBUG, MSG_SINGLE_LINE);
}

TEST_F(UnittestLogger, writeMemoryTypeDebugLogWithFormat)
{
    givenSetLogType(LogType_Memory);
    whenWriteLogWithFormat(LOG_DEBUG);
    thenExistLog(LOG_DEBUG, "formatted message - str : line number, int : 1");
}

TEST_F(UnittestLogger, writeMemoryTypeDebugLogs)
{
    givenSetLogType(LogType_Memory);
    whenWriteMultipleLineLog(LOG_DEBUG, 10);
    thenExistLog(LOG_DEBUG, "formatted message - str : line number, int : 5");
}

TEST_F(UnittestLogger, setLogFilePath)
{
    EXPECT_TRUE(m_logger->setLogType(LogType_File, LOG_FILE_PATH));
    EXPECT_STREQ(LOG_FILE_PATH, m_logger->getLogFilePath().c_str());
}

TEST_F(UnittestLogger, setFileLogTypeWithoutFilePath)
{
    EXPECT_FALSE(m_logger->setLogType(LogType_File));
}

TEST_F(UnittestLogger, writeFileTypeDebugLog)
{
    givenSetLogType(LogType_File);
    whenWriteLog(LOG_DEBUG);
    thenExistLog(LOG_DEBUG, MSG_SINGLE_LINE);
}

TEST_F(UnittestLogger, writeFileTypeDebugLogWithFormat)
{
    givenSetLogType(LogType_File);
    whenWriteLogWithFormat(LOG_DEBUG);
    thenExistLog(LOG_DEBUG, "formatted message - str : line number, int : 1");
}

TEST_F(UnittestLogger, writeFileTypeDebugLogs)
{
    givenSetLogType(LogType_File);
    whenWriteMultipleLineLog(LOG_DEBUG, 10);
    thenExistLog(LOG_DEBUG, "formatted message - str : line number, int : 5");
}

TEST_F(UnittestLogger, changeLogType)
{
    givenSetLogType(LogType_File);
    whenWriteMultipleLineLog(LOG_DEBUG, 10);
    thenExistLog(LOG_DEBUG, "formatted message - str : line number, int : 5");

    givenSetLogType(LogType_Memory);
    whenWriteMultipleLineLog(LOG_DEBUG, 10);
    thenExistLog(LOG_DEBUG, "formatted message - str : line number, int : 10");

    givenSetLogType(LogType_File);
    whenWriteMultipleLineLog(LOG_DEBUG, 10);
    thenExistLog(LOG_DEBUG, "formatted message - str : line number, int : 1");
}

TEST_F(UnittestLogger, writeMemoryTypeInfoLog)
{
    givenSetLogType(LogType_Memory);
    whenWriteLog(LOG_INFO);
    thenExistLog(LOG_INFO, MSG_SINGLE_LINE);
}

TEST_F(UnittestLogger, writeMemoryTypeInfoLogWithFormat)
{
    givenSetLogType(LogType_Memory);
    whenWriteLog(LOG_INFO);
    thenExistLog(LOG_INFO, MSG_SINGLE_LINE);
}

TEST_F(UnittestLogger, writeMemoryTypeInfoLogs)
{
    givenSetLogType(LogType_Memory);
    whenWriteMultipleLineLog(LOG_INFO, 10);
    thenExistLog(LOG_INFO, "formatted message - str : line number, int : 10");
}

TEST_F(UnittestLogger, writeConsole)
{
    givenSetLogType(LogType_Console);
    whenWriteLog(LOG_DEBUG);
    whenWriteLog(LOG_INFO);
    EXPECT_FALSE(m_logger->verbose(MSG_SINGLE_LINE));
}

TEST_F(UnittestLogger, writeFileTypeInfoLog)
{
    givenSetLogType(LogType_File);
    whenWriteLog(LOG_INFO);
    thenExistLog(LOG_INFO, MSG_SINGLE_LINE);
}

TEST_F(UnittestLogger, writeFileTypeInfoLogWithFormat)
{
    givenSetLogType(LogType_File);
    whenWriteLogWithFormat(LOG_INFO);
    thenExistLog(LOG_INFO, "formatted message - str : line number, int : 1");
}

TEST_F(UnittestLogger, writeFileTypeInfoLogs)
{
    givenSetLogType(LogType_File);
    whenWriteMultipleLineLog(LOG_INFO, 10);
    thenExistLog(LOG_INFO, "formatted message - str : line number, int : 7");
}

TEST_F(UnittestLogger, chagneLogLevel)
{
    givenSetLogType(LogType_Memory);

    whenWriteLog(LOG_DEBUG);
    whenWriteLog(LOG_INFO);
    whenWriteLog(LOG_WARNING);
    whenWriteLog(LOG_ERROR);

    thenExistLog(LOG_ERROR, MSG_SINGLE_LINE);
    thenExistLog(LOG_WARNING, MSG_SINGLE_LINE);
    thenExistLog(LOG_INFO, MSG_SINGLE_LINE);
    thenExistLog(LOG_DEBUG, MSG_SINGLE_LINE);
}

TEST_F(UnittestLogger, setGetLogLevel)
{
    m_logger->setLogLevel(LogLevel_Info);
    EXPECT_EQ(m_logger->getLogLevel(), LogLevel_Info);
}

TEST_F(UnittestLogger, printInfoLogLevel)
{
    givenSetLogType(LogType_Memory);

    m_logger->setLogLevel(LogLevel_Info);

    EXPECT_FALSE(m_logger->debug(MSG_SINGLE_LINE));
    EXPECT_FALSE(m_logger->verbose(MSG_SINGLE_LINE));
    whenWriteLog(LOG_INFO);
    whenWriteLog(LOG_WARNING);
    whenWriteLog(LOG_ERROR);

    thenExistLog(LOG_ERROR, MSG_SINGLE_LINE);
    thenExistLog(LOG_INFO, MSG_SINGLE_LINE);
    thenExistLog(LOG_WARNING, MSG_SINGLE_LINE);
    thenIsNotExistLog(LOG_DEBUG, MSG_SINGLE_LINE);
    thenIsNotExistLog(LOG_VERBOSE, MSG_SINGLE_LINE);
}

TEST_F(UnittestLogger, printERRORLogLevel)
{
    givenSetLogType(LogType_Memory);

    m_logger->setLogLevel(LogLevel_Error);

    EXPECT_TRUE(m_logger->error(MSGID_TEST, MSG_SINGLE_LINE));
    EXPECT_FALSE(m_logger->warning(MSGID_TEST, MSG_SINGLE_LINE));
    EXPECT_FALSE(m_logger->info(MSGID_TEST, MSG_SINGLE_LINE));
    EXPECT_FALSE(m_logger->debug(MSG_SINGLE_LINE));

    thenIsNotExistLog(LOG_DEBUG, MSG_SINGLE_LINE);
    thenIsNotExistLog(LOG_INFO, MSG_SINGLE_LINE);
    thenIsNotExistLog(LOG_WARNING, MSG_SINGLE_LINE);
    thenExistLog(LOG_ERROR, MSG_SINGLE_LINE);
}

TEST_F(UnittestLogger, printVerbose)
{
    givenSetLogType(LogType_Console);

    m_logger->setLogLevel(LogLevel_Verbose);

    whenWriteLog(LOG_VERBOSE);
    whenWriteLog(LOG_DEBUG);
    whenWriteLog(LOG_INFO);
    whenWriteLog(LOG_WARNING);
    whenWriteLog(LOG_ERROR);
}
