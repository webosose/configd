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

#include <gtest/gtest.h>
#include <pbnjson.hpp>

#include "Environment.h"
#include "config/Matcher.h"
#include "database/JsonDB.h"

using namespace pbnjson;
using namespace std;

class UnittestMatcher : public testing::Test {
protected:
    UnittestMatcher()
    {
    }

    UnittestMatcher(JValue where)
    {
        m_where = where.duplicate();
    }

    ~UnittestMatcher()
    {
    }

    void givenDB()
    {
        m_database = Object();
        m_jsonDB.insert(CATEGORY_NAME, MATCHED_CONFIG_KEY, MATCHED_CONFIG_VALUE);
        m_database.put(CATEGORY_NAME, Object());
        m_database[CATEGORY_NAME].put(MATCHED_CONFIG_KEY, MATCHED_CONFIG_VALUE);
    }

    void givenValidWhere()
    {
        m_where = pbnjson::Object();
        m_where.put("op", VALID_OPERATION);
        m_where.put("val", MATCHED_CONFIG_VALUE);
        m_where.put("prop", MATCHED_CONFIG_KEY_FULL);
    }

    void givenInvalidWhere()
    {
        m_where = pbnjson::Object();
        m_where.put("op", INVALID_OPERATION);
        m_where.put("val", MATCHED_CONFIG_VALUE);
        m_where.put("prop", MATCHED_CONFIG_KEY_FULL);
    }

    JValue m_where;
    JsonDB m_jsonDB;
    JValue m_database;
    const string CATEGORY_NAME = "com.webos.test1";

    const string MATCHED_CONFIG_KEY = "existed";
    const string MATCHED_CONFIG_KEY_FULL = CATEGORY_NAME + "." + "existed";
    const string MATCHED_CONFIG_VALUE = "value";
    const string UNMATCHED_CONFIG_VALUE = "unmatched value";

    const string INVALID_CONFIG_KEY = "not existed";
    const string INVALID_CONFIG_KEY_FULL = MATCHED_CONFIG_KEY + "." + "not existed";
    const string VALID_OPERATION = "=";
    const string INVALID_OPERATION = "*";
};

TEST_F(UnittestMatcher, isInvalid)
{
    givenDB();
    givenInvalidWhere();
    Matcher condition(m_where);
    ASSERT_FALSE(condition.validateCondition());
}

TEST_F(UnittestMatcher, isMatched)
{
    givenDB();
    givenValidWhere();

    Matcher condition(m_where);
    ASSERT_TRUE(condition.checkCondition(&m_jsonDB));
}

TEST_F(UnittestMatcher, isUnmatched)
{
    givenDB();
    givenValidWhere();
    m_where.put("val", UNMATCHED_CONFIG_VALUE);

    Matcher condition(m_where);
    ASSERT_FALSE(condition.checkCondition(&m_jsonDB));
}

TEST_F(UnittestMatcher, isNotExistProp)
{
    givenDB();
    givenValidWhere();
    m_where.put("prop", INVALID_CONFIG_KEY_FULL);

    Matcher condition(m_where);
    ASSERT_FALSE(condition.checkCondition(&m_jsonDB));
}
