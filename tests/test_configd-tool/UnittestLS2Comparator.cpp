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

#include "LS2Comparator.h"
#include "util/Platform.h"

#include "Environment.h"

using namespace pbnjson;
using namespace std;

class UnittestLS2Comparator : public testing::Test {
protected:
    UnittestLS2Comparator()
        : m_comparator(NULL)
    {
        m_comparator = new LS2Comparator();
    }

    virtual ~UnittestLS2Comparator()
    {
        if (m_comparator != NULL)
            delete m_comparator;
    }

    string convertFullFileName(string filename)
    {
        filename = "test_configd-tool/_data/ls-monitor/" + filename;
        return Platform::concatPaths(PATH_TEST_ROOT, filename);
    }

    void givenTwoJson(string a, string b)
    {
        string fileA = convertFullFileName(a);
        string fileB = convertFullFileName(b);

        if (!Platform::isFileExist(fileA) || !Platform::isFileExist(fileB))
            return;

        if (m_comparator != NULL)
            delete m_comparator;
        m_comparator = new LS2Comparator(fileA, fileB);
        m_fileA = fileA;
        m_fileB = fileB;
    }

    void thenFilesExist()
    {
        EXPECT_TRUE(Platform::isFileExist(m_fileA));
        EXPECT_TRUE(Platform::isFileExist(m_fileB));
    }

    void thenFilesLoaded(string a, string b)
    {
        EXPECT_TRUE(m_comparator->isLoaded(convertFullFileName(a)));
        EXPECT_TRUE(m_comparator->isLoaded(convertFullFileName(b)));
    }

    void thenFilesNotLoaded(string a, string b)
    {
        EXPECT_FALSE(m_comparator->isLoaded(convertFullFileName(a)));
        EXPECT_FALSE(m_comparator->isLoaded(convertFullFileName(b)));
    }

    void thenEqual()
    {
        EXPECT_TRUE(m_comparator->isEqual());
    }

    void thenNotEqual()
    {
        EXPECT_FALSE(m_comparator->isEqual());
    }

    string m_fileA;
    string m_fileB;
    LS2Comparator *m_comparator;
};

TEST_F(UnittestLS2Comparator, CheckTestData)
{
    givenTwoJson("Base.json", "Reverse.json");

    thenFilesExist();
}

TEST_F(UnittestLS2Comparator, PositiveIsLoadedTest)
{
    givenTwoJson("Base.json", "Reverse.json");

    thenFilesLoaded("Base.json", "Reverse.json");
}

TEST_F(UnittestLS2Comparator, NegativeIsLoadedTest)
{
    givenTwoJson("Base.json", "Reverse.json");

    thenFilesNotLoaded("BasePartial.json", "ReversePartial.json");
}

TEST_F(UnittestLS2Comparator, LoadedTestWithInvalidFileName)
{
    givenTwoJson("LS2InvalidFileA.json", "LS2InvalidFileB.json");

    thenFilesNotLoaded("LS2InvalidFileA.json", "LS2InvalidFileB.json");
}

TEST_F(UnittestLS2Comparator, ConvertInvalidJsonFile)
{
    string filename = convertFullFileName("LS2InvalidFile.json");
    JValue json = LS2Comparator::convertFileToJValue(filename);

    EXPECT_TRUE(json.isNull());
}

TEST_F(UnittestLS2Comparator, ConvertValidJsonFile)
{
    string filename = convertFullFileName("Base.json");
    JValue json = LS2Comparator::convertFileToJValue(filename);

    EXPECT_LT(1, json.arraySize());
    EXPECT_FALSE(json.isNull());
}

TEST_F(UnittestLS2Comparator, LoadedTestWithInvalidFileFormat)
{
    givenTwoJson("Base.json", "Invalid.json");

    thenFilesNotLoaded("Base.json", "Invalid.json");
}

TEST_F(UnittestLS2Comparator, CompareSameFile)
{
    givenTwoJson("Base.json", "Base.json");

    thenEqual();
}

//TEST_F(UnittestLS2Comparator, CompareReverseFile)
//{
//    givenTwoJson("Base.json", "Reverse.json");
//
//    thenNotEqual();
//}
