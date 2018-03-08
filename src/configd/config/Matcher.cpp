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

#include <string>

#include "Matcher.h"
#include "util/Logger.hpp"

Matcher::Matcher(const JValue& where)
{
    m_where = where.duplicate();
}

Matcher::~Matcher()
{

}

bool Matcher::validateCondition() {
    if (!m_where.hasKey("prop") || !m_where.hasKey("val") || !m_where.hasKey("op")) {
        Logger::warning(MSGID_CONFIGURE,
                        LOG_PREPIX_FORMAT "where condition should be provided prop, val, op %s",
                        LOG_PREPIX_ARGS, m_where.stringify("    ").c_str());
        return false;
    }

    if (m_where["op"].asString() != "=") {
        Logger::warning(MSGID_CONFIGURE,
                        LOG_PREPIX_FORMAT "operation %s is not allowed",
                        LOG_PREPIX_ARGS, m_where["op"].asString().c_str());
        return false;
    }
    return true;
}

bool Matcher::checkCondition(JsonDB *jsonDB) {
    Logger::info(MSGID_CONFIGURE,
                 LOG_PREPIX_FORMAT "Check condition where: %s",
                 LOG_PREPIX_ARGS, m_where.stringify().c_str());

    if (!validateCondition()) {
        Logger::warning(MSGID_CONFIGURE,
                        LOG_PREPIX_FORMAT "where state is not valid",
                        LOG_PREPIX_ARGS);
        return false;
    }

    string key = m_where["prop"].asString();
    JValue config = pbnjson::Object();
    JValue configValue = pbnjson::Object();

    if (jsonDB == NULL) {
        Logger::warning(MSGID_CONFIGURE,
                        LOG_PREPIX_FORMAT "jsonDB are not existed",
                        LOG_PREPIX_ARGS);
        return false;
    }
    else {
        if (!jsonDB->fetch(key, config) || !config.hasKey(key)) {
            return false;
        }
        configValue = config[key];
    }

    return isMatched(configValue);
}

bool Matcher::isMatched(JValue configValue) {
    bool returnValue = false;
    std::string operation = m_where["op"].asString();

    if (operation == "=")
        returnValue = (configValue == m_where["val"]);

    return returnValue;
}

