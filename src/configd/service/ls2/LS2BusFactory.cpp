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

#include <service/ls2/MessageAdapter.h>
#include "service/ls2/LS2BusFactory.h"
#include <memory>

LS2BusFactory::LS2BusFactory()
    : m_iHandle(new HandleAdapter)
{
    m_subscriptions.clear();
}

LS2BusFactory::~LS2BusFactory()
{
    m_subscriptions.clear();
}

void LS2BusFactory::setIHandle(shared_ptr<IHandle> iHandle)
{
    m_iHandle = iHandle;
}

shared_ptr<IHandle> LS2BusFactory::getIHandle()
{
    return m_iHandle;
}

shared_ptr<IMessage> LS2BusFactory::getIMessage(LSMessage *msg)
{
    return make_shared<MessageAdapter>(msg);
}

shared_ptr<IMessages> LS2BusFactory::getIMessages(string key)
{
    if (m_subscriptions.find(key) == m_subscriptions.end()) {
        m_subscriptions[key] = make_shared<LS2MessageContainer>();
        m_subscriptions[key]->setKey(key);
    }
    return m_subscriptions[key];
}

shared_ptr<ICall> LS2BusFactory::getICall()
{
    return make_shared<CallAdapter>();
}
