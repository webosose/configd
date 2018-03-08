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

#include "TestGlobal.h"
#include <unistd.h>

static void replaceAll(std::string *str, const std::string &from, const std::string &to)
{
    if (from.empty())
    {
        return;
    }

    size_t start_pos = 0;

    while ((start_pos = str->find(from, start_pos)) != std::string::npos)
    {
        str->replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

GlobalSettings::GlobalSettings()
{
    // find test_data at the current executable's path.
    char exe_name[512] = {0,};
    int len = readlink("/proc/self/exe", exe_name, sizeof(exe_name));

    if (len > 0)
    {
        m_data_path = exe_name;
        m_data_path = m_data_path.substr(0, m_data_path.find_last_of("/"));
        m_data_path += "/data";
    }

    value_list["{TEST_DATA}"] = m_data_path;
}

std::string GlobalSettings::get_test_data_path()
{
    return m_data_path;
}

void GlobalSettings::set_test_data_path(const std::string &path)
{
    m_data_path = path;
}


std::string GlobalSettings::get_settings_list()
{
    std::string result;
    std::map<std::string, std::string>::iterator pit = value_list.begin();

    for (; pit != value_list.end(); ++pit)
    {
        result = pit->first + "  = " + pit->second + "\n";
    }

    return result;
}

void GlobalSettings::replace_value_by_template(std::string *in_out)
{
    std::map<std::string, std::string>::iterator pit = value_list.begin();

    for (; pit != value_list.end(); ++pit)
    {
        replaceAll(in_out, pit->second, pit->first);
    }
}

void GlobalSettings::replace_template_by_value(std::string *in_out)
{
    std::map<std::string, std::string>::iterator pit = value_list.begin();

    for (; pit != value_list.end(); ++pit)
    {
        replaceAll(in_out, pit->first, pit->second);
    }
}

