// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#include "LS2Comparator.h"

#include <fstream>

#include "util/Platform.h"

JValue LS2Comparator::convertFileToJValue(const string &filename)
{
    if (!Platform::isFileExist(filename.c_str()))
        return nullptr;

    JValue array = pbnjson::Array();
    ifstream in(filename);
    string line;
    while (std::getline(in, line)) {
        JValue message = JDomParser::fromString(line);
        if (message.isNull()) // Invalid File format
            return nullptr;
        array.append(message);
    }
    in.close();
    return array;
}

LS2Comparator::LS2Comparator()
{

}

LS2Comparator::LS2Comparator(const string& a, const string& b)
{
    m_jsonA = convertFileToJValue(a);
    m_jsonB = convertFileToJValue(b);

    if (m_jsonA.isNull() || m_jsonB.isNull())
        return;

    m_filenameA = a;
    m_filenameB = b;
}

LS2Comparator::~LS2Comparator()
{

}

bool LS2Comparator::isEqual()
{
    return true;
}

bool LS2Comparator::isLoaded(const string& filename)
{
    if (filename == m_filenameA || filename == m_filenameB)
        return true;
    return false;
}
