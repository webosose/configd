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

#include <iostream>
#include <gtest/gtest.h>
#include <pbnjson.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include "Environment.h"

using namespace pbnjson;
using namespace std;
using namespace boost;

TEST(IntegrationBoost, trim)
{
    string str1 = "Text Text";
    string str2 = "    " + str1 + "     ";

    trim(str2);
    ASSERT_STREQ(str1.c_str(), str2.c_str());
}

TEST(IntegrationBoost, regexSearch)
{
    string str1 = "aaaa3bbbbcccc";
    boost::regex searchKey("3b{3}");

    ASSERT_TRUE(regex_search(str1, searchKey));
}

