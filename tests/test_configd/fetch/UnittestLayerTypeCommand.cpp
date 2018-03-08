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

class UnittestLayerTypeCommand : public UnittestLayerType, public testing::Test {
protected:
    UnittestLayerTypeCommand()
        : UnittestLayerType(TEST_DATA_PATH, "command")
    {
        m_info = pbnjson::Object();
    }

    virtual void givenCMDOneLayer()
    {
        givenInfo(COMMAND_ONE_NAME, COMMAND_ONE_VALUE);
        givenLayer();
    }

    virtual void givenCMDTwoLayer()
    {
        givenInfo(COMMAND_TWO_NAME, COMMAND_TWO_VALUE);
        givenLayer();
    }

    const string COMMAND_ONE_NAME = "CommandOneLayer";
    const string COMMAND_ONE_VALUE = "echo 'selection1'";
    const string COMMAND_ONE_RESULT = "selection1";

    const string COMMAND_TWO_NAME = "CommandTwoLayer";
    const string COMMAND_TWO_VALUE = "echo 'selection2'";
    const string COMMAND_TWO_RESULT = "selection2";
};

TEST_F(UnittestLayerTypeCommand, gettingWithoutSelectionOfCMDOneLayer)
{
    givenCMDOneLayer();

    thenTypeAndName(SelectorType_Command, COMMAND_ONE_NAME);
    thenUnselectedStatus(TEST_DATA_PATH);
    EXPECT_FALSE(m_layer->isReadOnlyType());
}

TEST_F(UnittestLayerTypeCommand, gettingWithSelectionOfCMDOneLayer)
{
    givenCMDOneLayer();

    EXPECT_TRUE(m_layer->select());

    thenSelectedStatus(TEST_DATA_PATH, COMMAND_ONE_RESULT);
}

TEST_F(UnittestLayerTypeCommand, checkListenerWithSelectionOfCMDOneLayer)
{
    givenCMDOneLayer();

    EXPECT_CALL(m_listener, onSelectionChanged(_, _, _));
    EXPECT_TRUE(m_layer->select());
}

TEST_F(UnittestLayerTypeCommand, gettingWithoutSelectionOfCMDTwoLayer)
{
    givenCMDTwoLayer();

    thenTypeAndName(SelectorType_Command, COMMAND_TWO_NAME);
    thenUnselectedStatus(TEST_DATA_PATH);

    EXPECT_FALSE(m_layer->isReadOnlyType());
}

TEST_F(UnittestLayerTypeCommand, gettingWithSelectionOfCMDTwoLayer)
{
    givenCMDTwoLayer();

    EXPECT_TRUE(m_layer->select());

    thenSelectedStatus(TEST_DATA_PATH, COMMAND_TWO_RESULT);
}

TEST_F(UnittestLayerTypeCommand, fetchConfigs)
{
    givenCMDTwoLayer();

    m_layer->select();
    string path = m_layer->getFullDirPath(true);

    thenFetchConfigs(path);
}

TEST_F(UnittestLayerTypeCommand, fetchFullConfigs)
{
    givenCMDOneLayer();

    m_layer->select();
    string path = m_layer->getFullDirPath(true);

    thenFetchFullConfigs(path);
}

TEST_F(UnittestLayerTypeCommand, clearSelectAndSelectAgain)
{
    givenCMDOneLayer();

    EXPECT_TRUE(m_layer->select());
    EXPECT_TRUE(m_layer->isSelected());

    m_layer->clearSelection();

    EXPECT_FALSE(m_layer->isSelected());
    EXPECT_TRUE(m_layer->setSelection(COMMAND_TWO_RESULT));
    thenSelectedStatus(TEST_DATA_PATH, COMMAND_TWO_RESULT);

    EXPECT_TRUE(m_layer->select());

    thenSelectedStatus(TEST_DATA_PATH, COMMAND_ONE_RESULT);
}

TEST_F(UnittestLayerTypeCommand, invalidCallOperation)
{
    givenCMDOneLayer();

    EXPECT_EQ(false, m_layer->call());
}

TEST_F(UnittestLayerTypeCommand, fetchFilesWithInvaildPath)
{
    givenCMDOneLayer();

    string path = "invalidPath";
    JsonDB A;
    EXPECT_FALSE(Layer::parseFiles(path, NULL, &A));
}

TEST_F(UnittestLayerTypeCommand, fetchFilesNotExistJsonInDir)
{
    givenCMDOneLayer();

    string path = TEST_DATA_PATH;
    JsonDB A;
    EXPECT_TRUE(Layer::parseFiles(path, NULL, &A));
}
