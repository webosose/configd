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

#ifndef _MESSAGE_ADAPTOR_H_
#define _MESSAGE_ADAPTOR_H_

#include <iostream>
#include <pbnjson.hpp>
#include <luna-service2++/message.hpp>

#include "../AbstractBusFactory.h"
#include "util/ObjectCounter.hpp"

using namespace std;
using namespace pbnjson;
using namespace LS;

class MessageAdapter : public IMessage, public ObjectCounter<MessageAdapter> {
public:
    MessageAdapter(LSMessage *msg);
    virtual ~MessageAdapter();

    // IMessage
    virtual void respond(JValue payload);
    virtual bool isSubscription();
    virtual string getPayload();
    virtual string clientName();

    // Local Method
    Message& getMessage();

private:
    Message m_message;

};

#endif /* _MESSAGE_ADAPTOR_H_ */
