Summary
=======

This directory contains unit tests for configd

Files
=====

configd is written by C but unit-test is written by C++ to utilize gtest framework.
Most files are copied As-Is or with minimal modification from bootd

## Unit Test Files
  These files have name of "[TestTargetFileName]"+"Test.cpp"
   src/UtilsTest.cpp

## Test Main
   src/Main.cpp

## Helper Classes
  These files are copied from bootd. It is slightly modifed.
   src/FakeLSMessage.cpp
   src/FakeLSMessage.h
   src/FakeLunaServiceProvider.cpp
   src/FakeLunaServiceProvider.h
   src/JUtil.cpp
   src/JUtil.h
   src/Singleton.cpp
   src/Singleton.hpp
   src/TestGlobal.cpp
   src/TestGlobal.h
   src/TestTemplates.cpp
   src/TestTemplates.h
   src/TestUtil.cpp
   src/TestUtil.h

Copyright and License Information
=================================
Unless otherwise specified, all content, including all source code files and documentation files in this repository are:

Copyright (c) 2018 LG Electronics, Inc.

Unless otherwise specified or set forth in the NOTICE file, all content, including all source code files and documentation files
in this repository are: Licensed under the Apache License, Version 2.0 (the "License"); you may not use this content except in
compliance with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS"
BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.

SPDX-License-Identifier: Apache-2.0
