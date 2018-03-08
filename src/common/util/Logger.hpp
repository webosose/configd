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

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <fstream>
#include <iostream>
#include <iterator>
#include <PmLogLib.h>
#include <sstream>
#include <string.h>
#include <glib.h>

#include "Environment.h"

// TODO: following include needs to be deleted in the future
//       currently, it is used only for PmLogContext
#include "Logging.h"

#define LOG_PREPIX_FORMAT       "(%s, %d) %s : "
#define LOG_PREPIX_ARGS         __FILE__, __LINE__, __FUNCTION__

#define LOG_PREPIX_FORMAT_EXT   "(%s, %d) '%s' : "
#define LOG_PREPIX_ARGS_EXT     __FILE__, __LINE__

#define MSGID_MAIN              "MAIN"
#define MSGID_MANAGER           "MANAGER"
#define MSGID_HANDLER           "HANDLER"

using namespace std;

typedef enum {
    LogType_PmLog = 0,
    LogType_Console,
    LogType_File,
    LogType_Memory
} LogType;

typedef enum {
    LogLevel_Error = 0,
    LogLevel_Warning,
    LogLevel_Info,
    LogLevel_Debug,
    LogLevel_Verbose
} LogLevel;


class Logger {
public:
    static Logger* getInstance()
    {
        static Logger s_logger;
        return &s_logger;
    }

    virtual ~Logger() {};

    void clear()
    {
        m_type = LogType_PmLog;
        m_level = LogLevel_Debug;
        m_logFilePath = "";

        m_strStream.str("");
        m_strStream.clear();

        m_fileStream.flush();
        m_fileStream.clear();
        m_fileStream.close();
    }

    LogType getLogType()
    {
        return m_type;
    }

    bool setLogType(LogType type, string path = "")
    {
        if (type == m_type) {
            return true;
        }

        if (type == LogType_File) {
            if (path.empty() || !setLogFilePath(path)) {
                return false;
            }
        }

        m_type = type;
        return true;
    }

    void setLogLevel(LogLevel lev)
    {
        m_level = lev;
    }

    LogLevel getLogLevel()
    {
        return m_level;
    }

    string getLogFilePath()
    {
        return m_logFilePath;
    }

    bool isExistLog(const char* level, const char* targetStr)
    {
        if (m_type == LogType_File) {
            return findFromFile(level, targetStr);
        } else if (m_type == LogType_Memory) {
            return findFromMemory(level, targetStr);
        }
        return false;
    }

    bool isEmpty()
    {
        if (LogType_Memory == m_type && m_strStream.tellp() == 0) {
            return true;
        }

        if (LogType_File == m_type && m_fileStream.tellp() == 0) {
            return true;
        }
        return false;
    }

    string getLogFromMemory()
    {
        return m_strStream.str();
    }

    template<typename... Ts>
    static bool verbose(const char* format, Ts ... args)
    {
        LogType type = Logger::getInstance()->getLogType();
        if (!(LogType_Console == type || LogType_File == type)) {
            return false;
        }
        return (Logger::getInstance()->checkEnabledLogLevel(LogLevel_Verbose)) ?
                Logger::getInstance()->write("verbose", "", format, args...) : false;
    }

    template<typename... Ts>
    static bool debug(const char* format, Ts... args)
    {
        return (Logger::getInstance()->checkEnabledLogLevel(LogLevel_Debug)) ?
                Logger::getInstance()->write("debug", "", format, args...) : false;
    }

    template<typename... Ts>
    static bool info(const char* msgid, const char* format, Ts... args)
    {
        return (Logger::getInstance()->checkEnabledLogLevel(LogLevel_Info)) ?
                Logger::getInstance()->write("info", msgid, format, args...) : false;
    }

    template<typename ... Ts>
    static bool warning(const char* msgid, const char* format, Ts ... args)
    {
        return (Logger::getInstance()->checkEnabledLogLevel(LogLevel_Warning)) ?
                Logger::getInstance()->write("warning", msgid, format, args...) : false;
    }

    template<typename ... Ts>
    static bool error(const char* msgid, const char* format, Ts ... args)
    {
        return (Logger::getInstance()->checkEnabledLogLevel(LogLevel_Error)) ?
                Logger::getInstance()->write("error", msgid, format, args...) : false;
    }


private:

    bool checkEnabledLogLevel(LogLevel level)
    {
        return (m_level >= level);
    }

    template<typename ... Ts>
    bool write(const char* level, const char* msgid, const char* format, Ts ... args)
    {
        if (m_type == LogType_Console) {
            return writeConsole(level, msgid, format, args...);
        }

        if (m_type == LogType_Memory) {
            return writeMemory(level, msgid, format, args...);
        }

        if (m_type == LogType_File) {
            return writeFile(level, msgid, format, args...);
        }

        if (m_type == LogType_PmLog) {
            if (strcmp(level, "debug") == 0) {
                PmLogMsg(confdServiceLogContext, Debug, NULL, 0, format, args...);
            } else if (strcmp(level, "info") == 0) {
                PmLogMsg(confdServiceLogContext, Info, msgid, 0, format, args...);
            } else if (strcmp(level, "warning") == 0) {
                PmLogMsg(confdServiceLogContext, Warning, msgid, 0, format, args...);
            } else if (strcmp(level, "error") == 0) {
                PmLogMsg(confdServiceLogContext, Error, msgid, 0, format, args...);
            }
            return true;
        }
        return false;
    }

    template<typename... Ts>
    bool writeConsole(const char* logLevel, const char* msgid, const char* format, Ts... args)
    {
        struct timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);

        printf("[%5ld.%09ld] [%-7s] %-15s ", time.tv_sec, time.tv_nsec, logLevel, msgid);
        printf(format, args...);
        printf("\n");
        return true;
    }

    template<typename... Ts>
    bool writeMemory(const char* logLevel, const char* msgid, const char* format, Ts... args)
    {
        if (sizeof...(args) == 0) {
            m_strStream << "[" << logLevel << "] " << msgid << " " << format << endl;
        } else {
            int cnt = 0;
            cnt = snprintf(m_buf, 1024, "[%s] %s ", logLevel, msgid);
            cnt += snprintf(m_buf + strlen(m_buf), 1024 - strlen(m_buf), format, args...);

            if (cnt < 0 || cnt > 1024) {
                return false;
            }

            m_strStream << m_buf << endl;
        }
        return true;
    }

    template<typename... Ts>
    bool writeFile(const char* logLevel, const char* msgid, const char* format, Ts... args)
    {
        struct timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);
        int cnt = 0;

        if (m_fileStream.fail() || !m_fileStream.is_open() || m_logFilePath == "") {
            return false;
        }
        if (sizeof...(args) == 0) {
            m_fileStream << "[" << logLevel << "] " << msgid << " "<< format << endl;
        } else {
            cnt = snprintf(m_buf, 1024, "[%5ld.%09ld] [%-7s] %-15s ", time.tv_sec, time.tv_nsec, logLevel, msgid);
            cnt += snprintf(m_buf + strlen(m_buf), 1024 - strlen(m_buf), format, args...);
            if (cnt < 0 || cnt > 1024) {
                return false;
            }
            m_fileStream << m_buf << endl;
        }
        return true;
    }

    bool findFromFile(const char* level, const char* targetStr)
    {
        ifstream fin(m_logFilePath);
        if (!fin) {
            return false;
        }

        fin.seekg(std::ios::beg);

        string str(1024, '\0');
        while (!fin.fail() || !fin.eof()) {
            fin.getline(&str[0], 1024, '\n');
            if (str.find(targetStr) != string::npos && str.find(level) != string::npos) {
                return true;
            }
        }
        return false;
    }

    bool findFromMemory(const char* level, const char* targetStr)
    {
        m_strStream.seekg(ios_base::beg);
        for (string line ; getline(m_strStream, line, '\n') ; ) {
            if (line.find(targetStr) != string::npos && line.find(level) != string::npos) {
                return true;
            }
        }
        m_strStream.clear();
        return false;
    }

    bool setLogFilePath(string path)
    {
        if (path.empty()) {
            return false;
        }

        if (path == m_logFilePath) {
            return true;
        }

        if (!m_fileStream.fail() && m_fileStream.is_open()) {
            m_fileStream.flush();
            m_fileStream.clear();
            m_fileStream.close();
        }

        m_fileStream.open(path, ios_base::out | ios::app);

        if (!m_fileStream.fail() || m_fileStream.is_open()) {
            m_logFilePath = path;
            return true;
        }
        return false;
    }

    Logger()
        : m_type(LogType_Console),
          m_level(LogLevel_Verbose),
          m_logFilePath("")
    {
    }

    LogType m_type;
    LogLevel m_level;
    stringstream m_strStream;
    ofstream m_fileStream;
    string m_logFilePath;

    char m_buf[1024];
};

#endif /* _LOGGER_H_ */
