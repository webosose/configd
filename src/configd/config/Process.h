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

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <iostream>
#include <pbnjson.hpp>

#include "Environment.h"

using namespace pbnjson;
using namespace std;

enum ProcessType {
    ProcessType_Invalid,
    ProcessType_Command,
    ProcessType_Json
};

class Process {
public:
    Process(JValue jvalue);
    virtual ~Process();

    void setArgs(string first, string second);
    void execute();

private:
    void executeCommand();
    void executeJson();

    string m_note;
    string m_first, m_second;

    ProcessType m_type;
    JValue m_jvalue;
};

#endif /* _PROCESS_H_ */
