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

#ifndef UTIL_PLATFORM_H_
#define UTIL_PLATFORM_H_

#include <iostream>
#include <stdbool.h>
#include <glib.h>

#include "Environment.h"

using namespace std;

class Platform {
public:
    static bool isFileExist(const string &path);
    static bool isDirExist(const string &path);
    static bool canWriteFile(const string &path);
    static string readFile(const string &filename);
    static bool writeFile(const string &filename, const string &content);
    static bool copyFile(const string &from, const string &to);
    static bool deleteFile(const string &dbFileName);

    static string executeCommand(const string &command, const string firstArg, const string secondArg);
    static bool executeCommand(const string &command, string &console);

    static string concatPaths(string parent, string child);
    static void extractFileName(string &fileName, string &name, string &extension);
    static void trim(string& str);
    static string timeStr();

private:
    Platform();
    virtual ~Platform();
};

#endif // UTIL_PLATFORM_H_

