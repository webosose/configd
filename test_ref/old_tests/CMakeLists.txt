# Copyright (c) 2014-2018 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8.7)
project(${TST_APP_NAME} C CXX)

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CPP_CFLAGS_OTHER})

set(TST_LIBS ${PBNJSON_CPP_LDFLAGS})

webos_configure_header_files(src/configservice/tests)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} TST_SRC_LOCAL)
add_executable(${TST_APP_NAME} ${TST_SRC_LOCAL} ${TST_SRC})
target_link_libraries(${TST_APP_NAME} ${TST_DEP} -lpthread ${WEBOS_GTEST_LIBRARIES} ${TST_LIBS})

install(TARGETS ${PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_LIBEXEC}/tests/
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)
install(DIRECTORY data DESTINATION ${WEBOS_INSTALL_LIBEXEC}/tests)
