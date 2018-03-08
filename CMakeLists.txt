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

project(configd)

include(webOS/webOS)

# modules
webos_modules_init(1 0 0 QUALIFIER RC2)
webos_component(1 2 0)

# headers
webos_machine_dep()
webos_distro_variant_dep()
webos_configure_header_files(${CMAKE_CURRENT_SOURCE_DIR}/include/private)

# bus
webos_build_system_bus_files()

# schema
set(SCHEMAS
    files/schema/ConfigdService.featureList.schema
    files/schema/ConfigdService.getConfigs.schema
    files/schema/ConfigdService.jsonProcess.schema
    files/schema/ConfigdService.layersConfig.schema
    files/schema/ConfigdService.reconfigure.schema
    files/schema/ConfigdService.setConfigs.schema
)
install(FILES ${SCHEMAS} DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/schemas/configd)

add_subdirectory(src/configd)
add_subdirectory(src/configd-tool)
add_subdirectory(src/libconfigd)

#TODO: Currently, configd has two test frameworks
#webos_use_gtest()
#local_webos_add_test(${EXE_NAME} ${LIBS})
# They should be merged into one framework after code refactoring
#function(local_webos_add_test target_name app_dep)
#    get_property(SOURCES TARGET ${target_name} PROPERTY "SOURCES")
#    set(TST_SRC)
#    foreach(SRC ${SOURCES})
#        if(NOT ${SRC} MATCHES "Main.c")
#            set(TST_SRC ${TST_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/${SRC})
#        endif()
#    endforeach()
#
#    set (TST_APP_NAME ${target_name}_test)
#    set (TST_DEP ${app_dep} ${ARGN} ${GTEST_LIBRARIES} )
#    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
#    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
#    add_subdirectory(src/configservice/tests)
#endfunction()

#TODO: change to ptest package.
#if (WEBOS_CONFIG_BUILD_TESTS)
# Uncomment below three lines to build test clients
#    add_subdirectory(tests)
#    webos_build_system_bus_files(tests/files/sysbus)
#    webos_build_daemon(LAUNCH tests/files/launch)
#else()
#    message(STATUS "configd: skipping test clients")
#endif()
