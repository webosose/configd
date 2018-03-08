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

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <glib.h>
#include "TestGlobal.h"
#include <glib-object.h>

//! Uses as temporary variable for test_data path changes
static char *data_path(NULL);

static GOptionEntry entries[] =
{
    { "data", 'd', 0, G_OPTION_ARG_STRING, &data_path, "Set custom test data path", NULL},
    { NULL }
};

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new("- test application");
    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_set_ignore_unknown_options(context, true);

    if (!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_warning("%s", error->message);
        g_option_context_free(context);
        return EXIT_FAILURE;
    }

    g_option_context_free(context);

    if (data_path)
    {
        GlobalSettingsImpl::instance().set_test_data_path(data_path);
    }

    return RUN_ALL_TESTS();
}

