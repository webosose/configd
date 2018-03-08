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

#ifndef _MOCK_LAYER_H_
#define _MOCK_LAYER_H_

#include <gmock/gmock.h>
#include <pbnjson.hpp>

#include "Environment.h"
#include "util/Platform.h"

#include "config/Layer.h"

using namespace pbnjson;
using namespace std;

class MockLayerListener : public LayerListener {
public:
    MOCK_METHOD3(onSelectionChanged, void(Layer &layer, string &oldSelection, string &newSelection));
};

#endif /* _MOCK_LAYER_H_ */
