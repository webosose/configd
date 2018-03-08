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

#include <iostream>
#include <gtest/gtest.h>
#include <pbnjson.hpp>

#include "Environment.h"
#include "util/Platform.h"

using namespace pbnjson;
using namespace std;

TEST(IntegrationPbnJsonCpp, FileSchema)
{
    JSchema schema = JSchema::fromFile(GETCONFIGS_SCHEMA);
    ASSERT_TRUE(schema.isInitialized());

    JValue result = pbnjson::Object();
    string path = "tests/external/_data/getConfigsValid.json";
    result = JDomParser::fromFile(path.c_str(), schema);
    ASSERT_TRUE(result.isValid() && !result.isNull());
}

TEST(IntegrationPbnJsonCpp, NullSchema)
{
    JValue result = JDomParser::fromString("{ \"test\": true }");

    ASSERT_TRUE(result.isValid() && !result.isNull());
    ASSERT_TRUE(result["test"].asBool());
}

TEST(IntegrationPbnJsonCpp, JValuePutMethod)
{
    JValue result;

    ASSERT_FALSE(result.put("test", true));
    result = pbnjson::Object();
    ASSERT_TRUE(result.put("test", true));
}

TEST(IntegrationPbnJsonCpp, JValueReference)
{
    JValue result = pbnjson::Object();

    ASSERT_TRUE(result.put("test", true));
    JValue &refer = result;
    ASSERT_TRUE(refer.put("test", false));
    ASSERT_FALSE(result["test"].asBool());
}

TEST(IntegrationPbnJsonCpp, JValueReferenceModification)
{
    JValue object = JDomParser::fromString("{ \"parent\": { \"child\": { \"child_child\": true } } }");
    JValue child = object["parent"]["child"];

    ASSERT_TRUE(child.put("child_child", false));
    ASSERT_FALSE(child["child_child"].asBool());
    ASSERT_FALSE(object["parent"]["child"]["child_child"].asBool());
}

TEST(IntegrationPbnJsonCpp, JValueComparison)
{
    JValue a = JDomParser::fromString("{ \"test1\": true, \"test2\": false }");
    JValue b = JDomParser::fromString("{ \"test2\": false, \"test1\": true }");

    ASSERT_TRUE(a == b);
    ASSERT_TRUE(b.remove("test1"));
    ASSERT_FALSE(a == b);
}

TEST(IntegrationPbnJsonCpp, ArraySize)
{
    JValue array = pbnjson::Array();

    ASSERT_TRUE(array.append("item1"));
    ASSERT_EQ(array.arraySize(), 1);
    ASSERT_TRUE(array.append("item2"));
    ASSERT_EQ(array.arraySize(), 2);
    ASSERT_TRUE(array.append("item3"));
    ASSERT_EQ(array.arraySize(), 3);
}

TEST(IntegrationPbnJsonCpp, ArrayRemove)
{
    JValue array = pbnjson::Array();

    ASSERT_TRUE(array.append("item1"));
    ASSERT_TRUE(array.append("item2"));
    ASSERT_TRUE(array.append("item3"));
    ASSERT_EQ(array.arraySize(), 3);
    jarray_remove(array.peekRaw(), 2);
    ASSERT_EQ(array.arraySize(), 2);
}

TEST(IntegrationPbnJsonCpp, ArrayItemCompare)
{
    JValue array = pbnjson::Array();

    ASSERT_TRUE(array.append("item1"));
    ASSERT_TRUE(array.append("item2"));
    ASSERT_TRUE(array.append("item3"));

    ASSERT_TRUE(array[2] == "item3");
    ASSERT_TRUE(array[2] != "item2");
}

TEST(IntegrationPbnJsonCpp, ObjectSize)
{
    JValue object = pbnjson::Object();

    ASSERT_TRUE(object.put("key1", true));
    ASSERT_EQ(object.objectSize(), 1);
    ASSERT_TRUE(object.put("key2", true));
    ASSERT_EQ(object.objectSize(), 2);
    ASSERT_TRUE(object.put("key3", true));
    ASSERT_EQ(object.objectSize(), 3);
    ASSERT_TRUE(object.put("key3", true));
    ASSERT_EQ(object.objectSize(), 3);
}

TEST(IntegrationPbnJsonCpp, ObjectDeepCopy)
{
    JValue object = JDomParser::fromString("{ \"parent\": { \"child\": { \"child_child\": true } } }");

    ASSERT_TRUE(object.isValid() && object.isObject());
    JValue ref = object;
    ASSERT_TRUE(object == ref);
    ASSERT_TRUE(object["parent"]["child"]["child_child"].asBool() == ref["parent"]["child"]["child_child"].asBool());
    ASSERT_TRUE(ref["parent"]["child"].put("child_child", false));
    // "child_child" values are false both object and ref.
    ASSERT_TRUE(object["parent"]["child"]["child_child"].asBool() == ref["parent"]["child"]["child_child"].asBool());
    JValue copy = object.duplicate();
    ASSERT_TRUE(object["parent"]["child"]["child_child"].asBool() == copy["parent"]["child"]["child_child"].asBool());
    ASSERT_TRUE(copy["parent"]["child"].put("child_child", true));
    ASSERT_TRUE(object["parent"]["child"]["child_child"].asBool() != copy["parent"]["child"]["child_child"].asBool());
}
