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

#include <stdlib.h>

#include <gtest/gtest.h>
#include <gtest/gtest.h>

#include <pbnjson.hpp>
#include <time.h>

#include "Environment.h"
#include "config/Configuration.h"
#include "service/Configd.h"
#include "service/ErrorDB.h"
#include "service/AbstractBusFactory.h"

#include "service/MockAbstractBusFactory.h"
#include "service/MockConfigd.h"

using namespace pbnjson;
using namespace std;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SetArgReferee;

#define TEST_DATA_PATH "tests/test_configd/service/_data"

class UnittestConfigdGetConfigs : public testing::Test {
protected:
    UnittestConfigdGetConfigs()
    {
        Configd::getInstance()->initialize(NULL, &m_listener);
        JsonDB::getUnifiedInstance().load(TEST_DATA_PATH "/configd_db.json");
        JsonDB::getPermissionInstance().load(TEST_DATA_PATH "/configd_permissions_db.json");

        m_expected = pbnjson::Object();
        m_payload = pbnjson::Object();
    }

    virtual ~UnittestConfigdGetConfigs()
    {

    }

    void givenClientRequest(JValue configNames = pbnjson::Array(), bool subscribe = false)
    {
        m_payload.put("configNames", configNames);
        m_payload.put("subscribe", false);
        m_factory.getMockIMessage()->givenMessage(m_payload);
    }

    void whenServiceHander(JValue configs = pbnjson::Object(), JValue missingConfigs = pbnjson::Array())
    {
        if (configs.objectSize() > 0)
            m_expected.put("configs", configs);
        if (missingConfigs.arraySize() > 0)
            m_expected.put("missingConfigs", missingConfigs);
    }

    void thenServiceResponse(bool subscribed, bool returnValue, int errorCode = ErrorDB::ERRORCODE_NOERROR)
    {
        m_expected.put("subscribed", subscribed);
        if (subscribed) {
            EXPECT_CALL(*m_factory.getMockIMessage(), isSubscription())
                .WillOnce(Return(true));
        }

        m_expected.put("returnValue", returnValue);

        if (errorCode == ErrorDB::ERRORCODE_NOERROR) {
            EXPECT_CALL(m_listener, onGetConfigs())
                .WillOnce(Return(ErrorDB::ERRORCODE_NOERROR));
        } else {
            m_expected.put("errorCode", errorCode);
            m_expected.put("errorText", ErrorDB::getErrorText(errorCode));
        }
        EXPECT_CALL(*m_factory.getMockIMessage(), respond(m_expected));
        Configd::getInstance()->getConfigs(m_factory.getMessage());
    }

    void thenSubscriptionResponse(JsonDB &database)
    {
        ON_CALL(*m_factory.getMockIMessage(), respond(m_expected.duplicate()));
        Configd::getInstance()->postGetConfigs(database, JsonDB::getUnifiedInstance());
    }

    MockAbstractBusFactory m_factory;
    MockConfigdListener m_listener;

    JValue m_expected;
    JValue m_payload;
};

TEST_F(UnittestConfigdGetConfigs, hasPermission)
{
    JValue permissions = pbnjson::Object();
    permissions.put(Configd::NAME_GET_PERMISSION, pbnjson::Array());
    permissions[Configd::NAME_GET_PERMISSION].append("app1");
    permissions[Configd::NAME_GET_PERMISSION].append("com.webos.*");
    permissions[Configd::NAME_GET_PERMISSION].append("netflix-*");

    EXPECT_TRUE(Configd::getInstance()->hasPermission(permissions, "com.webos.applicationManager", Configd::NAME_GET_PERMISSION));
    EXPECT_FALSE(Configd::getInstance()->hasPermission(permissions, "app2", Configd::NAME_GET_PERMISSION));
    EXPECT_TRUE(Configd::getInstance()->hasPermission(permissions, "netflix-", Configd::NAME_GET_PERMISSION));
    EXPECT_TRUE(Configd::getInstance()->hasPermission(permissions, "netflix-2", Configd::NAME_GET_PERMISSION));
    EXPECT_FALSE(Configd::getInstance()->hasPermission(permissions, "netflix", Configd::NAME_GET_PERMISSION));

    EXPECT_FALSE(Configd::getInstance()->hasPermission(permissions, "com.webosapplicationManager", Configd::NAME_GET_PERMISSION));

}

TEST_F(UnittestConfigdGetConfigs, msgGetConfigsWithPermission)
{
    JValue configNames = pbnjson::Array();
    configNames.append("com.webos.category1.key1");
    configNames.append("com.webos.category1.key2");
    configNames.append("com.webos.category2.*");

    givenClientRequest(configNames, true);

    JValue configs = pbnjson::Object();
    JValue missingConfigs = pbnjson::Array();

    configs.put("com.webos.category2.key2", "value2");
    configs.put("com.webos.category2.key3", "value3");
    configs.put("com.webos.category1.key2", "value2");

    missingConfigs.append("com.webos.category2.key1");
    missingConfigs.append("com.webos.category1.key1");

    whenServiceHander(configs, missingConfigs);
    thenServiceResponse(true, true);

}

TEST_F(UnittestConfigdGetConfigs, invalidRequestNullPayload)
{
    JValue null;
    givenClientRequest(null);

    thenServiceResponse(false, false, ErrorDB::ERRORCODE_INVALID_PARAMETER);
}

TEST_F(UnittestConfigdGetConfigs, invalidRequestNullRequired)
{
    givenClientRequest(pbnjson::Object(), false);

    thenServiceResponse(false, false, ErrorDB::ERRORCODE_INVALID_PARAMETER);
}

TEST_F(UnittestConfigdGetConfigs, invalidRequestAdditionalParameters)
{
    JValue invalid = pbnjson::Object();
    invalid.put("additional", true);
    givenClientRequest(invalid);

    thenServiceResponse(false, false, ErrorDB::ERRORCODE_INVALID_PARAMETER);
}

TEST_F(UnittestConfigdGetConfigs, validRequestWithEmptyConfigs)
{
    givenClientRequest(pbnjson::Array());

    whenServiceHander();

    thenServiceResponse(false, true);
}

TEST_F(UnittestConfigdGetConfigs, configsFull)
{
    JValue configNames = pbnjson::Array();
    configNames.append("com.webos.app.inputcommon.enableClearVoice");
    configNames.append("com.webos.app.inputcommon.enableSmartSoundMode");
    givenClientRequest(configNames);

    JValue configs = pbnjson::Object();
    configs.put("com.webos.app.inputcommon.enableClearVoice", true);
    configs.put("com.webos.app.inputcommon.enableSmartSoundMode", false);
    whenServiceHander(configs);

    thenServiceResponse(false, true);
}

TEST_F(UnittestConfigdGetConfigs, missingConfigsFull)
{
    JValue configNames = pbnjson::Array();
    configNames.append("missingConfigA");
    configNames.append("missingConfigB");
    givenClientRequest(configNames);

    whenServiceHander(pbnjson::Object(), configNames);

    thenServiceResponse(false, true);
}

TEST_F(UnittestConfigdGetConfigs, missingConfigsPartial)
{
    JValue configNames = pbnjson::Array();
    configNames.append("com.webos.app.inputcommon.enableClearVoice");
    configNames.append("missingConfig");
    givenClientRequest(configNames);

    JValue configs = pbnjson::Object();
    JValue missingConfigs = pbnjson::Array();
    configs.put("com.webos.app.inputcommon.enableClearVoice", true);
    missingConfigs.append("missingConfig");
    whenServiceHander(configs, missingConfigs);

    thenServiceResponse(false, true);
}

TEST_F(UnittestConfigdGetConfigs, starConfigsValid)
{
    JValue configNames = pbnjson::Array();
    configNames.append("com.webos.app.inputcommon.*");
    givenClientRequest(configNames);

    JValue configs = pbnjson::Object();
    configs.put("com.webos.app.inputcommon.enableClearVoice", true);
    configs.put("com.webos.app.inputcommon.enableSmartSoundMode", false);
    whenServiceHander(configs);

    thenServiceResponse(false, true);
}

TEST_F(UnittestConfigdGetConfigs, starConfigsInvalid)
{
    JValue configNames = pbnjson::Array();
    configNames.append("com.webos.app.inputcommon*");
    givenClientRequest(configNames);

    whenServiceHander(pbnjson::Object(), configNames);

    thenServiceResponse(false, true);
}

TEST_F(UnittestConfigdGetConfigs, subscriptionSetting)
{
    JValue configNames = pbnjson::Array();
    configNames.append("com.webos.app.inputcommon.enableClearVoice");
    configNames.append("com.webos.app.inputcommon.enableSmartSoundMode");
    givenClientRequest(configNames, true);

    JValue configs = pbnjson::Object();
    configs.put("com.webos.app.inputcommon.enableClearVoice", true);
    configs.put("com.webos.app.inputcommon.enableSmartSoundMode", false);
    whenServiceHander(configs);

    thenServiceResponse(true, true);
}

TEST_F(UnittestConfigdGetConfigs, subscriptionReply)
{
    JValue configNames = pbnjson::Array();
    configNames.append("com.webos.app.inputcommon.enableClearVoice");
    configNames.append("com.webos.app.inputcommon.enableSmartSoundMode");
    givenClientRequest(configNames, true);

    JValue configs = pbnjson::Object();
    configs.put("com.webos.app.inputcommon.enableClearVoice", true);
    configs.put("com.webos.app.inputcommon.enableSmartSoundMode", false);
    whenServiceHander(configs);

    thenServiceResponse(true, true);

    JValue diffConfigs = pbnjson::Object();
    diffConfigs.put("com.webos.app.inputcommon.enableClearVoice", false);
    whenServiceHander(diffConfigs);

    JsonDB newDB;
    newDB.copy(JsonDB::getUnifiedInstance());
    newDB.insert("com.webos.app.inputcommon", "enableClearVoice", false);

    thenSubscriptionResponse(newDB);
}

TEST_F(UnittestConfigdGetConfigs, subscriptionMutiple)
{
    JValue configNames = pbnjson::Array();
    configNames.append("randomNumberTest.randomNumber");
    givenClientRequest(configNames, true);

    JValue configs = pbnjson::Object();
    configs.put("randomNumberTest.randomNumber", 1);
    whenServiceHander(configs);

    thenServiceResponse(true, true);

    JsonDB newDB;
    newDB.copy(JsonDB::getUnifiedInstance());

    srand(time(NULL));
    for (int i = 0; i < 100; i++) {
        int random = rand() % 10000;
        if (random == m_expected["configs"]["randomNumberTest.randomNumber"].asNumber<int>())
            continue;

        newDB.insert("randomNumberTest", "randomNumber", random);
        m_expected["configs"].put("randomNumberTest.randomNumber", random);

        thenSubscriptionResponse(newDB);

        JsonDB::getUnifiedInstance().copy(newDB);
    }
}
