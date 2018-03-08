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

#ifndef TESTUTIL_H
#define TESTUTIL_H

#include <gtest/gtest.h>
#include <luna-service2/lunaservice.h>
#include <string>

//! List of useful for testing utulites and functions
class TestUtil
{
public:
    //! Create or return main loop
    static GMainLoop *get_main_loop();

    //! Release main loop
    static void release_mainloop();

    //! Return plain text file content
    static std::string get_file_content(const std::string &path);

    //! return pointer to the function from methid list by name of this pointer
    static LSMethodFunction get_cb_pointer(const char *name, LSMethod *methods);

    //! Create message and fill internal fields
    static LSMessage *create_message(LSHandle *sh, const char *category, const char *method, const char *payload,
                                     LSMessageToken token = 0);

    static const char *message_get_category(LSMessage *message);
    static const char *message_get_method(LSMessage *message);
    static const char *message_get_payload(LSMessage *message);

protected:
private:
    static GMainLoop *s_main_loop;
};
#endif // TESTUTIL_H

