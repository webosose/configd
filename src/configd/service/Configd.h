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

#ifndef _CONFIGD_H_
#define _CONFIGD_H_

#include <iostream>
#include <stdint.h>

#include <pbnjson.hpp>
#include <service/AbstractBusFactory.h>

#include "database/JsonDB.h"

using namespace std;
using namespace pbnjson;

class ConfigdListener {
public:
    ConfigdListener() {};
    virtual ~ConfigdListener() {};

    virtual int onGetConfigs() = 0;
    virtual int onSetConfigs(JValue configs, bool isVolatile) = 0;
    virtual int onDump(JValue &configs) = 0;
    virtual int onFullDump(JValue &configs) = 0;
    virtual int onReconfigure(int timeout) = 0;
    virtual int onReloadConfigs() = 0;
};

class Configd : public IMessagesListener {
public:
    static const string NAME_CONFIGD;
    static const string NAME_CONFIGD_SIGNALS;
    static const string NAME_CONFIGD_RELOAD_DONE;
    static const string NAME_GET_PERMISSION;

    static Configd* getInstance()
    {
        static Configd _instance;
        return &_instance;
    }

    virtual ~Configd();

    virtual void initialize(GMainLoop *mainLoop, ConfigdListener *listener);
    virtual void postGetConfigs(JsonDB &newDB, JsonDB &oldDB);
    virtual void sendSignal(const string &name);

    // IMessagesListener
    virtual void eachMessage(shared_ptr<IMessage> message, JsonDB &newDB, JsonDB &oldDB);

    // Configd APIs
    virtual bool getConfigs(LSMessage &message);
    virtual bool reconfigure(LSMessage &message);
    virtual bool setConfigs(LSMessage &message);
    virtual bool hasPermission(JValue permissions, string serviceName, string permissionName);

    JValue splitVolatileConfigs(JValue keys);

    static bool _getConfigs(LSHandle *sh, LSMessage *msg, void *context)
    {
        Configd *configd = (Configd*)context;
        return configd->getConfigs(*msg);
    }

    static bool _reconfigure(LSHandle *sh, LSMessage *msg, void *context)
    {
        Configd *configd = (Configd*)context;
        return configd->reconfigure(*msg);
    }

    static bool _setConfigs(LSHandle *sh, LSMessage *msg, void *context)
    {
        Configd *configd = (Configd*)context;
        return configd->setConfigs(*msg);
    }

protected:
    static const LSMethod METHOD_TABLE[4];
    static const LSSignal SIGNAL_TABLE[2];

    Configd();

    virtual bool msgGetConfigs(JsonDB &db, JsonDB &permissionDB, shared_ptr<IMessage> request, JValue &responsePayload);

    ConfigdListener *m_configdListener;

};

#endif // _CONFIGD_H_
