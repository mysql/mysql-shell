#!/bin/sh
# Copyright (c) 2000, 2016, Oracle and/or its affiliates. All rights reserved.
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

# This script reports various configuration settings that may be needed
# when using the MySQL client library.

which ()
{
  IFS="${IFS=   }"; save_ifs="$IFS"; IFS=':'
  for file
  do
    for dir in $PATH
    do
      if test -f $dir/$file
      then
        echo "$dir/$file"
        continue 2
      fi
    done
    echo "which: no $file in ($PATH)"
    exit 1
  done
  IFS="$save_ifs"
}


get_full_path ()
{
  file=$1

  # if the file is a symlink, try to resolve it
  if [ -h $file ];
  then
    file=`ls -l $file | awk '{ print $NF }'`
  fi

  case $file in
    /*) echo "$file";;
    */*) tmp=`pwd`/$file; echo $tmp | sed -e 's;/\./;/;' ;;
    *) which $file ;;
  esac
}

me=`get_full_path $0`

# Script might have been renamed but assume mysqlsh_<something>config<something>
basedir=`echo $me | sed -e 's;/bin/mysqlsh_.*config.*;;'`

execdir='@libexecdir@'
bindir='@bindir@'

# If installed, search for the compiled in directory first (might be "lib64")
pkglibdir='@pkglibdir@'
pkglibdir_rel=`echo $pkglibdir | sed -e "s;^$basedir/;;"`

pkgdatadir='@pkgdatadir@'
pkgincludedir='@pkgincludedir@'

version='@MYSH_VERSION@'

# Create options
mysqlshcore_libs="-L$pkglibdir@RPATH_OPTION@"
mysqlshcore_libs="$libs @MYSQLSHCORE_LIBLIST@"

cxxflags="-I$pkgincludedir @CXXFLAGS@"
include="-I$pkgincludedir"


usage () {
        cat <<EOF
Usage: $0 [OPTIONS]
Options:
        --cxxflags       [$cxxflags]
        --include        [$include]
        --libs           [$mysqlshcore_libs]
        --version        [$version]
        --variable=VAR   VAR is one of:
                pkgincludedir [$pkgincludedir]
                pkgdatadir     [$pkgdatadir]
EOF
        exit 1
}

if test $# -le 0; then usage; fi

while test $# -gt 0; do
        case $1 in
        --cxxflags)echo "$cxxflags";;
        --include) echo "$include" ;;
        --libs)    echo "$libs" ;;
        --version) echo "$version" ;;
        --variable=*)
          var=`echo "$1" | sed 's,^[^=]*=,,'`
          case "$var" in
            pkgincludedir) echo "$pkgincludedir" ;;
            pkglibdir) echo "$pkglibdir" ;;
            *) usage ;;
          esac
          ;;
        *)         usage ;;
        esac

        shift
done

exit 0
