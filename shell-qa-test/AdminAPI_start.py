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


if 'CONFIG_PATH' in os.environ and 'MYSQLX_PATH' in os.environ and os.path.isfile(os.environ['CONFIG_PATH']) and os.path.isfile(os.environ['MYSQLX_PATH']):
    # **** JENKINS EXECUTION ****
    XSHELL_QA_TEST_ROOT = os.environ['XSHELL_QA_TEST_ROOT']
    XMLReportFilePath = XSHELL_QA_TEST_ROOT+"/adminapi_qa_test.xml"
else:
    # **** LOCAL EXECUTION ****
    XMLReportFilePath = "adminapi_qa_test.xml"


###########################################################################################
if __name__ == '__main__':
  #######################################################3
  suite = unittest.TestSuite()
  suite.addTest(unittest.TestLoader().discover('./adminapi_testfiles', pattern="test_MYS_setup.py"))
  sortedtestfileNames = sorted(os.listdir('./adminapi_testfiles'))
  for Testfilename in sortedtestfileNames:
      if os.path.splitext(Testfilename)[1]== ".py":
          suite.addTests(unittest.TestLoader().discover('./adminapi_testfiles', pattern=Testfilename))

  testRunner = xmlrunner.XMLTestRunner(file(XMLReportFilePath, "w"))
  testRunner.verbosity=3
  testRunner.run(suite)