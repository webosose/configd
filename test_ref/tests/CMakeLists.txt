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

project(configd-luna-test C)

include_directories(src ../include/public)


set (SRC_CLIENT_BASE
    src/configd_client_base.c)

set (SRC_CLIENT_LIBCONFIGD
    src/configd_client_libconfigd.c)

add_executable(configd-client-base  ${SRC_CLIENT_BASE})
add_executable(configd-client-libconfigd  ${SRC_CLIENT_LIBCONFIGD})
target_link_libraries(configd-client-base ${LIBS})
target_link_libraries(configd-client-libconfigd ${LIBS} libconfigd)

install(TARGETS configd-client-base  DESTINATION ${WEBOS_INSTALL_BINDIR})
install(TARGETS configd-client-libconfigd  DESTINATION ${WEBOS_INSTALL_BINDIR})
