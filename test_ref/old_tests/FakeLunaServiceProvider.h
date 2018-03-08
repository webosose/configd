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

#ifndef FAKELUNASERVICEPROVIDER_H
#define FAKELUNASERVICEPROVIDER_H
#include "TestUtil.h"
#include <map>
#include <algorithm>
#include "FakeLSMessage.h"
#include "JUtil.h"

// Description :
//  reset g_service_storage.
//  g_service_storage has functions of the internal services
//  to be performed on luna service's method
// Usage :
//  TestEnvGuard object's destruction : ~TestEnvGuard()

void FakeLSReset();

// Description :
//  set expected return values of external services
// Usage :
//  pbnjson::JValue json = pbnjson::Object();
//  json.put("appId","com.yourdomain.helloworld");
//  json.put("returnValue",true);
//  FakeLSSetExtResponse("palm://com.palm.webappmanager/killApp",JUtil::jsonToString(json));

bool FakeLSSetExtResponse(const std::string &uriApi, const std::string &fakeResponse, const bool isSubscription = false,
                          const std::string &payload = "");

// Description :
//  Run Luna-service calls
//  that are stored in the LSCallOneReply of FakeLS.
// Usage :
//  call this function after calling LSCallOneReply
//  to run stored luna service
void FakeLSRunLSCall();

#endif // FAKELUNASERVICEPROVIDER_H

