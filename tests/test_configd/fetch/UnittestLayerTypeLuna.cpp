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

#include <config/MockLayer.h>
#include <pbnjson.hpp>
#include <gtest/gtest.h>
#include <service/MockAbstractBusFactory.h>

#include "config/Layer.h"
#include "util/Platform.h"

#include "Environment.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SetArgReferee;
using ::testing::SaveArg;

#define TEST_DATA_PATH "tests/test_configd/fetch/_data"

class UnittestLayerTypeLuna : public testing::Test {
protected:
    UnittestLayerTypeLuna()
        : m_layer(NULL)
    {
        m_info = pbnjson::Object();

        m_responseValid = pbnjson::Object();
        m_responseValid.put("key1", pbnjson::Object());
        m_responseValid["key1"].put("key2", pbnjson::Object());
        m_responseValid["key1"]["key2"].put("key3", "selection1");

        m_responseInvalid = pbnjson::Object();
    }

    virtual ~UnittestLayerTypeLuna()
    {
        if (m_layer != NULL)
            delete m_layer;
    }

    virtual void givenLayer()
    {
        if (m_layer != NULL)
            delete m_layer;

        m_layer = new Layer(m_info);
        m_layer->setListener(&m_listener);
    }

    virtual void givenInfo(string name, bool subscribe)
    {
        m_responseValid.put("subscribed", subscribe);
        m_responseInvalid.put("subscribed", subscribe);

        JValue params = pbnjson::Object();
        params.put("subscribe", subscribe);

        JValue luna_cmd = pbnjson::Object();
        luna_cmd.put("method", "nothing");
        luna_cmd.put("params", params);

        JValue key = pbnjson::Array();
        key.append("key1");
        key.append("key2");
        key.append("key3");

        JValue selector = pbnjson::Object();
        selector.put("key", key);
        selector.put("luna_cmd", luna_cmd);

        m_info.put("selector", selector);
        m_info.put("base_dir", TEST_DATA_PATH);
        m_info.put("name", name);
    }

    virtual void givenOneReplyLayer()
    {
        givenInfo(NAME_ONE_REPLY, false);
        givenLayer();
    }

    virtual void givenMultipleReplyLayer()
    {
        givenInfo(NAME_MULTIPLE_REPLY, true);
        givenLayer();
    }

    virtual void givenMultipleReplyWithAlternativeLayer()
    {
        givenInfo(NAME_MULTIPLE_REPLY_WITH_ALTERNATIVE, true);
        JValue alternative = pbnjson::Object();
        alternative.put("string", SELECTION_SETTING2);
        m_info["selector"].put("alternative", alternative);
        givenLayer();
    }

    const string NAME_ONE_REPLY = "OneReplyType";
    const string NAME_MULTIPLE_REPLY = "MultipleReplyType";
    const string NAME_MULTIPLE_REPLY_WITH_ALTERNATIVE = "MultipleReplyWithAlternativeType";
    const string SELECTION_SETTING1 = "selection1";
    const string SELECTION_SETTING2 = "selection2";

    JValue m_info;
    Layer *m_layer;

    MockLayerListener m_listener;
    MockAbstractBusFactory m_factory;

    JValue m_responseValid;
    JValue m_responseInvalid;
};

TEST_F(UnittestLayerTypeLuna, replyCheckGetters)
{
    givenMultipleReplyLayer();

    EXPECT_FALSE(m_layer->isSelected());
    EXPECT_FALSE(m_layer->isReadOnlyType());

    EXPECT_EQ(SelectorType_Luna, m_layer->getType());
    EXPECT_STREQ(NAME_MULTIPLE_REPLY.c_str(), m_layer->getName().c_str());
    EXPECT_STREQ("", m_layer->getSelection().c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(false).c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(true).c_str());
}

TEST_F(UnittestLayerTypeLuna, replyCheckSelectionResult)
{
    givenMultipleReplyLayer();

    EXPECT_CALL(*m_factory.getMockIHandle(), call(_, _, _))
        .WillOnce(Return(make_shared<MockICall>()));

    EXPECT_TRUE(m_layer->select());
    EXPECT_FALSE(m_layer->isSelected());
    EXPECT_STREQ("", m_layer->getSelection().c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(false).c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(true).c_str());
}

TEST_F(UnittestLayerTypeLuna, replyWithInvalidResponse)
{
    givenMultipleReplyLayer();

    IHandleListener *listener;
    EXPECT_CALL(*m_factory.getMockIHandle(), call(_, _, _))
        .WillOnce(DoAll(SaveArg<2>(&listener), Return(make_shared<MockICall>())));
    EXPECT_CALL(m_listener, onSelectionChanged(_, _, _))
        .Times(0);

    EXPECT_TRUE(m_layer->select());
    EXPECT_FALSE(m_layer->isSelected());

    listener->onReceiveCall(m_responseInvalid);

    EXPECT_FALSE(m_layer->isSelected());
    EXPECT_STREQ("", m_layer->getSelection().c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(false).c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(true).c_str());
}

TEST_F(UnittestLayerTypeLuna, replyWithValidResponse)
{
    IHandleListener *listener;
    givenMultipleReplyLayer();

    EXPECT_CALL(*m_factory.getMockIHandle(), call(_, _, _))
        .WillOnce(DoAll(SaveArg<2>(&listener), Return(make_shared<MockICall>())));

    EXPECT_TRUE(m_layer->select());
    EXPECT_FALSE(m_layer->isSelected());

    EXPECT_CALL(m_listener, onSelectionChanged(_, _, _));

    listener->onReceiveCall(m_responseValid);

    EXPECT_TRUE(m_layer->isSelected());
    EXPECT_STREQ(SELECTION_SETTING1.c_str(), m_layer->getSelection().c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(false).c_str());
    EXPECT_STREQ(Platform::concatPaths(TEST_DATA_PATH, SELECTION_SETTING1).c_str(), m_layer->getFullDirPath(true).c_str());
}

TEST_F(UnittestLayerTypeLuna, multipleReplyWithAlternativeGetters)
{
    givenMultipleReplyWithAlternativeLayer();

    EXPECT_FALSE(m_layer->isSelected());
    EXPECT_FALSE(m_layer->isReadOnlyType());

    EXPECT_EQ(SelectorType_Luna, m_layer->getType());
    EXPECT_STREQ(NAME_MULTIPLE_REPLY_WITH_ALTERNATIVE.c_str(), m_layer->getName().c_str());
    EXPECT_STREQ("", m_layer->getSelection().c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(false).c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(true).c_str());
}

TEST_F(UnittestLayerTypeLuna, multipleReplyWithAlternativeSelect)
{
    givenMultipleReplyWithAlternativeLayer();

    EXPECT_CALL(*m_factory.getMockIHandle(), call(_, _, _))
        .WillOnce(Return(make_shared<MockICall>()));

    EXPECT_TRUE(m_layer->select());
    EXPECT_TRUE(m_layer->isSelected());
    EXPECT_STREQ(SELECTION_SETTING2.c_str(), m_layer->getSelection().c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(false).c_str());
    EXPECT_STREQ(Platform::concatPaths(TEST_DATA_PATH, SELECTION_SETTING2).c_str(), m_layer->getFullDirPath(true).c_str());
}

TEST_F(UnittestLayerTypeLuna, multipleReplyWithAlternativeInvalidResponse)
{
    IHandleListener *listener;
    givenMultipleReplyWithAlternativeLayer();

    EXPECT_CALL(*m_factory.getMockIHandle(), call(_, _, _))
        .WillOnce(DoAll(SaveArg<2>(&listener), Return(make_shared<MockICall>())));

    EXPECT_CALL(m_listener, onSelectionChanged(_, _, _))
        .Times(1);

    EXPECT_TRUE(m_layer->select());
    EXPECT_TRUE(m_layer->isSelected());

    listener->onReceiveCall(m_responseInvalid);

    EXPECT_TRUE(m_layer->isSelected());
    EXPECT_STREQ(SELECTION_SETTING2.c_str(), m_layer->getSelection().c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(false).c_str());
    EXPECT_STREQ(Platform::concatPaths(TEST_DATA_PATH, SELECTION_SETTING2).c_str(), m_layer->getFullDirPath(true).c_str());
}

TEST_F(UnittestLayerTypeLuna, multipleReplyWithAlternativeWithValidResponse)
{
    givenMultipleReplyWithAlternativeLayer();

    IHandleListener *listener;
    EXPECT_CALL(*m_factory.getMockIHandle(), call(_, _, _))
        .WillOnce(DoAll(SaveArg<2>(&listener), Return(make_shared<MockICall>())));
    EXPECT_CALL(m_listener, onSelectionChanged(_, _, _))
        .Times(2);

    EXPECT_TRUE(m_layer->select());
    EXPECT_TRUE(m_layer->isSelected());

    listener->onReceiveCall(m_responseValid);

    EXPECT_TRUE(m_layer->isSelected());
    EXPECT_STREQ(SELECTION_SETTING1.c_str(), m_layer->getSelection().c_str());
    EXPECT_STREQ(TEST_DATA_PATH, m_layer->getFullDirPath(false).c_str());
    EXPECT_STREQ(Platform::concatPaths(TEST_DATA_PATH, SELECTION_SETTING1).c_str(), m_layer->getFullDirPath(true).c_str());
}
