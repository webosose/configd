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

#ifndef _DBCOMPARATOR_H_
#define _DBCOMPARATOR_H_

#include <iostream>
#include <pbnjson.hpp>

#include "database/JsonDB.h"

using namespace std;
using namespace pbnjson;

class DBComparator {
public:
    static JValue compareTwoFiles(const char* A, const char* B);

    DBComparator();
    virtual ~DBComparator();

    JValue getConfig(const string fullname);
    static JValue printDiffArray(JValue a, JValue b);
    static JValue printDiffObject(JValue a, JValue b);

    bool isEqual(const string& filename);

    const string& getBase()
    {
        return m_baseJsonDB.getFilename();
    }

    void setBase(const string& filename);

private:
    JsonDB m_baseJsonDB;
};

#endif /* _DBCOMPARATOR_H_ */
