// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#ifndef _ABSSTRACT_BUS_FACTORY_H_
#define _ABSSTRACT_BUS_FACTORY_H_

#include <iostream>
#include <memory>

#include <luna-service2/lunaservice.hpp>
#include <pbnjson.hpp>

#include "database/JsonDB.h"
#include "interface/IHandle.h"

using namespace std;
using namespace pbnjson;

class IMessage {
public:
    IMessage() {};
    virtual ~IMessage() {};

    virtual string getPayload() = 0;
    virtual string clientName() = 0;
    virtual bool isSubscription() = 0;
    virtual void respond(JValue payload) = 0;
};

class IMessagesListener {
public:
    IMessagesListener() {};
    virtual ~IMessagesListener() {};

    virtual void eachMessage(shared_ptr<IMessage> message, JsonDB &newDB, JsonDB &oldDB) = 0;
};

class IMessages {
public:
    IMessages() {};
    virtual ~IMessages() {};

    virtual bool pushMessage(shared_ptr<IMessage> message) = 0;
    virtual bool each(IMessagesListener& listener, JsonDB &newDB, JsonDB &oldDB) = 0;
};

class AbstractBusFactory {
public:
    static AbstractBusFactory* getInstance()
    {
        return s_instance;
    }

    static void setInstance(AbstractBusFactory *instance)
    {
        s_instance = instance;
    }

    virtual ~AbstractBusFactory() {};

    virtual shared_ptr<IHandle> getIHandle() = 0;
    virtual shared_ptr<IMessage> getIMessage(LSMessage *msg) = 0;
    virtual shared_ptr<IMessages> getIMessages(string key) = 0;
    virtual shared_ptr<ICall> getICall() = 0;

protected:
    static AbstractBusFactory *s_instance;

    AbstractBusFactory() {};

};

#endif /* _ABSSTRACT_BUS_FACTORY_H_ */
