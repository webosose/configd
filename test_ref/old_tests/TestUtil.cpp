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

#include "TestUtil.h"

#include "TestGlobal.h"
#include <luna-service2/lunaservice.h>
#include <string.h>

#include "FakeLSMessage.h"

GMainLoop *TestUtil::s_main_loop = NULL;

GMainLoop *TestUtil::get_main_loop()
{
    if (!s_main_loop)
    {
        s_main_loop = g_main_loop_new(NULL, FALSE);
    }

    return s_main_loop;
}


void TestUtil::release_mainloop()
{
    if (s_main_loop)
    {
        g_main_loop_unref(s_main_loop);
    }
}

std::string TestUtil::get_file_content(const std::string &path)
{
    std::FILE *fp = std::fopen(path.c_str(), "rb");

    if (fp)
    {
        std::string contents;
        std::fseek(fp, 0, SEEK_END);
        contents.resize(std::ftell(fp));
        std::rewind(fp);
        int ret = std::fread(&contents[0], 1, contents.size(), fp);
        std::fclose(fp);

        if (ret != 0)
        {
            return contents;
        }
    }

    return "";
}

LSMethodFunction TestUtil::get_cb_pointer(const char *name, LSMethod *methods)
{
    for (size_t i = 0; ; ++i)
    {
        if (!methods[i].name)
        {
            break;
        }

        if (strcmp(methods[i].name, name) == 0)
        {
            return methods[i].function;
        }
    }

    return NULL;
}

LSMessage *TestUtil::create_message(LSHandle *sh, const char *category, const char *method, const char *payload,
                                    LSMessageToken token)
{
    LSMessage *msg = new LSMessage();
    msg->ref = 1;
    msg->transport_msg = 0;
    msg->sh = sh;
    msg->category = category;
    msg->method = method;
    msg->payload = payload;
    msg->responseToken = token;
    return msg;
}

const char *TestUtil::message_get_category(LSMessage *message)
{
    return message->category.c_str();
}

const char *TestUtil::message_get_method(LSMessage *message)
{
    return message->method.c_str();
}

const char *TestUtil::message_get_payload(LSMessage *message)
{
    return message->payload.c_str();
}

