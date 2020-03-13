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

#include "JsonDB.h"

#include <boost/regex.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include "Environment.h"
#include "util/Platform.h"
#include "util/Logger.hpp"

const string JsonDB::FILENAME_MAIN_DB = INSTALL_LOCALSTATEDIR "/configd_db.json";
const string JsonDB::FILENAME_FACTORY_DB = INSTALL_LOCALSTATEDIR "/configd_factory_db.json";
const string JsonDB::FILENAME_DEBUG_DB = INSTALL_LOCALSTATEDIR "/configd_debug_db.json";
const string JsonDB::FILENAME_PERMISSION_DB = INSTALL_LOCALSTATEDIR "/configd_permissions_db.json";

const string JsonDB::CATEGORYNAME_CONFIGD = "com.webos.service.config";
const string JsonDB::FULLNAME_SELECTION = JsonDB::CATEGORYNAME_CONFIGD + ".selection";
const string JsonDB::FULLNAME_LAYERS = JsonDB::CATEGORYNAME_CONFIGD + ".layers";
const string JsonDB::FULLNAME_USER = JsonDB::CATEGORYNAME_CONFIGD + ".usrDefined";
const string JsonDB::FULLNAME_REASON = JsonDB::CATEGORYNAME_CONFIGD + ".dumpReason";
const string JsonDB::FULLNAME_LAYERSVERSION = JsonDB::CATEGORYNAME_CONFIGD + ".layersVersion";

const string JsonDB::FULLNAME_DEBUG_LOAD = JsonDB::CATEGORYNAME_CONFIGD + ".load";
const string JsonDB::FULLNAME_DEBUG_RECONFIGURE = JsonDB::CATEGORYNAME_CONFIGD + ".reconfigure";
const string JsonDB::FULLNAME_DEBUG_SETCONFIGS = JsonDB::CATEGORYNAME_CONFIGD + ".setconfigs";
const string JsonDB::FULLNAME_DEBUG_PREPROCESS = JsonDB::CATEGORYNAME_CONFIGD + ".preprocess";
const string JsonDB::FULLNAME_DEBUG_POSTPROCESS = JsonDB::CATEGORYNAME_CONFIGD + ".postprocess";

bool JsonDB::split(const string &fullName, string &categoryName, string &configName)
{
    string trimmedFullName = fullName;
    trimmedFullName.erase(remove_if(trimmedFullName.begin(), trimmedFullName.end(), ::isspace), trimmedFullName.end());

    int lastIndex = trimmedFullName.find_last_of('.');
    if (lastIndex < 0) {
        Logger::debug(LOG_PREPIX_FORMAT " \".\" character not Found in key %s",
                      LOG_PREPIX_ARGS, fullName.c_str());
        return false;
    }

    categoryName = trimmedFullName.substr(0, lastIndex);
    configName = trimmedFullName.substr(lastIndex + 1);
    return true;
}

/*
 * To get the full config key:value pairs from category
 * Usage: When user called getConfigs()  to get all the contents from category
 * @param prefixCategoryName - configName
 * @param categoryValueObj - The JSON object contains value for Category
 * @param returnValueObj - JSON object to insert key:value pairs
 * @return true when if return key:value inserted to returnValueObj
 * @return false when malloc() fails or if the categoryName is NULL
 */
bool JsonDB::getFullDBName(const string &categoryName, const JValue &category, JValue &result)
{
    if (categoryName.empty() || !category.isValid() || !result.isValid()) {
        Logger::debug(LOG_PREPIX_FORMAT "Input parameters are not valid", LOG_PREPIX_ARGS);
        return false;
    }

    for (JValue::KeyValue config : category.children()) {
        string configName = config.first.asString();
        string fullName = categoryName + "." + configName;

        if (configName.empty()) {
            Logger::debug(LOG_PREPIX_FORMAT "encountered invalid key when iterating %s category",
                          LOG_PREPIX_ARGS, categoryName.c_str());
            continue;
        }
        result.put(fullName, config.second);
    }
    return true;
}

JsonDB::JsonDB(string name)
    : m_name(name),
      m_filename(""),
      m_isUpdated(false)
{
    m_database = pbnjson::Object();
}

JsonDB::~JsonDB()
{
    if (m_isUpdated) {
        Logger::warning(MSGID_CONFIGUREDATA,
                        LOG_PREPIX_FORMAT_EXT "Database is modified but not sync into file (%s)",
                        LOG_PREPIX_ARGS_EXT, m_name.c_str(), m_filename.c_str());
    }
}

bool JsonDB::copy(JsonDB& db)
{
    if (m_database == db.m_database) {
        Logger::warning(MSGID_CONFIGUREDATA,
                        LOG_PREPIX_FORMAT "Failed to copy JsonDB because those are same values",
                        LOG_PREPIX_ARGS);
        return true;
    }

    // Only database values are copied
    m_database = db.m_database.duplicate();
    m_isUpdated = true;
    return true;
}

bool JsonDB::insert(const string& fullName, JValue value)
{
    string categoryName, configName;
    if (!JsonDB::split(fullName, categoryName, configName)) {
        Logger::error(MSGID_CONFIGUREDATA,
                      LOG_PREPIX_FORMAT "Failed to split fullName (%s)",
                      LOG_PREPIX_ARGS, fullName.c_str());
        return false;
    }
    return insert(categoryName, configName, value);
}

bool JsonDB::insert(const string &categoryName, const string &configName, JValue value)
{
    if (!m_database.hasKey(categoryName)) {
        m_database.put(categoryName, pbnjson::Object());
    }

    if (m_database[categoryName].hasKey(configName) &&
        m_database[categoryName][configName] == value) {
        Logger::verbose(LOG_PREPIX_FORMAT_EXT "'%s' - Ignore insert operation because of same value",
                        LOG_PREPIX_ARGS_EXT, categoryName.c_str(), configName.c_str());
        return true;
    } else if (m_database[categoryName].hasKey(configName)) {
        Logger::verbose(LOG_PREPIX_FORMAT_EXT "'%s'\n%s==========>\n%s",
                        LOG_PREPIX_ARGS_EXT, categoryName.c_str(), configName.c_str(),
                        m_database[categoryName][configName].stringify("    ").c_str(),
                        value.stringify("    ").c_str());
    }

    m_database[categoryName].put(configName, value);
    m_isUpdated = true;
    return true;
}

bool JsonDB::load(const string &filename)
{
    if (filename.empty()) {
        Logger::debug(LOG_PREPIX_FORMAT_EXT "Failed, empty file name received",
                      LOG_PREPIX_ARGS_EXT, filename.c_str());
        return false;
    }

    JSchema schema = JSchema::fromFile(CONFIGFEATUESLIST_SCHEMA);
    m_database = JDomParser::fromFile(filename.c_str(), schema);
    if (m_database.isNull()) {
        m_database = pbnjson::Object();
    }

    if (!m_filename.empty() && m_filename != filename) {
        Logger::warning(MSGID_CONFIGUREDATA,
                        LOG_PREPIX_FORMAT_EXT "Load another database file",
                        LOG_PREPIX_ARGS_EXT, m_name.c_str());
    }
    m_filename = filename;
    if (m_isUpdated) {
        Logger::warning(MSGID_CONFIGUREDATA,
                        LOG_PREPIX_FORMAT_EXT "Database is modified but not saved before loading",
                        LOG_PREPIX_ARGS_EXT, m_name.c_str());
    }
    m_isUpdated = false;
    return true;
}

bool JsonDB::remove(const string &fullName)
{
    string categoryName, configName;
    if (!JsonDB::split(fullName, categoryName, configName)) {
        Logger::error(MSGID_CONFIGUREDATA,
                      LOG_PREPIX_FORMAT "Failed to split fullName (%s)",
                      LOG_PREPIX_ARGS, fullName.c_str());
        return false;
    }
    return remove(categoryName, configName);
}

bool JsonDB::remove(const string &categoryName, const string &configName)
{
    if (!m_database.hasKey(categoryName) || !m_database[categoryName].hasKey(configName)) {
        Logger::debug(LOG_PREPIX_FORMAT "%s.%s is not in database",
                      LOG_PREPIX_ARGS,
                      categoryName.c_str(), configName.c_str());
        return true;
    }

    if (!m_database[categoryName].remove(configName)) {
        return false;
    }

    m_isUpdated = true;
    if (m_database[categoryName].objectSize() > 0) {
        return true;
    }

    if (!m_database.remove(categoryName)) {
        return false;
    }
    return true;
}

bool JsonDB::merge(JValue& database)
{
    for (JValue::KeyValue category : database.children()) {
        string categoryName = category.first.asString();

        for (JValue::KeyValue config : category.second.children()) {
            string configName = config.first.asString();

            if (!insert(categoryName, configName, config.second)) {
                Logger::debug(LOG_PREPIX_FORMAT "Failed to insert \"%s\" key under \"%s\" to configDB",
                              LOG_PREPIX_ARGS,
                              configName.c_str(), categoryName.c_str());
            }
        }
    }
    return true;
}

bool JsonDB::merge(JsonDB& jsonDB)
{
    return merge(jsonDB.getDatabase());
}

bool JsonDB::fetch(const string &categoryName, const string &configName, JValue &result)
{
    if (!m_database.hasKey(categoryName)) {
        Logger::debug(LOG_PREPIX_FORMAT "%s category does not exist",
                      LOG_PREPIX_ARGS, categoryName.c_str());
        return false;
    }

    if (result.isNull()) {
        result = pbnjson::Object();
    }

    if (configName == "*") {
        return JsonDB::getFullDBName(categoryName, m_database[categoryName], result);
    }

    if (!m_database[categoryName].isValid() || !m_database[categoryName].hasKey(configName)) {
        Logger::debug(LOG_PREPIX_FORMAT "%s config does not exist", LOG_PREPIX_ARGS, configName.c_str());
        return false;
    }

    return result.put(categoryName + "." + configName, m_database[categoryName][configName].duplicate());
}

bool JsonDB::fetch(const string &fullName, JValue &result)
{
    string categoryName;
    string configName;

    if (false == JsonDB::split(fullName, categoryName, configName)) {
        Logger::debug(LOG_PREPIX_FORMAT "Failed to get category/config name from \"%s\"",
                      LOG_PREPIX_ARGS,
                      fullName.c_str());
        return false;
    }

    if (!m_database.hasKey(categoryName)) {
        Logger::debug(LOG_PREPIX_FORMAT "%s category does not exist in DB",
                      LOG_PREPIX_ARGS,
                      categoryName.c_str());
        return false;
    }

    return fetch(categoryName, configName, result);
}

bool JsonDB::searchKey(const string &regEx, JValue &result)
{
    if (regEx.empty())
        return false;

    boost::regex name(regEx);
    string categoryName;
    string configName;
    bool returnValue = false;

    for (JValue::KeyValue category : m_database.children()) {
        string categoryName = category.first.asString();
        for (JValue::KeyValue config : category.second.children()) {
            configName = config.first.asString();
            string fullConfig = categoryName + "." + configName;
            try {
                if (boost::regex_search(fullConfig, name)) {
                    result.put(fullConfig, m_database[categoryName][configName].duplicate());
                    returnValue = true;
                }
            }
            catch (boost::regex_error& e) {
                Logger::debug(LOG_PREPIX_FORMAT "regex_search failed: %s",
                              LOG_PREPIX_ARGS,
                              e.what());
            }
        }
    }
    return returnValue;
}

bool JsonDB::clear()
{
    Platform::deleteFile(m_filename.c_str());
    m_database = pbnjson::Object();
    m_isUpdated = true;
    return false;
}

bool JsonDB::flush()
{
    if (!m_isUpdated) {
        Logger::debug(LOG_PREPIX_FORMAT "Database is not updated", LOG_PREPIX_ARGS);
        return true;
    } else if (m_filename.empty()) {
        Logger::debug(LOG_PREPIX_FORMAT "In memory database", LOG_PREPIX_ARGS);
        return false;
    }

    string configData = m_database.stringify("    ");

    gchar *dirname = g_path_get_dirname(m_filename.c_str());
    if (g_mkdir_with_parents(dirname, 0755)) {
        Logger::error(MSGID_CONFIGUREDATA,
                      LOG_PREPIX_FORMAT "Failed to mkdir %s: %s",
                      LOG_PREPIX_ARGS, dirname, g_strerror(errno));
        g_free(dirname);
        return false;
    }
    g_free(dirname);

    /**
     * g_file_set_contents() will internally create one temp file and write all the contents into
     * the temp file using write() system call. After written into the file, fsync() gets called.
     * If fsync() is success, then temp file will be renamed back to config DB using rename() system call.
     */
    GError *gerror = NULL;
    mode_t mask = umask(S_IRWXG | S_IRWXO);
    if (!g_file_set_contents(m_filename.c_str(), configData.c_str(), configData.length(), &gerror)) {
        Logger::error(MSGID_CONFIGUREDATA,
                      LOG_PREPIX_FORMAT "Failed to write content into %s: %s",
                      LOG_PREPIX_ARGS, m_filename.c_str(), gerror->message);
        umask(mask);
        g_error_free(gerror);
        return false;
    }
    umask(mask);
    g_error_free(gerror);
    m_isUpdated = false;
    return true;
}

JValue &JsonDB::getDatabase()
{
    return m_database;
}

string &JsonDB::getFilename()
{
    return m_filename;
}

bool JsonDB::setFilename(const string &filename)
{
    if (m_filename == filename) {
        return true;
    }
    m_filename = filename;
    m_isUpdated = true;
    return true;
}

bool JsonDB::isUpdated()
{
    return m_isUpdated;
}

bool JsonDB::isEqualDatabase(JsonDB& jsonDB)
{
    return (jsonDB.getDatabase() == m_database);
}

bool JsonDB::isEqualFilename(JsonDB& jsonDB)
{
    return (jsonDB.getFilename() == m_filename);
}

void JsonDB::printDebug()
{
    cout << m_database.stringify("    ") << endl;
}
