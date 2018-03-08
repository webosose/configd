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
#include <service/MockConfigd.h>

#include "Environment.h"
#include "Manager.h"
#include "config/Configuration.h"

using namespace pbnjson;
using namespace std;
using ::testing::AtLeast;

#define LAYERS_JSON_PATH "data/layers.json"

class UnittestManager : public testing::Test {
protected:
    UnittestManager()
        : m_configuration(Configuration::getInstance())
    {
        m_configuration.clear();
        m_configuration.append(LAYERS_JSON_PATH);
        m_rawJson = pbnjson::JDomParser::fromFile(LAYERS_JSON_PATH);
    }

    virtual ~UnittestManager()
    {

    }

    Configuration& m_configuration;
    JValue m_rawJson;
};

TEST_F(UnittestManager, Base)
{

}
