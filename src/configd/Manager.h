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

#ifndef _MANAGER_H_
#define _MANAGER_H_

#include <iostream>
#include <stdbool.h>
#include <dirent.h>

#include <luna-service2/lunaservice.h>

#include "Environment.h"
#include "config/Configuration.h"
#include "database/JsonDB.h"
#include "service/Configd.h"
#include "util/Timer.h"
#include "util/Logger.hpp"

using namespace std;

class Manager : public ConfigdListener, LayerListener {
public :
    // Time delay from initiate reconfigure to actual reconfigure happened
    // Reconfigure is delayed to avoid calling several times within seconds
    static const int MS_MINIMAL_DELAY = 10;
    static const int MS_DEFAULT_DELAY = 1000;
    static const int MS_LSCALL_DELAY = 10000;

    static Manager* getInstance()
    {
        static Manager _instance;
        return &_instance;
    }

    virtual ~Manager();

    // API callback
    virtual int onGetConfigs();
    virtual int onSetConfigs(JValue configs, bool isVolatile);

    virtual int onDump(JValue &configs);
    virtual int onFullDump(JValue &configs);
    virtual int onReconfigure(int timeout);
    virtual int onReloadConfigs();

    // EVENT callback
    virtual void onSelectionChanged(Layer &layer, string &oldSelection, string &newSelection);

    void initialize();
    void run();
    void printDebug();
    void writeDebugDatabase(string fullname);

private:
    Manager();

    bool load();
    bool reconfigure(bool runPreProcess, bool runPostProcess, int delayTime = 0);
    void updateUnifiedDatabase(string reason);
    void updateFactoryDatabase(JValue configs, bool isVolatile);

    GMainLoop *m_mainLoop;
    Timer m_reconfigureTimer;
    bool m_isLoaded;
};

#endif // _MANAGER_H_

