// Copyright (c) 2017-2020 LG Electronics, Inc.
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

#ifndef _SETTING_H_
#define _SETTING_H_

#include <iostream>
#include <initializer_list>
#include <glib.h>
#include <glib/gstdio.h>

#include <luna-service2/lunaservice.h>
#include "service/AbstractBusFactory.h"

#include <pbnjson.hpp>

#include "util/Logger.hpp"

using namespace std;
using namespace pbnjson;

#define CONFIGD_PID_FILE_PATH           INSTALL_RUNTIMEINFODIR "/configd.pid"

class Setting : public IHandleListener {
public:
    static Setting& getInstance()
    {
        static Setting _instance;
        return _instance;
    }

    virtual ~Setting();

    void initialize();
    virtual void onReceiveCall(JValue &response);

    void loadSetting(const string filename);

    // getters
    LogType getLogType();
    LogLevel getLogLevel();
    string getLogPath();

    bool isSnapshotBoot();
    bool isRespawned();
    bool isNormalStatus();

    // debug purpose
    void printSetting();
    void printDebug();
    void clearSettings();

private:
    Setting();

    void parseKernelCmdLine();
    void parsePlatform();
    void applySettings();

    void setSetting(JValue& value, JValue& local);
    JValue getSetting(initializer_list<const char*> list);

    static const char* DEFAULT_SETTING_FILE;
    static const char* DEBUG_SETTING_FILE;

    JValue m_configuration;

    bool m_isSnapshotBoot;
    bool m_isRespawned;
    string m_bootStatus;
    shared_ptr<ICall> m_call;
};

#endif // _SETTING_H_

