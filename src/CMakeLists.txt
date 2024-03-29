# src/CMakeLists.txt
#
# Copyright (c) 2010-2011, Ingo Bressler (qsldcalc at ingobressler.net)
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


# the next line sets up include and link directories and defines some
# variables that we will use.
# you can modify the behavior by setting some variables, e.g.
# -> this will cause cmake to include and link against the OpenGL module
#   set(QT_USE_OPENGL TRUE)
####
# btw, note, lupdate call in src path:
# lupdate-qt4 *.h *.cpp *.ui -ts ../res/lang/language_de_DE.ts
####

include(${QT_USE_FILE}) # includes qt4 cmake module
include(AddAppIconMacro.cmake) # script to set Windows icons&info

# compiler related settings
set(CXX_FLAGS "-Wall")
set(CMAKE_EXE_LINKER_FLAGS "-static")
if(WIN32)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
endif(WIN32)
if(DEBUG)
	set(CMAKE_BUILD_TYPE Debug)
	add_definitions(-DDEBUG)
	set(CMAKE_CXX_FLAGS_DEBUG "${CXX_FLAGS} -g")
else(DEBUG)
	set(CMAKE_BUILD_TYPE Release)
	#set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mfpmath=sse -msse -m3dnow -msse2 -msse3")
	set(CMAKE_CXX_FLAGS_RELEASE "${CXX_FLAGS} -O2")
endif(DEBUG)
message(STATUS ":: Configuring for >${CMAKE_BUILD_TYPE}< build")

# determine build date, expects unix ENV (mingw on win)
EXECUTE_PROCESS(COMMAND "date" "+%Y;%_m;%_d;" OUTPUT_VARIABLE DATE_LIST)
list(GET DATE_LIST 0 BUILD_YEAR)
list(GET DATE_LIST 1 BUILD_MONTH)
list(GET DATE_LIST 2 BUILD_DAY)
message(STATUS "Build Date: ${BUILD_YEAR} ${BUILD_MONTH} ${BUILD_DAY}")
add_definitions(-D'BUILD_YEAR=${BUILD_YEAR}' -D'BUILD_MONTH=${BUILD_MONTH}' -D'BUILD_DAY=${BUILD_DAY}')

# the variable "qsldcalc_SRCS" contains all .cpp files of this project
set(qsldcalc_SRC
	main.cpp
	mainwindow.cpp
	element.cpp
	elementdatabase.cpp
	xmlparser.cpp
	inputdata.cpp
	formulacompleter.cpp
	utils.cpp
	datavisualizer.cpp
	aliasnamedialog.cpp
)

set(qsldcalc_MOC_HDR
	mainwindow.h
	formulacompleter.h
	datavisualizer.h
	aliasnamedialog.h
)

set(qsldcalc_UI
	mainwindow.ui
	datavisualizer.ui
	aliasnamedialog.ui
)

# for all language files
foreach(lang de_DE en_GB)
	set(filename "${qsldcalc_SOURCE_DIR}/res/lang/language_${lang}.ts")
	list(APPEND qsldcalc_LANG_TS ${filename})
endforeach(lang)

set(BIN_DIR_OLD ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CURRENT_BINARY_DIR "${qsldcalc_SOURCE_DIR}/res/lang")
QT4_ADD_TRANSLATION(qsldcalc_LANG_QM ${qsldcalc_LANG_TS})
set(CMAKE_CURRENT_BINARY_DIR ${BIN_DIR_OLD})

QT4_ADD_RESOURCES(qsldcalc_RES_CXX
	"${qsldcalc_SOURCE_DIR}/res/qsldcalc.qrc"
	OPTIONS -no-compress
)

QT4_WRAP_UI(qsldcalc_UI_H ${qsldcalc_UI})

# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${qsldcalc_SOURCE_DIR}/src/
	${libcfp_INCLUDE_DIR}
)

QT4_WRAP_CPP(qsldcalc_MOC_SRC ${qsldcalc_MOC_HDR})

# tell cmake to create .moc files for all files in the variable qsldcalc_SRCS
# that require such a file.
# note: this assumes that you use #include "header.moc" in your files
qt4_automoc(${qsldcalc_SRC})

#message("CMAKE_SYSTEM: ${CMAKE_SYSTEM}")
#message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
#message("CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
#message("CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")
#message("CMAKE_SIZEOF_VOID_P: ${CMAKE_SIZEOF_VOID_P}")
math(EXPR NUM_BITS "${CMAKE_SIZEOF_VOID_P}*8")
#message("bits: ${NUM_BITS}")
set(EXEC_NAME "qsldcalc-${qsldcalc_version}_${CMAKE_SYSTEM_NAME}${NUM_BITS}")

 #
## Version number propagation code
 #

set(PROGRAM_NAME "SLD Calculator ${qsldcalc_version}")
add_definitions(-DPROGRAM_NAME="${PROGRAM_NAME}")

# let the documentation/doxygen know about the svn revision number
replace_str_in_file(${qsldcalc_SOURCE_DIR}/src/Doxyfile
	"PROJECT_NUMBER         = [a-z]?[0-9]+\\\\.?[0-9]*\\\\.?([a-z]|[0-9])*\\\\.?([a-z]|[0-9])*"
	"PROJECT_NUMBER         = ${qsldcalc_version}"
)

# windows icon&description
set(qsldcalc_icons "")
# qsldcalc_SOURCE_DIR
add_app_icon(qsldcalc_icons)

# create an executable file named "qsldcalc" from the source files in the
# variable "qsldcalc_SRC".
add_executable(${EXEC_NAME}
	${qsldcalc_LANG_QM}
	${qsldcalc_RES_CXX}
	${qsldcalc_SRC} 
	${qsldcalc_MOC_SRC} 
	${qsldcalc_UI_H}
	${qsldcalc_icons}
)

# link the "qsldcalc" target against the Qt libraries. which libraries
# exactly, is defined by the "include(${QT_USE_FILE})" line above, which sets
# up this variable.
#message("QT_LIBRARIES: ${QT_LIBRARIES}")
target_link_libraries(${EXEC_NAME}
	${QT_LIBRARIES}
	${libcfp_LIBRARY}
)

