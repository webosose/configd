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

#include <string>
#include <fstream>
#include <streambuf>
#include <boost/algorithm/string.hpp>

#include "Manager.h"
#include "Matcher.h"
#include "Layer.h"
#include "service/Configd.h"
#include "util/Json.h"
#include "util/Logger.hpp"
#include "util/Platform.h"

// TODO can be replace file schema?
bool Layer::isValidLSSelector(const JValue &selector)
{
    if (!selector.isValid() || selector.isNull()) {
       return false;
   }

   if (!selector["key"].isValid() || !selector["key"].isArray()) {
       return false;
   }

   if (!selector["luna_cmd"].isValid() || !selector["luna_cmd"].isObject()) {
       return false;
   }

   if (!selector["luna_cmd"].hasKey("method") || !selector["luna_cmd"]["method"].isString()) {
       return false;
   }

   if (!selector["luna_cmd"].hasKey("params") || !selector["luna_cmd"]["params"].isObject()) {
       return false;
   }
   return true;
}

bool Layer::isValidStrSelector(const JValue &selector, const string key)
{
    if (!selector[key].isValid())
        return false;
    if (!selector[key].isString())
        return false;
    if (selector[key].asString().empty())
        return false;
    return true;
}

SelectorType Layer::getSelectorType(const JValue &selector)
{
    SelectorType selectorType = SelectorType_None;

    if (selector.isNull()) {
        return selectorType;
    }
    if (isValidStrSelector(selector, "string")) {
        selectorType = SelectorType_String;
    } else if (isValidStrSelector(selector, "command")) {
        selectorType = SelectorType_Command;
    } else if (isValidStrSelector(selector, "file")) {
        selectorType = SelectorType_File;
    } else if (isValidLSSelector(selector)) {
        selectorType = SelectorType_Luna;
    }
    return selectorType;
}

char* Layer::getSelectorTypeStr(const SelectorType& type)
{
    switch (type) {
    case SelectorType_None:
        return (char*)"none";

    case SelectorType_String:
        return (char*)"string";

    case SelectorType_File:
        return (char*)"file";

    case SelectorType_Command:
        return (char*)"command";

    case SelectorType_Luna:
        return (char*)"luna";
    }
    return (char*)"invalid";
}

Layer::Layer(JValue layer)
    : IHandleListener(),
      m_selection(""),
      m_isSelected(false),
      m_listener(NULL),
      m_requirePreProcessing(false),
      m_requirePostProcessing(true), // basically, post processing is always needed.
      m_priority(0)
{
    // required
    m_layer = layer.duplicate();
    m_name = m_layer["name"].asString();
    m_baseDir = m_layer["base_dir"].asString();
    m_selector = m_layer["selector"];

    // optional
    if (m_layer.hasKey("requirePreProcessing") && m_layer["requirePreProcessing"].isBoolean()) {
        m_requirePreProcessing = m_layer["requirePreProcessing"].asBool();
    }
    if (m_layer.hasKey("requirePostProcessing") && m_layer["requirePostProcessing"].isBoolean()) {
        m_requirePostProcessing = m_layer["requirePostProcessing"].asBool();
    }
    if (m_layer.hasKey("priority")) {
        m_priority = m_layer["priority"].asNumber<int>();
    }
    string selection;
    m_type = getSelectorType(m_selector);

    if (m_type == SelectorType_None && !m_selector.isNull())
        return;

    switch(m_type) {
    case SelectorType_None:
        selection = "";
        break;

    case SelectorType_String:
        m_selector["string"].asString(selection);
        break;

    default:
        return;
    }

    Logger::debug(LOG_PREPIX_FORMAT_EXT "Select ReadOnly Layer in Constructor",
                  LOG_PREPIX_ARGS_EXT, m_name.c_str());
    if (!setSelection(selection)) {
        Logger::error(MSGID_CONFIGURE,
                      LOG_PREPIX_FORMAT_EXT "Failed to select (Changed into 'invalid' type)",
                      LOG_PREPIX_ARGS_EXT, m_name.c_str());
    }
}

Layer::~Layer()
{

}

void Layer::printDebug()
{
    cout << "=== " << m_name << " ===" << endl;
    cout << "Layer Type : " << Layer::getSelectorTypeStr(m_type) << endl;
    cout << "Selection : "
         << (isSelected() ? (m_selection.empty() ? "Empty" : m_selection) : "Not Selected")
         << endl;
    cout << "Listener : " << ((m_listener != NULL) ? "Exist" : "Not Exist") << endl;
    cout << "Layer Info : " << endl;
    cout << m_layer.stringify("    ") << endl;
}

void Layer::onReceiveCall(JValue &response)
{
    if (response.hasKey("subscribed")) {
        if(!response["subscribed"].asBool()) {
            Logger::warning(MSGID_CONFIGURE,
                            LOG_PREPIX_FORMAT_EXT "The API doesn't support 'subscription'",
                            LOG_PREPIX_ARGS_EXT, getName().c_str());
            this->cancelCall();
        }
    }
    else {
        Logger::warning(MSGID_CONFIGURE,
                        LOG_PREPIX_FORMAT_EXT "Response doesn't have 'subscribed' value",
                        LOG_PREPIX_ARGS_EXT, getName().c_str());
    }

    JValue result;
    if (Json::getValueWithKeys(response, m_selector["key"], result) && result.isString()) {
        Logger::info(MSGID_CONFIGURE,
                    LOG_PREPIX_FORMAT_EXT "Selection candidate is '%s' (%s in %s)",
                    LOG_PREPIX_ARGS_EXT, getName().c_str(),
                    result.asString().c_str(),
                    m_selector["key"].stringify().c_str(), response.stringify().c_str());
        setSelection(result.asString());
    } else {
        Logger::warning(MSGID_CONFIGURE,
                        LOG_PREPIX_FORMAT_EXT "Failed to get valid key (%s in %s)",
                        LOG_PREPIX_ARGS_EXT, getName().c_str(),
                        m_selector["key"].stringify().c_str(), response.stringify().c_str());
    }
}

bool Layer::call()
{
    if (SelectorType_Luna != m_type)
        return false;

    if (m_call && m_call->isActive()) {
        Logger::info(MSGID_CONFIGURE,
                     LOG_PREPIX_FORMAT_EXT "Subscription is already established",
                     LOG_PREPIX_ARGS_EXT, getName().c_str());
        return true;
    }

    JValue key = m_selector["key"];
    JValue luna_cmd = m_selector["luna_cmd"];
    JValue method = luna_cmd["method"];
    JValue params = luna_cmd["params"];

    m_call = AbstractBusFactory::getInstance()->getIHandle()->call(method.asString(),
                                                                   params.stringify(),
                                                                   this);

    return true;
}

void Layer::cancelCall()
{
    if (m_call->isActive()) {
        m_call->cancel();
    }
}

JValue Layer::getMatchedConfigs(JValue& configs, JsonDB *jsonDB)
{
    JValue matchedConfigs = pbnjson::Array();
    for (int i = 0; i < configs.arraySize(); i++) {
        if (!configs[i].hasKey("where")) {
            matchedConfigs.append(configs[i]);
            continue;
        }

        if (jsonDB == NULL) {
            continue;
        }

        Matcher condition(configs[i]["where"]);
        if (condition.checkCondition(jsonDB)) {
            matchedConfigs.append(configs[i]);
            Logger::info(MSGID_CONFIGDSERVICE,
                         LOG_PREPIX_FORMAT "where condition is matched",
                         LOG_PREPIX_ARGS);
        }
    }
    return matchedConfigs;
}

JValue Layer::refineContent(JValue& content)
{
    JValue resultArray = pbnjson::Array();
    JValue configs = pbnjson::Object();

    if (!content.hasKey("configs")) {
        configs.put("data", content);
        resultArray.append(configs);
        return resultArray;
    }

    if (!content["configs"].isArray()) {
        return resultArray;
    }

    return content["configs"];
}

bool Layer::parseFiles(string &dirPath, JValue *database, JsonDB *jsonDB, JsonDB *permissionsDB)
{
    DIR *dir = opendir(dirPath.c_str());
    if (NULL == dir) {
        Logger::warning(MSGID_CONFIGDSERVICE,
                        LOG_PREPIX_FORMAT "Failed to open DIR '%s' for parsing",
                        LOG_PREPIX_ARGS, dirPath.c_str());
        return false;
    }
    Logger::debug(LOG_PREPIX_FORMAT "Directory Name (%s)",
                  LOG_PREPIX_ARGS,
                  dirPath.c_str());

    struct dirent *targetFile = NULL;
    while (NULL != (targetFile = readdir(dir))) {
        string fileName = targetFile->d_name;
        string extension;
        string name;

        Platform::extractFileName(fileName, name, extension);
        if (fileName == ".." || fileName == ".") {
            continue;
        } else if (extension != "json") {
            Logger::warning(MSGID_CONFIGDSERVICE,
                            LOG_PREPIX_FORMAT "Invalid file extension '%s' in '%s'",
                            LOG_PREPIX_ARGS,
                            fileName.c_str(), dirPath.c_str());
            continue;
        }

        JValue content = JDomParser::fromFile(Platform::concatPaths(dirPath, fileName).c_str());
        JValue configs = pbnjson::Object();

        if (!content.isValid()) {
            Logger::error(MSGID_JSON_PARSE_FILE_ERR,
                          LOG_PREPIX_FORMAT "Invalid JSON format '%s/%s'",
                          LOG_PREPIX_ARGS,
                          dirPath.c_str(), fileName.c_str());
            continue;
        }

        configs = refineContent(content);
        configs = getMatchedConfigs(configs, jsonDB);

        if (!configs.isArray() || configs.arraySize() <= 0)
            continue;

        for (JValue config : configs.items()) {
            if (database != NULL)
                database->put(fileName, config);

            if (jsonDB == NULL)
                continue;

            for (JValue::KeyValue feature : config["data"].duplicate().children()) {
                std::string key = feature.first.asString();
                if (!jsonDB->insert(name, key, feature.second)) {
                    Logger::debug(LOG_PREPIX_FORMAT "Failed to insert '%s' key",
                                  LOG_PREPIX_ARGS,
                                  key.c_str());
                }
                if (config.hasKey("permissions")) {
                    permissionsDB->insert(name, key, config["permissions"]);
                }
            }
        }
    }

    closedir(dir);
    return true;
}

bool Layer::fetchConfigs(JValue &database)
{
    JValue data = pbnjson::Object();
    JValue configData = pbnjson::Object();
    string dirPath;

    if (!isSelected()) {
        Logger::warning(MSGID_CONFIGDSERVICE,
                        LOG_PREPIX_FORMAT_EXT "Selection not processed in Layer",
                        LOG_PREPIX_ARGS_EXT, getName().c_str());
        return false;
    }

    if (m_type == SelectorType_None) {
        dirPath = getFullDirPath();
    } else {
        dirPath = getFullDirPath(true);
    }

    if (!parseFiles(dirPath, &configData, NULL)) {
        return false;
    }

    data.put(dirPath, configData);
    database.append(data);

    return true;
}

bool Layer::fetchConfigs(JsonDB &jsonDB, JsonDB *permissionDB)
{
    if (!isSelected()) {
        Logger::debug(LOG_PREPIX_FORMAT_EXT "Fetch is skipped because it is not selected yet",
                      LOG_PREPIX_ARGS_EXT, m_name.c_str());
        return false;
    }

    string dirPath;
    if (getType() == SelectorType_None) {
        dirPath = getFullDirPath();
    } else {
        dirPath = getFullDirPath(true);
    }

    if (!parseFiles(dirPath, NULL, &jsonDB, permissionDB)) {
        return false;
    }
    return true;
}

bool Layer::findSelection(const JValue &selector, string &selection)
{
    string alternativeSelection;
    SelectorType type = getSelectorType(selector);

    if (m_selection.empty() && selector.hasKey("alternative")) {
        Logger::debug(LOG_PREPIX_FORMAT_EXT "Try to select alternative",
                      LOG_PREPIX_ARGS_EXT, m_name.c_str());
        if (!findSelection(selector["alternative"], alternativeSelection)) {
            Logger::warning(MSGID_CONFIGURE,
                            LOG_PREPIX_FORMAT_EXT "Failed to get alternative",
                            LOG_PREPIX_ARGS_EXT, m_name.c_str());
        }
    }

    switch(type) {
    case SelectorType_String:
        selection = selector["string"].asString();
        break;

    case SelectorType_File:
        selection = Platform::readFile(selector["file"].asString());
        break;

    case SelectorType_Command:
        if (!Platform::executeCommand(selector["command"].asString(), selection)) {
            Logger::warning(MSGID_CONFIGURE,
                            LOG_PREPIX_FORMAT "Failed to execute command : %s",
                            LOG_PREPIX_ARGS, selector["command"].asString().c_str());
        }
        break;

    case SelectorType_Luna:
        if (!call()) {
            Logger::warning(MSGID_CONFIGURE,
                            LOG_PREPIX_FORMAT "Failed to subscribe service : %s",
                            LOG_PREPIX_ARGS, m_name.c_str());
        }
        break;

    default:
        break;
    }

    if (selection.empty() && !alternativeSelection.empty()) {
        selection = alternativeSelection;
    }

    if (selection.empty()) {
        Logger::warning(MSGID_CONFIGURE,
                        LOG_PREPIX_FORMAT "Failed to find selection of '%s', it could be luna type without alternative",
                        LOG_PREPIX_ARGS, m_name.c_str());
    } else {
        Logger::info(MSGID_CONFIGURE,
                     LOG_PREPIX_FORMAT "Succeed to find selection of %s : '%s'",
                     LOG_PREPIX_ARGS, m_name.c_str(), selection.c_str());
    }
    return true;
}

bool Layer::select()
{
    if (isReadOnlyType()) {
        Logger::debug(LOG_PREPIX_FORMAT_EXT "ReadOnly Type (Skipped)",
                      LOG_PREPIX_ARGS_EXT, m_name.c_str());
        return false;
    }

    Logger::debug(LOG_PREPIX_FORMAT_EXT "Start to find selection", LOG_PREPIX_ARGS_EXT, m_name.c_str());
    string selection = "";
    if (!findSelection(m_selector, selection)) {
        Logger::warning(MSGID_CONFIGURE,
                        LOG_PREPIX_FORMAT_EXT "Failed to select (Not Changed into 'invalid' type. Retry Next)",
                        LOG_PREPIX_ARGS_EXT, m_name.c_str());
        return false;
    }
    if (selection.empty()) {
        Logger::verbose(LOG_PREPIX_FORMAT_EXT "Selection candidate is empty. It might be luna type",
                        LOG_PREPIX_ARGS_EXT, m_name.c_str());
        return true;
    }

    if (!setSelection(selection)) {
        Logger::warning(MSGID_CONFIGURE,
                        LOG_PREPIX_FORMAT_EXT "Failed to select (Not Changed into 'invalid' type. Retry Next)",
                        LOG_PREPIX_ARGS_EXT, m_name.c_str());
    }
    return isSelected();
}

bool Layer::setSelection(string selection)
{
    if (SelectorType_None != m_type && selection.empty()) {
        Logger::error(MSGID_CONFIGURE,
                      LOG_PREPIX_FORMAT_EXT "Empty selection is allowed only for 'None' type",
                      LOG_PREPIX_ARGS_EXT, m_name.c_str());
        return false;
    } else if (SelectorType_None == m_type && !selection.empty()) {
        Logger::error(MSGID_CONFIGURE,
                      LOG_PREPIX_FORMAT_EXT "Empty selection is allowed only for 'None' type",
                      LOG_PREPIX_ARGS_EXT, m_name.c_str());
        return false;
    }

    if (isReadOnlyType() && isSelected()) {
        Logger::info(MSGID_CONFIGURE,
                     LOG_PREPIX_FORMAT_EXT "The layer is ReadOnly. It is already selected",
                     LOG_PREPIX_ARGS_EXT, m_name.c_str());
        return false;
    }

    string selectedPath = Platform::concatPaths(getFullDirPath(false), selection);
    if (!Platform::isDirExist(selectedPath)) {
        Logger::warning(MSGID_CONFIGURE,
                        LOG_PREPIX_FORMAT_EXT "Directory is not founded (%s)",
                        LOG_PREPIX_ARGS_EXT, m_name.c_str(), selectedPath.c_str());
    }

    m_isSelected = true;
    if (m_selection == selection) {
        Logger::info(MSGID_CONFIGURE,
                     LOG_PREPIX_FORMAT_EXT "Same selection is selected (%s)",
                     LOG_PREPIX_ARGS_EXT, m_name.c_str(),
                     selection.empty() ? "empty" : selection.c_str());
        return true;
    }

    string old = m_selection;
    m_selection = selection;
    Logger::info(MSGID_CONFIGURE,
                 LOG_PREPIX_FORMAT_EXT "%s ==> %s : selection is changed",
                 LOG_PREPIX_ARGS_EXT, m_name.c_str(), old.c_str(), m_selection.c_str());

    if (m_listener != NULL) {
        m_listener->onSelectionChanged(*this, old, selection);
    }
    return true;
}

bool Layer::isSelected()
{
    return m_isSelected;
}

bool Layer::isReadOnlyType()
{
    switch(m_type) {
    case SelectorType_String:
    case SelectorType_None:
        return true;

    default:
        break;
    }
    return false;
}

void Layer::clearSelection()
{
    if (isReadOnlyType())
        return;
    m_isSelected = false;
    m_selection.clear();
}

void Layer::setListener(LayerListener *listener)
{
    m_listener = listener;
}

const string &Layer::getSelection()
{
    if (!isSelected()) {
        Logger::warning(MSGID_CONFIGURE,
                        LOG_PREPIX_FORMAT_EXT "Not selected yet",
                        LOG_PREPIX_ARGS_EXT, m_name.c_str());
    }
    return m_selection;
}

string &Layer::getName()
{
    return m_name;
}

string Layer::getFullDirPath(bool withSelection)
{
    if (withSelection && isSelected())
        return Platform::concatPaths(m_baseDir, m_selection);
    else
        return m_baseDir;
}

string Layer::getLeafDirPath(bool withSelection)
{
    int lastIndex = m_baseDir.find_last_of('/');
    if (lastIndex < 0)
        return m_baseDir;
    string leafDir = m_baseDir.substr(lastIndex + 1);

    if (withSelection && isSelected())
        return Platform::concatPaths(leafDir, m_selection);
    else
        return leafDir;
}

SelectorType &Layer::getType()
{
    return m_type;
}

JValue Layer::toJson()
{
    JValue json = pbnjson::Object();
    json.put("name", m_name);
    json.put("base", m_baseDir);
    json.put("type", getSelectorTypeStr(m_type));
    if (isSelected())
        json.put("selection", m_selection);
    return json;
}

void Layer::fromJson(JValue json)
{
    if (!json.hasKey("name") || !json["name"].isString() || json["name"].asString() != m_name)
        return;
    if (!json.hasKey("base") || !json["base"].isString() || json["base"].asString() != m_baseDir)
        return;
    if (json.hasKey("selection") && json["selection"].isString()) {
        setSelection(json["selection"].asString());
    }
}

bool Layer::requirePreProcessing()
{
    return m_requirePreProcessing;
}

bool Layer::requirePostProcessing()
{
    return m_requirePostProcessing;
}
