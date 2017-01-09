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
#sys.path.append('/home/guidev/Descargas/AdminAPI_Scripts/testsFiles/')
#sys.path.append('../')
from testFunctions import read_line
from testFunctions import read_til_getShell
from testFunctions import kill_process
from testFunctions import exec_xshell_commands
import logging
logger = logging.getLogger()



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


  def test_028_MYS_850(self):
      '''[MYS-850] Cluster.status output is mixed up, kind of nested and not easily readable'''
      logger.debug("--------- " + str(self._testMethodName) + " ---------")
      instance1="3312"
      instance2="3313"
      instance3="3314"
      instance4="3322"
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P ' + instance2, '--classic']
      x_cmds = [("dba.verbose=2\n","2"),
                ("cluster = dba.getCluster('Cluster1');\n", "<Cluster:Cluster1>"),
                ("cluster.addInstance(\"{0}:{1}@{2}:".format(LOCALHOST.user, LOCALHOST.password,
                                                             LOCALHOST.host) + instance4 + "\");\n",
                 "was successfully added to the cluster")]
      time.sleep(5)
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')
      time.sleep(5)
      # cluster.status() display recovering for some added instances, require some time to set it to ONLINE
      x_cmds = [("cluster = dba.getCluster('Cluster1');\n", "<Cluster:Cluster1>"),
                ("cluster.status()\n",
                 "{" + os.linesep +
                 "    \"clusterName\": \"Cluster1\", " + os.linesep +
                 "    \"defaultReplicaSet\": {" + os.linesep +
                 "        \"name\": \"default\", " + os.linesep +
                 "        \"status\": \"Cluster tolerant to up to ONE failure.\", " + os.linesep +
                 "        \"topology\": {" + os.linesep +
                 "            \"localhost:3313\": {" + os.linesep +
                 "                \"address\": \"localhost:3313\", " + os.linesep +
                 "                \"leaves\": {" + os.linesep +
                 "                    \"localhost:3312\": {" + os.linesep +
                 "                        \"address\": \"localhost:3313\", " + os.linesep +
                 "                        \"leaves\": {}, " + os.linesep +
                 "                        \"mode\": \"R/O\", " + os.linesep +
                 "                        \"role\": \"HA\", " + os.linesep +
                 "                        \"status\": \"ONLINE\"" + os.linesep +
                 "                    }, " + os.linesep +
                 "                    \"localhost:3313\": {" + os.linesep +
                 "                        \"address\": \"localhost:3313\", " + os.linesep +
                 "                        \"leaves\": {}, " + os.linesep +
                 "                        \"mode\": \"R/O\", " + os.linesep +
                 "                        \"role\": \"HA\", " + os.linesep +
                 "                        \"status\": \"ONLINE\"" + os.linesep +
                 "                    }, " + os.linesep +
                 "                    \"localhost:3314\": {" + os.linesep +
                 "                        \"address\": \"localhost:3314\", " + os.linesep +
                 "                        \"leaves\": {}, " + os.linesep +
                 "                        \"mode\": \"R/O\", " + os.linesep +
                 "                        \"role\": \"HA\", " + os.linesep +
                 "                        \"status\": \"ONLINE\"" + os.linesep +
                 "                    }" + os.linesep +
                 "                    \"localhost:3322\": {" + os.linesep +
                 "                        \"address\": \"localhost:3322\", " + os.linesep +
                 "                        \"leaves\": {}, " + os.linesep +
                 "                        \"mode\": \"R/O\", " + os.linesep +
                 "                        \"role\": \"HA\", " + os.linesep +
                 "                        \"status\": \"ONLINE\"" + os.linesep +
                 "                    }, " + os.linesep +
                 "                }, " + os.linesep +
                 "                \"mode\": \"R/W\", " + os.linesep +
                 "                \"role\": \"HA\", " + os.linesep +
                 "                \"status\": \"ONLINE\"" + os.linesep +
                 "            }" + os.linesep +
                 "        }" + os.linesep +
                 "    }" + os.linesep +
                 "}")]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')



  # ----------------------------------------------------------------------
#
# if __name__ == '__main__':
#     unittest.main()

if __name__ == '__main__':
  unittest.main( testRunner=xmlrunner.XMLTestRunner(file(XMLReportFilePath,"w")))
