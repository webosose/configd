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

#include "config/Layer.h"
#include "util/Platform.h"

#include "Environment.h"
#include "UnittestLayerType.hpp"

using namespace pbnjson;
using namespace std;
using ::testing::SetArgReferee;
using ::testing::_;

class UnittestLayerTypeFile : public UnittestLayerType, public testing::Test {
protected:
    UnittestLayerTypeFile()
        : UnittestLayerType(TEST_DATA_PATH, "file")
    {
        m_info = pbnjson::Object();
    }

    virtual void givenFileOneLayer()
    {
        givenInfo(FILE_ONE_NAME, FILE_ONE_VALUE);
        givenLayer();
    }

    virtual void givenFileTwoLayer()
    {
        givenInfo(FILE_TWO_NAME, FILE_TWO_VALUE);
        givenLayer();
    }

    const string FILE_ONE_NAME = "FileOneLayer";
    const string FILE_ONE_VALUE = TEST_DATA_PATH "/fileselector1";
    const string FILE_ONE_CONTENT = "selection1";

    const string FILE_TWO_NAME = "FileTwoLayer";
    const string FILE_TWO_VALUE = TEST_DATA_PATH "/fileselector2";
    const string FILE_TWO_CONTENT = "selection2";
};

TEST_F(UnittestLayerTypeFile, gettingWithoutSelectionOfFileOneLayer)
{
    givenFileOneLayer();

    EXPECT_FALSE(m_layer->isReadOnlyType());
    thenTypeAndName(SelectorType_File, FILE_ONE_NAME);
    thenUnselectedStatus(TEST_DATA_PATH);
}

TEST_F(UnittestLayerTypeFile, gettingWithSelectionOfFileOneLayer)
{
    givenFileOneLayer();

    EXPECT_TRUE(m_layer->select());
    thenSelectedStatus(TEST_DATA_PATH, FILE_ONE_CONTENT);
}

TEST_F(UnittestLayerTypeFile, checkListenerWithSelectionOfFileOneLayer)
{
    givenFileOneLayer();

    EXPECT_CALL(m_listener, onSelectionChanged(_, _, _));
    EXPECT_TRUE(m_layer->select());
}

TEST_F(UnittestLayerTypeFile, gettingWithoutSelectionOfFileTwoLayer)
{
    givenFileTwoLayer();

    thenTypeAndName(SelectorType_File, FILE_TWO_NAME);
    thenUnselectedStatus(TEST_DATA_PATH);
    EXPECT_FALSE(m_layer->isReadOnlyType());
}

TEST_F(UnittestLayerTypeFile, gettingWithSelectionOfFileTwoLayer)
{
    givenFileTwoLayer();

    EXPECT_TRUE(m_layer->select());
    thenSelectedStatus(TEST_DATA_PATH, FILE_TWO_CONTENT);
}

TEST_F(UnittestLayerTypeFile, fetchConfigs)
{
    givenFileTwoLayer();

    m_layer->select();
    string path = m_layer->getFullDirPath(true);
    thenFetchConfigs(path);
}

TEST_F(UnittestLayerTypeFile, fetchFullConfigs)
{
    givenFileOneLayer();

    m_layer->select();

    string path = m_layer->getFullDirPath(true);
    thenFetchFullConfigs(path);
}

TEST_F(UnittestLayerTypeFile, clearSelectAndSelectAgain)
{
    givenFileOneLayer();

    EXPECT_TRUE(m_layer->select());
    EXPECT_TRUE(m_layer->isSelected());

    m_layer->clearSelection();

    EXPECT_FALSE(m_layer->isSelected());
    EXPECT_TRUE(m_layer->setSelection(FILE_TWO_CONTENT));
    thenSelectedStatus(TEST_DATA_PATH, FILE_TWO_CONTENT);

    EXPECT_TRUE(m_layer->select());

    thenSelectedStatus(TEST_DATA_PATH, FILE_ONE_CONTENT);
}

TEST_F(UnittestLayerTypeFile, invalidCallOperation)
{
    givenFileOneLayer();

    EXPECT_EQ(false, m_layer->call());
}

TEST_F(UnittestLayerTypeFile, fetchFilesWithInvaildPath)
{
    givenFileOneLayer();

    string path = "invalidPath";
    JsonDB A;
    EXPECT_FALSE(Layer::parseFiles(path, NULL, &A));
}

TEST_F(UnittestLayerTypeFile, fetchFilesNotExistJsonInDir)
{
    givenFileOneLayer();

    string path = TEST_DATA_PATH;
    JsonDB A;
    EXPECT_TRUE(Layer::parseFiles(path, NULL, &A));
}
