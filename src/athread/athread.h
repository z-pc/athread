//////////////////////////////////////////////////////////////////////////
// Copyright 2024 Le Xuan Tuan Anh
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//////////////////////////////////////////////////////////////////////////

#ifndef ATHREAD_H__
#define ATHREAD_H__

#include <iostream>
#include <sstream>

#include "executor.h"
#include "node.h"
#include "runnable.h"
#include "task.h"
#include "threadgraph.h"
#include "threadpool.h"

namespace at
{
extern std::mutex at_console_mutex;

enum class WaitStatus
{
    Ready,        ///< Wait operation completed successfully.
    Timeout,      ///< Wait operation timed out.
    Interrupted,  ///< Wait operation was interrupted.
};
}  // namespace at

#define AT_COUT(text)                                     \
    {                                                     \
        std::lock_guard<std::mutex> lk(at_console_mutex); \
        std::cout << text;                                \
    }

#ifdef AT_TRACKING
#define AT_LOG(text) AT_COUT(">" << text << std::endl)
#else
#define AT_LOG(text)
#endif

#ifdef DEBUG
#define AT_EXCEPTION(text, _exception)                         \
    {                                                          \
        std::ostringstream oss;                                \
        oss << __FUNCTION__ << ":" << __LINE__ << " " << text; \
        throw _exception(oss.str().c_str());                   \
    }
#else
#define AT_EXCEPTION(text, _exception)       \
    {                                        \
        std::ostringstream oss;              \
        oss << text;                         \
        throw _exception(oss.str().c_str()); \
    }
#endif

// exception
#define AT_ERROR(text)            AT_EXCEPTION(text, std::exception)
#define AT_INVALID_ARGUMENT(text) AT_EXCEPTION(text, std::invalid_argument)
#define AT_RUNTIME_ERROR(text)    AT_EXCEPTION(text, std::runtime_error)

#endif  // ATHREAD_H__
