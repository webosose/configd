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

#include "DBComparator.h"
#include "util/Platform.h"

#include "Environment.h"

using namespace pbnjson;
using namespace std;

class UnittestDBComparator : public testing::Test {
protected:
    UnittestDBComparator()
    {
    }

    virtual ~UnittestDBComparator()
    {
    }

    string convertFullFileName(string filename)
    {
        filename = "test_configd-tool/_data/jsondb/" + filename;
        return Platform::concatPaths(PATH_TEST_ROOT, filename);
    }

    DBComparator m_comparator;
};

TEST_F(UnittestDBComparator, SetFirstFilename)
{
    string filename = convertFullFileName("Base.json");

    EXPECT_TRUE(m_comparator.setBase(filename));
    EXPECT_STREQ(filename.c_str(), m_comparator.getBase().c_str());
}

TEST_F(UnittestDBComparator, GetValidConfig)
{
    string filename = convertFullFileName("Base.json");

    EXPECT_TRUE(m_comparator.setBase(filename));
    JValue result = m_comparator.getConfig("tv.rmm.ttxMode");
    EXPECT_FALSE(result["tv.rmm.ttxMode"].isNull());
}

TEST_F(UnittestDBComparator, GetInvalidConfig)
{
    string filename = convertFullFileName("Base.json");

    EXPECT_TRUE(m_comparator.setBase(filename));
    JValue result = m_comparator.getConfig("tv.rmm.invalid");
    EXPECT_TRUE(result["tv.rmm.invalid"].isNull());
}

TEST_F(UnittestDBComparator, CompareBaseAndBase)
{
    string A = convertFullFileName("Base.json");
    string B = convertFullFileName("Base.json");

    EXPECT_TRUE(m_comparator.setBase(A));
    EXPECT_TRUE(m_comparator.isEqual(B));
}

TEST_F(UnittestDBComparator, EmptyBaseAndEmpty)
{
    string A = convertFullFileName("Empty.json");
    string B = convertFullFileName("Empty.json");

    EXPECT_TRUE(m_comparator.setBase(A));
    EXPECT_TRUE(m_comparator.isEqual(B));
}

TEST_F(UnittestDBComparator, CompareWithEmptyDatabase)
{
    string A = convertFullFileName("Base.json");
    string B = convertFullFileName("Empty.json");

    EXPECT_TRUE(m_comparator.setBase(A));
    EXPECT_FALSE(m_comparator.isEqual(B));
}

TEST_F(UnittestDBComparator, CompareBaseAndInvalid)
{
    string A = convertFullFileName("Base.json");
    string B = convertFullFileName("InValid.json");

    EXPECT_TRUE(m_comparator.setBase(A));
    EXPECT_FALSE(m_comparator.isEqual(B));
}

TEST_F(UnittestDBComparator, CompareBaseAndModifiedArraySize)
{
    string A = convertFullFileName("Base.json");
    string B = convertFullFileName("ModifiedArraySize.json");

    EXPECT_TRUE(m_comparator.setBase(A));
    EXPECT_FALSE(m_comparator.isEqual(B));
}

TEST_F(UnittestDBComparator, CompareBaseAndModifiedBool)
{
    string A = convertFullFileName("Base.json");
    string B = convertFullFileName("ModifiedBool.json");

    EXPECT_TRUE(m_comparator.setBase(A));
    EXPECT_FALSE(m_comparator.isEqual(B));
}

TEST_F(UnittestDBComparator, CompareBaseAndRemovedKey)
{
    string A = convertFullFileName("Base.json");
    string B = convertFullFileName("RemovedKey.json");

    EXPECT_TRUE(m_comparator.setBase(A));
    EXPECT_FALSE(m_comparator.isEqual(B));
}
