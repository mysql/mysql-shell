# -*- mode: python -*-
#
# Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

import os
import glob

# make sure we run Pyinstaller from correct path
_cwd = os.getcwd()
_cwd_directories = os.listdir(_cwd)
if not ("packaging" in _cwd_directories and "mysql_gadgets" in _cwd_directories):
    raise Exception("ERROR: Wrong working directory. Please run PyInstaller "
                    "from the directory that contains the mysql_gadgets and "
                    "packaging folders.")

block_cipher = None

# copy adapters' data
adapter_data = [("../mysql_gadgets/adapters/*.py", "mysql_gadgets/adapters"),
            ("../mysql_gadgets/adapters/__init__.py", "mysql_gadgets")]

# get list of adapter names
basenames = [os.path.basename(f)[:-3] for f in
             glob.glob(os.path.dirname("mysql_gadgets/adapters/")+"/*.py") if os.path.isfile(f)]

adapters_names = [f for f in basenames if f != "__init__"]

# Add all adapters to the list of hidden imports

hidden_imports = ["mysql_gadgets.adapters.{0}".format(f) for f in adapters_names]

# add hidden import made by connector python
hidden_imports.append("mysql.connector.locales.eng.client_error")

a = Analysis(['../front_end/mysqlprovision.py'],
             pathex=['.', 'packaging/'],
             binaries=None,
             datas=adapter_data,
             hiddenimports=hidden_imports,
             hookspath=[],
             runtime_hooks=[],
             excludes=[],
             win_no_prefer_redirects=False,
             win_private_assemblies=False,
             cipher=block_cipher)
pyz = PYZ(a.pure, a.zipped_data,
             cipher=block_cipher)
exe = EXE(pyz,
          a.scripts,
          a.binaries,
          a.zipfiles,
          a.datas,
          name='mysqlprovision',
          debug=False,
          strip=False,
          upx=True,
          console=True )
