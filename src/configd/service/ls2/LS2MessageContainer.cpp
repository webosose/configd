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

#include "HandleAdapter.h"
#include "LS2MessageContainer.h"
#include "MessageAdapter.h"
#include "util/Logger.hpp"

LS2MessageContainer::LS2MessageContainer()
{

}

LS2MessageContainer::~LS2MessageContainer()
{

}

bool LS2MessageContainer::pushMessage(shared_ptr<IMessage> message)
{
    shared_ptr<HandleAdapter> handleAdapter =
            std::dynamic_pointer_cast<HandleAdapter>(AbstractBusFactory::getInstance()->getIHandle());
    shared_ptr<MessageAdapter> messageAdapter =
            std::dynamic_pointer_cast<MessageAdapter>(message);
    LSError lserror;

    LSErrorInit(&lserror);
    if (!LSSubscriptionAdd(handleAdapter->getHandle().get(),
                           m_key.c_str(),
                           messageAdapter->getMessage().get(),
                           &lserror)) {
        Logger::error(MSGID_HANDLER, LOG_PREPIX_FORMAT
                      "LSSubscriptionAdd failed : %s",
                      LOG_PREPIX_ARGS, lserror.message);
        LSErrorFree(&lserror);
        return false;
    }
    return true;
}

bool LS2MessageContainer::each(IMessagesListener &listener, JsonDB &newDB, JsonDB &oldDB)
{
    shared_ptr<HandleAdapter> handleAdapter =
                std::dynamic_pointer_cast<HandleAdapter>(AbstractBusFactory::getInstance()->getIHandle());
    LSSubscriptionIter* iter = nullptr;
    LSError lserror;

    LSErrorInit(&lserror);
    if (!LSSubscriptionAcquire(handleAdapter->getHandle().get(), m_key.c_str(), &iter, &lserror)) {
        Logger::error(MSGID_HANDLER, LOG_PREPIX_FORMAT
                      "LSSubscriptionAcquire failed : %s",
                      LOG_PREPIX_ARGS, lserror.message);
        LSErrorFree(&lserror);
        return false;
    }

    while (LSSubscriptionHasNext(iter)) {
        LSMessage *lsmessage = LSSubscriptionNext(iter);
        std::shared_ptr<IMessage> message = make_shared<MessageAdapter>(lsmessage);
        listener.eachMessage(message, newDB, oldDB);
    }
    LSSubscriptionRelease(iter);
    return true;
}

void LS2MessageContainer::setKey(string key)
{
    m_key = key;
}
