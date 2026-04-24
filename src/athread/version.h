//////////////////////////////////////////////////////////////////////////
// Copyright 2026 Le Xuan Tuan Anh
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

#ifndef ATHREAD_VERSION_H__
#define ATHREAD_VERSION_H__

#include <cstdint>

#define ATHREAD_VERSION_MAJOR 1
#define ATHREAD_VERSION_MINOR 0
#define ATHREAD_VERSION_PATCH 1

#define ATHREAD_VERSION_ENCODE(major, minor, patch) \
    ((static_cast<std::uint32_t>(major) * 10000U) + (static_cast<std::uint32_t>(minor) * 100U) + static_cast<std::uint32_t>(patch))

#define ATHREAD_VERSION ATHREAD_VERSION_ENCODE(ATHREAD_VERSION_MAJOR, ATHREAD_VERSION_MINOR, ATHREAD_VERSION_PATCH)
#define ATHREAD_VERSION_STRING "1.0.1"

namespace at
{
namespace version
{
constexpr std::uint32_t make(std::uint32_t major, std::uint32_t minor, std::uint32_t patch) noexcept
{
    return (major * 10000U) + (minor * 100U) + patch;
}

constexpr std::uint32_t value() noexcept { return ATHREAD_VERSION; }

constexpr std::uint32_t major() noexcept { return ATHREAD_VERSION_MAJOR; }

constexpr std::uint32_t minor() noexcept { return ATHREAD_VERSION_MINOR; }

constexpr std::uint32_t patch() noexcept { return ATHREAD_VERSION_PATCH; }

constexpr const char* string() noexcept { return ATHREAD_VERSION_STRING; }

constexpr bool at_least(std::uint32_t major, std::uint32_t minor, std::uint32_t patch) noexcept
{
    return value() >= make(major, minor, patch);
}

}  // namespace version
}  // namespace at

#endif  // ATHREAD_VERSION_H__