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

#include "Json.h"
#include "Logger.hpp"

void Json::addUniqueStrIntoArray(JValue array, string uniqueStr)
{
    if (!array.isArray() || uniqueStr.empty())
        return;

    for (ssize_t i = 0; i < array.arraySize(); i++) {
        if (array[i].asString() == uniqueStr) {
            return;
        }
    }

    array.append(uniqueStr);
}

void Json::removeUniqueStrIntoArray(JValue array, string uniqueStr)
{
    if (!array.isArray() || uniqueStr.empty())
        return;

    for (ssize_t i = 0; i < array.arraySize(); i++) {
        if (array[i].asString() == uniqueStr) {
            jarray_remove(array.peekRaw(), i);
            return;
        }
    }
}

bool Json::getValueWithKeys(JValue root, JValue keys, JValue &value)
{
    JValue currentParent = root;
    for (int i = 0; i < keys.arraySize(); i++) {
        if (!currentParent.isObject() || !currentParent.hasKey(keys[i].asString())) {
            return false;
        }
        currentParent = currentParent[keys[i].asString()];
    }

    value = currentParent;
    return true;
}

void Json::printDiffValue(JValue a, JValue b)
{
    if (a == b)
        return;

    bool isDifferent = false;
    for (JValue::KeyValue category : a.children()) {
        string categoryName = category.first.asString();

        if (a[categoryName] != b[categoryName]) {
            isDifferent = true;
            Logger::verbose(LOG_PREPIX_FORMAT "\nA = (%s)\n\nB = (%s)",
                            LOG_PREPIX_ARGS,
                            a[categoryName].stringify("    ").c_str(),
                            b[categoryName].stringify("    ").c_str());
        }
    }

    if (isDifferent)
        Logger::debug(LOG_PREPIX_FORMAT "Two json objects are different", LOG_PREPIX_ARGS);
}
