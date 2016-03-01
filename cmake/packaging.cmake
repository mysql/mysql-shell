# Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

##############################################################################
#
#  Packaging
#
##############################################################################

# Some stuff for building RPMs, that is not using CPack
# RPMs doesn't allow "-" in version, so we use like "5.3.0_alpha"
string(REPLACE "-" "_" MYSH_NODASH_VERSION "${MYSH_VERSION}")

# The rest is mainly about CPack
set(CPACK_PACKAGE_VERSION_MAJOR ${MYSH_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${MYSH_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${MYSH_PATCH})

if(NOT EXTRA_NAME_SUFFIX)
  set(EXTRA_NAME_SUFFIX "")
endif()
if(NOT EXTRA_NAME_SUFFIX2)
  set(EXTRA_NAME_SUFFIX2 "")
endif()

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MySQL Shell ${MYSH_VERSION}, a command line shell and scripting environment for MySQL Hybrid")
set(CPACK_PACKAGE_NAME                "mysql-shell${EXTRA_NAME_SUFFIX}")
set(CPACK_PACKAGE_VENDOR              "Oracle and/or its affiliates")
#if(EXISTS "${CMAKE_SOURCE_DIR}/COPYING.txt")
#  set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_SOURCE_DIR}/COPYING.txt")
#else()
#  set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_SOURCE_DIR}/LICENSE.mysql")
#endif()
set(CPACK_PACKAGE_DESCRIPTION_FILE    "${CMAKE_SOURCE_DIR}/README")
IF(EXISTS "${CMAKE_SOURCE_DIR}/COPYING.txt")
  SET(LICFILE COPYING.txt)
  SET(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_SOURCE_DIR}/COPYING.txt")
ELSE(EXISTS "${CMAKE_SOURCE_DIR}/COPYING.txt")
  SET(LICFILE LICENSE.mysql)
  SET(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_SOURCE_DIR}/LICENSE.mysql")
ENDIF(EXISTS "${CMAKE_SOURCE_DIR}/COPYING.txt")
SET(CPACK_PACKAGE_DESCRIPTION_FILE    "${CMAKE_SOURCE_DIR}/README")
set(CPACK_SOURCE_PACKAGE_FILE_NAME    "${CPACK_PACKAGE_NAME}-${MYSH_VERSION}-src")
set(CPACK_PACKAGE_INSTALL_DIRECTORY   "${CPACK_PACKAGE_NAME}-${MYSH_VERSION}${EXTRA_NAME_SUFFIX2}${PACKAGE_DRIVER_TYPE_SUFFIX}-${MYSH_PLATFORM}")
set(CPACK_PACKAGE_FILE_NAME           "${CPACK_PACKAGE_INSTALL_DIRECTORY}")
SET(CPACK_STRIP_FILES                 "bin/mysqlsh")

if(WIN32)
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "MySQL/MySQL Shell")
  if(WINDOWS_RUNTIME_MD)
    set(CPACK_GENERATOR                 "ZIP")
  else()
    set(CPACK_GENERATOR                 "ZIP;WIX")
  endif()
  set(CPACK_PACKAGE_NAME              "MySQL Shell ${MYSH_VERSION}")
  set(CPACK_WIX_UPGRADE_GUID          "292FA6A2-8E70-4BC8-8C93-C1A374C8636C")
  set(CPACK_WIX_TEMPLATE              "${CMAKE_SOURCE_DIR}/cmake/WIX.template.in")
  set(CPACK_WIX_PROGRAM_MENU_FOLDER   "MySQL")
  set(CPACK_PACKAGE_EXECUTABLES       "mysqlsh;MySQL Shell")
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
  if(EXISTS "${CMAKE_SOURCE_DIR}/COPYING.txt")
    install(FILES COPYING.txt       DESTINATION COPYING.txt COMPONENT main)
    install(FILES COPYING.txt       DESTINATION COPYING.txt COMPONENT dev)
  else()
    install(FILES LICENSE.mysql DESTINATION . RENAME LICENSE.mysql.txt COMPONENT main)
    install(FILES LICENSE.mysql DESTINATION . RENAME LICENSE.mysql.txt COMPONENT dev)
  endif()

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
  install(FILES README       DESTINATION share/mysqlx/ COMPONENT main)
  install(FILES README       DESTINATION share/mysqlx/ COMPONENT dev)
# install(FILES INSTALL      DESTINATION .)
  if(EXISTS "${CMAKE_SOURCE_DIR}/COPYING.txt")
    install(FILES COPYING.txt       DESTINATION share/mysqlx/ COMPONENT main)
    install(FILES COPYING.txt       DESTINATION share/mysqlx/ COMPONENT dev)
  else()
    install(FILES LICENSE.mysql DESTINATION share/mysqlx/ COMPONENT main)
    install(FILES LICENSE.mysql DESTINATION share/mysqlx/ COMPONENT dev)
  endif()

endif()

#install(FILES Licenses_for_Third-Party_Components.txt DESTINATION .)

# Variable defined when the packages are generated (i.e. not in source builds)
IF(MYSH_PLATFORM)
  if(WIN32)
    if(WINDOWS_RUNTIME_MD)
      set(CPACK_COMPONENTS_ALL dev)
    else()
      set(CPACK_COMPONENTS_ALL main)
    endif()
  else()
    set(CPACK_COMPONENTS_ALL main dev)
  endif()
  set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_RPM_COMPONENT_INSTALL ON)
  set(CPACK_WIX_COMPONENT_INSTALL ON)
ENDIF()

include(CPack)
