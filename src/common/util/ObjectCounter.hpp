// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef _OBJECT_COUNTER_H_
#define _OBJECT_COUNTER_H_

template<typename T>
class ObjectCounter {
public:
    ObjectCounter()
    {
        ++s_count;
    }

    ObjectCounter(const ObjectCounter&)
    {
        ++s_count;
    }

    ~ObjectCounter()
    {
        --s_count;
    }

    static size_t getIntanceCount()
    {
        return s_count;
    }

private:
    static size_t s_count;
};

template<typename T>
size_t ObjectCounter<T>::s_count = 0;

#endif /* _OBJECT_COUNTER_H_ */
