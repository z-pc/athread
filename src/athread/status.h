//////////////////////////////////////////////////////////////////////////
// Copyright 2024 Le Xuan Tuan Anh
//
// https://github.com/z-pc/athread
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

#ifndef STATUS_H__
#define STATUS_H__

namespace at
{

enum class WaitStatus
{
    Ready,        ///< Wait operation completed successfully.
    Timeout,      ///< Wait operation timed out.
    Interrupted,  ///< Wait operation was interrupted.
};

enum class StopReason
{
    None,              ///< Not stopped yet / unknown.
    Completed,         ///< Completed all scheduled work.
    StoppedByRequest,  ///< Stopped by terminate()/external stop request.
    LimitReached,      ///< Stopped by max-times/limit policy.
    Timeout,           ///< Stopped by timeout policy.
    Error,             ///< Stopped due to runtime error.
};

}  // namespace at

#endif  // STATUS_H__