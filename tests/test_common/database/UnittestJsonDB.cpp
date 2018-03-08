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

#include "Environment.h"
#include "database/JsonDB.h"
#include "util/Platform.h"

using namespace pbnjson;
using namespace std;

#define TEST_DATA_PATH "tests/test_common/database/_data"

class UnittestJsonDB : public testing::Test {
protected:
    UnittestJsonDB()
    {
        Platform::copyFile(TEST_DATA_PATH "/MainDB.json", PATH_OUTPUT "/MainDB.json");

        m_fullNameFirst = NAME_CATEGORY1 + "." + NAME_CONFIG1;
        m_fullNameSecond = NAME_CATEGORY1 + "." + NAME_CONFIG2;
    }

    virtual ~UnittestJsonDB()
    {
        Platform::deleteFile(PATH_TEST_DB);
        Platform::deleteFile(PATH_SYNC_DB);
    }

    virtual void givenEmptyDB()
    {
    }

    virtual void givenFileDB(string file = TEST_DATA_PATH "/MainDB.json")
    {
        m_testDB.load(file);
    }

    virtual void givenMultiItemsDB()
    {
        m_testDB.insert(NAME_CATEGORY1, NAME_CONFIG1, NAME_CONFIG_VALUE1);
        m_testDB.insert(NAME_CATEGORY1, NAME_CONFIG2, NAME_CONFIG_VALUE2);
        m_testDB.insert(NAME_CATEGORY1, NAME_CONFIG3, NAME_CONFIG_VALUE3);
        m_testDB.insert(NAME_CATEGORY2, NAME_CONFIG2, NAME_CONFIG_VALUE2);
        m_testDB.insert(NAME_CATEGORY2, NAME_CONFIG3, NAME_CONFIG_VALUE3);
    }

    virtual void givenFactoryDB()
    {
        JValue configValue = pbnjson::Array();
        configValue.append(m_fullNameFirst);
        m_testFactoryDB.insert(NAME_CATEGORY1, NAME_CONFIG1, NAME_CONFIG_VALUE2);
        m_testFactoryDB.insert(JsonDB::FULLNAME_USER, configValue);
    }

    virtual void thenMergedDB()
    {
        JValue factoryConfigArray = pbnjson::Object();

        ASSERT_TRUE(m_unifiedDB.getDatabase().hasKey(JsonDB::CATEGORYNAME_CONFIGD));
        ASSERT_TRUE(m_unifiedDB.fetch(JsonDB::FULLNAME_USER, factoryConfigArray));
        ASSERT_TRUE(factoryConfigArray[JsonDB::FULLNAME_USER].isArray());
        ASSERT_STREQ(m_unifiedDB.getDatabase()[NAME_CATEGORY1][NAME_CONFIG1].asString().c_str(), NAME_CONFIG_VALUE2.c_str());
        ASSERT_EQ(3, m_unifiedDB.getDatabase().objectSize());
        ASSERT_TRUE(m_unifiedDB.isUpdated());
    }

    virtual void thenEmptyDB()
    {
        ASSERT_TRUE(m_testDB.getFilename().empty());
        ASSERT_TRUE(m_testDB.getDatabase().isValid());
        ASSERT_TRUE(m_testDB.getDatabase().isObject());
        ASSERT_FALSE(m_testDB.isUpdated());
        ASSERT_EQ(0, m_testDB.getDatabase().objectSize());
    }

    virtual void thenNotEmptyDB()
    {
        ASSERT_FALSE(m_testDB.getFilename().empty());
        ASSERT_TRUE(m_testDB.getDatabase().isValid());
        ASSERT_TRUE(m_testDB.getDatabase().isObject());
        ASSERT_FALSE(m_testDB.isUpdated());
        ASSERT_NE(0, m_testDB.getDatabase().objectSize());
    }

    const string NAME_CATEGORY1 = "testCategory1";
    const string NAME_CATEGORY2 = "testCategory2";

    const string NAME_CONFIG1 = "testConfig1";
    const string NAME_CONFIG2 = "testConfig2";
    const string NAME_CONFIG3 = "testConfig3";

    const string NAME_CONFIG_VALUE1 = "testConfigFirst";
    const string NAME_CONFIG_VALUE2 = "testConfigSecond";
    const string NAME_CONFIG_VALUE3 = "testConfigThird";

    const string PATH_TEST_DB = PATH_OUTPUT "/configd_db.json";
    const string PATH_SYNC_DB = PATH_OUTPUT "/configd_sync.json";

    JsonDB m_testDB;
    JsonDB m_testFactoryDB;
    JsonDB m_unifiedDB;

    string m_fullNameFirst;
    string m_fullNameSecond;

};

TEST_F(UnittestJsonDB, checkEmptyDatabaseStatus)
{
    givenEmptyDB();
    thenEmptyDB();
}

TEST_F(UnittestJsonDB, emptyDatabaseInsert)
{
    givenEmptyDB();

    ASSERT_TRUE(m_testDB.insert(NAME_CATEGORY1, NAME_CONFIG1, NAME_CONFIG_VALUE1));
    ASSERT_TRUE(m_testDB.getDatabase().hasKey(NAME_CATEGORY1));
    ASSERT_STREQ(m_testDB.getDatabase()[NAME_CATEGORY1][NAME_CONFIG1].asString().c_str(), NAME_CONFIG_VALUE1.c_str());
}

TEST_F(UnittestJsonDB, emptyDatabaseInsertAndFetch1)
{
    givenEmptyDB();

    JValue result = pbnjson::Object();

    ASSERT_TRUE(m_testDB.insert(NAME_CATEGORY1, NAME_CONFIG1, NAME_CONFIG_VALUE1));
    ASSERT_TRUE(m_testDB.fetch(m_fullNameFirst, result));
    ASSERT_EQ(NAME_CONFIG_VALUE1, result[m_fullNameFirst].asString());
}

TEST_F(UnittestJsonDB, emptyDatabaseInsertAndFetch2)
{
    givenEmptyDB();

    JValue result = pbnjson::Object();

    ASSERT_TRUE(m_testDB.insert(NAME_CATEGORY1, NAME_CONFIG1, NAME_CONFIG_VALUE1));
    ASSERT_TRUE(m_testDB.fetch(NAME_CATEGORY1, NAME_CONFIG1, result));
    ASSERT_EQ(NAME_CONFIG_VALUE1, result[m_fullNameFirst].asString());
}

TEST_F(UnittestJsonDB, setAndGetAboutFileName)
{
    givenEmptyDB();

    ASSERT_TRUE(m_testDB.setFilename(PATH_SYNC_DB));
    ASSERT_STREQ(PATH_SYNC_DB.c_str(), m_testDB.getFilename().c_str());
}

TEST_F(UnittestJsonDB, emptyDatabaseInsertAndFlushWithFileName)
{
    givenEmptyDB();

    ASSERT_TRUE(m_testDB.setFilename(PATH_SYNC_DB));
    ASSERT_TRUE(m_testDB.insert(NAME_CATEGORY1, NAME_CONFIG1, NAME_CONFIG_VALUE1));
    ASSERT_TRUE(m_testDB.flush());
    ASSERT_TRUE(Platform::isFileExist(PATH_SYNC_DB));
}

TEST_F(UnittestJsonDB, emptyDatabaseInsertAndFlushWithoutFileName)
{
    givenEmptyDB();

    ASSERT_TRUE(m_testDB.insert(NAME_CATEGORY1, NAME_CONFIG1, NAME_CONFIG_VALUE1));
    ASSERT_FALSE(m_testDB.flush());
    ASSERT_FALSE(Platform::isFileExist(PATH_SYNC_DB));
}

TEST_F(UnittestJsonDB, emptyDatabaseUpdate)
{
    givenEmptyDB();

    // insert operation
    ASSERT_TRUE(m_testDB.insert(NAME_CATEGORY1, NAME_CONFIG1, NAME_CONFIG_VALUE1));
    ASSERT_TRUE(m_testDB.getDatabase().hasKey(NAME_CATEGORY1));
    ASSERT_STREQ(m_testDB.getDatabase()[NAME_CATEGORY1][NAME_CONFIG1].asString().c_str(), NAME_CONFIG_VALUE1.c_str());

    // update operation
    ASSERT_TRUE(m_testDB.insert(NAME_CATEGORY1, NAME_CONFIG1, NAME_CONFIG_VALUE2));
    ASSERT_STREQ(m_testDB.getDatabase()[NAME_CATEGORY1][NAME_CONFIG1].asString().c_str(), NAME_CONFIG_VALUE2.c_str());
}

TEST_F(UnittestJsonDB, testDatabaseStatus)
{
    givenFileDB();
    thenNotEmptyDB();
}

TEST_F(UnittestJsonDB, sigletonsCheckFileName)
{
    givenEmptyDB();

    JsonDB &factory = JsonDB::getFactoryInstance();
    JsonDB &main = JsonDB::getMainInstance();

    ASSERT_STREQ(factory.getFilename().c_str(), JsonDB::FILENAME_FACTORY_DB.c_str());
    ASSERT_STREQ(main.getFilename().c_str(), JsonDB::FILENAME_MAIN_DB.c_str());
}

TEST_F(UnittestJsonDB, copySameDatabaseAndCheckIsUpdate)
{
    givenEmptyDB();

    ASSERT_TRUE(m_testDB.copy(m_testDB));
    ASSERT_TRUE(m_testDB.getFilename().empty());
    ASSERT_FALSE(m_testDB.isUpdated());
}

TEST_F(UnittestJsonDB, loadDatabaseWithNotExistFileName)
{
    givenFileDB("notExist.json");

    ASSERT_STREQ(m_testDB.getDatabase().stringify().c_str(), "{}");
}

TEST_F(UnittestJsonDB, loadDatabaseWhichContainNamelessObject)
{
    string tempDBName = TEST_DATA_PATH "/SimpleDB.json";
    givenFileDB(tempDBName);

    ASSERT_STREQ(m_testDB.getFilename().c_str(), tempDBName.c_str());
    ASSERT_FALSE(m_testDB.isUpdated());
    ASSERT_FALSE(m_testDB.getDatabase().isNull());
}

TEST_F(UnittestJsonDB, removeExistItem)
{
    givenMultiItemsDB();

    ASSERT_TRUE(m_testDB.remove(NAME_CATEGORY1, NAME_CONFIG1));
    ASSERT_EQ(2, m_testDB.getDatabase().objectSize());
    ASSERT_EQ(2, m_testDB.getDatabase()[NAME_CATEGORY1].objectSize());

    ASSERT_TRUE(m_testDB.remove(" " + NAME_CATEGORY1 + "." + NAME_CONFIG2));
    ASSERT_EQ(2, m_testDB.getDatabase().objectSize());
    ASSERT_EQ(1, m_testDB.getDatabase()[NAME_CATEGORY1].objectSize());
}

TEST_F(UnittestJsonDB, updateExistItem)
{
    givenMultiItemsDB();
    ASSERT_TRUE(m_testDB.insert(" " + NAME_CATEGORY1 + "." + NAME_CONFIG1, NAME_CONFIG_VALUE2));
    ASSERT_TRUE(m_testDB.insert(NAME_CATEGORY2, NAME_CONFIG1, NAME_CONFIG_VALUE2));
    ASSERT_EQ(2, m_testDB.getDatabase().objectSize());
    ASSERT_EQ(3, m_testDB.getDatabase()[NAME_CATEGORY1].objectSize());
    ASSERT_EQ(3, m_testDB.getDatabase()[NAME_CATEGORY2].objectSize());
    ASSERT_STREQ(m_testDB.getDatabase()[NAME_CATEGORY1][NAME_CONFIG1].asString().c_str(), NAME_CONFIG_VALUE2.c_str());
}

TEST_F(UnittestJsonDB, removeNotExistItem)
{
    givenMultiItemsDB();

    ASSERT_TRUE(m_testDB.remove(NAME_CONFIG1, NAME_CATEGORY1));
    ASSERT_EQ(2, m_testDB.getDatabase().objectSize());
    ASSERT_EQ(3, m_testDB.getDatabase()[NAME_CATEGORY1].objectSize());

    ASSERT_TRUE(m_testDB.remove(NAME_CATEGORY1, "NotExist"));
    ASSERT_EQ(2, m_testDB.getDatabase().objectSize());
    ASSERT_EQ(3, m_testDB.getDatabase()[NAME_CATEGORY1].objectSize());
}

TEST_F(UnittestJsonDB, overwriteEmptyDatabaseAndCompareDatabases)
{
    JsonDB empty;

    givenMultiItemsDB();

    ASSERT_TRUE(empty.merge(m_testDB));
    ASSERT_TRUE(empty.getDatabase() == m_testDB.getDatabase());
}

TEST_F(UnittestJsonDB, fetchFromEmptyDB)
{
    givenEmptyDB();

    JValue result = pbnjson::Object();

    ASSERT_FALSE(m_testDB.fetch(NAME_CATEGORY1, NAME_CONFIG1, result));
    ASSERT_FALSE(m_testDB.fetch(m_fullNameFirst, result));
    ASSERT_FALSE(m_testDB.fetch(m_fullNameSecond, result));
}

TEST_F(UnittestJsonDB, fetchNotExistItem)
{
    givenMultiItemsDB();

    JValue result = pbnjson::Object();

    ASSERT_FALSE(m_testDB.fetch("NotExist", NAME_CONFIG1, result));
    ASSERT_FALSE(m_testDB.fetch(NAME_CATEGORY1, "NotExist", result));
}

TEST_F(UnittestJsonDB, fetchExistItem)
{
    givenMultiItemsDB();

    JValue result = pbnjson::Object();

    ASSERT_TRUE(m_testDB.fetch(NAME_CATEGORY1, "*", result));
    ASSERT_STREQ(NAME_CONFIG_VALUE1.c_str(), result[m_fullNameFirst].asString().c_str());
    ASSERT_STREQ(NAME_CONFIG_VALUE2.c_str(), result[m_fullNameSecond].asString().c_str());
}

TEST_F(UnittestJsonDB, syncDatabaseWithoutDatabaseFileNameSetting)
{
    givenEmptyDB();

    ASSERT_TRUE(m_testDB.flush());
    ASSERT_TRUE(m_testDB.insert(NAME_CATEGORY1, NAME_CONFIG1, NAME_CONFIG_VALUE1));
    ASSERT_FALSE(m_testDB.flush());
}

TEST_F(UnittestJsonDB, makeFullNameDBOperation)
{
    givenFileDB(TEST_DATA_PATH "/SimpleDB.json");

    JValue result = pbnjson::Object();
    JValue category = m_testDB.getDatabase()[NAME_CATEGORY1];

    ASSERT_TRUE(category.isValid());
    ASSERT_FALSE(JsonDB::getFullDBName("", category, result));
    ASSERT_TRUE(result.isValid());

    ASSERT_TRUE(JsonDB::getFullDBName(NAME_CATEGORY1, category, result));
    ASSERT_TRUE(result.isValid());
}

TEST_F(UnittestJsonDB, splitInvalidFullName)
{
    string categoryName, configName;

    ASSERT_FALSE(JsonDB::split(NAME_CATEGORY1, categoryName, configName));
}

TEST_F(UnittestJsonDB, splitValidFullName)
{
    string categoryName, configName;

    ASSERT_TRUE(JsonDB::split(m_fullNameFirst, categoryName, configName));
    ASSERT_STREQ(categoryName.c_str(), NAME_CATEGORY1.c_str());
    ASSERT_STREQ(configName.c_str(), NAME_CONFIG1.c_str());
}

TEST_F(UnittestJsonDB, copyAndMergeDB)
{
    givenMultiItemsDB();
    givenFactoryDB();

    m_unifiedDB.copy(m_testDB);
    m_unifiedDB.merge(m_testFactoryDB);

    thenMergedDB();
}

TEST_F(UnittestJsonDB, searchCategoryName)
{
    givenMultiItemsDB();
    JValue result = pbnjson::Object();
    string searchRegEx = NAME_CATEGORY1;
    EXPECT_TRUE(m_testDB.searchKey(searchRegEx, result));

    EXPECT_TRUE(result.hasKey(NAME_CATEGORY1 + "." + NAME_CONFIG1));
    EXPECT_TRUE(result.hasKey(NAME_CATEGORY1 + "." + NAME_CONFIG2));
    EXPECT_TRUE(result.hasKey(NAME_CATEGORY1 + "." + NAME_CONFIG3));
    ASSERT_STREQ(result[NAME_CATEGORY1 + "." + NAME_CONFIG1].asString().c_str(), NAME_CONFIG_VALUE1.c_str());
    ASSERT_STREQ(result[NAME_CATEGORY1 + "." + NAME_CONFIG2].asString().c_str(), NAME_CONFIG_VALUE2.c_str());
    ASSERT_STREQ(result[NAME_CATEGORY1 + "." + NAME_CONFIG3].asString().c_str(), NAME_CONFIG_VALUE3.c_str());
}

TEST_F(UnittestJsonDB, searchKey)
{
    givenMultiItemsDB();
    JValue result = pbnjson::Object();
    string searchRegEx = "\." + NAME_CONFIG2 + "$";
    EXPECT_TRUE(m_testDB.searchKey(searchRegEx, result));

    EXPECT_TRUE(result.hasKey(NAME_CATEGORY1 + "." + NAME_CONFIG2));
    EXPECT_TRUE(result.hasKey(NAME_CATEGORY2 + "." + NAME_CONFIG2));
    ASSERT_STREQ(result[NAME_CATEGORY1 + "." + NAME_CONFIG2].asString().c_str(), NAME_CONFIG_VALUE2.c_str());
    ASSERT_STREQ(result[NAME_CATEGORY2 + "." + NAME_CONFIG2].asString().c_str(), NAME_CONFIG_VALUE2.c_str());
}
