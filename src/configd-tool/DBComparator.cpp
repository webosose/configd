// Copyright (c) 2017-2020 LG Electronics, Inc.
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

#include "DBComparator.h"

#include "util/Platform.h"

JValue DBComparator::compareTwoFiles(const char* A, const char* B)
{
    JValue contentA = JDomParser::fromFile(A);
    JValue contentB = JDomParser::fromFile(B);
    JValue result = pbnjson::Object();
    JValue removedKeys = pbnjson::Array();
    JValue addedKeys = pbnjson::Array();
    JValue changedKeys = pbnjson::Array();

    if (contentA.isNull() || contentB.isNull()) {
        return result;
    }

    if (contentA == contentB) {
        result.put("isDifferent", false);
        return result;
    }

    for (JValue::KeyValue category : contentA.children()) {
        string categoryName = category.first.asString();

        for (JValue::KeyValue config : category.second.children()) {
            string configName = config.first.asString();
            JValue key = pbnjson::Object();
            JValue diffValue = pbnjson::Object();

            if (!contentB.hasKey(categoryName) || !contentB[categoryName].hasKey(configName)) {
                removedKeys.append(categoryName + "." + configName);
                continue;
            }

            if (contentA[categoryName][configName] != contentB[categoryName][configName]) {
                if (contentA[categoryName][configName].isArray() && contentB[categoryName][configName].isArray()) {
                    diffValue = printDiffArray(contentA[categoryName][configName], contentB[categoryName][configName]);
                } else if(contentA[categoryName][configName].isObject() && contentB[categoryName][configName].isObject()) {
                    diffValue = printDiffObject(contentA[categoryName][configName], contentB[categoryName][configName]);
                } else {
                    diffValue.put("before", contentA[categoryName][configName]);
                    diffValue.put("after", contentB[categoryName][configName]);
                }

                if (diffValue.objectSize() > 0) {
                    key.put(categoryName + "." + configName, diffValue);
                    changedKeys.append(key);
                }
            }
        }
    }

    for (JValue::KeyValue category : contentB.children()) {
        string categoryName = category.first.asString();
        for (JValue::KeyValue config : category.second.children()) {
            string configName = config.first.asString();

            if (!contentA.hasKey(categoryName) || !contentA[categoryName].hasKey(configName)) {
                addedKeys.append(categoryName + "." + configName);
            }
        }
    }
    if (removedKeys.arraySize() > 0)
        result.put("removedKeys", removedKeys);
    if (addedKeys.arraySize() > 0)
        result.put("addedKeys", addedKeys);
    if (changedKeys.arraySize() > 0)
        result.put("changedKeys", changedKeys);

    result.put("isDifferent", true);
    return result;
}

DBComparator::DBComparator()
{
}

DBComparator::~DBComparator()
{
}

JValue DBComparator::printDiffArray(JValue objectA, JValue objectB)
{
    JValue result = pbnjson::Object();
    JValue removed = pbnjson::Array();
    JValue added = pbnjson::Array();

    if (!objectA.isArray() || !objectB.isArray())
        return result;

    for (ssize_t i = 0; i < objectA.arraySize(); i++) {
        bool exist = false;
        for (ssize_t j = 0; j < objectB.arraySize(); j++) {
            if (objectA[i].asString() == objectB[j].asString()) {
                exist = true;
            }
        }
        if (!exist) {
            removed.append(objectA[i].asString());
        }
    }
    for (ssize_t i = 0; i < objectB.arraySize(); i++) {
        bool exist = false;
        for (ssize_t j = 0; j < objectA.arraySize(); j++) {
            if (objectB[i].asString() == objectA[j].asString()) {
                exist = true;
            }
        }
        if (!exist) {
            added.append(objectB[i].asString());
        }
    }

    if (added.arraySize() > 0)
        result.put("addedElement", added);
    if (removed.arraySize() > 0)
        result.put("removedElement", removed);
    return result;
}

JValue DBComparator::printDiffObject(JValue objectA, JValue objectB)
{
    JValue result = pbnjson::Object();
    JValue removed = pbnjson::Array();
    JValue added = pbnjson::Array();
    JValue changed = pbnjson::Object();

    if (!objectA.isObject() || !objectA.isObject())
        return result;

    for (JValue::KeyValue keyA : objectA.children()) {
        string key = keyA.first.asString();
        if (!objectB.hasKey(key)) {
            removed.append(keyA.second);
        } else if (objectB[key] != keyA.second ) {
            changed.put("before", keyA.second);
            changed.put("after", objectB[key]);
        }
    }
    for (JValue::KeyValue keyB : objectB.children()) {
        string key = keyB.first.asString();
        if (!objectA.hasKey(key)) {
            added.append(key);
        }
    }

    if (added.arraySize() > 0)
        result.put("addedElement", added);
    if (removed.arraySize() > 0)
        result.put("removedElement", removed);
    if (changed.objectSize() > 0)
        result.put("changedObject", changed);

    return result;
}

JValue DBComparator::getConfig(const string fullname)
{
    JValue result = pbnjson::Object();
    if (!m_baseJsonDB.fetch(fullname, result)) {
        cerr << "Error in database fetch" << endl;
    }
    return result;
}

bool DBComparator::isEqual(const string& filename)
{
    JsonDB db;
    db.load(filename);
    return m_baseJsonDB.isEqualDatabase(db);
}

void DBComparator::setBase(const string& filename)
{
    return m_baseJsonDB.load(filename);
}
