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
 * Test for Schema.{h,c}
 */

#include "Schema.h"
#include "TestPath.h"
#include<gtest/gtest.h>

class SchemaTest : public ::testing::Test
{

protected:

    bool configdServiceLayersValidCase()
    {
        jvalue_ref layersFileObj = jinvalid();
        JSchemaInfo schemaInfogetSchema;

        Config_getSchema(layersValidSchema, &schemaInfogetSchema);

        layersFileObj = jdom_parse_file(layersValidJson, &schemaInfogetSchema, JFileOptMMap);

        return jvalue_check_schema(layersFileObj, &schemaInfogetSchema);
    }

    bool configdServiceLayersInvalidCase()
    {
        jvalue_ref layersFileObj = jinvalid();
        JSchemaInfo schemaInfogetSchema;

        Config_getSchema(layersValidSchema, &schemaInfogetSchema);

        layersFileObj = jdom_parse_file(layersInvalidJson, &schemaInfogetSchema, JFileOptMMap);

        return jvalue_check_schema(layersFileObj, &schemaInfogetSchema);
    }

    bool configdServiceGetConfigsValidCase()
    {
        jvalue_ref getConfigsFileObj = jinvalid();
        JSchemaInfo schemaInfogetSchema;

        Config_getSchema(getConfigsValidSchema, &schemaInfogetSchema);

        getConfigsFileObj = jdom_parse_file(getConfigsValidJson, &schemaInfogetSchema, JFileOptMMap);

        return jvalue_check_schema(getConfigsFileObj, &schemaInfogetSchema);;
    }

    bool configdServiceGetConfigsInvalidCase()
    {
        jvalue_ref getConfigsFileObj = jinvalid();
        JSchemaInfo schemaInfogetSchema;

        Config_getSchema(getConfigsValidSchema, &schemaInfogetSchema);

        getConfigsFileObj = jdom_parse_file(getConfigsInvalidJson, &schemaInfogetSchema, JFileOptMMap);

        return jvalue_check_schema(getConfigsFileObj, &schemaInfogetSchema);
    }

    const char *getConfigsValidSchema = TEST_DATA_PATH "schema/getConfigsValid.schema";
    const char *layersValidSchema = TEST_DATA_PATH "schema/layersValid.schema";
    const char *layersValidJson = TEST_DATA_PATH "json/layersValid.json";
    const char *layersInvalidJson = TEST_DATA_PATH "json/layersInvalid.json";
    const char *getConfigsValidJson = TEST_DATA_PATH "json/getConfigsValid.json";
    const char *getConfigsInvalidJson = TEST_DATA_PATH "json/getConfigsInvalid.json";
};

TEST_F(SchemaTest, Config_getSchema)
{
    //Case 1: valid layers json compared against valid schema file expects true
    EXPECT_TRUE(configdServiceLayersValidCase());

    //Case 2: Invalid layers json compared against valid schema file expects false
    EXPECT_FALSE(configdServiceLayersInvalidCase());

    //Case 3: Valid getConfigs json compared against valid schema file expects true
    EXPECT_TRUE(configdServiceGetConfigsValidCase());

    //Case 4: Invalid getConfigs json compared against valid schema file expects false
    EXPECT_FALSE(configdServiceGetConfigsInvalidCase());
}


