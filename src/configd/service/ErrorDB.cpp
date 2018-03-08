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

#include "ErrorDB.h"

const char *ErrorDB::ERRORTEXT_UNKNOWN = "Unknown error";
const char *ErrorDB::ERRORTEXT_INVALID_PARAMETER = "Invalid parameter error";
const char *ErrorDB::ERRORTEXT_INVALID_MAINDB = "Invalid main database error";
const char *ErrorDB::ERRORTEXT_JSON_PARSING = "Json parsing error";
const char *ErrorDB::ERRORTEXT_RESPONSE = "Response error";

const int ErrorDB::ERRORCODE_NOERROR = 0;
const int ErrorDB::ERRORCODE_UNKNOWN = -1;
const int ErrorDB::ERRORCODE_INVALID_PARAMETER = -2;
const int ErrorDB::ERRORCODE_INVALID_MAINDB = -3;
const int ErrorDB::ERRORCODE_JSON_PARSING = -4;
const int ErrorDB::ERRORCODE_RESPONSE = -5;

const char* ErrorDB::getErrorText(int errorCode)
{
    switch(errorCode) {
    case ERRORCODE_UNKNOWN:
        return ERRORTEXT_UNKNOWN;

    case ERRORCODE_INVALID_PARAMETER:
        return ERRORTEXT_INVALID_PARAMETER;

    case ERRORCODE_INVALID_MAINDB:
        return ERRORTEXT_INVALID_MAINDB;

    case ERRORCODE_JSON_PARSING:
        return ERRORTEXT_JSON_PARSING;

    case ERRORCODE_RESPONSE:
        return ERRORTEXT_RESPONSE;
    }
    return ERRORTEXT_UNKNOWN;
}

const int ErrorDB::getErrorCode(const char *errorText)
{
    return ERRORCODE_NOERROR;
}
