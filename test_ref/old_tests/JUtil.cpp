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

#include "JUtil.h"
#include <fstream>

class DefaultResolver: public pbnjson::JResolver
{
public:
    pbnjson::JSchema resolve(const ResolutionRequest &request, JSchemaResolutionResult &result)
    {
        pbnjson::JSchema resolved = pbnjson::JSchema::AllSchema();

        if (!resolved.isInitialized())
        {
            result = SCHEMA_IO_ERROR;
            return pbnjson::JSchema::NullSchema();
        }

        result = SCHEMA_RESOLVED;
        return resolved;
    }
};


jvalue_ref JUtil::parse_json_c(const char *rawData)
{
    JSchemaInfo schemaInfo;

    jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
    return jdom_parse(j_cstr_to_buffer(rawData), DOMOPT_NOOPT, &schemaInfo);
}

pbnjson::JValue JUtil::parse(const char *rawData)
{
    DefaultResolver resolver;

    pbnjson::JSchema schema = pbnjson::JSchema::AllSchema();
    pbnjson::JDomParser parser(&resolver);

    if (!parser.parse(rawData, schema))
    {
        return pbnjson::JValue();
    }

    return parser.getDom();
}

std::string JUtil::jsonToString(pbnjson::JValue json)
{
    return pbnjson::JGenerator::serialize(json, pbnjson::JSchemaFragment("{}"));
}

JUtil::Error::Error()
    : m_code(Error::None)
{
}

JUtil::Error::ErrorCode JUtil::Error::code()
{
    return m_code;
}

std::string JUtil::Error::detail()
{
    return m_detail;
}

void JUtil::Error::set(ErrorCode code, const char *detail)
{
    m_code = code;

    if (!detail)
    {
        switch (m_code)
        {
            case Error::None:
                m_detail = "Success";
                break;

            case Error::File_Io:
                m_detail = "Fail to read file";
                break;

            case Error::Schema:
                m_detail = "Fail to read schema";
                break;

            case Error::Parse:
                m_detail = "Fail to parse json";
                break;

            default:
                m_detail = "Unknown error";
                break;
        }
    }
    else
    {
        m_detail = detail;
    }
}

