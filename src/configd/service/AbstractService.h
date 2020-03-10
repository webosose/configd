// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#ifndef ABSSERVICE_H_
#define ABSSERVICE_H_

#include <iostream>

#include <luna-service2/lunaservice.hpp>
#include "util/Logger.hpp"

using namespace std;
using namespace LS;

class AbstractService {
public:
    AbstractService(string name)
        : m_name(name)
    {
    }

    virtual ~AbstractService()
    {
        try {
            if (m_serverStatus) {
                m_serverStatus.cancel();
            }
        } catch (const LS::Error &e) {
        }
    }

    bool registerServerStatus(Handle *sh, ServerStatusCallback callback)
    {
        Logger::debug("registerServerStatus : %s", m_name.c_str());
        if (m_serverStatus) {
            m_serverStatus.cancel();
        }
        // TODO: Do we need to consider service on/off status in service classes?
        // If it is 'yes', we need to implement some logic for that
        // For example, subscriptions should be closed if service is down.
        m_callback = callback;
        m_serverStatus = sh->registerServerStatus(m_name.c_str(), m_callback);
        return true;
    }

    string& getName()
    {
        return m_name;
    }

    int getTimeout()
    {
        return TIMEOUT_MAX;
    }

protected:
    static const int TIMEOUT_MAX = 10000;

private:
    string m_name;
    ServerStatusCallback m_callback;
    ServerStatus m_serverStatus;
};

#endif /* ABSSERVICE_H_ */
