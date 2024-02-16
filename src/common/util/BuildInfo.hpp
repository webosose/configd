// Copyright (c) 2024 LG Electronics, Inc.
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

#include <fstream>
#include <sstream>
#include <unordered_map>

class BuildInfo {
public:
    BuildInfo() {
        std::ifstream ifs("/etc/buildinfo");
        if (ifs.is_open()) {
            std::string line;
            while (std::getline(ifs, line)) {
                std::istringstream iss(line);
                std::string key, equals, value;
                if (iss >> key >> equals >> value) {
                    if (equals == "=") {
                        key = trim(key);
                        value = trim(value);
                        keyValueMap_[key] = std::move(value);
                    }
                }
            }
            ifs.close();
        }
    }

    std::string get(const std::string& key) const {
        auto it = keyValueMap_.find(key);
        return (it != keyValueMap_.end()) ? it->second : "";
    }

private:
    std::string trim(const std::string& str) const {
        size_t first = str.find_first_not_of(' ');
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }

    std::unordered_map<std::string, std::string> keyValueMap_;
};
