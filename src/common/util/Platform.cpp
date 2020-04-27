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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>
#include <boost/algorithm/string.hpp>
#include <string>
#include <fstream>
#include <iostream>

#include "Logging.h"
#include "util/Platform.h"
#include "util/Logger.hpp"

bool Platform::isFileExist(const string &path)
{
    if (access(path.c_str(), 0) != -1) {
        return true;
    }
    return false;
}

bool Platform::isDirExist(const string &path)
{
    if (g_file_test(path.c_str(), G_FILE_TEST_IS_DIR)) {
        return true;
    }
    return false;
}

bool Platform::canWriteFile(const string &path)
{
    FILE *fp = NULL;
    if(!(fp = fopen(path.c_str(), "w+"))) {
        return false;
    }

    if (0 != fclose(fp)) {
        Logger::warning(MSGID_CONFIGDSERVICE,
                        LOG_PREPIX_FORMAT "file (%s) close error",
                        LOG_PREPIX_ARGS, path.c_str());
    }
    return true;
}

string Platform::readFile(const string &filename)
{
    string file;

    if (isDirExist(filename))
        return file;
    else if (!isFileExist(filename))
        return file;
    std::ifstream stream(filename);
    file.assign((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    boost::trim(file);

    return file;
}

bool Platform::writeFile(const string &filename, const string &content)
{
    ofstream out(filename);
    out << content;
    out.close();
    return true;
}

bool Platform::copyFile(const string &from, const string &to)
{
    if (!Platform::isFileExist(from)) {
        return false;
    }
    string content = readFile(from);
    return writeFile(to, content);
}

bool Platform::deleteFile(const string &filename)
{
    if (filename.empty()) {
        return false;
    }

    if (!::remove(filename.c_str())) {
        Logger::debug(LOG_PREPIX_FORMAT "'%s' file deleted successfully.",
                      LOG_PREPIX_ARGS, filename.c_str());
        return true;
    }

    Logger::debug(LOG_PREPIX_FORMAT "Unable to delete file (%s)", LOG_PREPIX_ARGS, filename.c_str());
    return false;
}

string Platform::executeCommand(const string &command, const string firstArg, const string secondArg)
{
    string console = "";
    string newCommand = command + " " + firstArg + " " + secondArg;

    executeCommand(newCommand, console);
    return console;
}

bool Platform::executeCommand(const string &command, string &console)
{
    FILE *file;
    char buff[256];

    if (!(file = popen(command.c_str(), "r"))){
        return false;
    }

    console.clear();
    Logger::info(MSGID_CONFIGDSERVICE,
                 LOG_PREPIX_FORMAT "Execute command (%s)",
                 LOG_PREPIX_ARGS, command.c_str());
    while(fgets(buff, sizeof(buff), file)!=NULL){
        console += buff;
    }
    boost::trim(console);
    pclose(file);
    return true;
}

string Platform::concatPaths(string parent, string child)
{
    trim(parent);
    trim(child);
    if (parent.find_last_of('/') == parent.length() - 1) {
        parent.erase(parent.size() - 1, 1);
    }
    if (child.find_first_of('/') == 0) {
        child.erase(0, 1);
    }

    if (parent.empty())
        parent = "/" + child;
    else if (!child.empty())
        parent.append("/").append(child);
    return parent;
}

void Platform::extractFileName(string &fileName, string &name, string &extension)
{
    name = fileName;
    extension.clear();

    int lastIndex = fileName.find_last_of('.');
    if (lastIndex < 0)
        return;

    name = fileName.substr(0, lastIndex);
    extension = fileName.substr(lastIndex + 1);
}

void Platform::trim(string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first)
    {
        return;
    }
    size_t last = str.find_last_not_of(' ');
    str = str.substr(first, (last - first + 1));
}

string Platform::timeStr()
{
    char buffer[64];
    if (sprintf(buffer, "%u", (unsigned)time(NULL)) < 0)
        return "";
    return buffer;
}
