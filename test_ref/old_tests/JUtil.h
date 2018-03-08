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

#ifndef JUTIL_H
#define JUTIL_H

#include <pbnjson.h>
#include <pbnjson.hpp>
#include <string>
#include <map>

//! List of utilites for pbnjson
class JUtil
{
public:

    //! Parse json from null-terminated string
    static jvalue_ref parse_json_c(const char *rawData);

    /*! Parse given json data using schema.
     * If schemaName is empty, use JSchemaFragment("{}")
     */
    static pbnjson::JValue parse(const char *rawData);

    //! Convert json object to std::string
    static std::string jsonToString(pbnjson::JValue json);

    //! Error class used in JUtil
    class Error
    {
    public:
        typedef enum
        {
            None = 0,
            File_Io,
            Schema,
            Parse
        } ErrorCode;

        //! Constructor
        Error();

        //! Return ErrorCode
        ErrorCode code();

        //! Return Error Detail string
        std::string detail();

    private:
        friend class JUtil;

        /*! Set Error code and detail string.
         * If detail value is NULL, detail value set as default error message.
         */
        void set(ErrorCode code, const char *detail = NULL);

    private:
        ErrorCode m_code;
        std::string m_detail;
    };
};
#endif /* JUTIL_H */

