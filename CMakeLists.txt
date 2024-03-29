# CMakeLists.txt
#
# Copyright (c) 2010-2011 Ingo Bressler (qsldcalc at ingobressler.net)
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

project(qsldcalc)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${qsldcalc_SOURCE_DIR}/bin)

### <adjust here> ###

# libcfp library related settings
set(LIBCFP_PATH ${qsldcalc_SOURCE_DIR}/src/libcfp)
find_path(libcfp_INCLUDE_DIR cfp/cfp.h
	PATHS
	${LIBCFP_PATH}/include
)
find_library(libcfp_LIBRARY libcfp_static${CMAKE_STATIC_LIBRARY_SUFFIX}
	PATHS
	${LIBCFP_PATH}/lib
)

set(qsldcalc_version 0.6)

### </adjust here> ###

find_package(Qt4 REQUIRED)

# tell cmake to process CMakeLists.txt in that subdirectory
add_subdirectory(src)

