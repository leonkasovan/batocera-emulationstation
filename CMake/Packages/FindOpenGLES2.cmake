# FindOpenGLES
# ------------
# Finds the OpenGLES2 library
#
# This will define the following variables::
#
# OPENGLES2_FOUND - system has OpenGLES
# OPENGLES2_INCLUDE_DIRS - the OpenGLES include directory
# OPENGLES2_LIBRARIES - the OpenGLES libraries

if(NOT HINT_GLES_LIBNAME)
 set(HINT_GLES_LIBNAME GLESv2)
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(OPENGLES2 glesv2)
endif()

if(OPENGLES2_FOUND)
    message("OPENGLES2 FOUND")
    #set(OPENGLES2_INCLUDE_DIR ${OPENGLES2_INCLUDE_DIRS})
    set(OPENGLES2_INCLUDE_DIR "/usr/include")
    set(OPENGLES2_gl_LIBRARY ${OPENGLES2_LIBRARIES})
else()
message("OPENGLES2 NOT FOUND")
    find_path(OPENGLES2_INCLUDE_DIR GLES2/gl2.h
        PATHS "${CMAKE_FIND_ROOT_PATH}/usr/include"
        HINTS ${HINT_GLES_INCDIR}
    )

    find_library(OPENGLES2_gl_LIBRARY
        NAMES ${HINT_GLES_LIBNAME}
        HINTS ${HINT_GLES_LIBDIR}
    )
endif()

include(FindPackageHandleStandardArgs)
message("VDEBUG.1: ${OPENGLES2_gl_LIBRARY}")
message("VDEBUG.2: ${OPENGLES2_INCLUDE_DIR}")
find_package_handle_standard_args(OpenGLES2
            REQUIRED_VARS OPENGLES2_gl_LIBRARY OPENGLES2_INCLUDE_DIR)


if(OPENGLES2_FOUND)
    set(OPENGLES2_LIBRARIES ${OPENGLES2_gl_LIBRARY})
    set(OPENGLES2_INCLUDE_DIRS ${OPENGLES2_INCLUDE_DIR})
    mark_as_advanced(OPENGLES2_INCLUDE_DIR OPENGLES2_gl_LIBRARY)
endif()

