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

#ifndef TESTTEMPLATES_H
#define TESTTEMPLATES_H
#include <gtest/gtest.h>
#include <luna-service2/lunaservice.h>
#include <string>
#include <pbnjson.hpp>
#include "TestUtil.h"
#include "FakeLunaServiceProvider.h"

/* Used for initialization and deinitialization of test environment
 * * Initialize local path
 * * Initialize FakeLunaServiceProvider
 * * Hide g_log messages
 * * Initialize ApplicationManager
 */
class TestEnvGuard
{
public:
    TestEnvGuard();
    ~TestEnvGuard();

    void init();
private:
    //! Uses for hiding error messages from standart output
    static void log_function(const gchar */*log_domain*/, GLogLevelFlags /*log_level*/, const gchar */*message*/,
                             gpointer /*user_data*/);
    GLogFunc m_func;
};

/* Used for testing a class inherited from "ServiceBase" classes. (ex:AppMgrService)
 * In each test cases, TestEnvGuard is initialized for providing testing environment like initialization of app paths.
 * You can implement functions "void SetUp()" and "void TearDown()" for custom actions before and after each test case.
 *
 * You can write your own lscallback() to check the response value manually.
 * But there is a more simple way to do this with following approach.
 *
 * You can use TestLSCall() to test some LS2 API with a parameterized callback function (lambda is preffered for conciseness)
 * You should call FakeLSRunLSCall() to simulate LSCall processing (as g_main_loop does)
 * Check the unit test case - TEST_F(TestFakeLS, CheckCallBack)
 *
 * Otherwise, this class provides two helper functions, they internally call FakeLSRunLSCall()
 *  - TestLSCallCheckCB() : It will check if the callback function is called or not.
 *  - TestLSCallWithResponse() : It will check if the callback function is called or not, and it also compare the response of LSCall.
 */
template<typename T, typename GUARD = TestEnvGuard>
class TestFakeLSTemplate : public ::testing::Test
{
public:
    TestFakeLSTemplate()
    {
        m_guard.init();
    }

protected:

    // Access provider class. used only for grabbing service name
    class AccessProvider : public T
    {
    public:
        const char *get_service_name() const
        {
            return T::get_service_name();
        }
    };

    virtual void SetUp()
    {
        T::instance().attach(NULL);
    }

    static bool lscallback(LSHandle *lshandle, LSMessage *message, void *user_data)
    {
        std::function< void (LSMessage *) > *pFuctor = static_cast<std::function< void (LSMessage *) > *>(user_data);
        (*pFuctor)(message);

        return true;
    }

    std::string GetAPI_URI(std::string api_name)
    {
        std::string api_uri = m_class.get_service_name();
        api_uri = "palm://" + api_uri + "/" + api_name;
        return api_uri;
    }

    LSMessageToken TestLSCall(const std::string &api_uri, const std::string &payload,
                              std::function< void (LSMessage *) > &cb)
    {

        LSMessageToken ret_token;

        bool ret = LSCallOneReply(T::instance().getPrivateHandle(),
                                  api_uri.c_str(),
                                  payload.c_str(),
                                  lscallback, &cb, &ret_token, NULL);
        EXPECT_TRUE(ret);
        return ret_token;
    }

    void TestLSCallCheckCB(const std::string &api_uri, const std::string &payload)
    {

        LSMessageToken ret_token;
        bool checkCalled = false;

        std::function< void (LSMessage *) > user_data = [&checkCalled](LSMessage * message) -> void
        {
            checkCalled = true;
        };

        bool ret = LSCallOneReply(T::instance().getPrivateHandle(),
                                  api_uri.c_str(),
                                  payload.c_str(),
                                  lscallback, &user_data, &ret_token, NULL);

        EXPECT_TRUE(ret);
        EXPECT_FALSE(checkCalled);
        FakeLSRunLSCall();
        EXPECT_TRUE(checkCalled);
    }

    void TestLSCallWithResponse(const std::string &api_uri, const std::string &payload, pbnjson::JValue jsonExpected)
    {
        LSMessageToken ret_token;
        bool checkCalled = false;

        std::function< void (LSMessage *) > user_data = [&checkCalled, &jsonExpected](LSMessage * message) -> void
        {
            pbnjson::JValue actual = JUtil::parse(LSMessageGetPayload(message));
            EXPECT_EQ(jsonExpected, actual);

            if (jsonExpected != actual)
            {
                std::string jsonExpectedStr = pbnjson::JGenerator::serialize(jsonExpected, true);
                std::string actualStr = pbnjson::JGenerator::serialize(actual, true);
                EXPECT_STREQ(jsonExpectedStr.c_str(), actualStr.c_str());
            }

            checkCalled = true;
        };

        bool ret = LSCallOneReply(T::instance().getPrivateHandle(),
                                  api_uri.c_str(),
                                  payload.c_str(),
                                  lscallback, &user_data, &ret_token, NULL);

        EXPECT_TRUE(ret);
        EXPECT_FALSE(checkCalled);
        FakeLSRunLSCall();
        EXPECT_TRUE(checkCalled);
    }

protected:
    GUARD m_guard;
    AccessProvider m_class;
};


#endif // TESTTEMPLATES_H

