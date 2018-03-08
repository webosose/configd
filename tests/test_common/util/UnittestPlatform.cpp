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
#include "util/Platform.h"

using namespace pbnjson;
using namespace std;

#define TEST_DATA_PATH "tests/test_common/util/_data"

class UnittestPlatform : public testing::Test {
protected:
    UnittestPlatform()
    {
        if (!Platform::isFileExist(PATH_TEST_OUTPUT)) {
            Platform::writeFile(PATH_TEST_OUTPUT, "test");
        }
    }

    virtual ~UnittestPlatform()
    {
        if (Platform::isFileExist(PATH_TEST_OUTPUT))
            Platform::deleteFile(PATH_TEST_OUTPUT);
    }

    const char* PATH_TEST_FILE = TEST_DATA_PATH "/file";
    const char* PATH_TEST_OUTPUT = PATH_OUTPUT "test.txt";
};

TEST_F(UnittestPlatform, isFileExist)
{
    ASSERT_TRUE(Platform::isFileExist(PATH_OUTPUT "test.txt"));
    ASSERT_TRUE(Platform::isFileExist(PATH_TEST_FILE));
}

TEST_F(UnittestPlatform, isDirExist)
{
    ASSERT_FALSE(Platform::isDirExist("tests/FALSE"));
    ASSERT_FALSE(Platform::isDirExist(PATH_TEST_FILE));
    ASSERT_TRUE(Platform::isDirExist(TEST_DATA_PATH));
}

TEST_F(UnittestPlatform, checkBasicOperationOfReadFile)
{
    EXPECT_STRNE(Platform::readFile(PATH_TEST_FILE).c_str(), "");
    EXPECT_STREQ(Platform::readFile("").c_str(), "");
}

TEST_F(UnittestPlatform, basicFileOperation)
{
    ASSERT_TRUE(Platform::writeFile(PATH_TEST_OUTPUT, "TEST"));
    ASSERT_TRUE(Platform::isFileExist(PATH_TEST_OUTPUT));
    ASSERT_TRUE(Platform::deleteFile(PATH_TEST_OUTPUT));
    ASSERT_FALSE(Platform::isFileExist(PATH_TEST_OUTPUT));
}

TEST_F(UnittestPlatform, fileMoveOperation)
{
    ASSERT_TRUE(Platform::copyFile(PATH_TEST_FILE, PATH_TEST_OUTPUT));
    ASSERT_TRUE(Platform::isFileExist(PATH_TEST_OUTPUT));
    ASSERT_EQ(Platform::readFile(PATH_TEST_FILE), Platform::readFile(PATH_TEST_OUTPUT));
    ASSERT_TRUE(Platform::deleteFile(PATH_TEST_OUTPUT));
    ASSERT_FALSE(Platform::isFileExist(PATH_TEST_OUTPUT));
}

TEST_F(UnittestPlatform, writeAndCompareFileContent)
{
    ASSERT_TRUE(Platform::writeFile(PATH_TEST_OUTPUT, "TEST"));
    string content = Platform::readFile(PATH_TEST_OUTPUT);
    ASSERT_STREQ(content.c_str(), "TEST");
    ASSERT_STRNE(content.c_str(), "TEST_INVALID");
    ASSERT_TRUE(Platform::deleteFile(PATH_TEST_OUTPUT));
}

TEST_F(UnittestPlatform, commandExecution)
{
    string console;
    ASSERT_TRUE(Platform::executeCommand("find .", console));
    ASSERT_FALSE(console.empty());
}

TEST_F(UnittestPlatform, commandInputOutputExecution)
{
    cout << "1111" << endl;
    ASSERT_STREQ(Platform::executeCommand("echo ", "FIRST", "SECOND").c_str(), "FIRST SECOND");
    ASSERT_STRNE(Platform::executeCommand("echo ", "SECOND", "FIRST").c_str(), "FIRST SECOND");
    cout << "0000" << endl;
    EXPECT_STREQ(Platform::executeCommand("ls", "-a", "-l").c_str(),
                 Platform::executeCommand("ls", "-l", "-a").c_str());
    cout << "2222" << endl;
}

TEST_F(UnittestPlatform, trimOperation)
{
    string str;

    str = " XXX";
    ASSERT_STREQ("XXX", Platform::trim(str).c_str());

    str = "XXX ";
    ASSERT_STREQ("XXX", Platform::trim(str).c_str());

    str = " XXX ";
    ASSERT_STREQ("XXX", Platform::trim(str).c_str());

    str = "    XXX    ";
    ASSERT_STREQ("XXX", Platform::trim(str).c_str());
}


TEST_F(UnittestPlatform, concatePathOperation)
{
    string str;

    str = " /parent/ ";
    ASSERT_STREQ("/parent/child", Platform::concatPaths(str, "child").c_str());

    str = " /parent/ ";
    ASSERT_STREQ("/parent/child", Platform::concatPaths(str, "/child").c_str());

    str = " /parent ";
    ASSERT_STREQ("/parent/child", Platform::concatPaths(str, "child").c_str());

    str = " /parent ";
    ASSERT_STREQ("/parent/child", Platform::concatPaths(str, "/child").c_str());

}

TEST_F(UnittestPlatform, extractFileName)
{
    string fname, name, extention;

    fname = "filename";
    Platform::extractFileName(fname, name, extention);
    EXPECT_STREQ(fname.c_str(), name.c_str());

    fname = "filename2.test";
    Platform::extractFileName(fname, name, extention);
    EXPECT_STREQ("filename2", name.c_str());
    EXPECT_STREQ("test", extention.c_str());
}

