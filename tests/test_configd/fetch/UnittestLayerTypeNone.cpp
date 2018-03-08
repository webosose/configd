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

#include <config/MockLayer.h>
#include <gtest/gtest.h>
#include <pbnjson.hpp>

#include "Environment.h"
#include "UnittestLayerType.hpp"
#include "config/Layer.h"

using namespace pbnjson;
using namespace std;

#define NAME_LAYER      "noneType"

class UnittestLayerTypeNone : public UnittestLayerType, public testing::Test {
protected:
    UnittestLayerTypeNone()
        : UnittestLayerType(TEST_DATA_PATH, "none")
    {

    }

    virtual ~UnittestLayerTypeNone()
    {

    }

    virtual void givenDefaultNoneType()
    {
        m_info = pbnjson::Object();
        m_info.put("base_dir", TEST_DATA_PATH);
        m_info.put("name", NAME_LAYER);
        m_layer = new Layer(m_info);
    }
};

TEST_F(UnittestLayerTypeNone, getters)
{
    givenDefaultNoneType();

    EXPECT_TRUE(m_layer->isReadOnlyType());
    thenTypeAndName(SelectorType::SelectorType_None, NAME_LAYER);
    thenSelectedStatus(TEST_DATA_PATH);
}

TEST_F(UnittestLayerTypeNone, clearSelectAndSelectAgain)
{
    givenDefaultNoneType();
    EXPECT_TRUE(m_layer->isSelected());

    m_layer->clearSelection();

    EXPECT_TRUE(m_layer->isSelected());
    EXPECT_FALSE(m_layer->select());
    thenSelectedStatus(TEST_DATA_PATH);
}

TEST_F(UnittestLayerTypeNone, invalidCallOperation)
{
    givenDefaultNoneType();

    EXPECT_EQ(false, m_layer->call());
    EXPECT_FALSE(m_layer->setSelection("InvalidDir"));
}

TEST_F(UnittestLayerTypeNone, fetchConfigs)
{
    givenDefaultNoneType();

    string path = m_layer->getFullDirPath(true);
    thenFetchConfigs(path);
}

TEST_F(UnittestLayerTypeNone, fetchFullConfigs)
{
    givenDefaultNoneType();

    string path = m_layer->getFullDirPath(true);
    thenFetchFullConfigs(path);
}

TEST_F(UnittestLayerTypeNone, fetchFilesWithInvaildPath)
{
    givenDefaultNoneType();

    string path = "invalidPath";
    JsonDB A;
    EXPECT_FALSE(Layer::parseFiles(path, NULL, &A));
}

TEST_F(UnittestLayerTypeNone, fetchFilesNotExistJsonInDir)
{
    givenDefaultNoneType();

    string path = TEST_DATA_PATH;
    JsonDB A;
    EXPECT_TRUE(Layer::parseFiles(path, NULL, &A));
}
