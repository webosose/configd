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

#include <glib.h>
#include <strings.h>

#include <pbnjson.h>
#include <setting/Setting.h>

#include "Environment.h"
#include "util/Logger.hpp"

// TODO add below sentence an CMakeLists.txt
// webos_build_configured_file(files/conf/confd_setting.json sysconfdir "")
const char* Setting::DEFAULT_SETTING_FILE = INSTALL_WEBOS_SYSCONFDIR "/configd.json";
const char* Setting::DEBUG_SETTING_FILE = INSTALL_LOCALSTATEDIR "/configd_debug.json";

Setting::Setting()
    : m_configuration(Object()),
      m_isSnapshotBoot(false),
      m_isRespawned(false),
      m_bootStatus("unknown")
{
    if (access(DEBUG_SETTING_FILE, R_OK) == 0) {
        loadSetting(DEBUG_SETTING_FILE);
    } else if (access(DEFAULT_SETTING_FILE, R_OK) == 0) {
        loadSetting(DEFAULT_SETTING_FILE);
    }
    parseKernelCmdLine();
    parsePlatform();
    applySettings();
}

Setting::~Setting()
{
    if (m_call && m_call->isActive()) {
        m_call->cancel();
    }
}

void Setting::initialize()
{
    if (m_call && m_call->isActive()) {
        return;
    }
    string url = "luna://com.webos.bootManager/getBootStatus";
    pbnjson::JValue params = pbnjson::Object();
    params.put("subscribe", true);
    m_call = AbstractBusFactory::getInstance()->getIHandle()->call(url,
                                                                   params.stringify(),
                                                                   this);
}

bool Setting::isNormalStatus()
{
    if (m_bootStatus == "normal")
        return true;
    else
        return false;
}

void Setting::onReceiveCall(JValue &response)
{
    if (response.hasKey("bootStatus") && (m_bootStatus != response["bootStatus"].asString())) {
       m_bootStatus = response["bootStatus"].asString();
       Logger::info(MSGID_MAIN,
                    LOG_PREPIX_FORMAT "bootStatus is changed to '%s'",
                    LOG_PREPIX_ARGS, m_bootStatus.c_str());
    }
}

void Setting::parseKernelCmdLine()
{
    ifstream file;

    file.open("/proc/cmdline");
    if (file.fail()) {
        return;
    }

    string cmdline;
    cmdline.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (std::string::npos != cmdline.find("snapshot")) {
        m_isSnapshotBoot = true;
    }
    file.close();
}

void Setting::parsePlatform()
{
    m_isRespawned = g_file_test(CONFIGD_PID_FILE_PATH, G_FILE_TEST_EXISTS);
}

void Setting::applySettings()
{
    Logger::getInstance()->setLogType(getLogType(), getLogPath());
    Logger::getInstance()->setLogLevel(getLogLevel());
}

bool Setting::setSetting(JValue& source, JValue& local)
{
    auto it = source.children();
    for (auto object = it.begin() ; object != it.end() ; ++object) {
        string key = (*object).first.asString();
        JValue value = (*object).second;
        if (!local.hasKey(key)) {
            local.put(key, value);
        } else if (!value.isObject()){
            local.put(key, value);
        } else {
            JValue v = local[key];
            setSetting(value, v);
        }
    }
    return true;
}

JValue Setting::getSetting(initializer_list<const char*> list)
{
    JValue* pos = &m_configuration;
    JValue result;
    for (auto iter = list.begin() ; iter != list.end() ; ++iter) {
        if (!pos->hasKey(*iter)) {
            Logger::debug(LOG_PREPIX_FORMAT "key is not found : %s",
                          LOG_PREPIX_ARGS, *iter);
            return nullptr;
        } else {
            result = (*pos)[(*iter)];
            pos = &result;
        }
    }
    return result;
}

bool Setting::loadSetting(const string filename)
{
    JValue value = JDomParser::fromFile(filename.c_str());
    if (!value.isValid() || value.isNull()) {
        Logger::error(MSGID_JSON_PARSE_FILE_ERR,
                      LOG_PREPIX_FORMAT "Fail Invalid Json formmated file %s",
                      LOG_PREPIX_ARGS, filename.c_str());
        return false;
    }
    return setSetting(value, m_configuration);
}

LogType Setting::getLogType()
{
    JValue value = m_configuration["logger"]["type"];
    if (value.isNull()) {
        return LogType_PmLog;
    }
    return (LogType)value.asNumber<int32_t>();
}

LogLevel Setting::getLogLevel()
{
    JValue value = m_configuration["logger"]["level"];
    if (value.isNull()) {
        return LogLevel_Info;
    }
    return (LogLevel)value.asNumber<int32_t>();
}

string Setting::getLogPath()
{
    JValue value = m_configuration["logger"]["path"];
    if (value.isNull()) {
        return "";
    }
    return value.asString();
}

bool Setting::isSnapshotBoot()
{
    return m_isSnapshotBoot;
}

bool Setting::isRespawned()
{
    return m_isRespawned;
}

void Setting::clearSettings()
{
    m_configuration = Object();
}

void Setting::printSetting()
{
    Logger::info(MSGID_MAIN,
                 LOG_PREPIX_FORMAT "LogLevel(%d) / LogType(%d) / LogPath(%s) / BootStatus(%s)",
                 LOG_PREPIX_ARGS, getLogLevel(), getLogType(), getLogPath().c_str(), m_bootStatus.c_str());
}

void Setting::printDebug()
{
    cout << endl << "debug print" << endl;
    cout << m_configuration.stringify("  ");
    cout << endl << endl;
}
