# CMakeLists.txt
#
# Copyright (c) 2009 Technische Universität Berlin, 
# Stranski-Laboratory for Physical und Theoretical Chemistry
#
# This file is part of qSLDcalc.
#
# qSLDcalc is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# qSLDcalc is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with qSLDcalc. If not, see <http://www.gnu.org/licenses/>.

# Author(s) of this file:
# Ingo Bressler (ingo@cs.tu-berlin.de)

project(qsldcalc)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${qsldcalc_SOURCE_DIR}/bin)

### .<adjust here start>. ###
### v                   v ###

# libcfp library related settings
find_path(libcfp_INCLUDE_DIR cfp/cfp.h
	PATHS
	${qsldcalc_SOURCE_DIR}/../libcfp/include
	${qsldcalc_SOURCE_DIR}/../libcfp/trunk/include
	${qsldcalc_SOURCE_DIR}/../../libcfp/include
	${qsldcalc_SOURCE_DIR}/../../libcfp/trunk/include
)
find_library(libcfp_LIBRARY libcfp_static${CMAKE_STATIC_LIBRARY_SUFFIX}
	PATHS
	${qsldcalc_SOURCE_DIR}/../libcfp/lib/
	${qsldcalc_SOURCE_DIR}/../libcfp/trunk/lib/
	${qsldcalc_SOURCE_DIR}/../../libcfp/lib/
	${qsldcalc_SOURCE_DIR}/../../libcfp/trunk/lib/
)

# compiler related settings
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_DEBUG -g)
#set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mfpmath=sse -msse -m3dnow -msse2 -msse3")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
#message("CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
#set(CMAKE_EXE_LINKER_FLAGS "-static")
message(STATUS ":: Configuring for >${CMAKE_BUILD_TYPE}< build")

set(qsldcalc_version 0.5)

### ^                  ^ ###
### `<adjust here stop>´ ###

find_package(Qt4 REQUIRED)

# tell cmake to process CMakeLists.txt in that subdirectory
add_subdirectory(src)

