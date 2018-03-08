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

#include "service/ls2/LS2BusFactory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <luna-service2++/handle.hpp>
#include <memory>

#include "service/MockAbstractBusFactory.h"

using namespace pbnjson;
using namespace std;
using namespace LS;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SetArgReferee;

class UnittestLS2BusFactory : public testing::Test {
protected:
    UnittestLS2BusFactory()
        : m_iHandle(new MockIHandle())
    {
        LS2BusFactory::getInstance()->setIHandle(m_iHandle);
    }

    virtual ~UnittestLS2BusFactory()
    {
    }

    shared_ptr<MockIHandle> m_iHandle;
    Handle m_handle;
};

TEST_F(UnittestLS2BusFactory, checkIHandle)
{
    EXPECT_EQ(LS2BusFactory::getInstance()->getIHandle(), m_iHandle);
}

TEST_F(UnittestLS2BusFactory, compareSameSubscriptionObjs)
{
    shared_ptr<IMessages> A = LS2BusFactory::getInstance()->getIMessages("test");
    shared_ptr<IMessages> B = LS2BusFactory::getInstance()->getIMessages("test");

    EXPECT_NE(nullptr, A);
    EXPECT_NE(nullptr, B);
    EXPECT_EQ(A, B);
    EXPECT_EQ(1, LS2MessageContainer::getIntanceCount());
}

TEST_F(UnittestLS2BusFactory, invalidCreation)
{
}


// nullptr로 객체 생성이 안되는 것 확인하기
// 진짜 handle을 생성할 수있나?? 진짜 객체로 생성되는지 확인해보자
