// Copyright (c) 2014-2018 LG Electronics, Inc.
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

/*
 * Test for ConfigureData.{h,c}
 */

#include "ConfigureData.h"  // The file to test

#include "TestPath.h"
#include "TestUtil.h"
#include "JUtil.h"

#include <pbnjson.h>

const static char *kSampleSettingsServiceReply =
    "{"
    "    \"category\": \"option\","
    "    \"method\": \"getSystemSettings\","
    "    \"settings\": {"
    "        \"country\": \"KOR\","
    "        \"smartServiceCountryCode3\": \"KOR\""
    "    },"
    "    \"a\": { \"deep\": { \"deep\": { \"deep\": { \"object\": { \"for\": { \"test\": \"OK\"}}}}}},"
    "    \"returnValue\": true"
    "}";

class ConfigureDataTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        m_settings_reply_json = JUtil::parse_json_c(kSampleSettingsServiceReply);
        m_cd = ConfigureDataCreate();
        m_count = 0;
    }

    virtual void TearDown()
    {
        j_release(&m_settings_reply_json);
        ConfigureDataDestroy(m_cd);
        m_cd = NULL;
    }

    static void cbConfigureDataForEach(gpointer layer, gpointer userData)
    {
        ConfigLayer *layerInfo = (ConfigLayer *)layer;
        ConfigureDataTest *self = (ConfigureDataTest *)userData;

        if (layerInfo && jis_valid(layerInfo->layerObj))
        {
            self->m_count++;
        }
    }

    static void cbConfigureIsValidLayerInfo(gpointer layer, gpointer userData)
    {
        ConfigLayer *layerInfo = (ConfigLayer *)layer;
        ConfigureDataTest *self = (ConfigureDataTest *)userData;

        if (layerInfo && jis_valid(layerInfo->layerObj))
        {
            self->m_layerInfoSample = layerInfo;
        }
    }


    jvalue_ref m_settings_reply_json;
    ConfigureData *m_cd;
    int m_count;
    ConfigLayer *m_layerInfoSample;

    const std::string VALID_LAYERS_JSON_FILE_PATH = TEST_DATA_PATH "/layers.json";
    const std::string INVALID_LAYERS_JSON_FILE_PATH = TEST_DATA_PATH "/layers_invalid.json";
    const std::string LAYERS_JSON_FILE_INVALID_PATH = TEST_DATA_PATH "/not_exist_file.json";
};

TEST_F(ConfigureDataTest, ConfigureDataCreate)
{
    ConfigureData *cd = ConfigureDataCreate();

    EXPECT_FALSE(cd == NULL);
    EXPECT_FALSE(cd->firstScanDone);
    EXPECT_FALSE(cd->layerParsed);
    EXPECT_FALSE(cd->layers == NULL);
    EXPECT_EQ(cd->layers->len, 0);
    EXPECT_TRUE(cd->lsHandle == NULL);
    EXPECT_EQ(cd->watchingCount, 0);

    ConfigureDataDestroy(cd);
}

TEST_F(ConfigureDataTest, ConfigureDataDestroy)
{
    // No Test
}


TEST_F(ConfigureDataTest, ConfigureDataLoad)
{
    ConfigureData *cd = m_cd;
    // initialized
    EXPECT_EQ(cd->layers->len, 0);

    // cd is mandatory argument and NOT-NULL-PROOF. It should be cached by assert()
    // in gtest, assert is disabled. therefore it cause crash.
    EXPECT_DEATH_IF_SUPPORTED(ConfigureDataLoad(NULL, VALID_LAYERS_JSON_FILE_PATH.c_str()), "");

    // invalid filenames
    EXPECT_FALSE(ConfigureDataLoad(cd, NULL));
    EXPECT_FALSE(ConfigureDataLoad(cd, LAYERS_JSON_FILE_INVALID_PATH.c_str()));

    // valid layers.json
    EXPECT_TRUE(ConfigureDataLoad(cd, VALID_LAYERS_JSON_FILE_PATH.c_str()));
    EXPECT_TRUE(jis_valid(cd->layersObj));
    EXPECT_EQ(cd->layers->len, 5);

    // layers.json is valid json but insuffecient information
    EXPECT_FALSE(ConfigureDataLoad(cd, INVALID_LAYERS_JSON_FILE_PATH.c_str()));

    // recovering from invalid state
    EXPECT_TRUE(ConfigureDataLoad(cd, VALID_LAYERS_JSON_FILE_PATH.c_str()));
}
TEST_F(ConfigureDataTest, ConfigureDataUnload)
{
    ConfigureData *cd = m_cd;

    // initialized
    EXPECT_EQ(cd->layers->len, 0);

    // ConfigureDataUnload is NULL-PROOF. Should go through without problem.
    ConfigureDataUnload(NULL);

    // valid layers.json
    EXPECT_TRUE(ConfigureDataLoad(cd, VALID_LAYERS_JSON_FILE_PATH.c_str()));
    EXPECT_TRUE(jis_valid(cd->layersObj));
    EXPECT_EQ(cd->layers->len, 5);

    ConfigureDataUnload(cd);
    EXPECT_EQ(cd->layers->len, 0);
    EXPECT_TRUE(jis_null(cd->layersObj));
    EXPECT_FALSE(cd->firstScanDone);
    EXPECT_FALSE(cd->layerParsed);
}
TEST_F(ConfigureDataTest, ConfigureDataSetLSHandle)
{
    ConfigureData *cd = m_cd;

    ConfigureDataSetLSHandle(cd, (LSHandle *)0x1234);
    EXPECT_EQ(cd->lsHandle, (LSHandle *)0x1234);

    // reset to zero for proper teardown
    ConfigureDataSetLSHandle(cd, NULL);
}
TEST_F(ConfigureDataTest, ConfigureDataCancelCalls)
{
    // TODO: proper LS2 call test
}


TEST_F(ConfigureDataTest, ConfigureDataForeach)
{
    ConfigureData *cd = m_cd;

    // valid layers.json
    EXPECT_TRUE(ConfigureDataLoad(cd, VALID_LAYERS_JSON_FILE_PATH.c_str()));

    this->m_count = 0;
    EXPECT_TRUE(ConfigureDataForeach(cd, cbConfigureDataForEach, this));
    EXPECT_EQ(this->m_count, 5);
}
TEST_F(ConfigureDataTest, ConfigureDataIsValidLayerInfo)
{
    ConfigureData *cd = m_cd;

    // valid layers.json
    EXPECT_TRUE(ConfigureDataLoad(cd, VALID_LAYERS_JSON_FILE_PATH.c_str()));

    // get the last valid layerInfo from the list
    this->m_layerInfoSample = NULL;
    EXPECT_TRUE(ConfigureDataForeach(cd, cbConfigureIsValidLayerInfo, this));

    EXPECT_TRUE(ConfigureDataIsValidLayerInfo(cd, this->m_layerInfoSample));

    // Invalid pointer for layerInfo
    EXPECT_FALSE(ConfigureDataIsValidLayerInfo(cd, NULL));
}

