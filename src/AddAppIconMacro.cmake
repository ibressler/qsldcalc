# This macro is taken from kdelibs/cmake/modules/KDE4Macros.cmake.
#
# Copyright (c) 2006-2009 Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2006, 2007, Laurent Montel, <montel@kde.org>
# Copyright (c) 2007 Matthias Kretz <kretz@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file [in KDE repositories].
#
# Copyright (c) 2011, Ingo Breßler <dev@ingobressler.net>

macro(replace_str_in_file FILENAME PATTERN_STR REPLACE_STR)
    # let the documentation know about the svn revision number
    file(READ ${FILENAME} FILE_BODY)
    string(REGEX REPLACE "${PATTERN_STR}" "${REPLACE_STR}"
        FILE_BODY_NEW ${FILE_BODY})
    file(WRITE ${FILENAME} ${FILE_BODY_NEW})
endmacro(replace_str_in_file)

# adds application icon to target source list 
# for detailed documentation see the top of FindKDE4Internal.cmake
macro (ADD_APP_ICON appsources)
    set (_outfilename ${CMAKE_CURRENT_BINARY_DIR}/${appsources})

    if (WIN32)
        find_program(WINDRES_EXECUTABLE NAMES windres)
        message("WINDRES_EXECUTABLE: '${WINDRES_EXECUTABLE}'")
        if (NOT WINDRES_EXECUTABLE)
            message(STATUS "Unable to find windres utilities - application will not have an application icon!")
        endif (NOT WINDRES_EXECUTABLE)
    
        set(qsldcalc_icofile "${CMAKE_SOURCE_DIR}/res/img/qsldcalc.ico")
        set(qsldcalc_rcfile "${CMAKE_SOURCE_DIR}/res/qsldcalc.rc")
        message("exec name: '${EXEC_NAME}' '${CMAKE_EXECUTABLE_SUFFIX}'")
        # solved by relative path
        # replace_str_in_file(${qsldcalc_rcfile}
        #     "IDI_ICON[0-9] +ICON +(DISCARDABLE)? +\\\"[^\\\"]+\\\""
        #     "IDI_ICON1        ICON        \\\"${qsldcalc_icofile}\\\"")
        replace_str_in_file(${qsldcalc_rcfile}
            "\\\"InternalName\\\", \\\"[^\\\"]+\\\""
            "\\\"InternalName\\\", \\\"${EXEC_NAME}\\\"")
        replace_str_in_file(${qsldcalc_rcfile}
            "\\\"OriginalFilename\\\", \\\"[^\\\"]+\\\""
            "\\\"OriginalFilename\\\", \\\"${EXEC_NAME}${CMAKE_EXECUTABLE_SUFFIX}\\\"")
        replace_str_in_file(${qsldcalc_rcfile}
            "\\\"FileDescription\\\", \\\"[^\\\"]+\\\""
            "\\\"FileDescription\\\", \\\"${PROGRAM_NAME}\\\"")
        replace_str_in_file(${qsldcalc_rcfile}
            "\\\"ProductVersion\\\", \\\"[0-9]\\\\.[0-9]\\\""
            "\\\"ProductVersion\\\", \\\"${qsldcalc_version}\\\"")
        replace_str_in_file(${qsldcalc_rcfile}
            "\\\"FileVersion\\\", \\\"[0-9]\\\\.[0-9]\\\""
            "\\\"FileVersion\\\", \\\"${qsldcalc_version}\\\"")
        string(REPLACE "." "," qsldcalc_version_rc "${qsldcalc_version}")
        replace_str_in_file(${qsldcalc_rcfile}
            "PRODUCTVERSION +[0-9],[0-9],[0-9],[0-9]"
            "PRODUCTVERSION ${qsldcalc_version_rc},0,0")
        replace_str_in_file(${qsldcalc_rcfile}
            "FILEVERSION +[0-9],[0-9],[0-9],[0-9]"
            "FILEVERSION ${qsldcalc_version_rc},0,0")
        add_custom_command(OUTPUT ${_outfilename}_res.o
           COMMAND ${WINDRES_EXECUTABLE} ARGS -i ${qsldcalc_rcfile} -o ${_outfilename}_res.o --include-dir=${CMAKE_CURRENT_SOURCE_DIR}
               DEPENDS ${WINDRES_EXECUTABLE} ${qsldcalc_rcfile}
               WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        list(APPEND ${appsources} ${_outfilename}_res.o)
    endif(WIN32)
    if (Q_WS_MAC) # not tested yet
        # first convert image to a tiff using the Mac OS X "sips" utility,
        # then use tiff2icns to convert to an icon
        find_program(SIPS_EXECUTABLE NAMES sips)
        find_program(TIFF2ICNS_EXECUTABLE NAMES tiff2icns)
        if (SIPS_EXECUTABLE AND TIFF2ICNS_EXECUTABLE)
            file(GLOB_RECURSE files  "${pattern}")
            # we can only test for the 128-icon like that - we don't use patterns anymore
            foreach (it ${files})
                if (it MATCHES ".*128.*" )
                    set (_icon ${it})
                endif (it MATCHES ".*128.*")
            endforeach (it)

            if (_icon)
                
                # first, get the basename of our app icon
                add_custom_command(OUTPUT ${_outfilename}.icns ${outfilename}.tiff
                                   COMMAND ${SIPS_EXECUTABLE} -s format tiff ${_icon} --out ${outfilename}.tiff
                                   COMMAND ${TIFF2ICNS_EXECUTABLE} ${outfilename}.tiff ${_outfilename}.icns
                                   DEPENDS ${_icon}
                                   )

                # This will register the icon into the bundle
                set(MACOSX_BUNDLE_ICON_FILE ${appsources}.icns)

                # Append the icns file to the sources list so it will be a dependency to the
                # main target
                list(APPEND ${appsources} ${_outfilename}.icns)

                # Install the icon into the Resources dir in the bundle
                set_source_files_properties(${_outfilename}.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

            else(_icon)
                # TODO - try to scale a non-128 icon...? Try to convert an SVG on the fly?
                message(STATUS "Unable to find an 128x128 icon that matches pattern ${pattern} for variable ${appsources} - application will not have an application icon!")
            endif(_icon)

        else(SIPS_EXECUTABLE AND TIFF2ICNS_EXECUTABLE)
            message(STATUS "Unable to find the sips and tiff2icns utilities - application will not have an application icon!")
        endif(SIPS_EXECUTABLE AND TIFF2ICNS_EXECUTABLE)
    endif(Q_WS_MAC)
endmacro (ADD_APP_ICON)

# vim: set expandtab ts=4 sw=4 tw=0:
