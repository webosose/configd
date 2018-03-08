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

#include <gtest/gtest.h>
#include <pbnjson.hpp>

#include "Environment.h"
#include "util/Json.h"

using namespace pbnjson;
using namespace std;

class UnittestJson : public testing::Test {
protected:
    UnittestJson()
    {
        m_array = pbnjson::Array();
    }

    virtual ~UnittestJson()
    {

    }

    void givenThreeSizeArray()
    {
       ASSERT_TRUE(m_array.append("item1"));
       ASSERT_TRUE(m_array.append("item2"));
       ASSERT_TRUE(m_array.append("item3"));
    }

    JValue m_array;
};

TEST_F(UnittestJson, addUniqueString)
{
    givenThreeSizeArray();

    Json::addUniqueStrIntoArray(m_array, "item4");
    ASSERT_TRUE(m_array.arraySize() == 4);
    Json::addUniqueStrIntoArray(m_array, "item5");
    ASSERT_TRUE(m_array.arraySize() == 5);
    Json::addUniqueStrIntoArray(m_array, "item6");
    ASSERT_TRUE(m_array.arraySize() == 6);
}

TEST_F(UnittestJson, addDuplicatedString)
{
    givenThreeSizeArray();

    Json::addUniqueStrIntoArray(m_array, "item1");
    ASSERT_TRUE(m_array.arraySize() == 3);
    Json::addUniqueStrIntoArray(m_array, "item2");
    ASSERT_TRUE(m_array.arraySize() == 3);
    Json::addUniqueStrIntoArray(m_array, "item3");
    ASSERT_TRUE(m_array.arraySize() == 3);
}

TEST_F(UnittestJson, addSpecialCharacter)
{
    givenThreeSizeArray();

    Json::addUniqueStrIntoArray(m_array, " ");
    ASSERT_TRUE(m_array.arraySize() == 4);

    Json::addUniqueStrIntoArray(m_array, "\n");
    ASSERT_TRUE(m_array.arraySize() == 5);
}

TEST_F(UnittestJson, removeExistString)
{
    givenThreeSizeArray();

    Json::removeUniqueStrIntoArray(m_array, "item1");
    ASSERT_TRUE(m_array.arraySize() == 2);
    Json::removeUniqueStrIntoArray(m_array, "item2");
    ASSERT_TRUE(m_array.arraySize() == 1);
    Json::removeUniqueStrIntoArray(m_array, "item3");
    ASSERT_TRUE(m_array.arraySize() == 0);
}

TEST_F(UnittestJson, removeNotExistString)
{
    givenThreeSizeArray();

    Json::removeUniqueStrIntoArray(m_array, "item4");
    ASSERT_TRUE(m_array.arraySize() == 3);
    Json::removeUniqueStrIntoArray(m_array, "\n");
    ASSERT_TRUE(m_array.arraySize() == 3);
    Json::removeUniqueStrIntoArray(m_array, " ");
    ASSERT_TRUE(m_array.arraySize() == 3);
}

TEST_F(UnittestJson, getValueWithKeys)
{
    JValue root = pbnjson::JDomParser::fromString("{ \"grandParent\": { \"parent\": { \"boy\": \"test\", \"girl\": \"test2\" } } }");
    JValue keys = pbnjson::Array();
    JValue result;

    ASSERT_TRUE(keys.append("grandParent"));
    ASSERT_TRUE(Json::getValueWithKeys(root, keys, result));
    ASSERT_TRUE(result.hasKey("parent"));
    ASSERT_TRUE(keys.append("parent"));
    ASSERT_TRUE(Json::getValueWithKeys(root, keys, result));
    ASSERT_TRUE(result.hasKey("boy"));
    ASSERT_TRUE(result.hasKey("girl"));
    ASSERT_TRUE(keys.append("girl"));
    ASSERT_TRUE(Json::getValueWithKeys(root, keys, result));
    ASSERT_STREQ(result.asString().c_str(), "test2");
}
