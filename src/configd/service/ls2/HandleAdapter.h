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

#ifndef _HANDLEDE_ADAPTOR_H_
#define _HANDLEDE_ADAPTOR_H_

#include <luna-service2++/handle.hpp>

#include "../AbstractBusFactory.h"
#include "util/ObjectCounter.hpp"

using namespace std;
using namespace LS;

class HandleAdapter : public IHandle, public ObjectCounter<HandleAdapter> {
friend class LS2MessageContainer;
public:
    HandleAdapter();
    virtual ~HandleAdapter();

    virtual bool connect(const string &name);
    virtual void addMethods(const char *category, const LSMethod *methods);
    virtual void addSignals(const string &category, const LSSignal *signals);
    virtual void addData(const string &name, void *data);
    virtual void attach(GMainLoop *loop);
    virtual void sendSignal(const string &name, const string &payload);
    virtual bool handleSubscription(LSMessage *message, bool &subscribed);
    virtual shared_ptr<ICall> call(string method, string payload, IHandleListener* listener);

    virtual Handle& getHandle();
private:
    static bool _replyCallback(LSHandle *sh, LSMessage *reply, void *ctx);

    Handle m_handle;

};

#endif /* _HANDLEDE_ADAPTOR_H_ */
