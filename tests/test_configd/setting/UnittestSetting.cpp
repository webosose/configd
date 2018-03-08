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

#include <pbnjson.hpp>
#include <gtest/gtest.h>
#include <setting/Setting.h>
#include <service/MockAbstractBusFactory.h>

#include "Environment.h"
#include "util/Logger.hpp"

using namespace pbnjson;
using namespace std;

using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;

#define TEST_DATA_PATH "tests/test_configd/setting/_data"

class UnittestSetting : public testing::Test {
protected:
    UnittestSetting()
        : m_setting(Setting::getInstance())
    {
    }

    virtual ~UnittestSetting()
    {
        m_setting.clearSettings();
    }

    void givenLoadDefaultSetting()
    {
        EXPECT_TRUE(m_setting.loadSetting(TEST_DATA_PATH "/setting.json"));
    }
    MockAbstractBusFactory m_factory;
    Setting &m_setting;
};

TEST_F(UnittestSetting, checkDefaultSettingValue)
{
    EXPECT_EQ(LogType_PmLog, m_setting.getLogType());
    EXPECT_EQ(LogLevel_Info, m_setting.getLogLevel());
    EXPECT_STREQ("", m_setting.getLogPath().c_str());
}

TEST_F(UnittestSetting, checkBootStatus)
{
    IHandleListener *listener;
    JValue response = pbnjson::Object();

    EXPECT_FALSE(m_setting.isNormalStatus());
    EXPECT_CALL(*m_factory.getMockIHandle(), call(_, _, _))
                .WillOnce(DoAll(SaveArg<2>(&listener), Return(make_shared<MockICall>())));

    response.put("bootStatus", "normal");
    m_setting.initialize();

    listener->onReceiveCall(response);
    EXPECT_TRUE(m_setting.isNormalStatus());
}

TEST_F(UnittestSetting, checkLoadSettingValue)
{
    givenLoadDefaultSetting();

    EXPECT_EQ(LogType_Memory, m_setting.getLogType());
    EXPECT_EQ(LogLevel_Warning, m_setting.getLogLevel());
    EXPECT_STREQ("", m_setting.getLogPath().c_str());
}

TEST_F(UnittestSetting, updateSettingFromFile)
{
    givenLoadDefaultSetting();

    EXPECT_TRUE(m_setting.loadSetting(TEST_DATA_PATH "/setting_for_logging.json"));

    EXPECT_EQ(LogType_File, m_setting.getLogType());
    EXPECT_EQ(LogLevel_Debug, m_setting.getLogLevel());
    EXPECT_STREQ("tests/output/log", m_setting.getLogPath().c_str());
}
