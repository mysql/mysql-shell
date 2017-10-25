#
# Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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


import json
import sys

import mysqlsh
shell = mysqlsh.globals.shell

class LOGGER:
    # Need to be 2 for cluster_misconfiguration tests to get the warnings
    level = 2

    def debug(self, msg, *args):
        if self.level > 4:
            sys.stdout.write("%s\n" % {"type": "DEBUG", "msg": (msg % args).encode('utf8')})
        shell.log("DEBUG", "mysqlprovision: " + msg % args)

    def step(self, msg, *args):
        if self.level > 3:
            sys.stdout.write("%s\n" % {"type": "STEP", "msg": (msg % args).encode('utf8')})
        shell.log("INFO", "mysqlprovision: " + msg % args)

    def info(self, msg, *args):
        if self.level > 2:
            sys.stdout.write("%s\n" % {"type": "INFO", "msg": (msg % args).encode('utf8')})
        shell.log("INFO", "mysqlprovision: " + msg % args)

    def warning(self, msg, *args):
        if self.level > 1:
            sys.stdout.write("%s\n" % {"type": "WARNING", "msg": (msg % args).encode('utf8')})
        shell.log("WARNING", "mysqlprovision: " + msg % args)

    def error(self, msg, *args):
        if self.level > 0:
            sys.stderr.write("%s\n" % {"type": "ERROR", "msg": (msg % args)})
        shell.log("ERROR", "mysqlprovision: " + msg % args)

_instance = LOGGER()
def getLogger(name):
    return _instance


STEP_LOG_LEVEL_VALUE = 25
