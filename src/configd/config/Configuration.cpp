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

#include "Configuration.h"

#include <glib.h>
#include <Manager.h>
#include <strings.h>
#include <pbnjson.h>
#include <stdlib.h>

#include "Environment.h"
#include "Process.h"
#include "util/Platform.h"
#include "util/Logger.hpp"

const string Configuration::PATH_LAYERS_DIR = INSTALL_SYSCONFDIR "/configd";
const string Configuration::PATH_DEBUG_LAYERS_DIR = INSTALL_SYSMGR_LOCALSTATEDIR "/preferences";
const string Configuration::POST_PROCESS_OP_PREFIX = "cfgdi";
const string Configuration::POST_PROCESS_IP_PREFIX = "cfgdo";

Configuration::Configuration()
{
    string machineConf = Platform::concatPaths(PATH_LAYERS_DIR, "layers_" TARGET_MACHINE ".json");
    string distroConf = Platform::concatPaths(PATH_LAYERS_DIR, "layers_" TARGET_DISTRO".json");
    string defaultConf = Platform::concatPaths(PATH_LAYERS_DIR, "layers.json");
    string debugConf = Platform::concatPaths(PATH_DEBUG_LAYERS_DIR, "layers_debug.json");

    if (!append(machineConf)) {
        if (!append(defaultConf))
            Logger::warning(MSGID_CONFIGURE, LOG_PREPIX_FORMAT "Error in defaultConf append", LOG_PREPIX_ARGS);
    }
    if (!append(distroConf))
        Logger::warning(MSGID_CONFIGURE, LOG_PREPIX_FORMAT "Error in distroConf append", LOG_PREPIX_ARGS);
    if (!append(debugConf))
        Logger::warning(MSGID_CONFIGURE, LOG_PREPIX_FORMAT "Error in debugConf append", LOG_PREPIX_ARGS);
    Logger::debug(LOG_PREPIX_FORMAT "Layers Version is %s", LOG_PREPIX_ARGS, m_version.c_str());
}

Configuration::~Configuration()
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        it->cancelCall();
    }
}

bool Configuration::insertLayer(Layer &layerInfo)
{
    auto position = m_layers.end();
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        if (layerInfo.getName() == it->getName()) {
            it = m_layers.erase(it);
        }
        if (layerInfo.getPriority() == it->getPriority()) {
            return false;
        } else if (layerInfo.getPriority() < it->getPriority() &&
                   position == m_layers.end()) {
            position = it;
        }
    }
    if (position == m_layers.end()) {
        m_layers.push_back(layerInfo);
    } else {
        m_layers.insert(position, layerInfo);
    }
    return true;
}

bool Configuration::isLayersSorted()
{

    auto iter = std::adjacent_find(m_layers.begin(), m_layers.end(),
                                   [=] (Layer& layer1, Layer& layer2)
                                   -> bool { return (layer1.getPriority() > layer2.getPriority()); });
    if (iter == m_layers.end())
        return true;
    else
        return false;
}

std::vector<std::string> Configuration::getConfFilePaths() const
{
    return m_filePaths;
}

void Configuration::clear()
{
    m_filePaths.clear();
    m_layers.clear();
    m_postProcessing = nullptr;
    m_preProcessing = nullptr;
    m_version = "";
}

bool Configuration::append(std::string filename)
{
    if (!Platform::isFileExist(filename))
        return false;

    for (std::string path : m_filePaths) {
        if (filename == path)
            return true;
    }

    m_filePaths.push_back(filename);

    JSchema schema = JSchema::fromFile(CONFIGLAYERS_SCHEMA);
    JValue configuration = JDomParser::fromFile(filename.c_str(), schema);

    if (!configuration.isValid() || configuration.isNull()) {
        Logger::error(MSGID_JSON_PARSE_FILE_ERR,
                      LOG_PREPIX_FORMAT "Fail Invalid JSON Format in File: %s",
                      LOG_PREPIX_ARGS,
                      filename.c_str());
        return false;
    }

    if (configuration.hasKey("version")) {
        if (m_version.empty())
            m_version = configuration["version"].asString();
        else
            m_version.append("_" + configuration["version"].asString());
    }
    // higher layer value would replace the pre/postProcessing array.
    if (configuration.hasKey("post_process") && configuration["post_process"].isArray())
        m_postProcessing = configuration["post_process"];
    if (configuration.hasKey("pre_process") && configuration["pre_process"].isArray())
        m_preProcessing = configuration["pre_process"];

    Logger::info(MSGID_MANAGER,
                 LOG_PREPIX_FORMAT "Layer Configuration (%s)",
                 LOG_PREPIX_ARGS, filename.c_str());

    for (int i = 0; i < configuration["layers"].arraySize(); i++) {
        if (configuration["layers"][i].isNull() || !configuration["layers"][i].isObject()) {
            Logger::warning(MSGID_CONFIGUREDATA,
                            LOG_PREPIX_FORMAT "Invalid Layer : index = [%d]",
                            LOG_PREPIX_ARGS,
                            i);
            continue;
        }
        Layer layerInfo(configuration["layers"][i]);
        if (!insertLayer(layerInfo)) {
            Logger::warning(MSGID_CONFIGUREDATA,
                            LOG_PREPIX_FORMAT "insertLayer error",
                            LOG_PREPIX_ARGS);
        }
    }

    return true;
}

int Configuration::getLayersSize()
{
    return m_layers.size();
}

Layer* Configuration::getLayer(string name)
{
    auto it = std::find_if(m_layers.begin(), m_layers.end(),
                           [=] (Layer& layer) { return layer.getName() == name; });
    if (m_layers.end() == it)
        return NULL;
    else
        return &(*it);
}

void Configuration::setListener(LayerListener *listener)
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        it->setListener(listener);
    }
}

void Configuration::selectAll()
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        it->select();
    }
}

bool Configuration::isAllSelected()
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        if (!it->isSelected()) {
            Logger::debug(LOG_PREPIX_FORMAT "'%s' is not selected",
                          LOG_PREPIX_ARGS, it->getName().c_str());
            return false;
        }
    }
    return true;
}

void Configuration::updateSelections(JsonDB &jsonDB)
{
    JValue layers = pbnjson::Object();
    if (!jsonDB.fetch(JsonDB::FULLNAME_LAYERS, layers))
        Logger::warning(MSGID_CONFIGUREDATA, LOG_PREPIX_FORMAT "Error in database fetch", LOG_PREPIX_ARGS);

    if (!layers.hasKey(JsonDB::FULLNAME_LAYERS))
        return;

    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        if (layers[JsonDB::FULLNAME_LAYERS].hasKey(it->getName())) {
            it->fromJson(layers[JsonDB::FULLNAME_LAYERS][it->getName()]);
        }
    }
}

void Configuration::clearAllSelections()
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        it->clearSelection();
    }
}

void Configuration::fetchConfigs(JValue &database)
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        it->fetchConfigs(database);
    }
}

void Configuration::fetchConfigs(JsonDB &jsonDB, JsonDB *permissionDB)
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        it->fetchConfigs(jsonDB, permissionDB);
    }
}

void Configuration::fetchLayers(JsonDB &jsonDB)
{
    JValue selections = pbnjson::Object();
    JValue layers = pbnjson::Object();

    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        layers.put(it->getName(), it->toJson());

        // Some components use the following configs.
        // If you want to delete following configs, do test in firstUse in other distro.
        if (it->isSelected()) {
            selections.put(it->getLeafDirPath(), it->getSelection());
        }
    }

    if (!jsonDB.insert(JsonDB::FULLNAME_SELECTION, selections)) {
        Logger::debug(LOG_PREPIX_FORMAT "Failed to insert '%s' key under",
                      LOG_PREPIX_ARGS, JsonDB::FULLNAME_SELECTION.c_str());
    }
    if (!jsonDB.insert(JsonDB::FULLNAME_LAYERS, layers)) {
        Logger::debug(LOG_PREPIX_FORMAT "Failed to insert '%s' key under",
                      LOG_PREPIX_ARGS, JsonDB::FULLNAME_LAYERS.c_str());
    }
    if (!jsonDB.insert(JsonDB::FULLNAME_LAYERSVERSION, m_version)) {
        Logger::debug(LOG_PREPIX_FORMAT "Failed to insert '%s' key under",
                      LOG_PREPIX_ARGS, JsonDB::FULLNAME_LAYERSVERSION.c_str());
    }
}

bool Configuration::runPostProcess(JsonDB &jsonDB)
{
    bool result = true;
    JsonDB preDB("Before PostProcess"), postDB("After PostProcess");

    char *outputFilename = strdup(("/tmp/" + POST_PROCESS_OP_PREFIX + "XXXXXX").c_str());
    char *inputFilename = strdup(("/tmp/" + POST_PROCESS_IP_PREFIX + "XXXXXX").c_str());
    mode_t mask = umask(S_IRWXG | S_IRWXO);

    if (outputFilename == NULL || inputFilename == NULL) {
        Logger::warning(MSGID_CONFIGDSERVICE,
                        LOG_PREPIX_FORMAT "tmp file template is wrong",
                        LOG_PREPIX_ARGS);
        result = false;
        goto Done;
    }

    if (mkstemp(outputFilename) == -1 || mkstemp(inputFilename) == -1) {
        Logger::warning(MSGID_CONFIGDSERVICE,
                        LOG_PREPIX_FORMAT "tmp file name is wrong",
                        LOG_PREPIX_ARGS);
        result = false;
        goto Done;
    }

    umask(mask);

    if (!m_postProcessing.isArray() || m_postProcessing.arraySize() == 0) {
        Logger::info(MSGID_CONFIGURE,
                     LOG_PREPIX_FORMAT "Empty post_process or Invalid post_process (%s)",
                     LOG_PREPIX_ARGS, m_postProcessing.stringify("    ").c_str());
        goto Done;
    }

    if (!Platform::canWriteFile(jsonDB.getFilename())) {
        Logger::warning(MSGID_CONFIGDSERVICE,
                        LOG_PREPIX_FORMAT "Path (%s) is not ready to write",
                        LOG_PREPIX_ARGS, jsonDB.getFilename().c_str());
        result = false;
        goto Done;
    }

    // Create the post process input feature list
    // and give that file as arg to script
    preDB.copy(jsonDB);
    preDB.setFilename(inputFilename);
    if (!preDB.flush()) {
        Logger::warning(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Error in database flush", LOG_PREPIX_ARGS);
    }
    if (!Platform::isFileExist(inputFilename)) {
        result = false;
        goto Done;
    }

    for (JValue postProcess : m_postProcessing.items()) {
        Process process(postProcess);
        process.setArgs(inputFilename, outputFilename);
        process.execute();
    }

    // Validate the output from post process feature list
    postDB.load(outputFilename);
    jsonDB.copy(postDB);

    // TODO: Do we need to sync here?

Done:
    if (inputFilename != NULL) {
        g_unlink(inputFilename);
        free(inputFilename);
    }
    if (outputFilename != NULL) {
        g_unlink(outputFilename);
        free(outputFilename);
    }
    return result;
}

bool Configuration::runPreProcess()
{
    string result;

    if (!m_preProcessing.isArray() || m_preProcessing.arraySize() == 0) {
        Logger::info(MSGID_CONFIGURE,
                     LOG_PREPIX_FORMAT "Empty pre_process or Invalid pre_process (%s)",
                     LOG_PREPIX_ARGS, m_preProcessing.stringify("    ").c_str());
        return true;
    }

    for (JValue preProcess : m_preProcessing.items()) {
        Process process(preProcess);
        process.execute();
    }
    return true;
}

void Configuration::printSelections(string prefix)
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        Logger::info(MSGID_CONFIGURE,
                     LOG_PREPIX_FORMAT "%s %s : '%s(%s)' = '%s' %s",
                     LOG_PREPIX_ARGS,
                     prefix.c_str(),
                     (it->isSelected() ? "YES" : "NO"),
                     it->getName().c_str(),
                     Layer::getSelectorTypeStr(it->getType()),
                     (it->isSelected() ? it->getFullDirPath(true).c_str() : "N/A"),
                     prefix.c_str());
    }
}

void Configuration::printDebug()
{
    cout << endl << endl;
    printSelections("");
    cout << endl << endl;
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        it->printDebug();
    }
    cout << endl << endl;
}
