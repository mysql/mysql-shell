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
#import testFunctions
#
# def timeout(timeout):
#     def deco(func):
#         @functools.wraps(func)
#         def wrapper(*args, **kwargs):
#             # res = [Exception('function [%s] timeout [%s seconds] exceeded!' % (func.__name__, timeout))]
#             #res = [Exception('FAILED timeout [%s seconds] exceeded! ' % ( timeout))]
#             globales = func.func_globals
#             res = [Exception('FAILED timeout [%s seconds] exceeded! ' % (timeout))]
#             def newFunc():
#                 try:
#                     res[0] = func(*args, **kwargs)
#                 except ValueError:
#                     res[0] = ValueError
#             t = threading.Thread(target=newFunc)
#             t.daemon = True
#             try:
#                 t.start()
#                 t.join(timeout)
#             except ValueError:
#                 print ('error starting thread')
#                 raise ValueError
#             ret = res[0]
#             if isinstance(ret, BaseException):
#                 pass # raise ret
#             return ret
#         return wrapper
#     return deco
#
#
#
# def read_line(proc, fd, end_string):
#     data = ""
#     new_byte = b''
#     #t = time.time()
#     while (new_byte != b'\n'):
#         try:
#             new_byte = fd.read(1)
#             if new_byte == '' and proc.poll() != None:
#                 break
#             elif new_byte:
#                 # data += new_byte
#                 data += str(new_byte) ##, encoding='utf-8')
#                 if data.endswith(end_string):
#                     break;
#             elif proc.poll() is not None:
#                 break
#         except ValueError:
#             # timeout occurred
#             # print("read_line_timeout")
#             break
#     # print("read_line returned :"),
#     # sys.stdout.write(data)
#     return data
#
# def read_til_getShell(proc, fd, text):
#     globalvar.last_search = text
#     globalvar.last_found=""
#     data = []
#     line = ""
#     #t = time.time()
#     # while line != text  and proc.poll() == None:
#     while line.find(text,0,len(line))< 0  and proc.poll() == None and  globalvar.last_found.find(text,0,len(globalvar.last_found))< 0:
#     #while line.find(text,0,len(line))< 0  and proc.poll() == None:
#         try:
#             line = read_line(proc, fd, text)
#             globalvar.last_found = globalvar.last_found + line
#             if line:
#                 data.append(line)
#             elif proc.poll() is not None:
#                 break
#         except ValueError:
#             # timeout occurred
#             print("read_line_timeout")
#             break
#     return "".join(data)
#
# def kill_process(instance):
#     results="PASS"
#     home = os.path.expanduser("~")
#     try:
#         init_command = [MYSQL_SHELL, '--interactive=full']
#         if os.path.isdir(os.path.join(home,"mysql-sandboxes",instance)):
#             x_cmds = [
#                       ("dba.killSandboxInstance(" + instance + ");\n", "successfully killed."),
#                       ("dba.deleteSandboxInstance(" + instance + ");\n", "successfully deleted."),
#                       ]
#             results = exec_xshell_commands(init_command, x_cmds)
#         elif os.path.isdir(os.path.join(cluster_Path,instance)):
#             x_cmds = [
#                       ("dba.killSandboxInstance(" + instance + ", { sandboxDir: \"" + cluster_Path + "\"});\n", "successfully killed."),
#                       ("dba.deleteSandboxInstance(" + instance + ", { sandboxDir: \"" + cluster_Path + "\"});\n", "successfully deleted."),
#                      ]
#             results = exec_xshell_commands(init_command, x_cmds)
#     except Exception, e:
#         # kill instance failed
#         print("kill instance"+instance+"Failed, "+e)
#     return results
#
#
# @timeout(240)
# def exec_xshell_commands(init_cmdLine, commandList):
#     RESULTS = "PASS"
#     commandbefore = ""
#     if "--sql"  in init_cmdLine:
#         expectbefore = "mysql-sql>"
#     elif "--sqlc"  in init_cmdLine:
#         expectbefore = "mysql-sql>"
#     elif "--py" in init_cmdLine:
#         expectbefore = "mysql-py>"
#     elif "--js" in init_cmdLine:
#         expectbefore = "mysql-js>"
#     else:
#         expectbefore = "mysql-js>"
#     p = subprocess.Popen(init_cmdLine, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
#     for command, lookup in commandList:
#         # p.stdin.write(bytearray(command + "\n", 'ascii'))
#         p.stdin.write(bytearray(command , 'ascii'))
#         p.stdin.flush()
#         # stdin,stdout = p.communicate()
#         #found = read_til_getShell(p, p.stdout, lookup)
#         found = read_til_getShell(p, p.stdout, expectbefore)
#         if found.find(expectbefore, 0, len(found)) == -1:
#             stdin,stdout = p.communicate()
#             # return "FAIL \n\r"+stdin.decode("ascii") +stdout.decode("ascii")
#             RESULTS="FAILED"
#             return "FAIL: " + stdin.decode("ascii") + stdout.decode("ascii")
#             break
#         expectbefore = lookup
#         commandbefore =command
#     # p.stdin.write(bytearray(commandbefore, 'ascii'))
#     p.stdin.write(bytearray('', 'ascii'))
#     p.stdin.flush()
#     #p.stdout.reset()
#     stdin,stdout = p.communicate()
#     found = stdout.find(bytearray(expectbefore,"ascii"), 0, len(stdout))
#     if found == -1 and commandList.__len__() != 0 :
#             found = stdin.find(bytearray(expectbefore,"ascii"), 0, len(stdin))
#             if found == -1 :
#                 return "FAIL:  " + stdin.decode("ascii") + stdout.decode("ascii")
#             else :
#                 return "PASS"
#     else:
#         return "PASS"

# def load_tests(loader, tests, pattern):
#     ''' Discover and load all unit tests in all files named ``*_test.py`` in ``./src/``
#     '''
#     suite = TestSuite()
#     # for all_test_suite in unittest.defaultTestLoader.discover('src', pattern='Test_01*.py'):
#
#     for all_test_suite in unittest.defaultTestLoader.discover('./', pattern='Test_01*.py'):
#         for test_suite in all_test_suite:
#             suite.addTests(test_suite)
#     return suite


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
 #
 # class XShell_TestCases(unittest.TestCase):
 #
 #   @classmethod
 #   def setUpClass(self):
 #      # install xplugin
 #      results = ''
 #      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
 #                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--dba','enableXProtocol']
 #      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
 #      p.stdin.flush()
 #      stdin,stdout = p.communicate()
 #      if stdin.find(bytearray("X Protocol plugin is already enabled and listening for connections","ascii"), 0, len(stdin)) >= 0:
 #          results="PASS"
 #      else:
 #        raise ValueError("FAILED installing xplugin")
 #
 #      #def test_0_1(self):
 #      # create world_x and world_x-data
 #      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
 #                  '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--classic', '--file=' + Exec_files_location + 'world_x.sql']
 #      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
 #      stdin,stdout = p.communicate()
 #      results = ''
 #      init_command = [MYSQL_SHELL, '--interactive=full']
 #      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
 #                ("\\sql\n", "mysql-sql>"),
 #                ("use world_x;\n", "mysql-sql>"),
 #                ("show tables ;\n", "4 rows in set"),
 #                ]
 #
 #      results = exec_xshell_commands(init_command, x_cmds)
 #      if results !="PASS":
 #        raise ValueError("FAILED initializing schema world_x")
 #
 #      # create sakila and sakila-data
 #      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
 #                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--classic','--file=' + Exec_files_location + 'sakila-schema.sql']
 #      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
 #      stdin, stdout = p.communicate()
 #      init_command = [MYSQL_SHELL, '--interactive=full',  '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
 #                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port,'--sqlc','--classic','--file=' +Exec_files_location+'sakila-data-5712.sql']
 #      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
 #      stdin,stdout = p.communicate()
 #      #if stdout.find(bytearray("ERROR","ascii"),0,len(stdout))> -1:
 #      #  self.assertEqual(stdin, 'PASS')
 #      results = ''
 #      init_command = [MYSQL_SHELL, '--interactive=full']
 #      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
 #                ("\\sql\n","mysql-sql>"),
 #                ("use sakila;\n","mysql-sql>"),
 #                ("select count(*) from actor;\n","200"),
 #                ("select count(*) from city;\n","600"),
 #                ("select count(*) from rental;\n","16044"),
 #                ]
 #      results = exec_xshell_commands(init_command, x_cmds)
 #      if results !="PASS":
 #        raise ValueError("FAILED initializing schema sakila")
 #
 #      # create sakila_x
 #      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
 #                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--classic','--file=' + Exec_files_location + 'sakila_x.sql']
 #      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
 #      stdin, stdout = p.communicate()
 #      results = ''
 #      init_command = [MYSQL_SHELL, '--interactive=full']
 #      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
 #                ("\\sql\n","mysql-sql>"),
 #                ("use sakila_x;\n","mysql-sql>"),
 #                ("select count(*) from movies;\n","1 row in set"),
 #                ("select count(*) from users;\n","1 row in set"),
 #                ]
 #      results = exec_xshell_commands(init_command, x_cmds)
 #      if results !="PASS":
 #        raise ValueError("FAILED initializing schema sakila_x")




  # ----------------------------------------------------------------------

 #if __name__ == '__main__':
  #   unittest.main()

#if __name__ == '.':
if __name__ == '__main__':
  #unittest.main( testRunner=xmlrunner.XMLTestRunner(file(XMLReportFilePath,"w")))
  #suite = unittest.TestLoader().discover('.', pattern="test_MYS*.py")
  #unittest.TextTestRunner(verbosity=3).run(suite)

  #loader = unittest.TestLoader()
  #suite = loader.discover('.', pattern="test_MYS*.py")
  #unittest.main()
  ############################################################################################
  #suite = unittest.TestSuite()
  #suite.addTest(unittest.TestLoader().discover('./testsFiles', pattern="test_MYS_setup.py"))
  #suite.addTests(sorted(unittest.TestLoader().discover('./testsFiles', pattern="test_MYS_*.py")),)
  #######################################################3
  suite = unittest.TestSuite()
  suite.addTest(unittest.TestLoader().discover('./adminapi_testfiles', pattern="test_MYS_setup.py"))
  sortedtestfileNames = sorted(os.listdir('./adminapi_testfiles'))
  for Testfilename in sortedtestfileNames:
      if os.path.splitext(Testfilename)[1]== ".py":
          suite.addTests(unittest.TestLoader().discover('./adminapi_testfiles', pattern=Testfilename))

  #suite = unittest.TestLoader().discover('./adminapi_testfiles', pattern="test_MYS_6*.py")
  testRunner = xmlrunner.XMLTestRunner(file(XMLReportFilePath, "w"))
  testRunner.verbosity=3
  testRunner.run(suite)