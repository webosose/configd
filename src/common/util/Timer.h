// Copyright (c) 2014-2020 LG Electronics, Inc.
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

#ifndef UTIL_TIMER_H_
#define UTIL_TIMER_H_

#include <glib.h>

// TODO: Lock function is needed

class Timer {
public:
    Timer();
    virtual ~Timer();

    bool wait(int ms);
    bool isWaiting();
    bool isExpired();
    void clear();

private:
    static int _expired(void* ctx);

    bool m_isWaiting;
    bool m_isExpired;
    guint m_timerId;
    guint m_timeout;

};

#endif /* UTIL_TIMER_H_ */
