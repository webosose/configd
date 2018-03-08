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

#ifndef _CALL_ADAPTOR_H_
#define _CALL_ADAPTOR_H_

#include <luna-service2++/call.hpp>

#include "../AbstractBusFactory.h"
#include "util/ObjectCounter.hpp"

using namespace LS;

class CallAdapter : public ICall, public ObjectCounter<CallAdapter> {
public:
    CallAdapter();
    CallAdapter(Call &call);
    virtual ~CallAdapter();

    virtual bool isActive();
    virtual void cancel();

private:
    Call m_call;

};

#endif /* _CALL_ADAPTOR_H_ */
