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

#ifndef _JSONDB_H_
#define _JSONDB_H_

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pbnjson.h>
#include <pbnjson.hpp>
#include <pbnjson/c/jtypes.h>

#include "Environment.h"

using namespace std;
using namespace pbnjson;

class JsonDB {
public:
    static const string FILENAME_MAIN_DB;
    static const string FILENAME_FACTORY_DB;
    static const string FILENAME_DEBUG_DB;
    static const string FILENAME_PERMISSION_DB;

    static const string CATEGORYNAME_CONFIGD;

    static const string FULLNAME_SELECTION;
    static const string FULLNAME_LAYERS;
    static const string FULLNAME_USER;
    static const string FULLNAME_REASON;
    static const string FULLNAME_LAYERSVERSION;

    static const string FULLNAME_DEBUG_LOAD;
    static const string FULLNAME_DEBUG_RECONFIGURE;
    static const string FULLNAME_DEBUG_SETCONFIGS;
    static const string FULLNAME_DEBUG_PREPROCESS;
    static const string FULLNAME_DEBUG_POSTPROCESS;

    static JsonDB& getMainInstance()
    {
        static JsonDB _mainInstance("Main Database");
        if (_mainInstance.getFilename().empty()) {
            _mainInstance.load(FILENAME_MAIN_DB);
        }
        return _mainInstance;
    }

    static JsonDB& getFakeFactoryInstance()
    {
        static JsonDB _fakeFactoryInstance("FakeFactory Database");
        return _fakeFactoryInstance;
    }

    static JsonDB& getVolatileInstance()
    {
        static JsonDB _volatileInstance("Volatile Database");
        return _volatileInstance;
    }

    static JsonDB& getFactoryInstance()
    {
        static JsonDB _factoryInstance("Factory Database");
        if (_factoryInstance.getFilename().empty()) {
            _factoryInstance.load(FILENAME_FACTORY_DB);
        }
        return _factoryInstance;
    }

    static JsonDB& getPermissionInstance()
    {
        static JsonDB _permissionInstance("Permission Database");
        if (_permissionInstance.getFilename().empty()) {
            _permissionInstance.load(FILENAME_PERMISSION_DB);
        }
        return _permissionInstance;
    }

    static JsonDB& getDebugInstance()
    {
        static JsonDB _debugInstance("Debug Database");
        if (_debugInstance.getFilename().empty()) {
            _debugInstance.load(FILENAME_DEBUG_DB);
        }
        return _debugInstance;
    }

    static JsonDB& getUnifiedInstance()
    {
        static JsonDB _unifiedInstance("Unified Database");
        return _unifiedInstance;
    }

    static bool split(const string &fullName, string &categoryName, string &configName);
    static bool getFullDBName(const string &categoryName, const JValue &category, JValue &result);

    JsonDB(string name = "Unknown Database");
    virtual ~JsonDB();

    bool load(const string &filename);
    bool copy(JsonDB& jsonDB);
    bool merge(JValue& database);
    bool merge(JsonDB& jsonDB);
    bool clear();
    bool flush();

    bool insert(const string &fullName, JValue value);
    bool insert(const string &categoryName, const string &configName, JValue value);
    bool remove(const string &fullName);
    bool remove(const string &categoryName, const string &configName);
    bool fetch(const string &categoryName, const string &configName, JValue &result);
    bool fetch(const string &fullName, JValue &result);
    bool searchKey(const string &regEx, JValue &result);

    JValue &getDatabase();
    string &getFilename();
    bool setFilename(const string &filename);
    bool isUpdated();
    bool isEqualDatabase(JsonDB& jsonDB);
    bool isEqualFilename(JsonDB& jsonDB);
    void printDebug();

private:
    JValue m_database;

    string m_name;
    string m_filename;
    bool m_isUpdated;

};

#endif //_JSONDB_H_

