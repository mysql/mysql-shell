# Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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
import subprocess
import time
import sys
import datetime
import platform
import os
import threading
import functools
import unittest
import json
import xmlrunner
import shutil
from testFunctions import read_line
from testFunctions import read_til_getShell
from testFunctions import kill_process
from testFunctions import exec_xshell_commands

import logging
logger = logging.getLogger()
#logger.setLevel(logging.DEBUG)

#formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')

#fh = logging.FileHandler('AdminAPI_Log_Execution.txt', mode='a')
#fh.setLevel(logging.DEBUG)
#fh.setFormatter(formatter)
#logger.addHandler(fh)

############   Retrieve variables from configuration file    ##########################
class LOCALHOST:
    user =""
    password = ""
    host = ""
    xprotocol_port = ""
    port =""
class REMOTEHOST:
    user = ""
    password =""
    host = ""
    xprotocol_port = ""
    port = ""

if 'CONFIG_PATH' in os.environ and 'MYSQLX_PATH' in os.environ and os.path.isfile(os.environ['CONFIG_PATH']) and os.path.isfile(os.environ['MYSQLX_PATH']):
    # **** JENKINS EXECUTION ****
    config_path = os.environ['CONFIG_PATH']
    config=json.load(open(config_path))
    MYSQL_SHELL = os.environ['MYSQLX_PATH']
    Exec_files_location = os.environ['AUX_FILES_PATH']
    cluster_Path = os.environ['CLUSTER_PATH']
    XSHELL_QA_TEST_ROOT = os.environ['XSHELL_QA_TEST_ROOT']
    XMLReportFilePath = XSHELL_QA_TEST_ROOT+"/adminapi_qa_test.xml"
else:
    # **** LOCAL EXECUTION ****
    config=json.load(open('config_local.json'))
    MYSQL_SHELL = str(config["general"]["xshell_path"])
    Exec_files_location = str(config["general"]["aux_files_path"])
    cluster_Path = str(config["general"]["cluster_path"])
    XMLReportFilePath = "adminapi_qa_test.xml"

#########################################################################

LOCALHOST.user = str(config["local"]["user"])
LOCALHOST.password = str(config["local"]["password"])
LOCALHOST.host = str(config["local"]["host"])
LOCALHOST.xprotocol_port = str(config["local"]["xprotocol_port"])
LOCALHOST.port = str(config["local"]["port"])

REMOTEHOST.user = str(config["remote"]["user"])
REMOTEHOST.password = str(config["remote"]["password"])
REMOTEHOST.host = str(config["remote"]["host"])
REMOTEHOST.xprotocol_port = str(config["remote"]["xprotocol_port"])
REMOTEHOST.port = str(config["remote"]["port"])



class globalvar:
    last_found=""
    last_search=""

###########################################################################################

class XShell_TestCases(unittest.TestCase):

  def test_000_MYS_setUpClass(self):
      logger.info("--------- " + str(self._testMethodName) + " ---------")
      # install xplugin
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--dba', 'enableXProtocol']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
      p.stdin.flush()
      stdin, stdout = p.communicate()
      if stdin.find(bytearray("X Protocol plugin is already enabled and listening for connections", "ascii"), 0,
                    len(stdin)) >= 0:
          results = "PASS"
      else:
          raise ValueError("FAILED installing xplugin")

      # def test_0_1(self):
      # create world_x and world_x-data
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--mysql',
                      '--file=' + Exec_files_location + 'world_x.sql']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect --mx {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n", "mysql-sql>"),
                ("use world_x;\n", "mysql-sql>"),
                ("show tables ;\n", "4 rows in set"),
                ]

      results = exec_xshell_commands(init_command, x_cmds)
      if results != "PASS":
          raise ValueError("FAILED initializing schema world_x")

      # create sakila and sakila-data
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--mysql',
                      '--file=' + Exec_files_location + 'sakila-schema.sql']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--mysql',
                      '--file=' + Exec_files_location + 'sakila-data-5712.sql']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      # if stdout.find(bytearray("ERROR","ascii"),0,len(stdout))> -1:
      #  self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect --mx {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n", "mysql-sql>"),
                ("use sakila;\n", "mysql-sql>"),
                ("select count(*) from actor;\n", "200"),
                ("select count(*) from city;\n", "600"),
                ("select count(*) from rental;\n", "16044"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results != "PASS":
          raise ValueError("FAILED initializing schema sakila")

      # create sakila_x
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--mysql',
                      '--file=' + Exec_files_location + 'sakila_x.sql']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect --mx {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n", "mysql-sql>"),
                ("use sakila_x;\n", "mysql-sql>"),
                ("select count(*) from movies;\n", "1 row in set"),
                ("select count(*) from users;\n", "1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results != "PASS":
          raise ValueError("FAILED initializing schema sakila_x")


  # ----------------------------------------------------------------------
#
# if __name__ == '__main__':
#     unittest.main()

if __name__ == '__main__':
  unittest.main( testRunner=xmlrunner.XMLTestRunner(file(XMLReportFilePath,"w")))
