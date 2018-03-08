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

/*
 * Test for Utils.{h,c}
 */

#include "Utils.h"  // The file to test

#include "TestUtil.h"
#include "JUtil.h"

#include <pbnjson.h>

const static char *kSampleSettingsServiceReply =
    "{"
    "    \"category\": \"option\","
    "    \"method\": \"getSystemSettings\","
    "    \"settings\": {"
    "        \"country\": \"KOR\","
    "        \"smartServiceCountryCode3\": \"KOR\""
    "    },"
    "    \"a\": { \"deep\": { \"deep\": { \"deep\": { \"object\": { \"for\": { \"test\": \"OK\"}}}}}},"
    "    \"returnValue\": true"
    "}";

const static char *kSampleLunaSelectorKey =
    "{"
    "    \"invalidCategory\" : [\"country\"],"
    "    \"emptyArray\" : [],"
    "    \"invalidKey\" : [\"settings\", \"country\", \"KOR\"],"
    "    \"validCase\" : [\"settings\", \"country\"],"
    "    \"deepArray\" : [\"a\", \"deep\", \"deep\", \"deep\", \"object\", \"for\", \"test\"],"
    "    \"string\" : \"country\","
    "    \"object\" : {"
    "         \"country\":\"settings\""
    "    }"
    "}";

class UtilsTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        settings_reply_json = JUtil::parse_json_c(kSampleSettingsServiceReply);
        luna_selector_key_json = JUtil::parse_json_c(kSampleLunaSelectorKey);
    }

    virtual void TearDown()
    {
        j_release(&settings_reply_json);
        j_release(&luna_selector_key_json);
    }

    jvalue_ref settings_reply_json;
    jvalue_ref luna_selector_key_json;
};

TEST_F(UtilsTest, configdJobjectGetExistsRecursive)
{
    jvalue_ref ret_value = NULL;
    EXPECT_TRUE(configdJobjectGetExistsRecursive(settings_reply_json, J_CSTR_TO_BUF("country"), &ret_value));
    EXPECT_TRUE(jstring_equal2(ret_value, J_CSTR_TO_BUF("KOR")));
}

TEST_F(UtilsTest, configdJobjectGetExists)
{
    jvalue_ref ret_value = NULL;

    jvalue_ref key1Obj = jobject_get(luna_selector_key_json, J_CSTR_TO_BUF("invalidCategory"));
    EXPECT_FALSE(configdJobjectGetExists(settings_reply_json, key1Obj, &ret_value));

    jvalue_ref key2Obj = jobject_get(luna_selector_key_json, J_CSTR_TO_BUF("emptyArray"));
    EXPECT_FALSE(configdJobjectGetExists(settings_reply_json, key2Obj, &ret_value));

    jvalue_ref key3Obj = jobject_get(luna_selector_key_json, J_CSTR_TO_BUF("invalidKey"));
    EXPECT_FALSE(configdJobjectGetExists(settings_reply_json, key3Obj, &ret_value));

    ret_value = NULL;
    jvalue_ref key4Obj = jobject_get(luna_selector_key_json, J_CSTR_TO_BUF("validCase"));
    EXPECT_TRUE(configdJobjectGetExists(settings_reply_json, key4Obj, &ret_value));
    EXPECT_TRUE(jstring_equal2(ret_value, J_CSTR_TO_BUF("KOR")));

    ret_value = NULL;
    jvalue_ref key5Obj = jobject_get(luna_selector_key_json, J_CSTR_TO_BUF("deepArray"));
    EXPECT_TRUE(configdJobjectGetExists(settings_reply_json, key5Obj, &ret_value));
    EXPECT_TRUE(jstring_equal2(ret_value, J_CSTR_TO_BUF("OK")));

    jvalue_ref key6Obj = jobject_get(luna_selector_key_json, J_CSTR_TO_BUF("string"));
    EXPECT_FALSE(configdJobjectGetExists(settings_reply_json, key6Obj, &ret_value));

    jvalue_ref key7Obj = jobject_get(luna_selector_key_json, J_CSTR_TO_BUF("object"));
    EXPECT_FALSE(configdJobjectGetExists(settings_reply_json, key7Obj, &ret_value));
}

