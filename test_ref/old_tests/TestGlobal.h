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

#include <string>
#include <map>
#include "Singleton.hpp"

//! List of global settings like default path or something like this
class GlobalSettings
{
public:
    //! return path to test data
    std::string get_test_data_path();

    //! Set path to test data. Can be used if you would like to work with custom data.
    void set_test_data_path(const std::string &path);

    /*! replace values internall of the string by template.
     * for example /home/usr/src/testdata/my_app by {TST_DATA_PATH}/my_app
     */
    void replace_value_by_template(std::string *in_out);

    /*! replace specific templates with the value
     * for example {TST_DATA_PATH}/my_app into /home/usr/src/testdata/my_app
     */
    void replace_template_by_value(std::string *in_out);

    //! Return user readable key-value description of internal settings (path etc)
    std::string get_settings_list();
protected:
    friend class Singleton<GlobalSettings>;
    GlobalSettings();
private:
    std::string m_data_path;
    std::map<std::string, std::string> value_list;
};

typedef Singleton<GlobalSettings> GlobalSettingsImpl;

