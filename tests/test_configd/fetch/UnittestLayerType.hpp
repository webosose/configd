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

#ifndef _UNITTEST_LAYER_H_
#define _UNITTEST_LAYER_H_

#include <iostream>
#include <pbnjson.hpp>
#include <gmock/gmock.h>

using namespace std;
using namespace pbnjson;

#define TEST_DATA_PATH "tests/test_configd/fetch/_data"

class UnittestLayerType {
protected:
    UnittestLayerType(string dirPath, string selectorType)
        : NAME_FULLPATH(dirPath)
        , SELECTOR_TYPE(selectorType)
        , m_layer(NULL)
    {
    }

    virtual ~UnittestLayerType()
    {
        if (m_layer != NULL)
            delete m_layer;
    }

    virtual void givenInfo(string name, string selectorValue)
    {
        JValue selector = pbnjson::Object();
        selector.put(SELECTOR_TYPE, selectorValue);

        m_info.put("selector", selector);
        m_info.put("base_dir", NAME_FULLPATH);
        m_info.put("name", name);
    }

    virtual void givenLayer()
    {
        if (m_layer != NULL)
            delete m_layer;

        m_layer = new Layer(m_info);
        m_layer->setListener(&m_listener);
    }

    void thenTypeAndName(SelectorType type, string name)
    {
        EXPECT_EQ(type, m_layer->getType());
        EXPECT_STREQ(name.c_str(), m_layer->getName().c_str());
    }

    void thenUnselectedStatus(string baseDir)
    {
        EXPECT_FALSE(m_layer->isSelected());
        if (SelectorType_None == m_layer->getType()) {

        } else {
            EXPECT_STREQ("", m_layer->getSelection().c_str());
            EXPECT_STREQ(baseDir.c_str(), m_layer->getFullDirPath(true).c_str());
            EXPECT_STREQ(baseDir.c_str(), m_layer->getFullDirPath(false).c_str());
        }
    }

    void thenSelectedStatus(string baseDir, string selection = "")
    {
        EXPECT_TRUE(m_layer->isSelected());
        if (SelectorType_None == m_layer->getType()) {
            EXPECT_STREQ(baseDir.c_str(), m_layer->getFullDirPath(false).c_str());
            EXPECT_STREQ(baseDir.c_str(), m_layer->getFullDirPath(true).c_str());
            EXPECT_STREQ("", m_layer->getSelection().c_str());
        } else {
            string fullPath = Platform::concatPaths(baseDir, selection);

            EXPECT_STREQ(selection.c_str(), m_layer->getSelection().c_str());
            EXPECT_STREQ(baseDir.c_str(), m_layer->getFullDirPath(false).c_str());
            EXPECT_STREQ(fullPath.c_str(), m_layer->getFullDirPath(true).c_str());
        }
    }

    void thenFetchConfigs(string &path)
    {
        JsonDB A, B;

        EXPECT_TRUE(Layer::parseFiles(path, NULL, &A));
        EXPECT_TRUE(m_layer->fetchConfigs(B));
        EXPECT_EQ(A.getDatabase(), B.getDatabase());
    }

    void thenFetchFullConfigs(string &path)
    {
        JValue A = pbnjson::Array();
        JValue B = pbnjson::Array();

        JValue item = pbnjson::Object();
        JValue result = pbnjson::Object();

        EXPECT_TRUE(Layer::parseFiles(path, &result));
        EXPECT_TRUE(m_layer->fetchConfigs(B));

        EXPECT_TRUE(item.put(path, result));
        EXPECT_TRUE(A.append(item));
        EXPECT_EQ(A, B);
    }

    const string NAME_FULLPATH;
    const string SELECTOR_TYPE;

    Layer *m_layer;
    JValue m_info;
    MockLayerListener m_listener;
};

#endif /* _UNITTEST_LAYER_H_ */
