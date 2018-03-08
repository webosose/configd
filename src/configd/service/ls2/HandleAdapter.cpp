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

#include "HandleAdapter.h"
#include "util/Logger.hpp"
#include "CallAdapter.h"

HandleAdapter::HandleAdapter()
    : m_handle()
{

}

HandleAdapter::~HandleAdapter()
{

}

bool HandleAdapter::connect(const string &name)
{
    m_handle = Handle(name.c_str());
    return true;
}

void HandleAdapter::addMethods(const char *category, const LSMethod *methods)
{
    m_handle.registerCategory(category, methods, nullptr, nullptr);
}

void HandleAdapter::addSignals(const string &category, const LSSignal *signals)
{
    m_handle.registerCategory(category.c_str(), nullptr, signals, nullptr);
}

void HandleAdapter::addData(const string &name, void *data)
{
    m_handle.setCategoryData(name.c_str(), data);
}

void HandleAdapter::attach(GMainLoop *loop)
{
    m_handle.attachToLoop(loop);
}

void HandleAdapter::sendSignal(const string &name, const string &payload)
{
    m_handle.sendSignal(name.c_str(), payload.c_str(), false);
}

bool HandleAdapter::handleSubscription(LSMessage *message, bool &subscribed)
{
    return LSSubscriptionProcess(m_handle.get(), message, &subscribed, NULL);
}

bool HandleAdapter::_replyCallback(LSHandle *sh, LSMessage *reply, void *ctx)
{
    IHandleListener* listener = (IHandleListener*)ctx;
    Message response(reply);
    JValue responsePayload = JDomParser::fromString(response.getPayload());

    Logger::debug(LOG_PREPIX_FORMAT "Subscription (%s)",
                  LOG_PREPIX_ARGS, response.getPayload());
    if (listener)
        listener->onReceiveCall(responsePayload);
    return true;
}

shared_ptr<ICall> HandleAdapter::call(string method, string payload, IHandleListener *listener)
{
    shared_ptr<ICall> sharedCall;
    try {
        Logger::info(MSGID_HANDLER,
                     LOG_PREPIX_FORMAT "Call '%s' payload '%s'",
                     LOG_PREPIX_ARGS, method.c_str(), payload.c_str());

        Call call = m_handle.callMultiReply(
            method.c_str(),
            payload.c_str()
        );
        call.continueWith(_replyCallback, listener);
        sharedCall = make_shared<CallAdapter>(call);
    }
    catch (const LS::Error &e) {
        Logger::error(MSGID_HANDLER, LOG_PREPIX_FORMAT "Exception: %s", LOG_PREPIX_ARGS, e.what());
        return nullptr;
    }
    return sharedCall;
}

Handle& HandleAdapter::getHandle()
{
    return m_handle;
}
