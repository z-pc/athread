# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

########################################
# FUNCTION project_group
########################################
function(project_group target name)
    set_target_properties(${target} PROPERTIES FOLDER ${name})
endfunction(project_group)

# ##############################################################################
# MACRO target_install_lib
# ##############################################################################
macro(target_install_lib target)
  install(
    TARGETS ${target}
    CONFIGURATIONS Debug
    RUNTIME DESTINATION bin/Debug
    LIBRARY DESTINATION lib/Debug
    ARCHIVE DESTINATION lib/Debug)

  install(
    TARGETS ${target}
    CONFIGURATIONS Release
    RUNTIME DESTINATION bin/Release
    LIBRARY DESTINATION lib/Release
    ARCHIVE DESTINATION lib/Release)
endmacro()

# ##############################################################################
# MACRO target_install_dir
# ##############################################################################
macro(target_install_dir target)
  install(
    DIRECTORY ${target}
    DESTINATION include
    FILES_MATCHING
    PATTERN "*.h")
endmacro()

# ##############################################################################
# MACRO athread_target_properties
# this macro will set default properties for all lib.
# ##############################################################################
function(athread_lib_target_properties target)
    set_target_properties(${target} PROPERTIES DEFINE_SYMBOL athread_EXPORT)
endfunction()

# ##############################################################################
# FUNCTION athread_add_library
# ##############################################################################
function(athread_add_library name)
  add_library(${name} ${ARGN})
  athread_lib_target_properties(${name})
  target_include_directories(${name} PUBLIC ${PROJECT_SOURCE_DIR}/src)
endfunction()

