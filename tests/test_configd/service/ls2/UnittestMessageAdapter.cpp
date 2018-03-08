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

#include <stdlib.h>
#include <pbnjson.hpp>
#include <time.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "service/MockAbstractBusFactory.h"
#include "service/MockConfigd.h"

using namespace pbnjson;
using namespace std;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SetArgReferee;

class UnittestCallAdaptor : public testing::Test {
protected:
    UnittestCallAdaptor()
    {

    }

    virtual ~UnittestCallAdaptor()
    {

    }
};
