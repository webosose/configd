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

#include <gtest/gtest.h>
#include <pbnjson.hpp>

#include "Environment.h"
#include "util/Timer.h"

using namespace pbnjson;
using namespace std;

class UnittestTimer : public testing::Test {
protected:
    UnittestTimer()
    {
    }

    virtual ~UnittestTimer()
    {
    }

    Timer m_timer;
};

TEST_F(UnittestTimer, basicOperation)
{
    EXPECT_TRUE(m_timer.clear());
    EXPECT_FALSE(m_timer.isWaiting());
    EXPECT_FALSE(m_timer.isExpired());
}

TEST_F(UnittestTimer, basicWaiting)
{
    EXPECT_FALSE(m_timer.isWaiting());
    EXPECT_TRUE(m_timer.wait(1));
    EXPECT_TRUE(m_timer.isExpired());
}

TEST_F(UnittestTimer, multipleWaiting)
{
    EXPECT_TRUE(m_timer.wait(1));
    EXPECT_TRUE(m_timer.isExpired());
    EXPECT_FALSE(m_timer.wait(1));
}

TEST_F(UnittestTimer, waitingClearWaiting)
{
    EXPECT_TRUE(m_timer.wait(1));
    EXPECT_TRUE(m_timer.isExpired());
    EXPECT_TRUE(m_timer.clear());
    EXPECT_FALSE(m_timer.isExpired());
    EXPECT_TRUE(m_timer.wait(1));
    EXPECT_TRUE(m_timer.isExpired());
}

TEST_F(UnittestTimer, waitingAndWaitingAgain)
{
    EXPECT_TRUE(m_timer.wait(1));
    EXPECT_FALSE(m_timer.wait(1));
    EXPECT_TRUE(m_timer.isExpired());
    EXPECT_FALSE(m_timer.isWaiting());
}
