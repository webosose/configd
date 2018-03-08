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

#ifndef _IHANDLE_H_
#define _IHANDLE_H_

#include <iostream>

#include <pbnjson.hpp>

using namespace std;
using namespace pbnjson;

class ICall {
public:
    ICall() {};
    virtual ~ICall() {};

    virtual bool isActive() = 0;
    virtual void cancel() = 0;

};

class IHandleListener {
public:
    IHandleListener() { };
    virtual ~IHandleListener() {};

    virtual void onReceiveCall(JValue &response) = 0;
};

class IHandle {
public:
    IHandle() {};
    virtual ~IHandle() {};

    virtual bool connect(const string &name) = 0;
    virtual void addMethods(const char *category, const LSMethod *methods) = 0;
    virtual void addSignals(const string &category, const LSSignal *signals) = 0;
    virtual void addData(const string &name, void *data) = 0;
    virtual void attach(GMainLoop *loop) = 0;
    virtual void sendSignal(const string &name, const string &payload) = 0;
    virtual bool handleSubscription(LSMessage *message, bool &subscribed) = 0;
    virtual shared_ptr<ICall> call(string method, string payload, IHandleListener* listener) = 0;
};

#endif /* _IHANDLE_H_ */
