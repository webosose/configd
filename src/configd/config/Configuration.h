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

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include <iostream>
#include <list>
#include <stdbool.h>
#include <vector>
#include <glib.h>
#include <glib/gstdio.h>

#include <luna-service2/lunaservice.h>
#include <pbnjson.hpp>

#include "Layer.h"
#include "database/JsonDB.h"

using namespace std;
using namespace pbnjson;

class Configuration {
public:
    static Configuration& getInstance()
    {
        static Configuration _instance;
        return _instance;
    }
    virtual ~Configuration();

    // selection
    void setListener(LayerListener *listener);
    void selectAll();
    bool isAllSelected();
    void updateSelections(JsonDB &jsonDB);
    void clearAllSelections();

    // fetch
    void fetchConfigs(JValue &database);
    bool fetchConfigs(JsonDB &jsonDB, JsonDB *permissionDB = NULL);
    void fetchLayers(JsonDB &jsonDB);

    // layer
    int getLayersSize();
    Layer* getLayer(string name);
    bool insertLayer(Layer &layerInfo);
    bool isLayersSorted();
    string getLayersVersion() { return m_version; }

    // about conf file
    void clear();
    bool append(string filePath);
    std::vector<std::string> getConfFilePaths() const;
    bool runPostProcess(JsonDB &jsonDB);
    bool runPreProcess();

    // debug purpose
    void printSelections(string prefix);
    void printDebug();

private:
    static const string PATH_LAYERS_DIR;
    static const string PATH_DEBUG_LAYERS_DIR;
    static const string POST_PROCESS_OP_PREFIX;
    static const string POST_PROCESS_IP_PREFIX;

    Configuration();

    JValue m_postProcessing;
    JValue m_preProcessing;
    std::vector<std::string> m_filePaths;
    std::list<Layer> m_layers;
    string m_version;
};

#endif // _CONFIGURATION_H_

