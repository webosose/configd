// Copyright (c) 2014-2024 LG Electronics, Inc.
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

#ifndef _ENVIRONMENTS_H_IN_
#define _ENVIRONMENTS_H_IN_

#define INSTALL_SYSMGR_LOCALSTATEDIR     "tests/_data/layers"

#define INSTALL_LOCALSTATEDIR     "tests/_data/jsondb"

#define INSTALL_WEBOS_SYSCONFDIR    ""

#define INSTALL_SYSCONFDIR        "/etc"

#define INSTALL_RUNTIMEINFODIR    "/var/run"

#define GETCONFIGS_SCHEMA         "files/schema/ConfigdService.getConfigs.schema"

#define JSONPROCESS_SCHEMA        "files/schema/ConfigdService.jsonProcess.schema"

#define SETCONFIGS_SCHEMA         "files/schema/ConfigdService.setConfigs.schema"

#define RECONFIGS_SCHEMA          "files/schema/ConfigdService.reconfigure.schema"

#define CONFIGLAYERS_SCHEMA       "files/schema/ConfigdService.layersConfig.schema"

#define CONFIGFEATUESLIST_SCHEMA  "files/schema/ConfigdService.featureList.schema"

#define PATH_TEST_ROOT            "tests"

#define PATH_DATA                 "tests/_data"

#define PATH_OUTPUT               "tests/output"

#endif // _ENVIRONMENTS_H_IN_

