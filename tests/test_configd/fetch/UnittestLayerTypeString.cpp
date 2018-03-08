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
#include <pbnjson.hpp>
#include <gtest/gtest.h>

#include "config/Layer.h"
#include "util/Platform.h"

#include "Environment.h"
#include "UnittestLayerType.hpp"

using namespace pbnjson;
using namespace std;
using ::testing::SetArgReferee;
using ::testing::_;

class UnittestLayerTypeString : public UnittestLayerType, public testing::Test {
protected:
    UnittestLayerTypeString()
        : UnittestLayerType(TEST_DATA_PATH, "string")
    {
        m_info = pbnjson::Object();
    }

    virtual ~UnittestLayerTypeString()
    {

    }

    virtual void givenValidStringLayer()
    {
        givenInfo(DEFAULT_NAME, DEFAULT_VALUE);
        givenLayer();
    }

    virtual void givenEmptyStringLayer()
    {
        givenInfo(EMPTY_NAME, "");
        givenLayer();
    }

    const string EMPTY_NAME = "EmptyStringLayer";
    const string DEFAULT_NAME = "StringLayer";
    const string DEFAULT_VALUE = "selection2";
};

TEST_F(UnittestLayerTypeString, varifyEmptyStringLayerStatus)
{
    givenEmptyStringLayer();

    thenTypeAndName(SelectorType::SelectorType_None, EMPTY_NAME);
    thenUnselectedStatus(TEST_DATA_PATH);
    EXPECT_TRUE(m_layer->isReadOnlyType());
    EXPECT_FALSE(m_layer->isSelected());
}

TEST_F(UnittestLayerTypeString, varifyEmptyStringLayerFetches)
{
    givenEmptyStringLayer();

    JsonDB db;
    JValue array = pbnjson::Array();
    EXPECT_FALSE(m_layer->fetchConfigs(db));
    EXPECT_FALSE(m_layer->fetchConfigs(array));
}

TEST_F(UnittestLayerTypeString, varifyVaildStringLayerStatus)
{
    givenValidStringLayer();

    EXPECT_TRUE(m_layer->isReadOnlyType());
    thenTypeAndName(SelectorType_String, DEFAULT_NAME);
    thenSelectedStatus(TEST_DATA_PATH, DEFAULT_VALUE);
}

TEST_F(UnittestLayerTypeString, stringLayerFetchConfigs)
{
    givenValidStringLayer();

    string path = m_layer->getFullDirPath(true);

    thenFetchConfigs(path);
}

TEST_F(UnittestLayerTypeString, stringLayerFetchFullConfigs)
{
    givenValidStringLayer();

    string path = m_layer->getFullDirPath();

    thenFetchFullConfigs(path);
}

TEST_F(UnittestLayerTypeString, clearSelectAndSelectAgain)
{
    givenValidStringLayer();

    EXPECT_TRUE(m_layer->isSelected());

    m_layer->clearSelection();

    EXPECT_TRUE(m_layer->isSelected());
    EXPECT_FALSE(m_layer->select());
    thenSelectedStatus(TEST_DATA_PATH, DEFAULT_VALUE);
}

TEST_F(UnittestLayerTypeString, invalidCallOperation)
{
    givenValidStringLayer();

    EXPECT_EQ(false, m_layer->call());
    EXPECT_FALSE(m_layer->setSelection("InvalidDir"));
}

TEST_F(UnittestLayerTypeString, fetchFilesWithInvaildPath)
{
    givenEmptyStringLayer();

    string path = "invalidPath";
    JsonDB A;
    EXPECT_FALSE(Layer::parseFiles(path, NULL, &A));
}

TEST_F(UnittestLayerTypeString, fetchFilesNotExistJsonInDir)
{
    givenValidStringLayer();

    string path = TEST_DATA_PATH;
    JsonDB A;
    EXPECT_TRUE(Layer::parseFiles(path, NULL, &A));
}
