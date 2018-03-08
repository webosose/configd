Summary
-------
configd service provides simple get/set interfaces to access the configurations.

Description
-----------

When webOS platform is built and it reaches to the customer,
it goes through various configs. For example, configs based on country,
provider, language and etc.

Making and maintaining separate builds for each of the variants is not
practical.

At the moment, there are various non-uniform ways to customize the behavior
of various components.

Some depend on json config files stored on the root file system and
some depend on the file system flags. Such disparity makes it impossible
to provide the mechanism or tools to customize the product as a whole.

By specifying a common way to represent various flags and features,
config service can provide a streamlined mechanism to our customers to
customize the product.

Dependencies
---------------------
Below are the tools and libraries required to build configd:

* cmake 
* gcc
* glib-2.0
* make
* pkg-config
* Boost library (regex)
* webosose/cmake-modules-webos
* webosose/pmloglib
* webosose/libpbnjson
* webosose/luna-service2

Copyright and License Information
=================================
Unless otherwise specified, all content, including all source code files and documentation files in this repository are:

Copyright (c) 2014-2018 LG Electronics, Inc.

Unless otherwise specified or set forth in the NOTICE file, all content, including all source code files and documentation files
in this repository are: Licensed under the Apache License, Version 2.0 (the "License"); you may not use this content except in
compliance with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS"
BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.

SPDX-License-Identifier: Apache-2.0
