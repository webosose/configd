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

#ifndef MOCK_MOCK_CONFIGD_H_
#define MOCK_MOCK_CONFIGD_H_

#include <gmock/gmock.h>
#include <pbnjson.hpp>

#include "Environment.h"

#include "service/Configd.h"

using namespace pbnjson;
using namespace std;

class MockConfigdListener : public ConfigdListener {
public:
    MOCK_METHOD0(onGetConfigs, int());
    MOCK_METHOD2(onSetConfigs, int(JValue configs, bool isVolatile));
    MOCK_METHOD1(onDump, int(JValue &configs));
    MOCK_METHOD1(onFullDump, int(JValue &configs));
    MOCK_METHOD1(onReconfigure, int(int timeout));
    MOCK_METHOD0(onReloadConfigs, int());
};

class MockConfigd : public Configd {
public:
    MockConfigd() {};

    MOCK_METHOD2(initialize, void(GMainLoop*, ConfigdListener*));
};

#endif /* MOCK_MOCK_CONFIGD_H_ */
