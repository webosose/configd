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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>

#include "Manager.h"
#include "service/ErrorDB.h"
#include "service/ls2/LS2BusFactory.h"
#include "setting/Setting.h"
#include "util/Platform.h"
#include "util/Json.h"

Manager::Manager()
    : m_isLoaded(false)
{
    Logger::info(MSGID_MANAGER, LOG_PREPIX_FORMAT "Create GMainLoop", LOG_PREPIX_ARGS);
    m_mainLoop = g_main_loop_new(NULL, FALSE);
}

Manager::~Manager()
{
    if (g_main_loop_is_running(m_mainLoop))
        g_main_loop_quit(m_mainLoop);
    g_main_loop_unref(m_mainLoop);
}

void Manager::initialize()
{
    Logger::info(MSGID_MANAGER, LOG_PREPIX_FORMAT "Initialize Bus Instance", LOG_PREPIX_ARGS);
    Configd::getInstance()->initialize(m_mainLoop, this);
    Setting::getInstance().initialize();
    load();
}

void Manager::run()
{
    g_main_loop_run(m_mainLoop);
}

int Manager::onGetConfigs()
{
    return ErrorDB::ERRORCODE_NOERROR;
};

int Manager::onSetConfigs(JValue configs, bool isVolatile)
{
    updateFactoryDatabase(configs, isVolatile);
    updateUnifiedDatabase("setConfigs");
    writeDebugDatabase(JsonDB::FULLNAME_DEBUG_SETCONFIGS);
    return ErrorDB::ERRORCODE_NOERROR;
};

int Manager::onDump(JValue &configs)
{
    if (JsonDB::getUnifiedInstance().getDatabase().isNull()) {
        return ErrorDB::ERRORCODE_INVALID_MAINDB;
    }
    configs = JsonDB::getUnifiedInstance().getDatabase();
    return ErrorDB::ERRORCODE_NOERROR;
};

int Manager::onFullDump(JValue &configs)
{
    Configuration::getInstance().fetchConfigs(configs);
    return ErrorDB::ERRORCODE_NOERROR;
};

int Manager::onReconfigure(int timeout)
{
    // Force reconfigure regardless of selection state
    if (timeout <= 0)
        timeout = MS_DEFAULT_DELAY;
    else if (timeout > MS_LSCALL_DELAY)
        timeout = MS_LSCALL_DELAY;

    Logger::info(MSGID_CONFIGDSERVICE,
                 LOG_PREPIX_FORMAT "{\"timeout\":\"%d\"}: Execute Reconfigure",
                 LOG_PREPIX_ARGS, timeout);

    if (timeout < Manager::MS_MINIMAL_DELAY) {
        Manager::getInstance()->reconfigure(true, false);
    } else {
        Manager::getInstance()->reconfigure(true, true, timeout);
    }

    return ErrorDB::ERRORCODE_NOERROR;
};

int Manager::onReloadConfigs()
{
    // TODO: This API should be removed
    Configd::getInstance()->sendSignal(Configd::NAME_CONFIGD_RELOAD_DONE);
    return ErrorDB::ERRORCODE_NOERROR;
};

void Manager::onSelectionChanged(Layer &layer, string &oldSelection, string &newSelection)
{
    if (Configuration::getInstance().isAllSelected()) {
        Configuration::getInstance().printSelections("OOO");
    } else {
        Configuration::getInstance().printSelections("XXX");
    }

    int delay = MS_LSCALL_DELAY;
    if (Setting::getInstance().isNormalStatus())
        delay = MS_MINIMAL_DELAY;

    Logger::info(MSGID_CONFIGDSERVICE,
                     LOG_PREPIX_FORMAT "layer(%s) : %s --> %s reconfigure would be requested with delay %d ms",
                     LOG_PREPIX_ARGS,
                     layer.getName().c_str(), oldSelection.c_str(), newSelection.c_str(), delay);

    // MS_DEFAULT_DELAY - In order to avoid multiple reconfigurations
    // Subscriptions can be available at once.
    Manager::getInstance()->reconfigure(layer.requirePreProcessing(),
                                        layer.requirePostProcessing(),
                                        delay);
}

bool Manager::load()
{
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Initial Loading start", LOG_PREPIX_ARGS);

    JValue layersVersion;
    JsonDB::getMainInstance().load(JsonDB::FILENAME_MAIN_DB);
    JsonDB::getMainInstance().fetch(JsonDB::FULLNAME_LAYERSVERSION, layersVersion);
    if (layersVersion.objectSize() == 0 ||
       (layersVersion[JsonDB::FULLNAME_LAYERSVERSION].asString() != Configuration::getInstance().getLayersVersion())) {
        JsonDB::getMainInstance().clear();
    }

    bool isLoadExistDB = Platform::isFileExist(JsonDB::FILENAME_MAIN_DB);

    // Handle MainDB
    Configuration::getInstance().setListener(nullptr);
    if (!isLoadExistDB) {
        // There is no main db file. (FirstUse or Reboot during reconfigure)
        // configd needs to generate main db (pre-processing : true / post-processing : false)
        Configuration::getInstance().runPreProcess();
        Configuration::getInstance().selectAll();
        Configuration::getInstance().fetchConfigs(JsonDB::getMainInstance(), &JsonDB::getPermissionInstance());
        Configuration::getInstance().fetchLayers(JsonDB::getMainInstance());
        JsonDB::getMainInstance().flush();
        JsonDB::getPermissionInstance().flush();
    } else {
        JsonDB::getMainInstance().load(JsonDB::FILENAME_MAIN_DB);
        Configuration::getInstance().updateSelections(JsonDB::getMainInstance());
        JsonDB::getPermissionInstance().load(JsonDB::FILENAME_PERMISSION_DB);
    }

    // Handle FactoryDB
    JsonDB::getFactoryInstance().load(JsonDB::FILENAME_FACTORY_DB);
    if (!JsonDB::getFakeFactoryInstance().getDatabase().isNull()) {
        // FakeFactoryInstance is early database during snapshot-boot
        Logger::info(MSGID_CONFIGDSERVICE,
                     LOG_PREPIX_FORMAT "Apply Fake Factory database",
                     LOG_PREPIX_ARGS);
        JsonDB::getFactoryInstance().merge(JsonDB::getFakeFactoryInstance());
        JsonDB::getFactoryInstance().flush();
    }

    // Handle UnifiedDB
    updateUnifiedDatabase("load");

    // Handle DebugDB
    writeDebugDatabase(JsonDB::FULLNAME_DEBUG_LOAD);

    // Read only selections are ready.
    // Configd tries to reconfigure whenever other selections are available.
    Configuration::getInstance().setListener(this);
    if(isLoadExistDB) {
        Configuration::getInstance().selectAll();
    }

    m_isLoaded = true;
    return true;
}

bool Manager::reconfigure(bool runPreProcess, bool runPostProcess, int delayTime)
{
    static bool savedRunPreProcess = false;
    static bool savedRunPostProcess = false;

    if (runPreProcess) savedRunPreProcess = true;
    if (runPostProcess) savedRunPostProcess = true;

    if (m_reconfigureTimer.isWaiting()) {
        Logger::debug(LOG_PREPIX_FORMAT "Clear another reconfigure request", LOG_PREPIX_ARGS);
        m_reconfigureTimer.clear();
    }
    if (delayTime > 0) {
        // This is blocking call but mainloop is alive.
        Logger::debug(LOG_PREPIX_FORMAT "Wait reconfigure timeout (%d ms)", LOG_PREPIX_ARGS, delayTime);
        if (!m_reconfigureTimer.wait(delayTime)) {
            // 'false' return means another thread call 'clear'
            // the thread will handle reconfigure
            Logger::debug(LOG_PREPIX_FORMAT "Cleared by another reconfigure request", LOG_PREPIX_ARGS);
            return false;
        }
        m_reconfigureTimer.clear();
    }

    // Removing main db file will ensure that reconfigure is needed
    // because of sudden power off or any other unexpected things happened.
    JsonDB::getMainInstance().clear();

    Logger::info(MSGID_CONFIGDSERVICE,
                 LOG_PREPIX_FORMAT "Start Reconfigure - runPreProcess(%s), runPostProcess(%s), delayTime(%d)",
                 LOG_PREPIX_ARGS,
                 savedRunPreProcess ? "true" : "false",
                 savedRunPostProcess ? "true" : "false",
                 delayTime);

    if (savedRunPreProcess) {
        Logger::debug(LOG_PREPIX_FORMAT "Start PreProcess", LOG_PREPIX_ARGS);
        Configuration::getInstance().runPreProcess();
        writeDebugDatabase(JsonDB::FULLNAME_DEBUG_PREPROCESS);
    }

    Configuration::getInstance().setListener(nullptr);
    Configuration::getInstance().selectAll();
    Configuration::getInstance().fetchConfigs(JsonDB::getMainInstance(), &JsonDB::getPermissionInstance());
    Configuration::getInstance().fetchLayers(JsonDB::getMainInstance());
    Configuration::getInstance().setListener(this);

    if (savedRunPostProcess) {
        Logger::debug(LOG_PREPIX_FORMAT "Start PostProcess", LOG_PREPIX_ARGS);
        Configuration::getInstance().runPostProcess(JsonDB::getMainInstance());
        writeDebugDatabase(JsonDB::FULLNAME_DEBUG_POSTPROCESS);
    }

    JsonDB::getMainInstance().flush();
    JsonDB::getPermissionInstance().flush();
    updateUnifiedDatabase("reconfigure");
    writeDebugDatabase(JsonDB::FULLNAME_DEBUG_RECONFIGURE);

    savedRunPreProcess = false;
    savedRunPostProcess = false;
    return true;
}

void Manager::updateUnifiedDatabase(string reason)
{
    Logger::debug(LOG_PREPIX_FORMAT "Update Unified database (%s)",
                  LOG_PREPIX_ARGS, reason.c_str());
    JsonDB oldUnifiedDB("Database to update unified database");

    oldUnifiedDB.copy(JsonDB::getUnifiedInstance());
    JsonDB::getUnifiedInstance().copy(JsonDB::getMainInstance());
    JsonDB::getUnifiedInstance().merge(JsonDB::getFactoryInstance());

    // volatile configs(set by factorywin) would be initialized when reconfigure is called,
    // merged to unified DB when setConfigs are called.
    if (reason == "reconfigure") {
        JsonDB::getVolatileInstance().clear();
    } else if (reason == "setConfigs") {
        JsonDB::getUnifiedInstance().merge(JsonDB::getVolatileInstance());
    }

    if (oldUnifiedDB.getDatabase() == JsonDB::getUnifiedInstance().getDatabase()) {
        Logger::debug(LOG_PREPIX_FORMAT "Same unified db (%s)",
                      LOG_PREPIX_ARGS, reason.c_str());
        return;
    }

    Configd::getInstance()->postGetConfigs(JsonDB::getUnifiedInstance(), oldUnifiedDB);

    string filename = "/tmp/configd_" + Platform::timeStr() + "_before_" + reason + ".json";
    oldUnifiedDB.setFilename(filename);
    oldUnifiedDB.flush();
}

void Manager::writeDebugDatabase(string fullname)
{
    JValue json = pbnjson::Object();
    JValue event = pbnjson::Object();

    JsonDB::getDebugInstance().fetch(fullname, json);
    if (!json.hasKey(fullname)) {
        json.put(fullname, pbnjson::Array());
    }

    event.put("TimeStamp", Platform::timeStr());
    json[fullname].append(event);
    JsonDB::getDebugInstance().insert(fullname, json[fullname]);
    JsonDB::getDebugInstance().flush();
}

void Manager::printDebug()
{
    Configuration::getInstance().printDebug();
}

void Manager::updateFactoryDatabase(JValue configs, bool isVolatile)
{
    JValue database = pbnjson::Object();
    JsonDB* jsonDB = nullptr;

    if (isVolatile)
        jsonDB = &JsonDB::getVolatileInstance();
    else if (m_isLoaded)
        jsonDB = &JsonDB::getFactoryInstance();
    else
        jsonDB = &JsonDB::getFakeFactoryInstance();

    if (!jsonDB->fetch(JsonDB::FULLNAME_USER, database))
        database.put(JsonDB::FULLNAME_USER, pbnjson::Array());

    for (JValue::KeyValue config : configs.children()) {
        string fullName = config.first.asString();
        if (config.second.isNull())
            jsonDB->remove(fullName);
        else
            jsonDB->insert(fullName, config.second);

        Json::addUniqueStrIntoArray(database[JsonDB::FULLNAME_USER], fullName);
    }
    jsonDB->insert(JsonDB::FULLNAME_USER, database[JsonDB::FULLNAME_USER]);
    jsonDB->flush();
}

