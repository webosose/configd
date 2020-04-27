// Copyright (c) 2014-2020 LG Electronics, Inc.
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

#ifndef UTIL_JSON_H_
#define UTIL_JSON_H_

#include <iostream>

#include <pbnjson.h>
#include <pbnjson.hpp>

using namespace std;
using namespace pbnjson;

class Json {
public:
    static void addUniqueStrIntoArray(JValue array, string uniqueStr);
    static bool getValueWithKeys(JValue root, JValue array, JValue &value);
    static void printDiffValue(JValue a, JValue b);

private:
    Json() {};
    virtual ~Json() {};
};

#endif /* UTIL_JSON_H_ */
