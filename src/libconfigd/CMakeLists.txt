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

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

# Environment
set(LIB_NAME libconfigd)
set(SOURCE_FILES libconfigd.c)

# Compile
webos_add_compiler_flags(ALL C -std=gnu99)
include_directories(${PROJECT_SOURCE_DIR}/include/private)
include_directories(${PROJECT_SOURCE_DIR}/include/public)
add_library(${LIB_NAME} SHARED ${SOURCE_FILES})

# Link
set(LIBS
    ${GLIB2_LDFLAGS})
target_link_libraries(${LIB_NAME} ${LIBS})

# Install
webos_build_pkgconfig(files/pkgconfig/libconfigd)
webos_build_library(TARGET ${LIB_NAME} HEADERS include/public)
