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

#include <pbnjson.hpp>
#include <gtest/gtest.h>

#include "Environment.h"
#include "config/Configuration.h"

#include "MockLayer.h"
#include "service/MockAbstractBusFactory.h"

using namespace pbnjson;
using namespace std;

#define TEST_DATA_PATH "tests/test_configd/config/_data"

class UnittestConfiguration : public testing::Test {
protected:
    UnittestConfiguration()
        : m_configuration(Configuration::getInstance())
    {
        m_configuration.setListener(&m_listener);
    }

    virtual ~UnittestConfiguration()
    {

    }

    void givenDefaultConfiguration()
    {
        JSchema schema = JSchema::fromFile(CONFIGLAYERS_SCHEMA);
        JValue data = pbnjson::JDomParser::fromFile(PATH_LAYERS_DEAULT, schema);
        m_baseDirs.clear();
        for (JValue layer : data["layers"].items()) {
            if (find(m_baseDirs.begin(), m_baseDirs.end(), layer["base_dir"].asString()) == m_baseDirs.end())
                m_baseDirs.push_back(layer["base_dir"].asString());
        }
        m_configuration.clear();
        m_configuration.append(PATH_LAYERS_DEAULT);
    }

    void givenAdditionalConfiguration()
    {
        JSchema schema = JSchema::fromFile(CONFIGLAYERS_SCHEMA);
        JValue data = pbnjson::JDomParser::fromFile(PATH_LAYERS_MULTI, schema);
        for (JValue layer : data["layers"].items()) {
            if (find(m_baseDirs.begin(), m_baseDirs.end(), layer["base_dir"].asString()) == m_baseDirs.end())
                m_baseDirs.push_back(layer["base_dir"].asString());
        }
        m_configuration.append(PATH_LAYERS_MULTI);

    }

    void givenArrayPostProcess()
    {
        JSchema schema = JSchema::fromFile(CONFIGLAYERS_SCHEMA);
        JValue data = pbnjson::JDomParser::fromFile(PATH_LAYERS_ARRAY_POST_PROCESS, schema);
        m_configuration.append(PATH_LAYERS_ARRAY_POST_PROCESS);
    }

    Configuration &m_configuration;
    std::vector<std::string> m_baseDirs;

    MockLayerListener m_listener;
    MockAbstractBusFactory m_factory;

    const char *PATH_LAYERS_DEAULT = TEST_DATA_PATH "/layers/layers.json";
    const char *PATH_LAYERS_MULTI = TEST_DATA_PATH "/layers/layers_multi.json";

    const char *PATH_LAYERS_ARRAY_POST_PROCESS = TEST_DATA_PATH "/layers/layers_array_post_process.json";
    const char *PATH_JSON_DB = PATH_OUTPUT "/prepostprocess_jsondb.json";

    const char *CONFIG_CATEGORY_NAME1 = "com.webos.component1";
    const char *CONFIG_CATEGORY_NAME2 = "com.webos.component2";
    const char *CONFIG_CATEGORY_NAME3 = "com.webos.component3";
    const char *CONFIG_CATEGORY_NAME4 = "com.webos.component4";

    const char *CONFIG_KEY = "key1";
    const char *CONFIG_KEY_CONDITIONAL = "conditionalKey";
    const char *CONFIG_KEY_MULTILAYER = "multiLayerKey";

    const char *CONFIG_VALUE_CONDITIONAL = "whereMatched";
    const char *CONFIG_VALUE = "selection1";
    const char *CONFIG_VALUE_MULTILAYER = "selection2";

};

TEST_F(UnittestConfiguration, CompareFileAndConfiguration)
{
    givenDefaultConfiguration();

    std::vector<std::string> filePaths;
    ASSERT_EQ(m_baseDirs.size(), m_configuration.getLayersSize());
    filePaths = m_configuration.getConfFilePaths();
    ASSERT_STREQ(PATH_LAYERS_DEAULT, filePaths[0].c_str());
}

TEST_F(UnittestConfiguration, CompareFileAndMultiConfiguration)
{
    givenDefaultConfiguration();
    givenAdditionalConfiguration();
    std::vector<std::string> filePaths;

    ASSERT_EQ(m_configuration.getLayersSize(), m_baseDirs.size());
    filePaths = m_configuration.getConfFilePaths();
    ASSERT_EQ(filePaths.size(), 2);
    ASSERT_STREQ(PATH_LAYERS_DEAULT, filePaths[0].c_str());
    ASSERT_STREQ(PATH_LAYERS_MULTI, filePaths[1].c_str());
}

TEST_F(UnittestConfiguration, IsSortedLayer)
{
    givenDefaultConfiguration();
    givenAdditionalConfiguration();
    EXPECT_TRUE(m_configuration.isLayersSorted());
}

TEST_F(UnittestConfiguration, CheckReadOnly)
{
    givenDefaultConfiguration();

    Layer *layerNone = m_configuration.getLayer("none");
    Layer *layerString = m_configuration.getLayer("string");

    EXPECT_TRUE(NULL != layerString);
    EXPECT_TRUE(layerString->isSelected());
    EXPECT_TRUE(NULL != layerNone);
    EXPECT_TRUE(layerNone->isSelected());
    EXPECT_FALSE(m_configuration.isAllSelected());
}

TEST_F(UnittestConfiguration, SelectAndCheckAll)
{
    givenDefaultConfiguration();

    EXPECT_FALSE(m_configuration.isAllSelected());
    m_configuration.clearAllSelections();
    m_configuration.selectAll();
    EXPECT_TRUE(m_configuration.isAllSelected());
}

TEST_F(UnittestConfiguration, FetchBasicConfig)
{
    givenDefaultConfiguration();

    JsonDB jsonDB;
    JsonDB permissionDB("Permission Database");
    JValue result = pbnjson::Array();
    m_configuration.fetchConfigs(jsonDB, &permissionDB);
    m_configuration.fetchLayers(jsonDB);
    m_configuration.fetchConfigs(result);

    EXPECT_TRUE(jsonDB.getDatabase().hasKey(CONFIG_CATEGORY_NAME1));
    EXPECT_TRUE(jsonDB.getDatabase()[CONFIG_CATEGORY_NAME1][CONFIG_KEY].asBool());
}

TEST_F(UnittestConfiguration, FetchConditionalConfig)
{
    givenDefaultConfiguration();

    JsonDB jsonDB;
    JsonDB permissionDB("Permission Database");

    JValue result = pbnjson::Array();
    m_configuration.selectAll();
    m_configuration.fetchConfigs(jsonDB, &permissionDB);

    EXPECT_TRUE(jsonDB.getDatabase().hasKey(CONFIG_CATEGORY_NAME2));
    EXPECT_STREQ(jsonDB.getDatabase()[CONFIG_CATEGORY_NAME2][CONFIG_KEY_CONDITIONAL].asString().c_str(), CONFIG_VALUE_CONDITIONAL);
}

TEST_F(UnittestConfiguration, FetchExtendedConfig)
{
    givenDefaultConfiguration();

    JsonDB jsonDB;
    JsonDB permissionDB;

    JValue result = pbnjson::Array();
    m_configuration.fetchConfigs(jsonDB, &permissionDB);
    m_configuration.fetchLayers(jsonDB);
    EXPECT_TRUE(jsonDB.getDatabase().hasKey(CONFIG_CATEGORY_NAME3));
    EXPECT_TRUE(jsonDB.getDatabase()[CONFIG_CATEGORY_NAME3][CONFIG_KEY].asBool());
}

TEST_F(UnittestConfiguration, FetchMultiConfigs)
{
    givenDefaultConfiguration();

    JsonDB jsonDB;
    JsonDB permissionDB;

    JValue result = pbnjson::Array();
    m_configuration.fetchConfigs(jsonDB, &permissionDB);
    m_configuration.fetchLayers(jsonDB);
    EXPECT_TRUE(jsonDB.getDatabase().hasKey(CONFIG_CATEGORY_NAME4));
    EXPECT_TRUE(jsonDB.getDatabase()[CONFIG_CATEGORY_NAME4][CONFIG_KEY].asBool());
    EXPECT_STREQ(jsonDB.getDatabase()[CONFIG_CATEGORY_NAME4]["key3"].asString().c_str(), "value3");
    JValue permission = permissionDB.getDatabase();
    EXPECT_STREQ(permission[CONFIG_CATEGORY_NAME4]["key3"]["read"][0].asString().c_str(), "app1");
    EXPECT_STREQ(permission[CONFIG_CATEGORY_NAME4]["key4"]["read"][0].asString().c_str(), "app1");
    EXPECT_EQ(permission[CONFIG_CATEGORY_NAME4]["key4"]["read"].arraySize(), 1);
    EXPECT_STREQ(jsonDB.getDatabase()[CONFIG_CATEGORY_NAME1][CONFIG_KEY_CONDITIONAL].asString().c_str(), CONFIG_VALUE_CONDITIONAL);
}

TEST_F(UnittestConfiguration, FetchPermission)
{
    givenDefaultConfiguration();

    JsonDB jsonDB;
    JsonDB permissionDB;
    JValue result = pbnjson::Array();
    m_configuration.fetchConfigs(jsonDB, &permissionDB);
    JValue permission = permissionDB.getDatabase();

    EXPECT_TRUE(permission.hasKey(CONFIG_CATEGORY_NAME3));
    EXPECT_TRUE(permission[CONFIG_CATEGORY_NAME3].hasKey("key1"));
    EXPECT_TRUE(permission[CONFIG_CATEGORY_NAME3]["key1"].hasKey("read"));
    EXPECT_TRUE(permission[CONFIG_CATEGORY_NAME3]["key1"]["read"].isArray());

    EXPECT_STREQ(permission[CONFIG_CATEGORY_NAME3]["key1"]["read"][0].asString().c_str(), "app1");
    EXPECT_STREQ(permission[CONFIG_CATEGORY_NAME3]["key1"]["read"][1].asString().c_str(), "app2");

}

TEST_F(UnittestConfiguration, FetchMultiLayer)
{
    givenDefaultConfiguration();
    givenAdditionalConfiguration();
    JsonDB jsonDB;
    JsonDB permissionDB;
    JValue result = pbnjson::Array();
    m_configuration.selectAll();
    m_configuration.fetchConfigs(jsonDB, &permissionDB);

    EXPECT_TRUE(jsonDB.getDatabase().hasKey(CONFIG_CATEGORY_NAME1));
    EXPECT_FALSE(jsonDB.getDatabase()[CONFIG_CATEGORY_NAME1][CONFIG_KEY_MULTILAYER].asBool());

}

TEST_F(UnittestConfiguration, PreProcess)
{
    givenDefaultConfiguration();

    JsonDB jsonDB;
    ASSERT_TRUE(m_configuration.runPreProcess());
}

TEST_F(UnittestConfiguration, postProcess)
{
    givenArrayPostProcess();

    JsonDB jsonDB;
    jsonDB.setFilename(PATH_JSON_DB);
    ASSERT_TRUE(m_configuration.runPostProcess(jsonDB));
}

TEST_F(UnittestConfiguration, AdditionalPostProcess)
{
    givenDefaultConfiguration();
    givenAdditionalConfiguration();

    JsonDB jsonDB;
    jsonDB.setFilename(PATH_JSON_DB);
    ASSERT_TRUE(m_configuration.runPostProcess(jsonDB));
}
