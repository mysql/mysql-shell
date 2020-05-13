# Copyright (c) 2015, 2020, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

##############################################################################
#
#  Packaging
#
##############################################################################

# The rest is mainly about CPack
if(NOT EXTRA_NAME_SUFFIX)
  set(EXTRA_NAME_SUFFIX "")
endif()
if(NOT EXTRA_NAME_SUFFIX2)
  set(EXTRA_NAME_SUFFIX2 "")
endif()

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MySQL Shell ${MYSH_VERSION}, a command line shell and scripting environment for MySQL Hybrid")
set(CPACK_PACKAGE_NAME                "mysql-shell${EXTRA_NAME_SUFFIX}")
set(CPACK_PACKAGE_VENDOR              "Oracle and/or its affiliates")
set(CPACK_PACKAGE_DESCRIPTION_FILE    "${CMAKE_SOURCE_DIR}/README")
if(WIN32)
  # WiX wants the license file to end in ".txt"
  configure_file(${CMAKE_SOURCE_DIR}/LICENSE
                 ${CMAKE_BINARY_DIR}/LICENSE.txt COPYONLY)
  set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_BINARY_DIR}/LICENSE.txt")
else()
  set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_SOURCE_DIR}/LICENSE")
endif()
set(CPACK_SOURCE_PACKAGE_FILE_NAME    "${CPACK_PACKAGE_NAME}-${MYSH_VERSION}-src")

set(CPACK_PACKAGE_INSTALL_DIRECTORY   "${CPACK_PACKAGE_NAME}-${MYSH_VERSION}${EXTRA_NAME_SUFFIX2}${PACKAGE_DRIVER_TYPE_SUFFIX}-${MYSH_PLATFORM}")

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY   "${CPACK_PACKAGE_INSTALL_DIRECTORY}-debug")
ENDIF()

set(CPACK_PACKAGE_FILE_NAME           "${CPACK_PACKAGE_INSTALL_DIRECTORY}")
set(CPACK_STRIP_FILES                 "bin/mysqlsh")

if(WIN32)
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "MySQL/MySQL Shell ${MYSH_BASE_VERSION}")
  IF(WITH_DEV)
    SET(CPACK_GENERATOR                 "ZIP")
  ELSE()
    set(CPACK_GENERATOR                 "ZIP;WIX")
  ENDIF()
  set(CPACK_PACKAGE_NAME              "MySQL Shell ${MYSH_VERSION}")
  set(CPACK_WIX_UPGRADE_GUID          "292FA6A2-8E70-4BC8-8C93-C1A374C8636C")
  set(CPACK_WIX_TEMPLATE              "${CMAKE_SOURCE_DIR}/cmake/WIX.template.in")
  set(CPACK_WIX_PROGRAM_MENU_FOLDER   "MySQL")
  if(NOT BUNDLE_RUNTIME_LIBRARIES)
    set(CPACK_WIX_VS_REDIST_CHECK     "1")
    if(MSVC_VERSION GREATER_EQUAL 1920 AND MSVC_VERSION LESS_EQUAL 1929)
      set(CPACK_WIX_REDIST_YEAR "2019")
      set(CPACK_WIX_REDIST_VERSION "14.20.0")
    elseif(MSVC_VERSION GREATER_EQUAL 1910 AND MSVC_VERSION LESS_EQUAL 1919)
      set(CPACK_WIX_REDIST_YEAR "2017")
      set(CPACK_WIX_REDIST_VERSION "14.10.0")
    elseif(MSVC_VERSION EQUAL 1900)
      set(CPACK_WIX_REDIST_YEAR "2015")
      set(CPACK_WIX_REDIST_VERSION "14.0.0")
    else()
      message(FATAL_ERROR "Unknown Visual Studio version")
    endif()
  endif()
  set(CPACK_PACKAGE_EXECUTABLES       "mysqlsh;MySQL Shell")
  if(HAVE_PYTHON)
    set(CPACK_WIX_WITH_PYTHON "1")
    set(CPACK_WIX_PYTHON_DIR "Python${PYTHONLIBS_MAJOR_MINOR}")
  endif()
  if(WITH_OCI)
    set(CPACK_WIX_WITH_OCI "1")
  endif()
  set(CPACK_WIX_EXTENSIONS "WixUtilExtension.dll")
else()
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MySQL RE")
  FIND_PROGRAM(RPMBUILD_EXECUTABLE rpmbuild)

  if(NOT RPMBUILD_EXECUTABLE)
    set(CPACK_SET_DESTDIR               "on")
    set(CPACK_GENERATOR                 "TGZ;DEB")
  else()
    set(CPACK_GENERATOR                 "TGZ;RPM")
    set(CPACK_RPM_PACKAGE_LICENSE       "GPL")
  endif(NOT RPMBUILD_EXECUTABLE)
endif()

set(CPACK_SOURCE_IGNORE_FILES
\\\\.git/
\\\\.gitignore
CMakeCache\\\\.txt
CPackSourceConfig\\\\.cmake
CPackConfig\\\\.cmake
VersionInfo\\\\.h$
postflight$
/cmake_install\\\\.cmake
/CTestTestfile\\\\.cmake
/CMakeFiles/
/_CPack_Packages/
Makefile$
cmake/sql.*\\\\.c$
)

#------------ Installation ---------------------------

if(WIN32)

# TODO: line-ending conversions unix->dos

# install(FILES ChangeLog     DESTINATION . RENAME ChangeLog.txt)
  install(FILES README        DESTINATION . RENAME README.txt COMPONENT main)
  install(FILES README        DESTINATION . RENAME README.txt COMPONENT dev)
# install(FILES INSTALL       DESTINATION . RENAME INSTALL.txt)
  install(FILES LICENSE       DESTINATION . RENAME LICENSE.txt COMPONENT main)
  install(FILES LICENSE       DESTINATION . RENAME LICENSE.txt COMPONENT dev)

  # Install all .pdb files to enable debugging. Note that what build
  # type and what sub directory the binaries ends up in, like
  # "Release" and "Debug", is not determined until we run "devenv" or
  # similar. So when running "cmake" we don't know the location. We
  # can't test for the location here, a if(EXISTS ...) is run at
  # "cmake" invocation time, not when we are to install. So we do a
  # bit of a hack here until finding a better solution.
  install(DIRECTORY
    ${PROJECT_BINARY_DIR}/bin/RelWithDebInfo/
    ${PROJECT_BINARY_DIR}/bin/Debug/
    DESTINATION bin
    COMPONENT dev
    FILES_MATCHING
    PATTERN *.pdb
  )
  install(DIRECTORY
    ${PROJECT_BINARY_DIR}/lib/RelWithDebInfo/
    ${PROJECT_BINARY_DIR}/lib/Debug/
    DESTINATION lib
    COMPONENT dev
    FILES_MATCHING
    PATTERN *.pdb
  )


else()

# install(FILES ChangeLog    DESTINATION .)
  install(FILES README       DESTINATION share/mysqlsh/ COMPONENT main)
  install(FILES README       DESTINATION share/mysqlsh/ COMPONENT dev)
# install(FILES INSTALL      DESTINATION .)
  install(FILES LICENSE      DESTINATION share/mysqlsh/ COMPONENT main)
  install(FILES LICENSE      DESTINATION share/mysqlsh/ COMPONENT dev)

endif()

# Variable defined when the packages are generated (i.e. not in source builds)
IF(MYSH_PLATFORM)
  IF(WITH_DEV)
      SET(CPACK_COMPONENTS_ALL main dev)
  ELSE()
    SET(CPACK_COMPONENTS_ALL main)
  ENDIF()

  set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_RPM_COMPONENT_INSTALL ON)
  set(CPACK_WIX_COMPONENT_INSTALL ON)
ENDIF()

include(CPack)
