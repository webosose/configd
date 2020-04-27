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

#include <signal.h>
#include <glib.h>
#include <time.h>

#include <pbnjson.hpp>

#include "Environment.h"
#include "config/Configuration.h"
#include "Configd.h"
#include "Logging.h"
#include "ErrorDB.h"
#include "util/Json.h"
#include "util/Platform.h"
#include "util/Logger.hpp"

const string Configd::NAME_CONFIGD = "com.webos.service.config";
const string Configd::NAME_CONFIGD_SIGNALS = "/com/webos/config";
const string Configd::NAME_CONFIGD_RELOAD_DONE = "reloadDone";
const string Configd::NAME_GET_PERMISSION = "read";

const LSMethod Configd::METHOD_TABLE[7] = {
    { "getConfigs", Configd::_getConfigs, LUNA_METHOD_FLAGS_NONE },
    { "reconfigure", Configd::_reconfigure, LUNA_METHOD_FLAGS_NONE },
    { "setConfigs", Configd::_setConfigs, LUNA_METHOD_FLAGS_NONE },
    { "reloadConfigs", Configd::_reloadConfigs, LUNA_METHOD_FLAGS_NONE },
    { "dump", Configd::_dump, LUNA_METHOD_FLAGS_NONE },
    { "fullDump", Configd::_fullDump, LUNA_METHOD_FLAGS_NONE },
    { nullptr, nullptr }
};

const LSSignal Configd::SIGNAL_TABLE[2] = {
    { NAME_CONFIGD_RELOAD_DONE.c_str(), LUNA_SIGNAL_FLAGS_NONE },
    { nullptr }
};

Configd::Configd()
    : m_configdListener(NULL)
{

}

Configd::~Configd()
{

}

void Configd::initialize(GMainLoop *mainLoop, ConfigdListener *listener)
{
    m_configdListener = listener;

    if (mainLoop == NULL)
        return;

    try {
        Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Try to connect bus", LOG_PREPIX_ARGS);
        if (!AbstractBusFactory::getInstance()->getIHandle()->connect(NAME_CONFIGD)) {
            Logger::warning(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Error in bus connect", LOG_PREPIX_ARGS);
        }

        Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Try to enroll methods", LOG_PREPIX_ARGS);
        AbstractBusFactory::getInstance()->getIHandle()->addMethods("/", Configd::METHOD_TABLE);

        Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Try to add signals", LOG_PREPIX_ARGS);
        AbstractBusFactory::getInstance()->getIHandle()->addSignals(NAME_CONFIGD_SIGNALS, Configd::SIGNAL_TABLE);

        Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Try to add data", LOG_PREPIX_ARGS);
        AbstractBusFactory::getInstance()->getIHandle()->addData("/", this);

        Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Try to attach mainloop", LOG_PREPIX_ARGS);
        AbstractBusFactory::getInstance()->getIHandle()->attach(mainLoop);
    } catch (LS::Error &error) {
        Logger::error(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Error: %s", error.what());
    }
}

void Configd::sendSignal(const string &name)
{
    string url = "luna://" + NAME_CONFIGD + NAME_CONFIGD_SIGNALS + "/" + name;

    pbnjson::JValue signalPayload = pbnjson::Object();
    signalPayload.put("returnValue", true);
    signalPayload.put("reloadDone", true);
    AbstractBusFactory::getInstance()->getIHandle()->sendSignal(url, signalPayload.stringify());
}

void Configd::eachMessage(shared_ptr<IMessage> message, JsonDB &newDB, JsonDB &oldDB)
{
    JValue requestPayload = JDomParser::fromString(message->getPayload());
    if (!requestPayload.isValid() || requestPayload.isNull()) {
        Logger::warning(MSGID_CONFIGDSERVICE,
                        LOG_PREPIX_FORMAT "Request Payload is invalid",
                        LOG_PREPIX_ARGS);
        return;
    }
    JValue newResponsePayload = pbnjson::Object();
    JValue oldResponsePayload = pbnjson::Object();

    if (!msgGetConfigs(oldDB, JsonDB::getPermissionInstance(), message, oldResponsePayload)) {
        Logger::warning(MSGID_CONFIGDSERVICE,
                        LOG_PREPIX_FORMAT "Error in oldDB msgGetConfigs",
                        LOG_PREPIX_ARGS);
    }
    if (!msgGetConfigs(newDB, JsonDB::getPermissionInstance(), message, newResponsePayload)) {
        Logger::warning(MSGID_CONFIGDSERVICE,
                        LOG_PREPIX_FORMAT "Error in newDB msgGetConfigs",
                        LOG_PREPIX_ARGS);
    }

    if (oldResponsePayload == newResponsePayload) {
        Logger::debug(LOG_PREPIX_FORMAT "Same response) Client (%s) Request (%s)",
                      LOG_PREPIX_ARGS,
                      message->clientName().c_str(),
                      requestPayload.stringify("    ").c_str());
        return;
    }

    // "configs" would not exist if all values are $undefined
    // send only changed value
    if (oldResponsePayload.hasKey("configs")) {
        for (JValue::KeyValue feature : oldResponsePayload["configs"].children()) {
            std::string key = feature.first.asString();
            if (feature.second == newResponsePayload["configs"][key])
                newResponsePayload["configs"].remove(key);
        }
    }

    newResponsePayload.put("returnValue", true);
    newResponsePayload.put("subscribed", true);
    message->respond(newResponsePayload);
    Logger::debug(LOG_PREPIX_FORMAT "Subscription) Client (%s) Request (%s)",
                  LOG_PREPIX_ARGS,
                  message->clientName().c_str(),
                  requestPayload.stringify("    ").c_str());
    Logger::verbose(LOG_PREPIX_FORMAT "Response (%s)",
                    LOG_PREPIX_ARGS,
                    newResponsePayload.stringify("    ").c_str());
}

void Configd::postGetConfigs(JsonDB &newDB, JsonDB &oldDB)
{
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Start Post-getConfigs", LOG_PREPIX_ARGS);
    shared_ptr<IMessages> container = AbstractBusFactory::getInstance()->getIMessages("getConfigs");
    if (!container->each(*this, newDB, oldDB)) {
        Logger::warning(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Error in postGetConfigs each", LOG_PREPIX_ARGS);
    }
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "End Post-getConfigs", LOG_PREPIX_ARGS);
}

bool Configd::hasPermission(JValue permissions, string serviceName, string permissionType)
{
    if (!permissions.hasKey(permissionType) || !permissions[permissionType].isArray())
        return true;

    for (JValue permission : permissions[permissionType].items()) {
        string permissionName = permission.asString();
        if (permissionName == "*" || permissionName == serviceName)
            return true;

        if (permissionName.substr(permissionName.length()-1) != "*" ||(permissionName.length()-1 > serviceName.length()))
            continue;

        if (serviceName.substr(0, permissionName.length()-1) == permissionName.substr(0, permissionName.length()-1))
            return true;
    }
    return false;
}

bool Configd::msgGetConfigs(JsonDB &db, JsonDB &permissionDB, shared_ptr<IMessage> request, JValue &response)
{
    JValue requestPayload = JDomParser::fromString(request->getPayload());
    string serviceName = request->clientName();
    JValue configs = pbnjson::Object();
    JValue missingConfigs = pbnjson::Array();

    if (!requestPayload.hasKey("configNames") || !requestPayload["configNames"].isArray()) {
        return false;
    }

    for (JValue config : requestPayload["configNames"].items()) {
        string fullName = config.asString();
        if (!db.fetch(fullName, configs)) {
            missingConfigs.append(fullName);
            continue;
        }
    }

    JValue responseConfigs = configs.duplicate();
    for (JValue::KeyValue feature : configs.children()) {
        std::string key = feature.first.asString();
        JValue permissions = pbnjson::Object();
        if (!permissionDB.fetch(key, permissions)) {
            continue;
        }
        if (!hasPermission(permissions[key], serviceName, NAME_GET_PERMISSION)) {
            responseConfigs.remove(key);
            missingConfigs.append(key);
            Logger::debug(LOG_PREPIX_FORMAT "Subscription) Client (%s) has no permission to get",
                          LOG_PREPIX_ARGS,
                          request->clientName().c_str(),
                          key.c_str());
        }
    }

    if (responseConfigs.objectSize() > 0)
        response.put("configs", responseConfigs);
    if (missingConfigs.arraySize() > 0)
        response.put("missingConfigs", missingConfigs);
    return true;
}

bool Configd::getConfigs(LSMessage &message)
{
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Start Handle-getConfigs", LOG_PREPIX_ARGS);
    std::shared_ptr<IMessage> request = AbstractBusFactory::getInstance()->getIMessage(&message);
    JValue requestPayload;
    JValue responsePayload = pbnjson::Object();
    int errorCode = ErrorDB::ERRORCODE_UNKNOWN;
    bool returnValue = true;
    bool subscribed = false;

    JSchema schema = JSchema::fromFile(GETCONFIGS_SCHEMA);
    requestPayload = JDomParser::fromString(request->getPayload(), schema);
    if (requestPayload.isNull()) {
        errorCode = ErrorDB::ERRORCODE_INVALID_PARAMETER;
        returnValue = false;
        goto Exit;
    }

    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Request (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), requestPayload.stringify("    ").c_str());

    if (!msgGetConfigs(JsonDB::getUnifiedInstance(), JsonDB::getPermissionInstance(), request, responsePayload)) {
        errorCode = ErrorDB::ERRORCODE_RESPONSE;
        returnValue = false;
        goto Exit;
    }

    if (request->isSubscription()) {
        subscribed = true;
        if (!AbstractBusFactory::getInstance()->getIMessages("getConfigs")->pushMessage(request)) {
            Logger::warning(MSGID_CONFIGDSERVICE,
                            LOG_PREPIX_FORMAT "Error in pushMessage",
                            LOG_PREPIX_ARGS);
        }
    }

    errorCode = m_configdListener->onGetConfigs();
    if (errorCode < 0) {
        returnValue = false;
    }

Exit:
    responsePayload.put("returnValue", returnValue);
    responsePayload.put("subscribed", subscribed);
    if (!returnValue) {
        Logger::error(MSGID_CONFIGDSERVICE,
                      LOG_PREPIX_FORMAT "Error: %s",
                      LOG_PREPIX_ARGS, ErrorDB::getErrorText(errorCode));
        responsePayload.put("errorCode", errorCode);
        responsePayload.put("errorText", ErrorDB::getErrorText(errorCode));
    }

    request->respond(responsePayload);
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "End Handle-getConfigs", LOG_PREPIX_ARGS);
    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Response (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), responsePayload.stringify("    ").c_str());
    return true;
}

JValue Configd::splitVolatileConfigs(JValue oldConfigs)
{
    JValue volatileConfigs = Object();

    for (JValue::KeyValue config : oldConfigs.children()) {
        string fullName = config.first.asString();
        string categoryName;
        string configName;

        if (JsonDB::split(fullName, categoryName, configName) && categoryName == "tv.model") {
            volatileConfigs.put(fullName, config.second);
            oldConfigs.remove(fullName);
        }
    }
    return volatileConfigs;
}

bool Configd::setConfigs(LSMessage &message)
{
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Start Handle-setConfigs", LOG_PREPIX_ARGS);
    std::shared_ptr<IMessage> request = AbstractBusFactory::getInstance()->getIMessage(&message);
    JValue requestPayload;
    JValue responsePayload = pbnjson::Object();
    int errorCode = ErrorDB::ERRORCODE_UNKNOWN;
    bool returnValue = true;

    JSchema schema = JSchema::fromFile(SETCONFIGS_SCHEMA);
    requestPayload = JDomParser::fromString(request->getPayload(), schema);

    if (requestPayload.isNull()) {
        errorCode = ErrorDB::ERRORCODE_JSON_PARSING;
        returnValue = false;
        goto Exit;
    }

    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Request (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), requestPayload.stringify("    ").c_str());

    if (!requestPayload.hasKey("configs")) {
        errorCode = ErrorDB::ERRORCODE_INVALID_PARAMETER;
        returnValue = false;
        goto Exit;
    }

    if (requestPayload["configs"].objectSize() <= 0) {
        errorCode = ErrorDB::ERRORCODE_INVALID_PARAMETER;
        returnValue = false;
        goto Exit;
    }

    if (requestPayload.hasKey("volatile") && requestPayload["volatile"].asBool()) {
        errorCode = m_configdListener->onSetConfigs(requestPayload["configs"], true);
    } else if (request->clientName() == "com.webos.app.factorywin") {
        // TODO: Workaround code for TV factory policy.
        // It should be modified in the future.
        JValue volatileConfigs = splitVolatileConfigs(requestPayload["configs"]);

        if (volatileConfigs.objectSize() > 0)
            errorCode = m_configdListener->onSetConfigs(volatileConfigs, true);
        if (requestPayload["configs"].objectSize() > 0)
            errorCode = m_configdListener->onSetConfigs(requestPayload["configs"], false);
    } else {
        errorCode = m_configdListener->onSetConfigs(requestPayload["configs"], false);
    }
    if (errorCode < 0) {
        returnValue = false;
        goto Exit;
    }

Exit:
    responsePayload.put("returnValue", returnValue);
    if (!returnValue) {
        Logger::error(MSGID_CONFIGDSERVICE,
                      LOG_PREPIX_FORMAT "Error: %s",
                      LOG_PREPIX_ARGS, ErrorDB::getErrorText(errorCode));
        responsePayload.put("errorCode", errorCode);
        responsePayload.put("errorText", ErrorDB::getErrorText(errorCode));
    }
    request->respond(responsePayload);
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "End Handle-setConfigs", LOG_PREPIX_ARGS);
    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Response (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), responsePayload.stringify("    ").c_str());
    return true;
}

bool Configd::dump(LSMessage &message)
{
    Logger::warning(MSGID_DEPRECATED_API,
                    LOG_PREPIX_FORMAT "dump API is deprecated. Do not use.",
                    LOG_PREPIX_ARGS);
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Start Handle-dump", LOG_PREPIX_ARGS);
    std::shared_ptr<IMessage> request = AbstractBusFactory::getInstance()->getIMessage(&message);
    JValue requestPayload;
    JValue responsePayload = pbnjson::Object();
    int errorCode = ErrorDB::ERRORCODE_UNKNOWN;
    bool returnValue = true;

    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Request (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), requestPayload.stringify("    ").c_str());

    JValue configs = pbnjson::Object();
    errorCode = m_configdListener->onDump(configs);
    if (errorCode < 0) {
        returnValue = false;
        goto Exit;
    }

Exit:
    responsePayload.put("returnValue", returnValue);
    if (returnValue) {
        responsePayload.put("configs", configs);
    } else {
        Logger::error(MSGID_CONFIGDSERVICE,
                      LOG_PREPIX_FORMAT "Error: %s",
                      LOG_PREPIX_ARGS, ErrorDB::getErrorText(errorCode));
        responsePayload.put("errorCode", errorCode);
        responsePayload.put("errorText", ErrorDB::getErrorText(errorCode));
    }

    request->respond(responsePayload);
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "End Handle-dump", LOG_PREPIX_ARGS);
    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Response (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), responsePayload.stringify("    ").c_str());
    return true;
}

bool Configd::fullDump(LSMessage &message)
{
    Logger::warning(MSGID_DEPRECATED_API,
                    LOG_PREPIX_FORMAT "fullDump API is deprecated. Do not use.",
                    LOG_PREPIX_ARGS);
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Start Handle-fullDump", LOG_PREPIX_ARGS);
    std::shared_ptr<IMessage> request = AbstractBusFactory::getInstance()->getIMessage(&message);
    JValue requestPayload;
    JValue responsePayload = pbnjson::Object();
    int errorCode = ErrorDB::ERRORCODE_UNKNOWN;
    bool returnValue = true;

    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Request (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), requestPayload.stringify("    ").c_str());

    JValue configs = pbnjson::Array();
    errorCode = m_configdListener->onFullDump(configs);
    if (errorCode < 0) {
        returnValue = false;
        goto Exit;
    }

Exit:
    responsePayload.put("returnValue", returnValue);
    if (returnValue) {
        responsePayload.put("configs", configs);
    } else {
        Logger::error(MSGID_CONFIGDSERVICE,
                      LOG_PREPIX_FORMAT "Error: %s",
                      LOG_PREPIX_ARGS, ErrorDB::getErrorText(errorCode));
        responsePayload.put("errorCode", errorCode);
        responsePayload.put("errorText", ErrorDB::getErrorText(errorCode));
    }

    request->respond(responsePayload);
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "End Handle-fullDump", LOG_PREPIX_ARGS);
    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Response (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), responsePayload.stringify("    ").c_str());
    return true;
}

bool Configd::reconfigure(LSMessage &message)
{
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Start Handle-reconfigure", LOG_PREPIX_ARGS);
    std::shared_ptr<IMessage> request = AbstractBusFactory::getInstance()->getIMessage(&message);
    JValue requestPayload;
    JValue responsePayload = pbnjson::Object();
    int errorCode = ErrorDB::ERRORCODE_UNKNOWN;
    bool returnValue = true;

    int timeout = -1;
    JSchema schema = JSchema::fromFile(RECONFIGS_SCHEMA);
    requestPayload = JDomParser::fromString(request->getPayload(), schema);

    if (requestPayload.isNull()) {
        errorCode = ErrorDB::ERRORCODE_JSON_PARSING;
        returnValue = false;
        goto Exit;
    }

    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Request (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), requestPayload.stringify("    ").c_str());

    if (requestPayload.hasKey("timeout") &&  requestPayload["timeout"].asNumber<int>(timeout) != CONV_OK) {
        errorCode = ErrorDB::ERRORCODE_INVALID_PARAMETER;
        returnValue = false;
        goto Exit;
    }

    errorCode = m_configdListener->onReconfigure(timeout);
    if (errorCode < 0) {
        returnValue = false;
        goto Exit;
    }

Exit:
    responsePayload.put("returnValue", returnValue);
    if (!returnValue) {
        Logger::error(MSGID_CONFIGDSERVICE,
                      LOG_PREPIX_FORMAT "Error: %s",
                      LOG_PREPIX_ARGS, ErrorDB::getErrorText(errorCode));
        responsePayload.put("errorCode", errorCode);
        responsePayload.put("errorText", ErrorDB::getErrorText(errorCode));
    }

    request->respond(responsePayload);
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "End Handle-reconfigure", LOG_PREPIX_ARGS);
    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Response (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), responsePayload.stringify("    ").c_str());
    return true;
}

bool Configd::reloadConfigs(LSMessage &message)
{
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "Start Handle-reloadConfigs", LOG_PREPIX_ARGS);
    std::shared_ptr<IMessage> request = AbstractBusFactory::getInstance()->getIMessage(&message);
    JValue requestPayload;
    JValue responsePayload = pbnjson::Object();
    int errorCode = ErrorDB::ERRORCODE_UNKNOWN;
    bool returnValue = true;

    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Request (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), requestPayload.stringify("    ").c_str());

    errorCode = m_configdListener->onReloadConfigs();
    if (errorCode < 0) {
        returnValue = false;
        goto Exit;
    }

Exit:
    responsePayload.put("returnValue", returnValue);
    if (!returnValue) {
        Logger::error(MSGID_CONFIGDSERVICE,
                      LOG_PREPIX_FORMAT "Error: %s",
                      LOG_PREPIX_ARGS, ErrorDB::getErrorText(errorCode));
        responsePayload.put("errorCode", errorCode);
        responsePayload.put("errorText", ErrorDB::getErrorText(errorCode));
    }

    request->respond(responsePayload);
    Logger::info(MSGID_CONFIGDSERVICE, LOG_PREPIX_FORMAT "End Handle-reloadConfigs", LOG_PREPIX_ARGS);
    Logger::verbose(LOG_PREPIX_FORMAT "Client (%s) Response (%s)",
                    LOG_PREPIX_ARGS,
                    request->clientName().c_str(), responsePayload.stringify("    ").c_str());
    return true;
}
