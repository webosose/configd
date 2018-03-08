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

#ifndef _LS2COMPARATOR_H_
#define _LS2COMPARATOR_H_

#include <iostream>
#include <pbnjson.hpp>

using namespace std;
using namespace pbnjson;

class LS2Comparator {
public:
    static JValue convertFileToJValue(const string &filename);

    LS2Comparator();
    LS2Comparator(const string& a, const string& b);
    virtual ~LS2Comparator();

    bool isEqual();
    bool isLoaded(const string& filename);

private:
    string m_filenameA;
    string m_filenameB;

    JValue m_jsonA;
    JValue m_jsonB;
};

#endif /* _LS2COMPARATOR_H_ */
