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

#include "Process.h"
#include "util/Logger.hpp"
#include "util/Platform.h"

Process::Process(JValue jvalue)
    : m_first(""),
      m_second("")
{
    m_type = ProcessType_Invalid;

    if (jvalue.hasKey("note")) {
        m_note = jvalue["note"].asString();
    }
    if (!jvalue.hasKey("process"))
        return;

    if (jvalue["process"].hasKey("command")) {
        m_type = ProcessType_Command;
    } else if (jvalue["process"].hasKey("json")) {
        m_type = ProcessType_Json;
    }
    m_jvalue = jvalue.duplicate();
}

Process::~Process()
{

}

void Process::setArgs(string first, string second)
{
    m_first = first;
    m_second = second;
}

void Process::execute()
{
    switch (m_type) {
    case ProcessType_Command:
        executeCommand();
        break;

    case ProcessType_Json:
        executeJson();
        break;

    default:
        break;
    }
}

void Process::executeCommand()
{
    string command = m_jvalue["process"]["command"].asString();
    string console = Platform::executeCommand(command, m_first, m_second);
    Logger::info(MSGID_CONFIGURE,
                 LOG_PREPIX_FORMAT "With arguments : command(%s), first(%s), second(%s), result(%s)",
                 LOG_PREPIX_ARGS, command.c_str(), m_first.c_str(), m_second.c_str(), console.c_str());
}

void Process::executeJson()
{
    Logger::info(MSGID_CONFIGURE,
                 LOG_PREPIX_FORMAT "JSON PROCESS",
                 LOG_PREPIX_ARGS);
}
