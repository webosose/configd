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

#ifndef _LAYER_H_
#define _LAYER_H_

#include <glib.h>
#include <strings.h>
#include <iostream>

#include <luna-service2/lunaservice.h>
#include <pbnjson.hpp>

#include "database/JsonDB.h"
#include "service/AbstractBusFactory.h"

using namespace std;
using namespace pbnjson;

typedef enum {
    SelectorType_None = 0,      // No selection
    SelectorType_String,        // Just string
    SelectorType_File,          // file
    SelectorType_Command,       // command
    SelectorType_Luna,          // luna
} SelectorType;

class Layer;

class LayerListener {
public:
    LayerListener() {};
    virtual ~LayerListener() {};

    virtual void onSelectionChanged(Layer &layer, string &oldSelection, string &newSelection) = 0;
};

class Layer : public IHandleListener {
public:
    static char* getSelectorTypeStr(const SelectorType& type);

    Layer(JValue layer);
    virtual ~Layer();

    void printDebug();

    // LS2 related methods
    virtual void onReceiveCall(JValue &response);
    bool call();
    void cancelCall();

    // fetches
    static bool parseFiles(string &dirPath, JValue *database, JsonDB *jsonDB = NULL, JsonDB *permissionDB = NULL);
    bool fetchConfigs(JValue &database);
    bool fetchConfigs(JsonDB &jsonDB, JsonDB *permissionDB = NULL);

    // save & restore
    void fromJson(JValue json);
    JValue toJson();

    void select();
    bool isSelected();
    bool isReadOnlyType();
    void clearSelection();
    bool setSelection(string selection);

    // event
    void setListener(LayerListener* listener);

    // getter
    const string &getSelection();
    string &getName();
    string getFullDirPath(bool withSelection = true);
    string getLeafDirPath(bool withSelection = false);
    SelectorType &getType();
    int getPriority() { return m_priority; };

    bool requirePreProcessing();
    bool requirePostProcessing();
    static JValue refineContent(JValue& content);
    static JValue getMatchedConfigs(JValue& content, JsonDB *jsonDB);

private:
    static bool isValidLSSelector(const JValue &selector);
    static bool isValidStrSelector(const JValue &selector, const string key);
    static SelectorType getSelectorType(const JValue &selector);

    // recursive function
    bool findSelection(const JValue &selector, string &selection);

    // R/W members
    string m_selection;
    bool m_isSelected;
    LayerListener *m_listener;
    shared_ptr<ICall> m_call;

    // R/O members
    JValue m_layer;
    JValue m_selector;
    string m_name;
    string m_baseDir;
    bool m_requirePreProcessing;
    bool m_requirePostProcessing;
    SelectorType m_type;
    int m_priority;

};

#endif /* _LAYER_H_ */
