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

#ifndef _FAKEHANDLE_H_
#define _FAKEHANDLE_H_

#include <iostream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <luna-service2/lunaservice.hpp>
#include <pbnjson.hpp>

#include "service/AbstractBusFactory.h"

using namespace std;
using namespace LS;
using namespace pbnjson;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SetArgReferee;

struct LSMessage {

};

class MockIHandle : public IHandle {
public:
    MOCK_METHOD1(connect, bool(const string &name));
    MOCK_METHOD2(addMethods, void(const char *category, const LSMethod *methods));
    MOCK_METHOD2(addSignals, void(const string &category, const LSSignal *signals));
    MOCK_METHOD2(addData, void(const string &name, void *data));
    MOCK_METHOD1(attach, void(GMainLoop *loop));
    MOCK_METHOD2(sendSignal, void(const string &name, const string &payload));
    MOCK_METHOD2(handleSubscription, bool(LSMessage *message, bool &subscribed));
    MOCK_METHOD3(call, shared_ptr<ICall>(string method, string payload, IHandleListener* listener));
};

class MockIMessage : public IMessage {
public:
    MOCK_METHOD0(getPayload, string());
    MOCK_METHOD0(isSubscription, bool());
    MOCK_METHOD1(respond, void(JValue response));
    MOCK_METHOD0(clientName, string());

    MockIMessage()
    {

    }

    LSMessage& getLSMessage()
    {
        return m_message;
    }

    void givenMessage(JValue &payload)
    {
        m_payload = payload.stringify();

        // m_mockIMessage
        ON_CALL(*this, getPayload())
            .WillByDefault(ReturnPointee(&m_payload));
        ON_CALL(*this, getPayload())
            .WillByDefault(ReturnPointee(&m_payload));
        ON_CALL(*this, isSubscription())
            .WillByDefault(Return(false));
    }

private:
    LSMessage m_message;
    string m_payload;
};

class MockIMessages : public IMessages {
public:
    MOCK_METHOD1(pushMessage, bool(shared_ptr<IMessage> message));
    MOCK_METHOD3(each, bool(IMessagesListener &listener, JsonDB &newDB, JsonDB &oldDB));

    MockIMessages()
    {
        ON_CALL(*this, pushMessage(_))
            .WillByDefault(Return(true));
    }

    void givenMessage(shared_ptr<IMessage> message)
    {

    }
};

class MockICall : public ICall {
public:
    MOCK_METHOD0(isActive, bool());
    MOCK_METHOD0(cancel, void());

    MockICall()
    {
    }
};

class MockAbstractBusFactory : public AbstractBusFactory {
public:
    MOCK_METHOD0(getIHandle, shared_ptr<IHandle>());
    MOCK_METHOD1(getIMessage, shared_ptr<IMessage>(LSMessage *msg));
    MOCK_METHOD1(getIMessages, shared_ptr<IMessages>(string key));
    MOCK_METHOD0(getICall, shared_ptr<ICall>());

    MockAbstractBusFactory()
        : m_mockIHandle(new MockIHandle()),
          m_mockIMessage(new MockIMessage()),
          m_mockIMessages(new MockIMessages())
    {
        AbstractBusFactory::setInstance(this);

        ON_CALL(*this, getIHandle())
            .WillByDefault(Return(m_mockIHandle));
        ON_CALL(*this, getIMessage(_))
            .WillByDefault(Return(m_mockIMessage));
        ON_CALL(*this, getIMessages("getConfigs"))
            .WillByDefault(Return(m_mockIMessages));

        m_mockIMessages->givenMessage(m_mockIMessage);
    }

    shared_ptr<MockIHandle> getMockIHandle()
    {
        return m_mockIHandle;
    }

    shared_ptr<MockIMessage> getMockIMessage()
    {
        return m_mockIMessage;
    }

    shared_ptr<MockIMessages> getMockIMessages()
    {
        return m_mockIMessages;
    }

    LSMessage& getMessage()
    {
        return m_mockIMessage->getLSMessage();
    }

private:
    shared_ptr<MockIHandle> m_mockIHandle;
    shared_ptr<MockIMessage> m_mockIMessage;
    shared_ptr<MockIMessages> m_mockIMessages;

};

#endif /* _FAKEHANDLE_H_ */
