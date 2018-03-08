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
 * Test for dbInterface.{h,c}
 */

#include "dbInterface.h"
#include "TestUtil.h"
#include "JUtil.h"
#include "TestPath.h"
#include <pbnjson.h>

class dbInterfaceTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mDbHandle = dbLoadIf(validFilePath);
    }

    virtual void TearDown()
    {
        dbFree(mDbHandle);
    }

    dbObjHandle *dbLoadIf(const char *dbFileName)
    {
        return dbLoad((char *) dbFileName);
    }

    dbObjHandle *dbCloneIf(dbObjHandle *dbHandle, const char *dbFileName, bool deepCopy)
    {
        return dbClone(dbHandle, (char *)dbFileName, deepCopy);
    }

    bool dbDeleteRecordIf(dbObjHandle *dbHandle, const char *category, const char *key)
    {
        return dbDeleteRecord(dbHandle, (char *)category, (char *)key);
    }

    const char *validFilePath = TEST_DATA_PATH "configd_feature_list.json";
    const char *invalidFilePath = TEST_DATA_PATH "configd_feature_list_notexist.json";
    const char *invalidJsonFilePath = TEST_DATA_PATH "configd_feature_list_invalid.json";
    const char *nonJsonFilePath = TEST_DATA_PATH "configd_feature_list_non_json.txt";
    dbObjHandle *mDbHandle = NULL;

};

TEST_F(dbInterfaceTest, dbIsExist)
{
    EXPECT_FALSE(dbIsExist(invalidFilePath));
    EXPECT_TRUE(dbIsExist(validFilePath));
}

TEST_F(dbInterfaceTest, dbCreate)
{
    dbObjHandle *dbHandle = NULL;

    //negative cases
    dbHandle = dbCreate(NULL);
    EXPECT_TRUE((NULL == dbHandle));

    //positive cases
    dbHandle = dbCreate(validFilePath);
    EXPECT_TRUE((NULL != dbHandle));
    EXPECT_EQ(0, strcmp(validFilePath, dbHandle->dbFileName));
    EXPECT_FALSE(dbHandle->modified);
    EXPECT_TRUE(jis_valid(dbHandle->dbObject));
}

TEST_F(dbInterfaceTest, dbLoad)
{
    //negative cases
    EXPECT_EQ(NULL, dbLoadIf(NULL));

    EXPECT_EQ(NULL, dbLoadIf(invalidFilePath));

    EXPECT_EQ(NULL, dbLoadIf(invalidJsonFilePath));

    EXPECT_EQ(NULL, dbLoadIf(nonJsonFilePath));

    //positive cases
    EXPECT_TRUE(NULL != mDbHandle);
    EXPECT_TRUE(NULL != mDbHandle->dbObject);
    EXPECT_TRUE(jis_valid(mDbHandle->dbObject));
    EXPECT_EQ(0, strcmp(validFilePath, mDbHandle->dbFileName));
    EXPECT_FALSE(mDbHandle->modified);
}

TEST_F(dbInterfaceTest, dbClone)
{
    dbObjHandle *dbHandleClone = NULL;
    dbObjHandle *dbHandleCloneDeep = NULL;

    //negative cases
    EXPECT_EQ(NULL, dbCloneIf(mDbHandle, NULL, true));

    EXPECT_EQ(NULL, dbCloneIf(NULL, NULL, true));

    EXPECT_EQ(NULL, dbCloneIf(NULL, validFilePath, true));

    //positive cases
    dbHandleCloneDeep = dbCloneIf(mDbHandle, validFilePath, true);
    EXPECT_TRUE(NULL != dbHandleCloneDeep);
    EXPECT_EQ(0, strcmp(validFilePath, dbHandleCloneDeep->dbFileName));
    EXPECT_TRUE(dbHandleCloneDeep->modified);

    dbHandleClone = dbCloneIf(mDbHandle, validFilePath, false);
    EXPECT_TRUE(NULL != dbHandleClone);
    EXPECT_EQ(0, strcmp(validFilePath, dbHandleClone->dbFileName));
    EXPECT_TRUE(dbHandleClone->modified);
    EXPECT_TRUE(jvalue_equal(mDbHandle->dbObject, dbHandleClone->dbObject));

    dbFree(dbHandleClone);
    dbFree(dbHandleCloneDeep);
}

TEST_F(dbInterfaceTest, dbCopy)
{
    dbObjHandle *dbHandleDeepCopy = NULL;
    dbObjHandle *dbHandleCopy = NULL;

    //negative cases
    EXPECT_FALSE(dbCopy(NULL, NULL, true));

    EXPECT_FALSE(dbCopy(NULL, mDbHandle, true));

    dbHandleDeepCopy = dbCreate(validFilePath);
    EXPECT_TRUE(NULL != dbHandleDeepCopy);
    EXPECT_FALSE(dbCopy(dbHandleDeepCopy, NULL, true));

    //positive cases
    EXPECT_TRUE(dbCopy(dbHandleDeepCopy, mDbHandle, true));
    EXPECT_TRUE(NULL != dbHandleDeepCopy);
    EXPECT_TRUE(dbHandleDeepCopy->modified);

    dbHandleCopy = dbCreate(validFilePath);
    EXPECT_TRUE(NULL != dbHandleCopy);
    EXPECT_TRUE(dbCopy(dbHandleCopy, mDbHandle, false));
    EXPECT_TRUE(NULL != dbHandleCopy);
    EXPECT_TRUE(dbHandleCopy->modified);
    EXPECT_TRUE(jvalue_equal(mDbHandle->dbObject, dbHandleCopy->dbObject));

    dbFree(dbHandleDeepCopy);
    dbFree(dbHandleCopy);
}

TEST_F(dbInterfaceTest, dbInsertRecord)
{
    dbObjHandle *dbHandle = NULL;
    const char *category = "systemtest";
    const char *key = "systemtestkey";
    jvalue_ref value;
    jvalue_ref valObject;
    jvalue_ref compValue;
    value = jstring_create("systemtestval");

    //in dbInsterRecord api only dbHandle NULL is been verified other argument null verification not done.
    //negative cases
    EXPECT_FALSE(dbInsertRecord(dbHandle, category, key, value));

    dbHandle = dbCreate(validFilePath);
    EXPECT_TRUE(NULL != dbHandle);

    //positive cases
    //case: category not exist in db
    EXPECT_TRUE(dbInsertRecord(dbHandle, category, key, jvalue_copy(value)));
    EXPECT_TRUE(jobject_get_exists(dbHandle->dbObject, j_cstr_to_buffer(category), &valObject));
    EXPECT_TRUE(jobject_get_exists(valObject, j_cstr_to_buffer(key), &compValue));
    EXPECT_TRUE(jvalue_equal(value, compValue));
    j_release(&value);

    //case: category present and key also present only val changed.
    value = jstring_create("systemtestmodifiedval");
    EXPECT_TRUE(dbInsertRecord(dbHandle, category, key, jvalue_copy(value)));
    EXPECT_TRUE(jobject_get_exists(dbHandle->dbObject, j_cstr_to_buffer(category), &valObject));
    EXPECT_TRUE(jobject_get_exists(valObject, j_cstr_to_buffer(key), &compValue));
    EXPECT_TRUE(jvalue_equal(value, compValue));
    j_release(&value);

    dbFree(dbHandle);
}

TEST_F(dbInterfaceTest, splitCategoryandConfigName)
{
    const char *key = "system.key";
    char *category;
    char *configSuffix;
    //negative cases
    EXPECT_FALSE(splitCategoryandConfigName(NULL, &category, &configSuffix));

    EXPECT_FALSE(splitCategoryandConfigName("system", &category, &configSuffix));

    //positive cases
    //case: single "." in key
    EXPECT_TRUE(splitCategoryandConfigName(key, &category, &configSuffix));
    EXPECT_EQ(0, strcmp("system", category));
    EXPECT_EQ(0, strcmp("key", configSuffix));

    //case: multiple "." in key
    EXPECT_TRUE(splitCategoryandConfigName("system.multiple.dot.key", &category, &configSuffix));
    EXPECT_EQ(0, strcmp("system.multiple.dot", category));
    EXPECT_EQ(0, strcmp("key", configSuffix));
}

TEST_F(dbInterfaceTest, dbDeleteRecord)
{
    jvalue_ref categoryValue;
    dbObjHandle *dbHandle = NULL;

    dbHandle = dbLoadIf(validFilePath);
    EXPECT_TRUE(NULL != dbHandle);

    //null validatoin for all arguments not done.
    //negative cases
    EXPECT_FALSE(dbDeleteRecordIf(dbHandle, "", ""));

    EXPECT_FALSE(dbDeleteRecordIf(dbHandle, "system", ""));

    EXPECT_FALSE(dbDeleteRecordIf(dbHandle, "system", "not_exists"));

    EXPECT_FALSE(dbDeleteRecordIf(dbHandle, "not_exists", "country"));

    EXPECT_FALSE(dbDeleteRecordIf(dbHandle, "not_exists", "not_exists"));

    //positive cases
    EXPECT_TRUE(dbDeleteRecordIf(dbHandle, "system", "country"));
    EXPECT_TRUE(jobject_get_exists(dbHandle->dbObject, j_cstr_to_buffer("system"), &categoryValue));
    EXPECT_TRUE(NULL != categoryValue);
    EXPECT_FALSE(jobject_containskey(categoryValue, j_cstr_to_buffer("country")));

    dbFree(dbHandle);
}

TEST_F(dbInterfaceTest, dbDelete)
{
    const char *deleteDbFilePath = TEST_DATA_PATH "configd_feature_list_delete.json";
    dbObjHandle *dbHandle = NULL;

    dbHandle = dbLoadIf(validFilePath);
    EXPECT_TRUE(NULL != dbHandle);
    dbHandle->dbFileName = (char *)deleteDbFilePath;
    dbHandle->modified = true;
    EXPECT_TRUE(dbFinalize(dbHandle, false));

    //negative cases
    EXPECT_FALSE(dbDelete(NULL));

    EXPECT_FALSE(dbDelete(invalidFilePath));

    //positive cases
    EXPECT_TRUE(dbDelete(deleteDbFilePath));
    EXPECT_FALSE(dbIsExist(deleteDbFilePath));
}

TEST_F(dbInterfaceTest, dbFetchRecord)
{
    jvalue_ref configValue = jstring_create("IND");

    //negative cases
    EXPECT_EQ(NULL, dbFetchRecord(NULL, "system.country"));

    EXPECT_TRUE(NULL != mDbHandle);
    EXPECT_EQ(NULL, dbFetchRecord(mDbHandle, NULL));

    EXPECT_EQ(NULL, dbFetchRecord(mDbHandle, "system"));

    EXPECT_EQ(NULL, dbFetchRecord(mDbHandle, "system.not_exist"));

    //positive cases
    EXPECT_TRUE(jvalue_equal(configValue, dbFetchRecord(mDbHandle, "system.country")));
}

TEST_F(dbInterfaceTest, makeFlatConfigList)
{
    char *category = (char *) "system.";
    jvalue_ref categoryVlaue = jobject_create();
    jvalue_ref returnValue = jobject_create();

    EXPECT_TRUE(NULL != categoryVlaue);
    EXPECT_TRUE(NULL != returnValue);

    //negative cases
    EXPECT_FALSE(makeFlatConfigList(NULL, NULL, NULL));

    EXPECT_FALSE(makeFlatConfigList(category, NULL, NULL));

    EXPECT_FALSE(makeFlatConfigList(category, categoryVlaue, NULL));

    EXPECT_FALSE(makeFlatConfigList(category, NULL, returnValue));

    EXPECT_FALSE(makeFlatConfigList(category, jstring_create("not_object"), returnValue));

    //positive cases
    jobject_put(categoryVlaue, jstring_create("country"), jstring_create("IND"));
    jobject_put(categoryVlaue, jstring_create("integer"), jnumber_create_i32(1));

    EXPECT_TRUE(makeFlatConfigList(category, categoryVlaue, returnValue));
    EXPECT_TRUE(jobject_containskey(returnValue, j_cstr_to_buffer("system.country")));
    EXPECT_TRUE(jobject_containskey(returnValue, j_cstr_to_buffer("system.integer")));

    j_release(&categoryVlaue);
    j_release(&returnValue);
}

TEST_F(dbInterfaceTest, dbFetchConfigRecords)
{
    raw_buffer configBuf;
    jvalue_ref returnValue = jobject_create();

    configBuf = j_cstr_to_buffer("not_exist.country");

    //negative cases
    EXPECT_FALSE(dbFetchConfigRecords(NULL, &configBuf, NULL));

    EXPECT_FALSE(dbFetchConfigRecords(mDbHandle, &configBuf, NULL));

    EXPECT_FALSE(dbFetchConfigRecords(mDbHandle, &configBuf, returnValue));

    configBuf = j_cstr_to_buffer("system.not_exist");
    EXPECT_FALSE(dbFetchConfigRecords(mDbHandle, &configBuf, returnValue));

    //positive cases
    configBuf = j_cstr_to_buffer("system.country");
    EXPECT_TRUE(dbFetchConfigRecords(mDbHandle, &configBuf, returnValue));
    EXPECT_TRUE(jobject_containskey(returnValue, configBuf));

    configBuf = j_cstr_to_buffer("system.*");
    EXPECT_TRUE(dbFetchConfigRecords(mDbHandle, &configBuf, returnValue));
    EXPECT_TRUE(jobject_containskey(returnValue, j_cstr_to_buffer("system.country")));
    EXPECT_TRUE(jobject_containskey(returnValue, j_cstr_to_buffer("system.smartServiceCountry")));
    EXPECT_TRUE(jobject_containskey(returnValue, j_cstr_to_buffer("system.enable_smart_service")));

    j_release(&returnValue);
}

TEST_F(dbInterfaceTest, dbFinalize)
{
    dbObjHandle *dbHandle = NULL;
    const char *newDbFilePath = TEST_DATA_PATH "configd_feature_list_dbFinalize.json";

    dbHandle = dbLoadIf(validFilePath);
    EXPECT_TRUE(NULL != dbHandle);
    EXPECT_FALSE(dbHandle->modified);
    dbHandle->dbFileName = (char *) newDbFilePath;

    //negative cases
    EXPECT_FALSE(dbFinalize(NULL, true));

    //positive cases
    EXPECT_TRUE(dbFinalize(dbHandle, true));
    EXPECT_FALSE(dbIsExist(newDbFilePath));
    EXPECT_FALSE(dbHandle->modified);

    dbHandle->modified = true;
    EXPECT_TRUE(dbFinalize(dbHandle, false));
    EXPECT_TRUE(dbIsExist(newDbFilePath));
    EXPECT_FALSE(dbHandle->modified);

    EXPECT_TRUE(dbDelete(newDbFilePath));
    dbHandle->modified = true;
    EXPECT_TRUE(dbFinalize(dbHandle, false));
    EXPECT_TRUE(dbIsExist(newDbFilePath));
    EXPECT_TRUE(dbDelete(newDbFilePath));
}

