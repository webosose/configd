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
#include "service/ErrorDB.h"

using namespace pbnjson;
using namespace std;

class UnittestErrorDB : public testing::Test {
protected:
    UnittestErrorDB()
    {
    }

    virtual ~UnittestErrorDB()
    {
    }
};

TEST_F(UnittestErrorDB, compareCodeAndText)
{
    ASSERT_STREQ(ErrorDB::ERRORTEXT_UNKNOWN, ErrorDB::getErrorText(ErrorDB::ERRORCODE_UNKNOWN));
    ASSERT_STREQ(ErrorDB::ERRORTEXT_INVALID_MAINDB, ErrorDB::getErrorText(ErrorDB::ERRORCODE_INVALID_MAINDB));
    ASSERT_STREQ(ErrorDB::ERRORTEXT_INVALID_PARAMETER, ErrorDB::getErrorText(ErrorDB::ERRORCODE_INVALID_PARAMETER));
    ASSERT_STREQ(ErrorDB::ERRORTEXT_RESPONSE, ErrorDB::getErrorText(ErrorDB::ERRORCODE_RESPONSE));
}
