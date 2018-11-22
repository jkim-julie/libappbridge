#
# Copyright (c) 2018 Igalia S.L.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

include(FindPkgConfig)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(ILMCONTROL REQUIRED ilmControl)

set(TARGET_LIBRE appbridge)

add_library(${TARGET_LIBRE} SHARED
   libappbridge.cpp
   libappbridge.h
)

target_include_directories(${TARGET_LIBRE}
  PRIVATE
    "${ILMCONTROL_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
    "${GIO_INCLUDE_DIRS}"
)

target_link_libraries(${TARGET_LIBRE}
  PUBLIC
    windowmanager
    homescreen
    ${ILMCONTROL_LIBRARIES}
    afbwsc
    json-c
    pthread
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
)

if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release")
   target_compile_definitions(${TARGET_LIBRE}
           PRIVATE
           _GLIBCXX_DEBUG)
endif()

target_compile_options(${TARGET_LIBRE}
    PRIVATE
        -Wall -Wextra -Wno-unused-parameter -Wno-comment)

set_target_properties(${TARGET_LIBRE}
    PROPERTIES
    # INTERPROCEDURAL_OPTIMIZATION ON
        CXX_EXTENSIONS OFF
        CXX_STANDARD 14
        VERSION ${PACKAGE_VERSION}
        CXX_STANDARD_REQUIRED ON)

if (LINK_LIBCXX)
   set_target_properties(${TARGET_LIBRE}
           PROPERTIES
           LINK_FLAGS "-lc++")
endif()

if (NOT ${SANITIZER_MODE} STREQUAL "none" AND NOT ${SANITIZER_MODE} STREQUAL "")
   target_compile_options(${TARGET_LIBRE}
      PRIVATE
         -fsanitize=${SANITIZER_MODE} -g -fno-omit-frame-pointer)
   set_target_properties(${TARGET_LIBRE}
      PROPERTIES
         LINK_FLAGS "-fsanitize=${SANITIZER_MODE} -g")
endif()

install(
   TARGETS ${TARGET_LIBRE}
   DESTINATION ${CMAKE_INSTALL_LIBDIR}
   COMPONENT "runtime")

install(
   FILES libappbridge.h
   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
   COMPONENT "development")
