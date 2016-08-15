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

def timeout(timeout):
    def deco(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            # res = [Exception('function [%s] timeout [%s seconds] exceeded!' % (func.__name__, timeout))]
            #res = [Exception('FAILED timeout [%s seconds] exceeded! ' % ( timeout))]
            globales = func.func_globals
            res = [Exception('FAILED timeout [%s seconds] exceeded! ' % (timeout))]
            def newFunc():
                try:
                    res[0] = func(*args, **kwargs)
                except ValueError:
                    res[0] = ValueError
            t = threading.Thread(target=newFunc)
            t.daemon = True
            try:
                t.start()
                t.join(timeout)
            except ValueError:
                print ('error starting thread')
                raise ValueError
            ret = res[0]
            if isinstance(ret, BaseException):
                pass # raise ret
            return ret
        return wrapper
    return deco



def read_line(proc, fd, end_string):
    data = ""
    new_byte = b''
    #t = time.time()
    while (new_byte != b'\n'):
        try:
            new_byte = fd.read(1)
            if new_byte == '' and proc.poll() != None:
                break
            elif new_byte:
                # data += new_byte
                data += str(new_byte) ##, encoding='utf-8')
                if data.endswith(end_string):
                    break;
            elif proc.poll() is not None:
                break
        except ValueError:
            # timeout occurred
            # print("read_line_timeout")
            break
    # print("read_line returned :"),
    # sys.stdout.write(data)
    return data

def read_til_getShell(proc, fd, text):
    globalvar.last_search = text
    globalvar.last_found=""
    data = []
    line = ""
    #t = time.time()
    # while line != text  and proc.poll() == None:
    while line.find(text,0,len(line))< 0  and proc.poll() == None and  globalvar.last_found.find(text,0,len(globalvar.last_found))< 0:
    #while line.find(text,0,len(line))< 0  and proc.poll() == None:
        try:
            line = read_line(proc, fd, text)
            globalvar.last_found = globalvar.last_found + line
            if line:
                data.append(line)
            elif proc.poll() is not None:
                break
        except ValueError:
            # timeout occurred
            print("read_line_timeout")
            break
    return "".join(data)


@timeout(15)
def exec_xshell_commands(init_cmdLine, commandList):
    RESULTS = "PASS"
    commandbefore = ""
    if "--sql"  in init_cmdLine:
        expectbefore = "mysql-sql>"
    elif "--sqlc"  in init_cmdLine:
        expectbefore = "mysql-sql>"
    elif "--py" in init_cmdLine:
        expectbefore = "mysql-py>"
    elif "--js" in init_cmdLine:
        expectbefore = "mysql-js>"
    else:
        expectbefore = "mysql-js>"
    p = subprocess.Popen(init_cmdLine, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    for command, lookup in commandList:
        # p.stdin.write(bytearray(command + "\n", 'ascii'))
        p.stdin.write(bytearray(command , 'ascii'))
        p.stdin.flush()
        # stdin,stdout = p.communicate()
        found = read_til_getShell(p, p.stdout, expectbefore)
        if found.find(expectbefore, 0, len(found)) == -1:
            stdin,stdout = p.communicate()
            # return "FAIL \n\r"+stdin.decode("ascii") +stdout.decode("ascii")
            RESULTS="FAILED"
            return "FAIL: " + stdin.decode("ascii") + stdout.decode("ascii")
            break
        expectbefore = lookup
        commandbefore =command
    # p.stdin.write(bytearray(commandbefore, 'ascii'))
    p.stdin.write(bytearray('', 'ascii'))
    p.stdin.flush()
    #p.stdout.reset()
    stdin,stdout = p.communicate()
    found = stdout.find(bytearray(expectbefore,"ascii"), 0, len(stdout))
    if found == -1 and commandList.__len__() != 0 :
            found = stdin.find(bytearray(expectbefore,"ascii"), 0, len(stdin))
            if found == -1 :
                return "FAIL:  " + stdin.decode("ascii") + stdout.decode("ascii")
            else :
                return "PASS"
    else:
        return "PASS"

###############   Retrieve variables from configuration file    ##########################
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
    XSHELL_QA_TEST_ROOT = os.environ['XSHELL_QA_TEST_ROOT']
    XMLReportFilePath = XSHELL_QA_TEST_ROOT+"/xshell_qa_test.xml"
else:
    # **** LOCAL EXECUTION ****
    config=json.load(open('config_local.json'))
    MYSQL_SHELL = str(config["general"]["xshell_path"])
    Exec_files_location = str(config["general"]["aux_files_path"])
    XMLReportFilePath = "xshell_qa_test.xml"

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

  @classmethod
  def setUpClass(self):
      # install xplugin
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--dba','enableXProtocol']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("X Protocol plugin is already enabled and listening for connections","ascii"), 0, len(stdin)) >= 0:
          results="PASS"
      else:
        raise ValueError("FAILED installing xplugin")

      #def test_0_1(self):
      # create world_x and world_x-data
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                  '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--classic', '--file=' + Exec_files_location + 'world_x.sql']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin,stdout = p.communicate()
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use world_x;\n","mysql-sql>"),
                ("show tables ;\n","4 rows in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results !="PASS":
        raise ValueError("FAILED initializing schema world_x")

      # create sakila and sakila-data
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--classic','--file=' + Exec_files_location + 'sakila-schema.sql']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      init_command = [MYSQL_SHELL, '--interactive=full',  '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port,'--sqlc','--classic','--file=' +Exec_files_location+'sakila-data-5712.sql']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin,stdout = p.communicate()
      #if stdout.find(bytearray("ERROR","ascii"),0,len(stdout))> -1:
      #  self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("select count(*) from actor;\n","200"),
                ("select count(*) from city;\n","600"),
                ("select count(*) from rental;\n","16044"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results !="PASS":
        raise ValueError("FAILED initializing schema sakila")

      # create sakila_x
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--classic','--file=' + Exec_files_location + 'sakila_x.sql']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila_x;\n","mysql-sql>"),
                ("select count(*) from movies;\n","1 row in set"),
                ("select count(*) from users;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results !="PASS":
        raise ValueError("FAILED initializing schema sakila_x")


  def test_2_0_01_01(self):
      '''[2.0.01]:1 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x', '--sql']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(";\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-sql>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_01_02(self):
      '''[2.0.01]:2 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '-p','--passwords-from-stdin',
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(LOCALHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_01_03(self):
      '''[2.0.01]:3 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '-p','--passwords-from-stdin',
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port]
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(LOCALHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_01_04(self):
      '''[2.0.01]:4 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                        LOCALHOST.xprotocol_port), '--x', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_05(self):
      '''[2.0.01]:5 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),
                      '--x', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_06(self):
      '''[2.0.01]:6 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
           '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_07(self):
      '''[2.0.01]:7 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '-p', '-h' + LOCALHOST.host, '--node',
                      '--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(LOCALHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_01_08(self):
      '''[2.0.01]:8 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                        LOCALHOST.xprotocol_port), '--node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_09(self):
      '''[2.0.01]:9 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),
                      '--node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_10(self):
      '''[2.0.01]:10 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_11(self):
      '''[2.0.01]:11 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '-p', '-P'+LOCALHOST.port,'-h' + LOCALHOST.host, '--classic',
                      '--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(LOCALHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_01_13(self):
      '''[2.0.01]:13 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                        LOCALHOST.port),'--classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_01(self):
      '''[2.0.02]:1 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--x', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_02(self):
      '''[2.0.02]:2 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p', '-h' + REMOTEHOST.host, '--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(REMOTEHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_02_03(self):
      '''[2.0.02]:3 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p', '-h' + REMOTEHOST.host, '--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(REMOTEHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_02_04(self):
      '''[2.0.02]:4 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                        REMOTEHOST.xprotocol_port), '--x', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_05(self):
      '''[2.0.02]:5 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host),
                      '--x', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_06(self):
      '''[2.0.02]:6 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host, '-P' + REMOTEHOST.xprotocol_port, '--node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_07(self):
      '''[2.0.02]:7 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p', '-h' + REMOTEHOST.host,
                      '--node', '--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(REMOTEHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_02_08(self):
      '''[2.0.02]:8 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                        REMOTEHOST.xprotocol_port), '--node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_09(self):
      '''[2.0.02]:9 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host),
                      '--node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_10(self):
      '''[2.0.02]:10 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.port, '--classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_11(self):
      '''[2.0.02]:11 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p', '-h' + REMOTEHOST.host,
                      '--classic', '-P' + REMOTEHOST.port,'--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(REMOTEHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_02_12(self):
      '''[2.0.02]:12 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                        REMOTEHOST.port), '--classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_13(self):
      '''[2.0.02]:13 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.port), '--classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_03_02(self):
      '''[2.0.03]:2 Connect local Server on SQL mode: APPLICATION SESSION W/O PORT'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--sql']
      x_cmds = [("\\connect {0}:{1}@{2}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),"Creating an X Session"),
                ("\\js\n", "mysql-js>"),
                ("print(session);\n", "XSession:")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("using semicolon (;) at the end of \connect statement, behaves weird: ISSUE MYS-562")
  def test_2_0_03_03(self):
      '''[2.0.03]:3 Connect local Server on SQL mode: APPLICATION SESSION WITH PORT'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--sql']
      x_cmds = [("\\connect {0}:{1}@{2}:{3};\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,LOCALHOST.xprotocol_port),
                 "Creating an X Session"),
                ("\\js\n", "mysql-js"),
                ("print(session);\n", "XSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_03_04(self):
      '''[2.0.03]:4 Connect local Server on SQL mode: NODE SESSION W/O PORT'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect -n {0}:{1}@{2}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "Creating a Node Session"),
                ("print(session);\n", "NodeSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("using semicolon (;) at the end of \connect statement, behaves weird: ISSUE MYS-562")
  def test_2_0_03_05(self):
      '''[2.0.03]:5 Connect local Server on SQL mode: NODE SESSION WITH PORT'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect -n {0}:{1}@{2}:{3};\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                    LOCALHOST.xprotocol_port),"Creating a Node Session"),
                ("print(session);\n", "NodeSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_03_06(self):
      '''[2.0.03]:6 Connect local Server on SQL mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect -c {0}:{1}@{2}:{3}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.port),"Creating a Classic Session"),
                ("print(session);\n", "ClassicSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_04_02(self):
      '''[2.0.04]:2 Connect remote Server on SQL mode: APPLICATION SESSION W/O PORT'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--sql']
      x_cmds = [("\\connect {0}:{1}@{2}\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host),"Creating an X Session"),
                ("\\js\n", "mysql-js>"),
                ("print(session);\n", "XSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("using semicolon (;) at the end of \connect statement, behaves weird: ISSUE MYS-562")
  def test_2_0_04_03(self):
      '''[2.0.04]:3 Connect remote Server on SQL mode: APPLICATION SESSION WITH PORT'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--sql']
      x_cmds = [("\\connect {0}:{1}@{2}:{3};\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,REMOTEHOST.xprotocol_port),
                 "Creating an X Session"),
                ("\\js\n", "mysql-js"),
                ("print(session);\n", "XSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_04_04(self):
      '''[2.0.04]:4 Connect remote Server on SQL mode: NODE SESSION W/O PORT'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect -n {0}:{1}@{2}\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host),
                 "Creating a Node Session"),
                ("print(session);\n", "NodeSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("using semicolon (;) at the end of \connect statement, behaves weird: ISSUE MYS-562")
  def test_2_0_04_05(self):
      '''[2.0.04]:5 Connect remote Server on SQL mode: NODE SESSION WITH PORT'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect -n {0}:{1}@{2}:{3};\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                                    REMOTEHOST.xprotocol_port),"Creating a Node Session"),
                ("print(session);\n", "NodeSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_04_06(self):
      '''[2.0.04]:6 Connect remote Server on SQL mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect -c {0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.port),"Creating a Classic Session"),
                ("print(session);\n", "ClassicSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_05_02(self):
      '''[2.0.05]:2 Connect local Server on JS mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host), "mysql-js>"),
                ("var schemaList = session.getSchemas();\n", "mysql-js>"),
                ("print(schemaList);\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_05_03(self):
      '''[2.0.05]:3 Connect local Server on JS mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession({host: '" + LOCALHOST.host + "', dbUser: '"
                 + LOCALHOST.user +  "', dbPassword: '" + LOCALHOST.password + "'});\n", "mysql-js>"),
                ("var schemaList = session.getSchemas();\n", "mysql-js>"),
                ("print(schemaList);\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_05_04(self):
      '''[2.0.05]:4 Connect local Server on JS mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require(\'mysql\');\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                            LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("var schemaList = session.getSchemas();\n", "mysql-js>"),
                ("print(schemaList);\n", "sakila")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_06_02(self):
      '''[2.0.06]:2 Connect remote Server on JS mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\');\n","mysql-js>"),
                 ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(REMOTEHOST.user, REMOTEHOST.password,
                                                                                REMOTEHOST.host), "mysql-js>"),
                ("var schemaList = session.getSchemas();\n", "mysql-js>"),
                ("print(schemaList);\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_06_03(self):
      '''[2.0.06]:3 Connect remote Server on JS mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession({host: '" + REMOTEHOST.host + "', dbUser: '"
                 + REMOTEHOST.user +  "', dbPassword: '" + REMOTEHOST.password + "'});\n", "mysql-js>"),
                ("var schemaList = session.getSchemas();\n", "mysql-js>"),
                ("print(schemaList);\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_06_04(self):
      '''[2.0.06]:4 Connect remote Server on JS mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require(\'mysql\');\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(REMOTEHOST.user, REMOTEHOST.password,
                                                                            REMOTEHOST.host, REMOTEHOST.port), "mysql-js>"),
                ("var schemaList = session.getSchemas();\n", "mysql-js>"),
                ("print(schemaList);\n", "sakila")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_07_02(self):
      '''[2.0.07]:2 Connect local Server on PY mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                           LOCALHOST.host), "mysql-py>"),
                ("schemaList = session.get_schemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_07_03(self):
      '''[2.0.07]:3 Connect local Server on PY mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session({\'host\': \'" + LOCALHOST.host + "\', \'dbUser\': \'"
                 + LOCALHOST.user + "\', \'dbPassword\': \'" + LOCALHOST.password + "\'})\n", "mysql-py>"),
                ("schemaList = session.get_schemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_07_04(self):
      '''[2.0.07]:4 Connect local Server on PY mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("schemaList = session.get_schemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_2_0_08_02(self):
      '''[2.0.08]:2 Connect remote Server on PY mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(REMOTEHOST.user, REMOTEHOST.password,
                                                                           REMOTEHOST.host), "mysql-py>"),
                ("schemaList = session.get_schemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_08_03(self):
      '''[2.0.08]:3 Connect remote Server on PY mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session({\'host\': \'" + REMOTEHOST.host + "\', \'dbUser\': \'"
                 + REMOTEHOST.user + "\', \'dbPassword\': \'" + REMOTEHOST.password + "\'})\n", "mysql-py>"),
                ("schemaList = session.get_schemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_2_0_08_04(self):
      '''[2.0.08]:4 Connect remote Server on PY mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(REMOTEHOST.user, REMOTEHOST.password,
                                                                                 REMOTEHOST.host, REMOTEHOST.port),
                 "mysql-py>"),
                ("schemaList = session.get_schemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_01(self):
      '''[2.0.09]:1 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_02(self):
      '''[2.0.09]:2 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_03(self):
      '''[2.0.09]:3 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_04(self):
      '''4 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.port), '--classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_05(self):
      '''[2.0.09]:5 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.port), '--classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_06(self):
      '''[2.0.09]:6 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.port), '--classic', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_07(self):
      '''[2.0.09]:7 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_08(self):
      '''[2.0.09]:8 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_09(self):
      '''[2.0.09]:9 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_10(self):
      '''[2.0.09]:10 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_11(self):
      '''[2.0.09]:11 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--node', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_12(self):
      '''[2.0.09]:12 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--node', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_13(self):
      '''[2.0.09]:13 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_14(self):
      '''[2.0.09]:14 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_2_0_09_15(self):
      '''[2.0.09]:15 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_16(self):
      '''[2.0.09]:16 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--x', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_17(self):
      '''[2.0.09]:17 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--x', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_18(self):
      '''[2.0.09]:18 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--x', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_01(self):
      '''[2.0.10]:1 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.port, '--classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_02(self):
      '''[2.0.10]:2 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.port, '--classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_03(self):
      '''[2.0.10]:3 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.port, '--classic', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_04(self):
      '''[2.0.10]:4 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.port), '--classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_05(self):
      '''[2.0.10]:5 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.port), '--classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_06(self):
      '''[2.0.10]:6 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.port), '--classic', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_07(self):
      '''[2.0.10]:7 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_08(self):
      '''[2.0.10]:8 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--node', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_09(self):
      '''[2.0.10]:9 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_10(self):
      '''[2.0.10]:10 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_11(self):
      '''[2.0.10]:11 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--node', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_12(self):
      '''[2.0.10]:12 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--node', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_13(self):
      '''[2.0.10]:13 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--x', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_14(self):
      '''[2.0.10]:14 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--x', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_15(self):
      '''[2.0.10]:15 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--x', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_16(self):
      '''[2.0.10]:16 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--x', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_17(self):
      '''[2.0.10]:17 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--x', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_18(self):
      '''[2.0.10]:18 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--x', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_11_01(self):
      '''[2.0.11]:1 Connect local Server w/Command Line Args FAILOVER: Wrong Password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=g' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x', '--sql']
      x_cmds = [(";\n", "Invalid user or password")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find("Invalid user or password", 0, len(results))>0:
        results="PASS"
      self.assertEqual(results, 'PASS')

  def test_2_0_11_02(self):
      '''[2.0.11]:2 Connect local Server w/Command Line Args FAILOVER: unknown option'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-vu' + LOCALHOST.user, '--password=', '-h' + LOCALHOST.host,
                      '-P' + LOCALHOST.xprotocol_port, '--x', '--sql']
      x_cmds = [(";\n", "unknown option")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find("unknown option -vu", 0, len(results))>0:
        results="PASS"
      self.assertEqual(results, 'PASS')

  def test_2_0_11_03(self):
      '''[2.0.11]:3 Connect local Server w/Command Line Args FAILOVER: --uri wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, "wrongpass", LOCALHOST.host,
                                                        LOCALHOST.xprotocol_port), '--x', '--sql']
      x_cmds = [(";\n", "Invalid user or password")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find("Invalid user or password", 0, len(results))>0:
        results="PASS"
      self.assertEqual(results, 'PASS')

  def test_2_0_12_01(self):
      '''[2.0.11]:1 Connect remote Server w/Command Line Args FAILOVER: Wrong Password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=g' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--x', '--sql']
      x_cmds = [(";\n", "Invalid user or password")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find("Invalid user or password", 0, len(results))>0:
        results="PASS"
      self.assertEqual(results, 'PASS')

  def test_2_0_12_02(self):
      '''[2.0.11]:2 Connect remote Server w/Command Line Args FAILOVER: unknown option'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-vu' + REMOTEHOST.user, '--password=', '-h' + REMOTEHOST.host,
                      '-P' + REMOTEHOST.xprotocol_port, '--x', '--sql']
      x_cmds = [(";\n", "unknown option")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find("unknown option -vu", 0, len(results))>0:
        results="PASS"
      self.assertEqual(results, 'PASS')


  def test_2_0_12_03(self):
      '''[2.0.11]:3 Connect remote Server w/Command Line Args FAILOVER: --uri wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, "wrongpass", REMOTEHOST.host,
                                                        REMOTEHOST.xprotocol_port), '--x', '--sql']
      x_cmds = [(";\n", "Invalid user or password"),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find("Invalid user or password", 0, len(results))>0:
        results="PASS"
      self.assertEqual(results, 'PASS')

  def test_2_0_13_02(self):
      '''[2.0.13]:2 Connect local Server inside mysqlshell FAILOVER: \connect  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect {0}:{1}@{2}\n".format(LOCALHOST.user, "wronpassw", LOCALHOST.host), "mysql-js>"),
                ("print(session)\n", "Undefined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_13_03(self):
      '''[2.0.13]:3 Connect local Server inside mysqlshell FAILOVER: \connect -n  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect -n {0}:{1}@{2}\n".format(LOCALHOST.user, "wrongpassw", LOCALHOST.host), "mysql-js>"),
                ("print(session)\n", "Undefined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_13_04(self):
      '''[2.0.13]:4 Connect local Server inside mysqlshell FAILOVER: \connect -c  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect -c {0}:{1}@{2}:{3}\n".format(LOCALHOST.user, "wrongpass", LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("print(session)\n", "Undefined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_14_02(self):
      '''[2.0.14]:2 Connect remote Server inside mysqlshell FAILOVER: \connect  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect {0}:{1}@{2}\n".format(REMOTEHOST.user, "wronpassw", REMOTEHOST.host), "mysql-js>"),
                ("print(session)\n", "Undefined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_14_03(self):
      '''[2.0.14]:3 Connect remote Server inside mysqlshell FAILOVER: \connect -n  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect -n {0}:{1}@{2}\n".format(REMOTEHOST.user, "wrongpassw", REMOTEHOST.host), "mysql-js>"),
                ("print(session)\n", "Undefined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_14_04(self):
      '''[2.0.14]:4 Connect remote Server inside mysqlshell FAILOVER: \connect -c  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect -c {0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, "wrongpass", REMOTEHOST.host, REMOTEHOST.port), "mysql-js>"),
                ("print(session)\n", "Undefined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_15_01(self):
      '''[2.0.15]:1 Connect local Server w/Init Exec mode: --[sql/js/py] FAILOVER: wrong Exec mode --sqxx'''
      results = ''
      p = subprocess.Popen([MYSQL_SHELL, '--interactive=full', '--uri', 'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, "wrongpass", LOCALHOST.host, LOCALHOST.xprotocol_port),
                          '--x', '--sqx'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin,stdout = p.communicate()
      found = stdout.find(bytearray("unknown option","ascii"), 0, len(stdout))
      if found == -1:
          results= "FAIL \n\r" + stdout.decode("ascii")
      else:
          results= "PASS"
      self.assertEqual(results, 'PASS')

  def test_2_0_16_01(self):
      '''[2.0.16]:1 Connect remote Server w/Init Exec mode: --[sql/js/py] FAILOVER: wrong Exec mode --sqxx'''
      results = ''
      p = subprocess.Popen([MYSQL_SHELL, '--interactive=full', '--uri', 'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, "wrongpass", REMOTEHOST.host, REMOTEHOST.xprotocol_port),
                          '--x', '--sqx'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin,stdout = p.communicate()
      found = stdout.find(bytearray("unknown option","ascii"), 0, len(stdout))
      if found == -1 :
          results= "FAIL \n\r" + stdout.decode("ascii")
      else:
          results= "PASS"
      self.assertEqual(results, 'PASS')

  def test_3_1_01_01(self):
      '''[3.1.001]:1 Check that command  [ \help, \h, \? ] works: \help'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\help\n", "Global Commands")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_01_02(self):
      '''[3.1.001]:2 Check that command  [ \help, \h, \? ] works: \h'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\h\n", "Global Commands")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_01_03(self):
      '''[3.1.001]:3 Check that command  [ \help, \h, \? ] works: \?'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\?\n", "Global Commands")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_02_01(self):
      '''[3.1.002]:1 Check that help command with parameter  works: \help connect'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\help connect\n", "Connect to a server." + os.linesep)]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_02_02(self):
      '''[3.1.002]:2 Check that help command with parameter  works: \h connect'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\h connect\n", "Connect to a server.")]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_02_03(self):
      '''[3.1.002]:3 Check that help command with parameter  works: \? connect'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\? connect\n", "Connect to a server.")]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_3_1_03_01(self):
      '''[3.1.003]:1 Check that help command with wrong parameter works: \help connect'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\help conect\n", "Global Commands")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_03_02(self):
      '''[3.1.003]:2 Check that help command with wrong parameter works: \h connect'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\h conect\n", "Global Commands")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_03_03(self):
      '''[3.1.003]:3 Check that help command with wrong parameter works: \? conect'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\? conect\n", "Global Commands")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_04_01(self):
      '''[3.1.004]:1 Check that command [ \quit, \q, \exit ] works: \quit'''
      results = ''
      p = subprocess.Popen([MYSQL_SHELL, '--interactive=full'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray('\\quit\n', 'ascii'))
      p.stdin.flush()
      stdout,stderr = p.communicate()
      found = stderr.find(bytearray("Error","ascii"), 0, len(stdout))
      if found == -1:
          results= "PASS"
      else:
          results= "FAIL \n\r" + stdout.decode("ascii")
      self.assertEqual(results, 'PASS')

  def test_3_1_04_02(self):
      '''[3.1.004]:2 Check that command [ \quit, \q, \exit ] works: \q '''
      results = ''
      p = subprocess.Popen([MYSQL_SHELL, '--interactive=full'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray('\\q\n', 'ascii'))
      p.stdin.flush()
      stdout,stderr = p.communicate()
      found = stderr.find(bytearray("Error","ascii"), 0, len(stdout))
      if found == -1:
          results= "PASS"
      else:
          results= "FAIL \n\r" + stdout.decode("ascii")
      self.assertEqual(results, 'PASS')

  def test_3_1_04_03(self):
      '''[3.1.004]:3 Check that command [ \quit, \q, \exit ] works: \exit'''
      results = ''
      p = subprocess.Popen([MYSQL_SHELL, '--interactive=full'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray('\\exit\n', 'ascii'))
      p.stdin.flush()
      stdout,stderr = p.communicate()
      found = stderr.find(bytearray("Error","ascii"), 0, len(stdout))
      if found == -1:
          results= "PASS"
      else:
          results= "FAIL \n\r" + stdout.decode("ascii")
      self.assertEqual(results, 'PASS')


  def test_3_1_05_01(self):
      '''[3.1.005]:1 Check that MODE SQL command [ \sql ] works: \sql '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\sql\n", "mysql-sql>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_06_01(self):
      '''[3.1.006]:1 Check that MODE JavaScript command [ \js ] works: \js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\js\n", "mysql-js>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_07_01(self):
      '''[3.1.007] Check that MODE Python command [ \py ] works: \py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\py\n", "mysql-py>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_09_01(self):
      '''[3.1.009]:1 Check that STATUS command [ \status, \s ] works: app session \status'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), '--x', '--js']
      x_cmds = [("\\status\n", "Current user:                 " + LOCALHOST.user + "@localhost")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_09_02(self):
      '''[3.1.009]:2 Check that STATUS command [ \status, \s ] works: classic session \status'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,LOCALHOST.port), '--classic', '--js']
      x_cmds = [("\\status\n", "Current user:                 " + LOCALHOST.user + "@localhost")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_3_1_09_03(self):
      '''[3.1.009]:3 Check that STATUS command [ \status, \s ] works: node session \status'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,REMOTEHOST.xprotocol_port), '--node', '--sql']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin =subprocess.PIPE )
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      if stdin.find(bytearray("Creating a Node Session to","ascii"),0,len(stdin))> -1 and stdin.find(bytearray("mysql-sql>","ascii"),0,len(stdin))> -1:
        results = 'PASS'
      self.assertEqual(results, 'PASS')




  def test_3_1_10_1(self):
      '''[3.1.010]:1 Check that EXECUTE SCRIPT FILE command [ \source, \. ] works: node session \source select_actor_10.sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect -n {0}:{1}@{2}\n".format(LOCALHOST.user, LOCALHOST.password,LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("\\source {0}select_actor_10.sql\n".format(Exec_files_location),"rows in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_10_2(self):
      '''[3.1.010]:2 Check that EXECUTE SCRIPT FILE command [ \source, \. ] works: node session \. select_actor_10.sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("\\. {0}select_actor_10.sql\n".format(Exec_files_location),"rows in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("not working delimiter with multiline")
  def test_3_1_11_1(self):
      '''[3.1.011]:1 Check that MULTI LINE MODE command [ \ ] works'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("DROP PROCEDURE IF EXISTS get_actors;\n","mysql-sql>"),
                ("delimiter #\n","mysql-sql>"),
                ("create procedure get_actors()\n",""),
                ("begin\n",""),
                ("select first_name from sakila.actor;\n",""),
                ("end#\n","mysql-sql>"),
                #("\n","mysql-sql>"),
                ("delimiter ;\n","mysql-sql>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_2_04_1(self):
      '''[3.2.004] Check SQL command SHOW WARNINGS [ \W ] works'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\W\n", "Show warnings enabled"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_2_05_1(self):
      '''[3.2.005] Check SQL command NO SHOW WARNINGS [ \w ] works'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\w\n", "Show warnings disabled"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_0_01_01(self):
      '''[4.0.001]:1 Batch Exec - Loading code from file:  --file= createtable.js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--file=' + Exec_files_location + 'CreateTable.js']
      x_cmds = []
      results = exec_xshell_commands(init_command, x_cmds)
      results2=str(results)
      if results2.find(bytearray("FAIL","ascii"),0,len(results2))> -1:
        self.assertEqual(results2, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show tables like \'testdb\';\n","1 row in set"),
                ("drop table if exists testdb;\n","Query OK"),
                ("show tables like 'testdb';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')



  def test_4_0_02_01(self):
      '''[4.0.002]:1 Batch Exec - Loading code from file:  < createtable.js'''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location+'CreateTable.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show tables like 'testdb';\n","1 row in set"),
                ("drop table if exists testdb;\n","Query OK"),
                ("show tables like 'testdb';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_1_01_01(self):
      '''[4.1.001]:1 SQL Create a table: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node']
      x_cmds = [('\\sql\n', "mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("DROP TABLE IF EXISTS example_automation;\n","mysql-sql>"),
                ("CREATE TABLE example_automation ( id INT, data VARCHAR(100) );\n","mysql-sql>"),
                ("show tables like \'example_automation\';\n","1 row in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_1_02_01(self):
      '''[4.1.002] SQL Create a table using STDIN batch process: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node','--sql','--schema=sakila',
                      '--file='+ Exec_files_location +'CreateTable_SQL.sql']

      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE )
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show tables like \'example_SQLTABLE\';\n","1 row in set"),
                ("drop table if exists example_SQLTABLE;\n","Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_2_16_1(self):
      '''[4.2.016]:1 PY Read executing the stored procedure: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7','--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host ), "mysql-py>"),
                ("session.sql('use sakila;').execute()\n","Query OK"),
                ("session.sql('drop procedure if exists get_actors;').execute()\n","Query OK"),
                ("session.sql(\"CREATE PROCEDURE get_actors() BEGIN select first_name from actor where actor_id < 5 ; END;\").execute()\n","Query OK"),
                ("session.sql('call get_actors();').execute()\n","rows in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_2_16_2(self):
      '''[4.2.016]:2 PY Read executing the stored procedure: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7','--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session('{0}:{1}@{2}:{3}')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host,LOCALHOST.port ), "mysql-py>"),
                ("session.run_sql('use sakila;')\n","Query OK"),
                ("session.run_sql('drop procedure if exists get_actors;')\n","Query OK"),
                ("session.run_sql('CREATE PROCEDURE get_actors() BEGIN  "
                 "select first_name from actor where actor_id < 5 ; END;')\n","Query OK"),
                ("session.run_sql('call get_actors();')\n","rows in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_1_1(self):
      '''[4.3.001]:1 SQL Update table using multiline mode:CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--classic','--sqlc']
      x_cmds = [("\\\n","..."),
                ("use sakila;\n","..."),
                ("Update actor set last_name ='Test Last Name', last_update = now() where actor_id = 2;\n","..."),
                ("\n","Query OK, 1 row affected"),
                ("select last_name from actor where actor_id = 2;\n","Test Last Name")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_2_1(self):
      '''[4.3.002]:1 SQL Update table using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sqlc', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open( Exec_files_location + 'UpdateTable_SQL.sql'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')

      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT first_name FROM actor WHERE first_name='Test';\n","Test"),
                ("UPDATE actor SET first_name='Testback' WHERE actor_id=2;\n","Query OK"),
                ("SELECT first_name FROM actor WHERE first_name='Testback';\n","Testback"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_2_2(self):
      '''[4.3.002]:2 SQL Update table using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open( Exec_files_location + 'UpdateTable_SQL.sql'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')

      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT first_name FROM actor WHERE first_name='Test';\n","Test"),
                ("UPDATE actor SET first_name='Testback' WHERE actor_id=2;\n","Query OK"),
                ("SELECT first_name FROM actor WHERE first_name=\'Testback\';\n","Testback"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_3_1(self):
      '''[4.3.003]:1 SQL Update database using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--classic','--sqlc']
      x_cmds = [("drop schema if exists AUTOMATION;\n","mysql-sql>"),
                ("create schema if not exists AUTOMATION;\n","mysql-sql>"),
                ("\\\n","..."),
                ("ALTER SCHEMA AUTOMATION DEFAULT COLLATE utf8_general_ci ;\n","..."),
                ("\n","Query OK"),
                ("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = \'AUTOMATION' LIMIT 1;\n","1 row in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_4_1(self):
      '''[4.3.004]:1 SQL Update database using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sqlc', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open( Exec_files_location + 'SchemaDatabaseUpdate_SQL.sql'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')

      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = \'AUTOMATION' LIMIT 1;\n","1 row in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_4_2(self):
      '''[4.3.004]:2 SQL Update database using STDIN batch code'''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'SchemaDatabaseUpdate_SQL.sql'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = \'AUTOMATION' LIMIT 1;\n","1 row in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_5_1(self):
      '''[4.3.005]:1 SQL Update Alter view using multiline mode'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--classic','--sqlc']
      x_cmds = [("use sakila;\n","mysql-sql>"),
                ("DROP VIEW IF EXISTS sql_viewtest;\n","mysql-sql>"),
                ("create view sql_viewtest as select * from actor where first_name like \'%as%\';\n","mysql-sql>"),
                ("\\\n","       ..."),
                ("alter view sql_viewtest as select count(*) as ActorQuantity from actor;\n","       ..."),
                ("\n","mysql-sql>"),
                ("select * from sql_viewtest;\n","row in set"),
                ("DROP VIEW IF EXISTS sql_viewtest;\n","mysql-sql>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_6_1(self):
      '''[4.3.006]:1 SQL Update Alter view using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sqlc', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila', '--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           stdin=open(Exec_files_location + 'AlterView_SQL.sql'))
      stdin, stdout = p.communicate()
      # if stdout.find(bytearray("ERROR", "ascii"), 0, len(stdin)) > -1:
      #     self.assertEqual(stdin, 'PASS')
      # results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n", "mysql-sql>"),
                ("use sakila;\n", "mysql-sql>"),
                ("select * from sql_viewtest;\n", "row in set"),
                ("DROP VIEW IF EXISTS sql_viewtest;\n", "mysql-sql>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')



  def test_4_3_6_2(self):
      '''[4.3.006]:2 SQL Update Alter view using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'AlterView_SQL.sql'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("select * from sql_viewtest;\n","row in set"),
                ("DROP VIEW IF EXISTS sql_viewtest;\n","mysql-sql>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_7_1(self):
      '''[4.3.007]:1 SQL Update Alter stored procedure using multiline mode'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--classic','--sqlc']
      x_cmds = [("use sakila;\n","mysql-sql>"),
                ("DROP procedure IF EXISTS sql_sptest;\n","mysql-sql>"),
                ("DELIMITER $$\n","mysql-sql>"),
                ("\\\n","..."),
                ("create procedure sql_sptest (OUT param1 INT)\n","       ..."),
                ("BEGIN\n","       ..."),
                ("SELECT count(*) INTO param1 FROM country;\n","       ..."),
                ("END$$\n","       ..."),
                ("\n","mysql-sql>"),
                ("DELIMITER ;\n","mysql-sql>"),
                ("call  sql_sptest(@a);\n","Query OK"),
                ("select @a;\n","1 row in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_8_1(self):
      '''[4.3.008]:1 SQL Update Alter stored procedure using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sqlc', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'AlterStoreProcedure_SQL.sql'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("call  sql_sptest(@a);\n","Query OK"),
                ("select @a;\n","1 row in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_8_2(self):
      '''[4.3.008]:2 SQL Update Alter stored procedure using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'AlterStoreProcedure_SQL.sql'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("call  sql_sptest(@a);\n","Query OK"),
                ("select @a;\n","1 row in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_9_1(self):
      '''[4.3.009]:1 JS Update table using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require(\'mysql\');\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host, LOCALHOST.port),"mysql-js>"),
                ("session.runSql(\"use sakila;\");\n","Query OK"),
                ("session.runSql(\"drop table if exists sakila.friends;\");\n","Query OK"),
                ("session.runSql(\'create table sakila.friends (name varchar(50), last_name varchar(50), "
                 "age integer, gender varchar(20));\');\n","Query OK"),
                ("session.runSql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES (\'jack\',"
                 "\'black\', 17, \'male\');\");\n","mysql-js>"),
                ("session.runSql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES (\'ruben\',"
                 "\'morquecho\', 40, \'male\');\");\n","mysql-js>"),
                ("session.runSql(\"UPDATE friends SET name=\'ruben dario\' where name =  '\ruben\';\");\n","mysql-js>"),
                ("session.runSql(\"SELECT * from friends where name LIKE '\%ruben%\';\");\n","ruben dario")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_9_2(self):
      '''[4.3.009]:2 JS Update table using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql(\"use sakila;\").execute();\n","Query OK"),
                ("session.sql(\"drop table if exists sakila.friends;\").execute();\n","Query OK"),
                ("session.sql(\'create table sakila.friends (name varchar(50), last_name varchar(50), "
                 "age integer, gender varchar(20));\').execute();\n","Query OK"),
                ("var table = session.getSchema('sakila').friends;\n","mysql-js>"),
                ("table.insert('name','last_name','age','gender').values('jack','black', 17, 'male');\n","Query OK"),
                ("table.insert('name','last_name','age','gender').values('ruben','morquecho', 40, 'male');\n","Query OK"),
                ("var res_ruben = table.update().set('name','ruben dario').set('age',42).where('name=\"ruben\"').execute();\n","mysql-js>"),
                ("table.select();\n","ruben dario")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_10_1(self):
      '''[4.3.010]:1 JS Update table using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require('mysql');\n","mysql-js>"),
                ("var session=mysql.getClassicSession('{0}:{1}@{2}:{3}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql('use sakila;');\n","Query OK"),
                ("session.runSql('drop table if exists sakila.friends;');\n","Query OK"),
                ("session.runSql('create table sakila.friends (name varchar(50), last_name varchar(50), "
                 "age integer, gender varchar(20));');\n","Query OK"),
                ("var table = session.sakila.friends;\n","mysql-js>"),
                ("session.\n","..."),
                ("runSql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES ('jack','black', 17, 'male');\");\n","..."),
                ("\n","Query OK, 1 row affected"),
                ("session.\n","..."),
                ("runSql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES ('ruben','morquecho', 40, 'male');\");\n","..."),
                ("\n","Query OK, 1 row affected"),
                ("session.runSql('select name from sakila.friends;');\n","ruben"),
                ("session.runSql('select name from sakila.friends;');\n","jack"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_10_2(self):
      '''[4.3.010]:2 JS Update table using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql('use sakila;').execute();\n","Query OK"),
                ("session.sql('drop table if exists sakila.friends;').execute();\n","Query OK"),
                ("session.sql('create table sakila.friends (name varchar(50), last_name varchar(50), "
                 "age integer, gender varchar(20));').execute();\n","Query OK"),
                ("var table = session.getSchema('sakila').friends;\n","mysql-js>"),
                ("table.insert('name','last_name','age','gender').\n","..."),
                ("values('jack','black', 17, 'male');\n","..."),
                ("\n","Query OK, 1 item affected"),
                ("table.insert('name','last_name','age','gender').values('ruben','morquecho', 40, 'male');\n","Query OK"),
                ("var res_ruben = table.update().set('name','ruben dario').\n","..."),
                ("set('age',42).where('name=\"ruben\"').execute();\n","..."),
                ("var res_jack = table.update().set('name','jackie chan').set('age',18).\n","..."),
                ("where('name=\"jack\"').execute();\n","..."),
                ("\n","mysql-js>"),
                ("table.select();\n","ruben dario"),
                ("table.select();\n","jackie chan")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_11_1(self):
      '''[4.3.011]:1 JS Update table using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateTable_ClassicMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT * FROM sakila.actor where actor_id = 50;\n","1 row in set"),
                ("UPDATE actor SET first_name = 'Old value' where actor_id = 50 ;\n","Query OK, 1 row affected")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_11_2(self):
      '''[4.3.011]:2 JS Update table using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateTable_NodeMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT * FROM sakila.actor where actor_id = 50;\n","1 row in set"),
                ("UPDATE actor SET first_name = 'Old value' where actor_id = 50 ;\n","Query OK, 1 row affected")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_12_1(self):
      '''[4.3.012]:1 JS Update database using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require('mysql');\n","mysql-js>"),
                ("var session=mysql.getClassicSession('{0}:{1}@{2}:{3}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql('drop database if exists automation_test;');\n","Query OK"),
                ("session.runSql('create database automation_test;');\n","Query OK"),
                ("session.runSql('ALTER SCHEMA automation_test  DEFAULT COLLATE utf8_general_ci;');\n","Query OK"),
                ("session.runSql(\"SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = 'automation_test' ;\");\n","utf8_general_ci")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_12_2(self):
      '''[4.3.012]:2 JS Update database using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql(\'drop database if exists automation_test;\').execute();\n","Query OK"),
                ("session.sql('create database automation_test;').execute();\n","Query OK"),
                ("session.sql('ALTER SCHEMA automation_test  DEFAULT COLLATE utf8_general_ci;').execute();\n","Query OK"),
                ("session.sql(\"SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = "
                 "'automation_test' ;\").execute();\n","utf8_general_ci")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_13_1(self):
      '''[4.3.013]:1 JS Update database using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require('mysql');\n","mysql-js>"),
                ("var session=mysql.getClassicSession('{0}:{1}@{2}:{3}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql('drop database if exists automation_test;');\n","Query OK"),
                ("session.\n","..."),
                ("runSql('create database automation_test;');\n","..."),
                ("\n","Query OK"),
                ("session.\n","..."),
                ("runSql('ALTER SCHEMA automation_test  DEFAULT COLLATE utf8_general_ci;');\n","..."),
                ("\n","mysql-js>"),
                ("session.runSql(\"SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = "
                 "'automation_test' ;\");\n","utf8_general_ci")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_13_2(self):
      '''[4.3.013]:2 JS Update database using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession('{0}:{1}@{2}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql('drop database if exists automation_test;').execute();\n","Query OK"),
                ("session.\n","..."),
                ("sql('create database automation_test;').execute();\n","..."),
                ("\n","Query OK"),
                ("session.\n","..."),
                ("sql('ALTER SCHEMA automation_test  DEFAULT COLLATE utf8_general_ci;').execute();\n","..."),
                ("\n","mysql-js>"),
                ("session.sql(\"SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = "
                 "'automation_test' ;\").execute();\n","utf8_general_ci")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_14_1(self):
      '''[4.3.014]:1 JS Update database using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateSchema_ClassicMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = 'schema_test' LIMIT 1;;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_14_2(self):
      '''[4.3.014]:2 JS Update database using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateSchema_NodeMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = 'schema_test' LIMIT 1;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_15_1(self):
      '''[4.3.015]:1 JS Update alter view using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require('mysql');\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql('use sakila;');\n","Query OK"),
                ("session.runSql('drop view if exists js_view;');\n","Query OK"),
                ("session.runSql(\"create view js_view as select first_name from actor where first_name like '%a%';\");\n","Query OK"),
                ("session.runSql(\"alter view js_view as select * from actor where first_name like '%a%';\");\n","Query OK"),
                ("session.runSql('SELECT * from js_view;');\n","actor_id")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_15_2(self):
      '''[4.3.015]:2 JS Update alter view using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql('use sakila;').execute();\n","Query OK"),
                ("session.sql('drop view if exists js_view;').execute();\n","Query OK"),
                ("session.sql(\"create view js_view as select first_name from actor where first_name like '%a%';\").execute();\n","Query OK"),
                ("session.sql(\"alter view js_view as select * from actor where first_name like '%a%';\").execute();\n","Query OK"),
                ("session.sql('SELECT * from js_view;').execute();\n","actor_id")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_16_1(self):
      '''[4.3.016]:1 JS Update alter view using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require('mysql');\n","mysql-js>"),
                ("var session=mysql.getClassicSession('{0}:{1}@{2}:{3}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql('use sakila;');\n","Query OK"),
                ("session.runSql('drop view if exists js_view;');\n","Query OK"),
                ("session.\n","..."),
                ("runSql(\"create view js_view as select first_name from actor where first_name like '%a%';\");\n","..."),
                ("\n","Query OK"),
                ("session.\n","..."),
                ("runSql(\"alter view js_view as select * from actor where first_name like '%a%';\");\n","..."),
                ("\n","Query OK"),
                ("session.runSql('SELECT * from js_view;');\n","actor_id")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_16_2(self):
      '''[4.3.016]:2 JS Update alter view using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession('{0}:{1}@{2}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql('use sakila;').execute();\n","Query OK"),
                ("session.sql('drop view if exists js_view;').execute();\n","Query OK"),
                ("session.\n","..."),
                ("sql(\"create view js_view as select first_name from actor where first_name like '%a%';\").execute();\n","..."),
                ("session.\n","..."),
                ("sql(\"alter view js_view as select * from actor where first_name like '%a%';\").execute();\n","..."),
                ("\n","mysql-js>"),
                ("session.sql(\'SELECT * from js_view;\').execute();\n","actor_id")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_17_1(self):
      '''[4.3.017]:1 JS Update alter view using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateView_ClassicMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT * FROM js_view ;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_17_2(self):
      '''[4.3.017]:2 JS Update alter view using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateView_NodeMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT * FROM js_viewnode ;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_3_18_1(self):
      '''[4.3.018]:1 JS Update alter stored procedure using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require('mysql');\n","mysql-js>"),
                ("var session=mysql.getClassicSession('{0}:{1}@{2}:{3}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql('use sakila;');\n","Query OK"),
                ("session.runSql('DROP PROCEDURE IF EXISTS my_automated_procedure;');\n","Query OK"),
                ("session.runSql('delimiter \\\\');\n","mysql-js>"),
                ("session.runSql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND\\\\\");\n","mysql-js>"),
                ("delimiter ;\n","mysql-js>"),
                ("session.runSql(\"select name from mysql.proc  where name like '%my_automated_procedure%';\");\n","1 row")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_3_18_2(self):
      '''[4.3.018]:2 JS Update alter stored procedure using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql('use sakila;').execute();\n","Query OK"),
                ("session.sql('DROP PROCEDURE IF EXISTS my_automated_procedure;').execute();\n","Query OK"),
                ("session.sql('delimiter \\\\').execute();\n","mysql-js>"),
                ("session.sql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND\\\\\").execute();\n","mysql-js>"),
                ("delimiter ;\n","mysql-js>"),
                ("session.sql(\"select name from mysql.proc  where name like '%my_automated_procedure%';\").execute();\n","1 row ")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_3_19_1(self):
      '''[4.3.019]:1 JS Update alter stored procedure using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require(\'mysql\');\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql(\'use sakila;\');\n","Query OK"),
                ("session.runSql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\');\n","Query OK"),
                ("session.runSql('delimiter \\\\');\n","mysql-js>"),
                ("session.\n ","..."),
                ("runSql(\"create procedure my_automated_procedure (INOUT incr_param INT) BEGIN \n    SET incr_param = incr_param + 1 ;\nEND\\\\\");\n","..."),
                ("\n","mysql-js>"),
                ("session.runSql('delimiter ;');\n","mysql-js>"),
                ("session.runSql(\"select name from mysql.proc where name like '%my_automated_procedure%';\");\n","1 row ")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_3_19_2(self):
      '''[4.3.019]:2 JS Update alter stored procedure using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession('{0}:{1}@{2}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql('use sakila;').execute();\n","Query OK"),
                ("session.sql('DROP PROCEDURE IF EXISTS my_automated_procedure;').execute();\n","Query OK"),
                ("session.sql('delimiter \\\\').execute();\n","..."),
                ("session.sql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND\\\\\").execute();\n","..."),
                ("\n","mysql-js>"),
                ("session.sql('delimiter ;').execute();\n","mysql-js>"),
                ("session.sql(\"select name from mysql.proc where name like '%my_automated_procedure%';\").execute();\n","1 row")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_20_1(self):
      '''[4.3.020]:1 JS Update alter stored procedure using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateProcedure_ClassicMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("call Test;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_20_2(self):
      '''[4.3.020]:2 JS Update alter stored procedure using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateProcedure_NodeMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("call Test2;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_21_1(self):
      '''[4.3.021]:1 PY Update table using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysql\n","mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host, LOCALHOST.port),"mysql-py>"),
                ("session.run_sql(\'use sakila;\')\n","Query OK"),
                ("session.run_sql(\'drop table if exists sakila.friends;\')\n","Query OK"),
                ("session.run_sql(\'create table sakila.friends (name varchar(50), last_name varchar(50), "
                 "age integer, gender varchar(20))\')\n","Query OK"),
                ("session.run_sql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES (\'jack\',"
                 "\'black\', 17, \'male\');\")\n","mysql-py>"),
                ("session.run_sql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES (\'ruben\',"
                 "\'morquecho\', 40, \'male\');\")\n","mysql-py>"),
                ("session.run_sql(\"UPDATE friends SET name=\'ruben dario\' where name =  '\ruben\';\")\n","mysql-py>"),
                ("session.run_sql(\"SELECT * from friends where name LIKE '\%ruben%\';\")\n","ruben dario")
                ]

      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_21_2(self):
      '''[4.3.021]:2 PY Update table using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                           LOCALHOST.host), "mysql-py>"),
                ("session.sql(\"use sakila;\").execute()\n", "Query OK"),
                ("session.sql(\"drop table if exists sakila.friends;\").execute()\n", "Query OK"),
                ("session.sql(\'create table sakila.friends (name varchar(50), last_name varchar(50), "
                 "age integer, gender varchar(20));\').execute()\n", "Query OK"),
                ("table = session.get_schema('sakila').friends\n", "mysql-py>"),
                ("table.insert(\'name\',\'last_name\',\'age\',\'gender\').values(\'jack\',\'black\', 17, \'male\')\n",
                 "Query OK"),
                (
                "table.insert(\'name\',\'last_name\',\'age\',\'gender\').values(\'ruben\',\'morquecho\', 40, \'male\')\n",
                "Query OK"),
                (
                "res_ruben = table.update().set(\'name\',\'ruben dario\').set(\'age\',42).where(\'name=\"ruben\"\').execute()\n",
                "mysql-py>"),
                ("table.select()\n", "mysql-py>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_22_1(self):
      '''[4.3.022]:1 PY Update table using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.run_sql(\"use sakila;\")\n", "Query OK"),
                ("session.run_sql(\"drop table if exists sakila.friends;\")\n", "Query OK"),
                ("session.run_sql(\'create table sakila.friends (name varchar(50), last_name varchar(50), "
                 "age integer, gender varchar(20));\')\n", "Query OK"),
                ("session.run_sql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES (\'jack\',"
                 "\'black\', 17, \'male\');\")\n", "mysql-py>"),
                ("session.run_sql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES (\'ruben\',"
                 "\'morquecho\', 40, \'male\');\")\n", "mysql-py>"),
                ("\\\n", "..."),
                ("session.run_sql(\"UPDATE friends SET name=\'ruben dario\' where name =  \'ruben\';\")\n", "..."),
                ("session.run_sql(\"UPDATE friends SET name=\'jackie chan\' where name =  \'jack\';\")\n", "..."),
                ("\n", "mysql-py>"),
                ("session.run_sql(\"SELECT * from friends where name LIKE \'%ruben%\';\")\n", "ruben dario"),
                ("session.run_sql(\"SELECT * from friends where name LIKE \'%jackie chan%\';\")\n", "jackie chan")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_22_2(self):
      '''[4.3.022]:2 PY Update table using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session('{0}:{1}@{2}')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                         LOCALHOST.host), "mysql-py>"),
                ("session.sql('use sakila;').execute()\n", "Query OK"),
                ("session.sql('drop table if exists sakila.friends;').execute()\n", "Query OK"),
                ("session.sql('create table sakila.friends (name varchar(50), last_name varchar(50), "
                 "age integer, gender varchar(20));').execute()\n", "Query OK"),
                ("table = session.get_schema('sakila').friends\n", "mysql-py>"),
                ("table.insert('name','last_name','age','gender').\\\n", "..."),
                ("values('jack','black', 17, 'male')\n", "..."),
                ("\n", "Query OK, 1 item affected"),
                ("table.insert('name','last_name','age','gender').values('ruben','morquecho', 40, 'male')\n",
                 "Query OK"),
                ("res_ruben = table.update().set('name','ruben dario').\\\n", "..."),
                ("set('age',42).where('name=\"ruben\"').execute()\n", "..."),
                ("\n", "mysql-py>"),
                ("res_jack = table.update().set('name','jackie chan').set('age',18).\\\n", "..."),
                ("where('name=\"jack\"').execute()\n", "..."),
                ("\n", "mysql-py>"),
                ("table.select()\n", "ruben dario"),
                ("table.select()\n", "jackie chan")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_23_1(self):
      '''[4.3.023]:1 PY Update table using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateTable_ClassicMode.py'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT * FROM sakila.actor where actor_id = 50;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_23_2(self):
      '''[4.3.023]:2 PY Update table using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila', '--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           stdin=open(Exec_files_location + 'UpdateTable_NodeMode.py'))
      stdin, stdout = p.communicate()
      if stdout.find(bytearray("ERROR", "ascii"), 0, len(stdin)) > -1:
          self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n", "mysql-sql>"),
                ("use sakila;\n", "mysql-sql>"),
                ("SELECT * FROM sakila.friends;\n", "7 rows in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_24_1(self):
      '''[4.3.024]:1 PY Update database using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.run_sql(\"drop database if exists automation_test;\")\n", "Query OK"),
                ("session.run_sql(\'create database automation_test;\')\n", "Query OK"),
                ("session.run_sql(\'ALTER SCHEMA automation_test  DEFAULT COLLATE utf8_general_ci;\')\n", "Query OK"),
                ("session.run_sql(\"SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = "
                 "\'automation_test\' ;\")\n", "utf8_general_ci")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_24_2(self):
      '''[4.3.024]:2 PY Update database using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                           LOCALHOST.host), "mysql-py>"),
                ("session.sql(\"drop database if exists automation_test;\").execute()\n", "Query OK"),
                ("session.sql(\'create database automation_test;\').execute()\n", "Query OK"),
                ("session.sql(\'ALTER SCHEMA automation_test  DEFAULT COLLATE utf8_general_ci;\').execute()\n",
                 "Query OK"),
                ("session.sql(\"SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = "
                 "\'automation_test\' ;\").execute()\n", "utf8_general_ci")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_25_1(self):
      '''[4.3.025]:1 PY Update database using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.\\\n", "..."),
                ("run_sql('drop database if exists automation_test;')\n", "..."),
                ("\n", "Query OK"),
                ("session.\\\n", "..."),
                ("run_sql('create database automation_test;')\n", "..."),
                ("\n", "Query OK"),
                ("session.\\\n", "..."),
                ("run_sql('ALTER SCHEMA automation_test  DEFAULT COLLATE utf8_general_ci;')\n", "..."),
                ("\n", "mysql-py>"),
                ("session.run_sql(\"SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = "
                 "'automation_test' ;\")\n", "utf8_general_ci")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_25_2(self):
      '''[4.3.025]:2 PY Update database using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                           LOCALHOST.host), "mysql-py>"),
                ("session.sql('drop database if exists automation_test;').execute()\n", "Query OK"),
                ("session.\\\n", "..."),
                ("sql('create database automation_test;').execute()\n", "..."),
                ("\n", "Query OK"),
                ("session.\\\n", "..."),
                ("sql(\'ALTER SCHEMA automation_test  DEFAULT COLLATE utf8_general_ci;\').execute()\n", "..."),
                ("\n", "mysql-py>"),
                ("session.sql(\"SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = "
                 "'automation_test';\").execute()\n", "utf8_general_ci")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_26_1(self):
      '''[4.3.026]:1 PY Update database using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateSchema_ClassicMode.py'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = 'schema_test' LIMIT 1;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_26_2(self):
      '''[4.3.026]:2 PY Update database using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateSchema_NodeMode.py'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = 'schema_test' LIMIT 1;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_27_1(self):
      '''[4.3.027]:1 PY Update alter view using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.run_sql('use sakila;')\n", "Query OK"),
                ("session.run_sql('drop view if exists js_view;')\n", "Query OK"),
                (
                "session.run_sql(\"create view js_view as select first_name from actor where first_name like '%a%';\")\n",
                "Query OK"),
                ("session.run_sql(\"alter view js_view as select * from actor where first_name like '%a%';\")\n",
                 "Query OK"),
                ("session.run_sql('SELECT * from js_view;')\n", "actor_id")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_27_2(self):
      '''[4.3.027]:2 PY Update alter view using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                           LOCALHOST.host), "mysql-py>"),
                ("session.sql(\'use sakila;\').execute()\n", "Query OK"),
                ("session.sql(\'drop view if exists js_view;\').execute()\n", "Query OK"),
                (
                "session.sql(\"create view js_view as select first_name from actor where first_name like \'%a%\';\").execute()\n",
                "Query OK"),
                (
                "session.sql(\"alter view js_view as select * from actor where first_name like \'%a%\';\").execute()\n",
                "Query OK"),
                ("session.sql(\"SELECT * from js_view;\").execute()\n", "actor_id")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_28_1(self):
      '''[4.3.028]:1 PY Update alter view using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.run_sql('use sakila;')\n", "Query OK"),
                ("session.run_sql('drop view if exists js_view;')\n", "Query OK"),
                ("session.\\\n", "..."),
                ("run_sql(\"create view js_view as select first_name from actor where first_name like '%a%';\")\n",
                 "..."),
                ("\n", "Query OK"),
                ("session.\\\n", "..."),
                ("run_sql(\"alter view js_view as select * from actor where first_name like '%a%';\")\n", "..."),
                ("\n", "Query OK"),
                ("session.run_sql('SELECT * from js_view');\n", "rows"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_28_2(self):
      '''[4.3.028]:2 PY Update alter view using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session('{0}:{1}@{2}')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                         LOCALHOST.host), "mysql-py>"),
                ("session.sql('use sakila;').execute()\n", "Query OK"),
                ("session.sql('drop view if exists js_view;').execute()\n", "Query OK"),
                ("session.\\\n", "..."),
                (
                "sql(\"create view js_view as select first_name from actor where first_name like '%a%';\").execute()\n",
                "..."),
                ("\n", "Query OK"),
                ("session.\\\n", "..."),
                ("sql(\"alter view js_view as select * from actor where first_name like '%a%';\").execute()\n", "..."),
                ("\n", "mysql-py>"),
                ("session.sql('SELECT * from js_view;').execute()\n", "rows")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_29_1(self):
      '''[4.3.029]:1 PY Update alter view using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila', '--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           stdin=open(Exec_files_location + 'UpdateView_ClassicMode.py'))
      stdin, stdout = p.communicate()
      if stdout.find(bytearray("ERROR", "ascii"), 0, len(stdin)) > -1:
          self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n", "mysql-sql>"),
                ("use sakila;\n", "mysql-sql>"),
                ("SELECT * FROM py_view ;\n", "1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_29_2(self):
      '''[4.3.029]:2 PY Update alter view using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateView_NodeMode.py'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT * FROM py_view ;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_30_1(self):
      '''[4.3.030]:1 PY Update alter stored procedure using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.run_sql(\'use sakila;\')\n", "Query OK"),
                ("session.run_sql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\')\n", "Query OK"),
                ("session.run_sql(\"delimiter \\\\\")\n", "mysql-py>"),
                ("session.run_sql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND\\\\\")\n", "mysql-py>"),
                ("delimiter ;\n", "mysql-py>"),
                ("session.run_sql(\"select name from mysql.proc;\")\n", "my_automated_procedure")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_30_2(self):
      '''[4.3.030]:2 PY Update alter stored procedure using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                           LOCALHOST.host), "mysql-py>"),
                ("session.sql(\'use sakila;\').execute()\n", "Query OK"),
                ("session.sql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\').execute()\n", "Query OK"),
                ("session.sql(\"delimiter \\\\\").execute()\n", "mysql-py>"),
                ("session.sql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND\\\\\").execute()\n", "mysql-py>"),
                ("delimiter ;\n", "mysql-py>"),
                ("session.sql(\"select name from mysql.proc;\").execute()\n", "my_automated_procedure")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_3_31_1(self):
      '''[4.3.031]:1 PY Update alter stored procedure using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysql\n","mysql-py>"),
                ("session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-py>"),
                ("session.runSql(\'use sakila;\')\n","Query OK"),
                ("session.runSql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\')\n","Query OK"),
                ("\\\n","..."),
                ("session.runSql(\"delimiter \\\\\")\n","..."),
                ("session.runSql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND\\\\\")\n","..."),
                ("delimiter ;\n","..."),
                ("\n","mysql-py>"),
                ("session.runSql(\"select name from mysql.proc;\")\n","my_automated_procedure")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_3_31_2(self):
      '''[4.3.031]:2 PY Update alter stored procedure using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysqlx\n","mysql-py>"),
                ("session=mysqlx.getNodeSession(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-py>"),
                ("session.sql(\'use sakila;\').execute()\n","Query OK"),
                ("session.sql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\').execute()\n","Query OK"),
                ("\\\n","..."),
                ("session.sql(\'delimiter \\\\\').execute()\n","..."),
                ("session.sql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND\\\\\").execute()\n","..."),
                ("delimiter ;\n","..."),
                ("\n","mysql-py>"),
                ("session.sql(\"select name from mysql.proc;\").execute()\n","my_automated_procedure")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_3_32_1(self):
      '''[4.3.032]:1 PY Update alter stored procedure using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateProcedure_ClassicMode.py'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("call Test;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_32_2(self):
      '''[4.3.032]:2 PY Update alter stored procedure using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'UpdateProcedure_NodeMode.py'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("call Test;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_1_1(self):
      '''[4.4.001]:1 SQL Delete table using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--classic','--sqlc']
      x_cmds = [("use sakila;\n","Query OK"),
                ("DROP TABLE IF EXISTS example_automation;\n","Query OK"),
                ("CREATE TABLE example_automation \n","..."),
                ("( id INT, data VARCHAR(100) );\n","Query OK"),
                ("select table_name from information_schema.tables where table_name = 'example_automation';\n","1 row"),
                ("DROP TABLE IF EXISTS \n","..."),
                ("example_automation;\n\n","mysql-sql>"),
                ("select table_name from information_schema.tables where table_name = \"example_automation\";\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_1_2(self):
      '''[4.4.001]:2 SQL Delete table using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--node','--sql']
      x_cmds = [("use sakila;\n","Query OK"),
                ("DROP TABLE IF EXISTS example_automation;\n","Query OK"),
                ("CREATE TABLE example_automation \n","..."),
                ("( id INT, data VARCHAR(100) );\n","Query OK"),
                ("show tables like 'example_automation';\n","1 row in set"),
                ("DROP TABLE IF EXISTS \n","..."),
                ("example_automation;\n","Query OK"),
                ("select table_name from information_schema.tables where table_name = 'example_automation';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_2_1(self):
      '''[4.4.002]:1 SQL Delete table using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sqlc', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteTable_SQL.sql'))
      stdin,stdout = p.communicate()
      # if stderr.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
      #   self.assertEqual(stdin, 'PASS')
      # results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT * from information_schema.tables WHERE table_schema ='example_SQLTABLE';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_2_2(self):
      '''[4.4.002]:2 SQL Delete table using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteTable_SQL.sql'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT * from information_schema.tables WHERE table_schema ='example_SQLTABLE';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_3_1(self):
      '''[4.4.003]:1 SQL Delete database using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' +
                      LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic','--sqlc']
      x_cmds = [("use sakila;\n", "mysql-sql>"),
                ("DROP DATABASE IF EXISTS dbtest;\n","mysql-sql>"),
                ("CREATE DATABASE dbtest;\n","mysql-sql>"),
                ("show databases;\n","dbtest"),
                ("\\\n","..."),
                ("DROP DATABASE IF EXISTS dbtest;\n","..."),
                ("\n","mysql-sql>"),
                ("show databases like 'dbtest';\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_3_2(self):
      '''[4.4.003]:2 SQL Delete database using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' +
                      LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node','--sql']
      x_cmds = [("use sakila;\n", "mysql-sql>"),
                ("DROP DATABASE IF EXISTS dbtest;\n","mysql-sql>"),
                ("CREATE DATABASE dbtest;\n","mysql-sql>"),
                ("show databases;\n","dbtest"),
                ("\\\n","..."),
                ("DROP DATABASE IF EXISTS dbtest;\n","..."),
                ("\n","mysql-sql>"),
                ("show databases like 'dbtest';\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_4_1(self):
      '''[4.4.004]:1 SQL Delete database using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sqlc', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteSchema_SQL.sql'))
      stdin,stdout = p.communicate()
      # if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
      #   self.assertEqual(stdin, 'PASS')
      # results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show databases like 'dbtest';\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_4_2(self):
      '''[4.4.004]:2 SQL Delete database using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteSchema_SQL.sql'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show databases like 'dbtest';\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_5_1(self):
      '''[4.4.005]:1 SQL Delete view using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' +
                      LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic','--sqlc']
      x_cmds = [("use sakila;\n", "mysql-sql>"),
                ("DROP VIEW IF EXISTS sql_viewtest;\n","mysql-sql>"),
                ("create view sql_viewtest as select * from actor where first_name like \'%as%\';\n","mysql-sql>"),
                ("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.VIEWS;\n","sql_viewtest"),
                ("\\\n","..."),
                ("DROP VIEW IF EXISTS sql_viewtest;\n","..."),
                ("\n","mysql-sql>"),
                ("SELECT * from information_schema.views WHERE TABLE_NAME ='sql_viewtest';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_5_2(self):
      '''[4.4.005]:2 SQL Delete view using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' +
                      LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node','--sql']
      x_cmds = [("use sakila;\n", "mysql-sql>"),
                ("DROP VIEW IF EXISTS sql_viewtest;\n","mysql-sql>"),
                ("create view sql_viewtest as select * from actor where first_name like \'%as%\';\n","mysql-sql>"),
                ("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.VIEWS WHERE TABLE_NAME LIKE \'%viewtest%\';\n","sql_viewtest"),
                ("\\\n","..."),
                ("DROP VIEW IF EXISTS sql_viewtest;\n","..."),
                ("\n","mysql-sql>"),
                ("SELECT * from information_schema.views WHERE TABLE_NAME ='sql_viewtest';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_6_1(self):
      '''[4.4.006]:1 SQL Delete view using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteView_SQL.sql'))
      stdin,stdout = p.communicate()
      # if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
      #  self.assertEqual(stdin, 'PASS')
      # results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT * from information_schema.views WHERE TABLE_NAME ='sql_viewtest';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_6_2(self):
      '''[4.4.006]:2 SQL Delete view using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteView_SQL.sql'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT * from information_schema.views WHERE TABLE_NAME ='sql_viewtest';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_7_1(self):
      '''[4.4.007]:1 SQL Delete stored procedure using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' +
                      LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic','--sqlc']
      x_cmds = [("use sakila;\n", "mysql-sql>"),
                ("DROP PROCEDURE IF EXISTS my_automated_procedure;\n","mysql-sql>"),
                ("delimiter \\\\ \n","mysql-sql>"),
                ("create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND\\\\\n","mysql-sql>"),
                ("delimiter ;\n","mysql-sql>"),
                ("select name from mysql.proc where name like \'%automated_procedure%\';\n","my_automated_procedure"),
                ("\\\n","..."),
                ("DROP PROCEDURE IF EXISTS my_automated_procedure;\n","..."),
                ("\n","mysql-sql>"),
                ("select name from mysql.proc where name like \'%automated_procedure%\';\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_7_2(self):
      '''[4.4.007]:2 SQL Delete stored procedure using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' +
                      LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node','--sql']
      x_cmds = [("use sakila;\n", "mysql-sql>"),
                ("DROP PROCEDURE IF EXISTS my_automated_procedure;\n","mysql-sql>"),
                ("delimiter \\\\ \n","mysql-sql>"),
                ("create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND\\\\\n","mysql-sql>"),
                ("delimiter ;\n","mysql-sql>"),
                ("select name from mysql.proc where name like \'%automated_procedure%\';\n","my_automated_procedure"),
                ("\\\n","..."),
                ("DROP PROCEDURE IF EXISTS my_automated_procedure;\n","..."),
                ("\n","mysql-sql>"),
                ("select name from mysql.proc where name like \'%automated_procedure%\';\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_8_1(self):
      '''[4.4.008]:1 SQL Delete stored procedure using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sqlc', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteProcedure_SQL.sql'))
      stdin,stdout = p.communicate()
      # if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
      #  self.assertEqual(stdin, 'PASS')
      # results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("call test_procedure;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_8_2(self):
      '''[4.4.008]:2 SQL Delete stored procedure using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteProcedure_SQL.sql'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("call test_procedure;\n","1 row in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_9_1(self):
      '''[4.4.009]:1 JS Delete table using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require(\'mysql\');\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host, LOCALHOST.port),"mysql-js>"),
                ("session.runSql(\"use sakila;\");\n","Query OK"),
                ("session.runSql(\"drop table if exists sakila.friends;\");\n","Query OK"),
                ("session.runSql(\"create table sakila.friends (name varchar(50), last_name varchar(50), age integer, gender varchar(20));\");\n","Query OK"),
                ("session.runSql(\"show tables like \'friends\';\");\n","1 row in set"),
                ("session.runSql(\"INSERT INTO sakila.friends (name, last_name,age,gender) VALUES(\'ruben\',\'morquecho\', "
                 "40,\'male\');\");\n","Query OK"),
                ("session.runSql(\"UPDATE sakila.friends SET name=\'ruben dario\' where name =  \'ruben\';\");\n","Query OK"),
                ("session.runSql(\"SELECT * from friends where name LIKE \'%ruben%\';\");\n","1 row in set"),
                ("session.runSql(\"drop table if exists sakila.friends;\");\n","Query OK"),
                ("session.runSql(\"show tables like \'friends\';\");\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_9_2(self):
      '''[4.4.009]:2 JS Delete table using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql(\"use sakila;\").execute();\n","Query OK"),
                ("session.sql(\"drop table if exists sakila.friends;\").execute();\n","Query OK"),
                ("session.sql(\"create table sakila.friends (name varchar(50), last_name varchar(50), age integer, gender varchar(20));\").execute();\n","Query OK"),
                ("session.sql(\"show tables like \'friends\';\").execute();\n","1 row in set"),
                ("session.sql(\"INSERT INTO sakila.friends (name, last_name,age,gender) VALUES(\'ruben\',\'morquecho\', "
                 "40,\'male\');\").execute();\n","Query OK"),
                ("session.sql(\"UPDATE sakila.friends SET name=\'ruben dario\' where name =  \'ruben\';\").execute();\n","Query OK"),
                ("session.sql(\"SELECT * from friends where name LIKE \'%ruben%\';\").execute();\n","1 row in set"),
                ("session.sql(\"drop table if exists sakila.friends;\").execute();\n","Query OK"),
                ("session.sql(\"show tables like \'friends\';\").execute();\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_10_1(self):
      '''[4.4.010]:1 JS Delete table using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require('mysql');\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host, LOCALHOST.port),"mysql-js>"),
                ("session.runSql('use sakila;');\n","Query OK"),
                ("session.runSql('drop table if exists sakila.friends;');\n","Query OK"),
                ("session.runSql('create table sakila.friends (name varchar(50), last_name varchar(50), age integer, gender varchar(20));');\n","Query OK"),
                ("session.runSql(\"show tables like 'friends';\");\n","1 row in set"),
                ("session.runSql(\"INSERT INTO sakila.friends (name, last_name,age,gender) VALUES('ruben','morquecho', "
                 "40,'male');\");\n","Query OK"),
                ("session.\n","..."),
                ("runSql(\"UPDATE sakila.friends SET name='ruben dario' where name =  'ruben';\");\n","..."),
                ("\n","mysql-js>"),
                ("session.runSql(\"SELECT * from friends where name LIKE '%ruben%';\");\n","1 row in set"),
                ("session.\n","..."),
                ("runSql(\"drop table if exists sakila.friends;\");\n","..."),
                ("\n","mysql-js>"),
                ("session.runSql(\"show tables like \'friends\';\");\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_10_2(self):
      '''[4.4.010]:2 JS Delete table using multiline modet: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession('{0}:{1}@{2}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql('use sakila;').execute();\n","Query OK"),
                ("session.sql('drop table if exists sakila.friends;').execute();\n","Query OK"),
                ("session.\n","..."),
                ("sql('create table sakila.friends (name varchar(50), last_name varchar(50), age integer, gender varchar(20));').execute();\n","..."),
                ("\n","mysql-js>"),
                ("session.sql(\"show tables like 'friends';\").execute();\n","1 row in set"),
                ("session.sql(\"INSERT INTO sakila.friends (name, last_name,age,gender) VALUES('ruben','morquecho', "
                 "40,'male');\").execute();\n","Query OK"),
                ("session.sql(\"UPDATE sakila.friends SET name='ruben dario' where name =  'ruben';\").execute();\n","Query OK"),
                ("session.\n","..."),
                ("sql(\"SELECT * from friends where name LIKE '%ruben dario%';\").execute();\n","1 row in set"),
                ("\n","mysql-js>"),
                ("session.\n","..."),
                ("sql('drop table if exists sakila.friends;').execute();\n","..."),
                ("\n","mysql-js>"),
                ("session.sql(\"show tables like 'friends';\").execute();\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_11_1(self):
      '''[4.4.011]:1 JS Delete table using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteTable_ClassicMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("select * from sakila.friends where name = 'ruben';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_11_2(self):
      '''[4.4.011]:2 JS Delete table using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteTable_NodeMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("select * from sakila.friends where name = 'ruben';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_12_1(self):
      '''[4.4.012]:1 JS Delete database using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require(\'mysql\');\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql(\"drop database if exists automation_test;\");\n","Query OK"),
                ("session.runSql(\'create database automation_test;\');\n","Query OK"),
                ("session.dropSchema(\'automation_test\');\n","Query OK"),
                ("session.runSql(\"show schemas like \'automation_test\';\");\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_12_2(self):
      '''[4.4.012]:2 JS Delete database using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql(\"drop database if exists automation_test;\").execute();\n","Query OK"),
                ("session.sql(\'create database automation_test;\').execute();\n","Query OK"),
                ("session.dropSchema(\'automation_test\');\n","Query OK"),
                ("session.sql(\"show schemas like \'automation_test\';\").execute();\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_13_1(self):
      '''[4.4.013]:1 JS Delete database using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require('mysql');\n","mysql-js>"),
                ("var session=mysql.getClassicSession('{0}:{1}@{2}:{3}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql('drop database if exists automation_test;');\n","Query OK"),
                ("session.\n","..."),
                ("runSql('create database automation_test;');\n","..."),
                ("\n","Query OK"),
                ("session.\n","..."),
                ("dropSchema('automation_test');\n","..."),
                ("\n","mysql-js>"),
                ("session.runSql(\"show schemas like 'automation_test';\");\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_13_2(self):
      '''[4.4.013]:2 JS Delete database using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession('{0}:{1}@{2}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.\n","..."),
                ("sql('drop database if exists automation_test;').execute();\n","..."),
                ("\n","mysql-js>"),
                ("session.\n","..."),
                ("sql('create database automation_test;').execute();\n","..."),
                ("\n","Query OK"),
                ("session.\n","..."),
                ("dropSchema('automation_test');\n","..."),
                ("\n","mysql-js>"),
                ("session.sql(\"show schemas like 'automation_test';\").execute();\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_14_1(self):
      '''[4.4.014]:1 JS Delete database using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteSchema_ClassicMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show schemas like 'schema_test';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_14_2(self):
      '''[4.4.014]:2 JS Delete database using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteSchema_NodeMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show schemas like 'schema_test';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_15_1(self):
      '''[4.4.015]:1 JS Delete view using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require('mysql');\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql('use sakila;');\n","Query OK"),
                ("session.runSql('drop view if exists js_view;');\n","Query OK"),
                ("session.runSql(\"create view js_view as select first_name from actor where first_name like '%a%';\");\n","Query OK"),
                ("session.getSchema('sakila').getViews();\n","js_view"),
                ("session.dropView('sakila','js_view');\n","Query OK"),
                ("session.runSql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'js_view';\");\n", 'Empty set')                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_15_2(self):
      '''[4.4.015]:2 JS Delete view using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql(\'use sakila;\').execute();\n","Query OK"),
                ("session.sql(\'drop view if exists js_view;\').execute();\n","Query OK"),
                ("session.sql(\"create view js_view as select first_name from actor where first_name like '%a%';\").execute();\n","Query OK"),
                ("session.dropView(\'sakila\',\'js_view\');\n","Query OK"),
                ("session.sql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'js_view';\").execute();\n", 'Empty set')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_16_1(self):
      '''[4.3.016]:1 JS Update alter view using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysql=require('mysql');\n","mysql-js>"),
                ("var session=mysql.getClassicSession('{0}:{1}@{2}:{3}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql('use sakila;');\n","Query OK"),
                ("session.runSql('drop view if exists js_view;');\n","Query OK"),
                ("session.\n","..."),
                ("runSql(\"create view js_view as select first_name from actor where first_name like '%a%';\");\n","..."),
                ("\n","Query OK"),
                ("session.runSql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'js_view';\");\n", '1 row'),
                ("session.\n","..."),
                ("dropView('sakila','js_view');\n","..."),
                ("\n","mysql-js>"),
                ("session.runSql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'js_view';\");\n", 'Empty set')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')



  def test_4_4_16_2(self):
      '''[4.3.016]:2 JS Update alter view using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession('{0}:{1}@{2}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql('use sakila;').execute();\n","Query OK"),
                ("session.sql('drop view if exists js_view;').execute();\n","Query OK"),
                ("session.\n","..."),
                ("sql(\"create view js_view as select first_name from actor where first_name like '%a%';\").execute();\n","..."),
                ("\n","Query OK"),
                ("session.sql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'js_view';\").execute();\n", '1 row'),
                ("session.\n","..."),
                ("dropView(\'sakila\',\'js_view\');\n","..."),
                ("\n","mysql-js>"),
                ("session.sql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'js_view';\").execute();\n", 'Empty set')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_17_1(self):
      '''[4.4.017]:1 JS Delete view using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteView_ClassicMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'js_view';\n","Empty set"),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_17_2(self):
      '''[4.4.017]:2 JS Delete view using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteView_NodeMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'js_view';\n","Empty set"),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_4_18_1(self):
      '''[4.4.018]:1 JS Delete stored procedure using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--js']
      x_cmds = [("var mysql=require('mysql');\n","mysql-js>"),
                ("var session=mysql.getClassicSession('{0}:{1}@{2}:{3}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql('use sakila;');\n","Query OK"),
                ("session.runSql('DROP PROCEDURE IF EXISTS my_procedure;');\n","Query OK"),
                ("session.runSql(\"delimiter //  \");\n","mysql-js>"),
                ("session.runSql(\"create procedure my_procedure (INOUT incr_param INT)\n BEGIN \n SET incr_param = incr_param + 1 ;\nEND//  \");\n","mysql-js>"),
                ("session.runSql(\"delimiter ; \");\n","mysql-js>"),
                ("session.runSql(\"select name from mysql.proc where name like 'my_procedure';\");\n","1 row in set"),
                ("session.runSql(\'DROP PROCEDURE IF EXISTS my_procedure;\');\n","Query OK"),
                ("session.runSql(\"select name from mysql.proc where name like 'my_procedure';\");\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_4_18_2(self):
      '''[4.4.018]:2 JS Delete stored procedure using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql(\'use sakila;\').execute();\n","Query OK"),
                ("session.sql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\').execute();\n","Query OK"),
                ("session.sql(\"delimiter // \").execute();\n","mysql-js>"),
                ("session.sql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND// \").execute();\n","mysql-js>"),
                ("session.sql(\"delimiter ;\").execute();\n","mysql-js>"),
                ("session.sql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\').execute();\n","Query OK"),
                ("session.sql(\"select name from mysql.proc;\").execute();\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_4_19_1(self):
      '''[4.4.019]:1 JS Delete stored procedure using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--js']
      x_cmds = [("var mysql=require(\'mysql\');\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("session.runSql(\'use sakila;\');\n","Query OK"),
                ("session.runSql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\');\n","Query OK"),
                ("session.runSql(\"delimiter // ; \");\n","mysql-js>"),
                ("session.runSql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND// \; \");\n","mysql-js>"),
                ("session.runSql(\"delimiter ;\");\n","mysql-js>"),
                ("\\\n","..."),
                ("session.runSql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\');\n","..."),
                ("\n","mysql-js>"),
                ("session.runSql(\"select name from mysql.proc;\");\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_4_19_2(self):
      '''[4.4.019]:2 JS Delete stored procedure using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-js>"),
                ("session.sql(\'use sakila;\').execute();\n","Query OK"),
                ("session.sql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\').execute();\n","Query OK"),
                ("session.sql(\"delimiter // \").execute();\n","mysql-js>"),
                ("session.sql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND// \").execute();\n","mysql-js>"),
                ("session.sql(\"delimiter ;\").execute();\n","mysql-js>"),
                ("\\\n","..."),
                ("session.sql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\').execute();\n","..."),
                ("\n","mysql-js>"),
                ("session.sql(\"select name from mysql.proc;\").execute();\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  #FAILING........
  #@unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_4_20_1(self):
      '''[4.4.020]:1 JS Delete stored procedure using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteProcedure_ClassicMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("select name from mysql.proc where name like 'my_procedure';\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_4_20_2(self):
      '''[4.4.020]:2 JS Delete stored procedure using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteProcedure_NodeMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("select name from mysql.proc where name like 'my_procedure';\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


      x_cmds = [(";", "mysql-js>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_21_1(self):
      '''[4.4.021]:1 PY Delete table using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import  mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.run_sql(\"use sakila;\")\n", "Query OK"),
                ("session.run_sql(\"drop table if exists sakila.friends;\")\n", "Query OK"),
                (
                "session.run_sql(\"create table sakila.friends (name varchar(50), last_name varchar(50), age integer, gender varchar(20));\")\n",
                "Query OK"),
                ("session.run_sql(\"show tables like \'friends\';\")\n", "1 row in set"),
                (
                "session.run_sql(\"INSERT INTO sakila.friends (name, last_name,age,gender) VALUES(\'ruben\',\'morquecho\', "
                "40,\'male\');\")\n", "Query OK"),
                ("session.run_sql(\"UPDATE sakila.friends SET name=\'ruben dario\' where name =  \'ruben\';\")\n",
                 "Query OK"),
                ("session.run_sql(\"SELECT * from friends where name LIKE \'%ruben%\';\")\n", "1 row in set"),
                ("session.run_sql(\"drop table if exists sakila.friends;\")\n", "Query OK"),
                ("session.run_sql(\"show tables like \'friends\';\")\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_21_2(self):
      '''[4.4.021]:2 PY Delete table using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx;\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                           LOCALHOST.host), "mysql-py>"),
                ("session.sql(\"use sakila;\").execute()\n", "Query OK"),
                ("session.sql(\"drop table if exists sakila.friends;\").execute()\n", "Query OK"),
                (
                "session.sql(\"create table sakila.friends (name varchar(50), last_name varchar(50), age integer, gender varchar(20));\").execute()\n",
                "Query OK"),
                ("session.sql(\"show tables like \'friends\';\").execute()\n", "1 row in set"),
                (
                "session.sql(\"INSERT INTO sakila.friends (name, last_name,age,gender) VALUES(\'ruben\',\'morquecho\', "
                "40,\'male\');\").execute()\n", "Query OK"),
                ("session.sql(\"UPDATE sakila.friends SET name=\'ruben dario\' where name =  \'ruben\';\").execute()\n",
                 "Query OK"),
                ("session.sql(\"SELECT * from friends where name LIKE \'%ruben%\';\").execute()\n", "1 row in set"),
                ("session.sql(\"drop table if exists sakila.friends;\").execute()\n", "Query OK"),
                ("session.sql(\"show tables like \'friends\';\").execute()\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_22_1(self):
      '''[4.4.022]:1 PY Delete table using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session('{0}:{1}@{2}:{3}')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                               LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.run_sql('use sakila;')\n", "Query OK"),
                ("session.run_sql('drop table if exists sakila.friends;')\n", "Query OK"),
                ("session.\\\n", "..."),
                (
                "run_sql('create table sakila.friends (name varchar(50), last_name varchar(50), age integer, gender varchar(20));')\n",
                "..."),
                ("\n", "Query OK"),
                ("session.run_sql(\"show tables like 'friends';\")\n", "1 row in set"),
                ("session.run_sql(\"INSERT INTO sakila.friends (name, last_name,age,gender) VALUES('ruben','morquecho', "
                 "40,'male');\")\n", "Query OK"),
                ("session.run_sql(\"UPDATE sakila.friends SET name='ruben dario' where name =  'ruben';\")\n",
                 "Query OK"),
                ("session.\\\n", "..."),
                ("run_sql(\"SELECT * from friends where name LIKE '%ruben dario%'\");\n", "..."),
                ("\n", "1 row in set"),
                ("session.\\\n", "..."),
                ("run_sql('drop table if exists sakila.friends;')\n", "..."),
                ("\n", "mysql-py>"),
                ("session.run_sql(\"show tables like 'friends';\")\n", "Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_22_2(self):
      '''[4.4.022]:2 PY Delete table using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session('{0}:{1}@{2}')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                         LOCALHOST.host), "mysql-py>"),
                ("session.sql('use sakila;').execute()\n", "Query OK"),
                ("session.sql('drop table if exists sakila.friends;').execute()\n", "Query OK"),
                ("session.\\\n", "..."),
                (
                "sql('create table sakila.friends (name varchar(50), last_name varchar(50), age integer, gender varchar(20));').execute();\\\n",
                "..."),
                ("\n", "mysql-py>"),
                ("session.sql(\"show tables like 'friends';\").execute()\n", "1 row in set"),
                ("session.sql(\"INSERT INTO sakila.friends (name, last_name,age,gender) VALUES('ruben','morquecho', "
                 "40,'male');\").execute()\n", "Query OK"),
                ("session.sql(\"UPDATE sakila.friends SET name='ruben dario' where name =  'ruben';\").execute()\n",
                 "Query OK"),
                ("session.\\\n", "..."),
                ("sql(\"SELECT * from friends where name LIKE '%ruben%';\").execute()\n", "..."),
                ("\n", "1 row in set"),
                ("session.\\\n", "..."),
                ("sql('drop table if exists sakila.friends;').execute()\n", "..."),
                ("\n", "mysql-py>"),
                ("session.sql(\"show tables like 'friends';\").execute()\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_23_1(self):
      '''[4.4.023]:1 PY Delete table using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteTable_ClassicMode.py'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("select * from sakila.friends where name = 'ruben';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_23_2(self):
      '''[4.4.023]:2 PY Delete table using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteTable_NodeMode.js'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("select * from sakila.friends where name = 'ruben';\n","Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_24_1(self):
      '''[4.4.024]:1 PY Delete database using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.run_sql(\"drop database if exists automation_test;\")\n", "Query OK"),
                ("session.run_sql(\'create database automation_test;\')\n", "Query OK"),
                ("session.drop_schema(\'automation_test\')\n", "Query OK"),
                ("session.run_sql(\"show schemas like \'automation_test\';\")\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_24_2(self):
      '''[4.4.024]:2 PY Delete database using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                           LOCALHOST.host), "mysql-py>"),
                ("session.sql(\"drop database if exists automation_test;\").execute()\n", "Query OK"),
                ("session.sql(\'create database automation_test;\').execute()\n", "Query OK"),
                ("session.drop_schema(\'automation_test\')\n", "Query OK"),
                ("session.sql(\"show schemas like \'automation_test\';\").execute()\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_4_25_1(self):
      '''[4.4.025]:1 PY Delete database using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.run_sql(\"drop database if exists automation_test;\")\n", "Query OK"),
                ("session.run_sql(\'create database automation_test;\')\n", "Query OK"),
                ("\\\n", "..."),
                ("session.drop_schema(\'automation_test\')\n", "..."),
                ("\n", "mysql-py>"),
                ("session.run_sql(\"show schemas like \'automation_test\';\")\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_25_2(self):
      '''[4.4.025]:2 PY Delete database using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                           LOCALHOST.host), "mysql-py>"),
                ("session.sql(\"drop database if exists automation_test;\").execute()\n", "Query OK"),
                ("session.sql(\'create database automation_test;\').execute()\n", "Query OK"),
                ("\\\n", "..."),
                ("session.drop_schema(\'automation_test\')\n", "..."),
                ("\n", "mysql-py>"),
                ("session.sql(\"show schemas like \'automation_test\';\").execute()\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  #FAILING........
  #@unittest.skip("issue MYS388 refresh schema after creation on py session")
  def test_4_4_26_1(self):
      '''[4.4.026]:1 PY Delete database using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteSchema_ClassicMode.py'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show schemas like 'schema_test';\n","1 row"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  #@unittest.skip("issue MYS388 refresh schema after creation on py session")
  def test_4_4_26_2(self):
      '''[4.4.026]:2 PY Delete database using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteSchema_NodeMode.py'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show schemas like 'schema_test';\n","1 row"),
                ("DROP DATABASE IF EXISTS schema_test;\n","mysql-sql>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_27_1(self):
      '''[4.4.027]:1 PY Delete view using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.run_sql('use sakila;')\n", "Query OK"),
                ("session.run_sql('drop view if exists py_view;')\n", "Query OK"),
                (
                "session.run_sql(\"create view py_view as select first_name from actor where first_name like '%a%';\")\n",
                "Query OK"),
                ("session.drop_view('sakila','py_view')\n", "Query OK"),
                (
                "session.run_sql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'py_view';\")\n",
                "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_27_2(self):
      '''[4.4.027]:2 PY Delete view using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                           LOCALHOST.host), "mysql-py>"),
                ("session.sql('use sakila;').execute()\n", "Query OK"),
                ("session.sql('drop view if exists py_view;').execute()\n", "Query OK"),
                (
                "session.sql(\"create view py_view as select first_name from actor where first_name like '%a%';\").execute()\n",
                "Query OK"),
                (
                "session.sql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'py_view';\").execute()\n",
                "1 row"),
                ("session.drop_view('sakila','py_view')\n", "Query OK"),
                (
                "session.sql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'py_view';\").execute()\n",
                "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_28_1(self):
      '''[4.4.028]:1 PY Delete view using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.get_classic_session('{0}:{1}@{2}:{3}')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host, LOCALHOST.port),
                 "mysql-py>"),
                ("session.run_sql('use sakila;')\n", "Query OK"),
                ("session.run_sql('drop view if exists py_view;')\n", "Query OK"),
                ("session.\\\n", "..."),
                ("run_sql(\"create view py_view as select first_name from actor where first_name like '%a%';\")\n",
                 "..."),
                ("\n", "Query OK"),
                ("session.\\\n", "..."),
                ("drop_view('sakila','py_view')\n", "..."),
                ("\n", "mysql-py>"),
                (
                    "session.run_sql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'py_view';\")\n",
                    "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_28_2(self):
      '''[4.4.028]:2 PY Delete view using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session('{0}:{1}@{2}')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                           LOCALHOST.host), "mysql-py>"),
                ("session.sql('use sakila;').execute()\n", "Query OK"),
                ("session.sql('drop view if exists py_view;').execute()\n", "Query OK"),
                ("session.\\\n", "..."),
                (
                    "sql(\"create view py_view as select first_name from actor where first_name like '%a%';\").execute()\n",
                    "..."),
                ("\n", "Query OK"),
                (
                    "session.sql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'py_view';\")\n",
                    "1 row in set"),
                ("session.\\\n", "..."),
                ("drop_view('sakila','py_view').execute()\n", "..."),
                ("\n", "mysql-py>"),
                (
                    "session.sql(\"SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'py_view';\")\n",
                    "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_29_1(self):
      '''[4.4.029]:1 PY Delete view using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila', '--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           stdin=open(Exec_files_location + 'DeleteView_ClassicMode.py'))
      stdin, stdout = p.communicate()
      if stdout.find(bytearray("ERROR", "ascii"), 0, len(stdin)) > -1:
          self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n", "mysql-sql>"),
                ("use sakila;\n", "mysql-sql>"),
                (
                "SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'py_view';\n",
                "Empty set"),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_29_2(self):
      '''[4.4.029]:2 PY Delete view using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila', '--node']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           stdin=open(Exec_files_location + 'DeleteView_NodeMode.py'))
      stdin, stdout = p.communicate()
      if stdout.find(bytearray("ERROR", "ascii"), 0, len(stdin)) > -1:
          self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n", "mysql-sql>"),
                ("use sakila;\n", "mysql-sql>"),
                (
                "SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'py_view';\n",
                "Empty set"),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_4_29_3(self):
      '''[4.4.029]:2 PY Delete view using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila', '--node', '--file=' + Exec_files_location + 'DeleteView_NodeMode.py']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      if stdout.find(bytearray("ERROR", "ascii"), 0, len(stdin)) > -1:
          self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n", "mysql-sql>"),
                ("use sakila;\n", "mysql-sql>"),
                (
                "SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'py_view';\n",
                "Empty set"),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  # def test_big_data(self):
  #     '''[4.4.029]:2 PY Delete view using STDIN batch code: NODE SESSION'''
  #     results = ''
  #     init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
  #                     '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
  #                     '--schema=sakila','--classic','--file='+Exec_files_location + 'BigCreate_Classic.py']
  #     p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
  #     stdin,stdout = p.communicate()
  #     if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
  #       self.assertEqual(stdin, 'PASS')
  #     results = ''
  #     init_command = [MYSQL_SHELL, '--interactive=full']
  #     x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
  #               ("\\sql\n","mysql-sql>"),
  #               ("use world_x;\n","mysql-sql>"),
  #               #("CREATE TABLE big_data_classic_py ( id INT NOT NULL AUTO_INCREMENT, stringCol VARCHAR(45) NOT NULL, datetimeCol DATETIME NOT NULL, blobCol BLOB NOT NULL, geometryCol GEOMETRY NOT NULL, PRIMARY KEY (id));"),
  #               ("show tables;\n","mysql-sql>"),
  #               ("select count(*) from world_x.big_data_classic_py;;\n","rows in set"),
  #
  #               ]
  #     results = exec_xshell_commands(init_command, x_cmds)
  #     self.assertEqual(results, 'PASS')


  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in py is not recongnized")
  def test_4_4_30_1(self):
      '''[4.4.030]:1 PY Delete stored procedure using session object: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import  mysql\n","mysql-py>"),
                ("session=mysql.getClassicSession('{0}:{1}@{2}:{3}')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-py>"),
                ("session.runSql('use sakila;')\n","Query OK"),
                ("session.runSql('DROP PROCEDURE IF EXISTS my_automated_procedure;')\n","Query OK"),
                ("session.runSql('delimiter // ; ')\n","mysql-py>"),
                ("session.runSql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n BEGIN \n    SET incr_param = incr_param + 1 ;\nEND// ; \")\n","mysql-py>"),
                ("session.runSql('delimiter ;')\n","mysql-py>"),
                ("session.runSql('DROP PROCEDURE IF EXISTS my_automated_procedure;')\n","Query OK"),
                ("session.runSql(\'select name from mysql.proc\');\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_4_30_2(self):
      '''[4.4.030]:2 PY Delete stored procedure using session object: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysqlx\n","mysql-py>"),
                ("session=mysqlx.getNodeSession(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-py>"),
                ("session.sql(\'use sakila;\').execute()\n","Query OK"),
                ("session.sql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\').execute()\n","Query OK"),
                ("session.sql(\"delimiter // \").execute()\n","mysql-py>"),
                ("session.sql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND// \").execute();\n","mysql-py>"),
                ("session.sql(\"delimiter ;\").execute()\n","mysql-py>"),
                ("session.sql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\').execute()\n","Query OK"),
                ("session.sql(\"select name from mysql.proc;\").execute()\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_4_31_1(self):
      '''[4.4.031]:1 PY Delete stored procedure using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysql\n","mysql-py>"),
                ("session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                      LOCALHOST.host, LOCALHOST.port), "mysql-py>"),
                ("session.runSql(\'use sakila;\')\n","Query OK"),
                ("session.runSql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\')\n","Query OK"),
                ("session.runSql(\"delimiter //  \");\n","mysql-py>"),
                ("session.runSql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND// \")\n","mysql-py>"),
                ("session.runSql(\"delimiter ;\")\n","mysql-py>"),
                ("\\\n","..."),
                ("session.runSql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\')\n","..."),
                ("\n","mysql-py>"),
                ("session.runSql(\"select name from mysql.proc;\")\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_4_31_2(self):
      '''[4.4.031]:2 PY Delete stored procedure using multiline mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysqlx\n","mysql-py>"),
                ("session=mysqlx.getNodeSession(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host),"mysql-py>"),
                ("session.sql(\'use sakila;\').execute()\n","Query OK"),
                ("session.sql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\').execute();\n","Query OK"),
                ("session.sql(\"delimiter // \").execute()\n","mysql-py>"),
                ("session.sql(\"create procedure my_automated_procedure (INOUT incr_param INT)\n "
                 "BEGIN \n    SET incr_param = incr_param + 1 ;\nEND// \").execute()\n","mysql-py>"),
                ("session.sql(\"delimiter ;\").execute()\n","mysql-py>"),
                ("\\\n","..."),
                ("session.sql(\'DROP PROCEDURE IF EXISTS my_automated_procedure;\').execute()\n","..."),
                ("\n","mysql-py>"),
                ("session.sql(\"select name from mysql.proc;\").execute()\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_4_32_1(self):
      '''[4.4.032]:1 PY Delete stored procedure using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--classic']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteProcedure_ClassicMode.py'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'my_procedure';\n","Empty set"),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_4_4_32_2(self):
      '''[4.4.032]:2 PY Delete stored procedure using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location + 'DeleteProcedure_NodeMode.py'))
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("SELECT table_name FROM information_schema.views WHERE information_schema.views.table_name LIKE 'my_procedure';\n","Empty set"),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_5_01_1(self):
      '''[4.5.001]:1 JS Transaction with Rollback: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic','--schema=sakila', '--js']
      x_cmds = [("session.startTransaction();\n", "Query OK"),
                ("session.runSql(\'select * from sakila.actor where actor_ID = 2;\');\n","1 row"),
                ("session.runSql(\"update sakila.actor set first_name = \'Updated45011\' where actor_ID = 2;\");\n","Query OK"),
                ("session.runSql(\"select * from sakila.actor where first_name = \'Updated45011\';\");\n","1 row"),
                ("session.rollback();\n", "Query OK"),
                ("session.runSql(\"select * from sakila.actor where first_name = \'Updated45011\';\");\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_5_01_2(self):
      '''[4.5.001]:2 JS Transaction with Rollback: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--js']
      x_cmds = [("session.startTransaction();\n", "Query OK"),
                ("session.sql(\'select * from sakila.actor where actor_ID = 2;\').execute();\n","1 row"),
                ("session.sql(\"update sakila.actor set first_name = \'Updated45012\' where actor_ID = 2;\").execute();\n","Query OK"),
                ("session.sql(\"select * from sakila.actor where first_name = \'Updated45012\';\").execute();\n","1 row"),
                ("session.rollback();\n", "Query OK"),
                ("session.sql(\"select * from sakila.actor where first_name = \'Updated45012\';\").execute();\n","Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_5_02_1(self):
      '''[4.5.002]:1 PY Transaction with Rollback: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--schema=sakila', '--py']
      x_cmds = [("session.start_transaction()\n", "Query OK"),
                ("session.run_sql(\'select * from sakila.actor where actor_ID = 2;\')\n", "1 row"),
                ("session.run_sql(\"update sakila.actor set first_name = \'Updated\' where actor_ID = 2;\")\n",
                 "Query OK"),
                ("session.run_sql(\"select * from sakila.actor where first_name = \'Updated\';\")\n", "1 row"),
                ("session.rollback()\n", "Query OK"),
                ("session.run_sql(\"select * from sakila.actor where first_name = \'Updated\';\")\n", "Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_5_02_2(self):
      '''[4.5.002]:2 PY Transaction with Rollback: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--schema=sakila', '--py']
      x_cmds = [("session.start_transaction()\n", "Query OK"),
                ("session.sql(\'select * from sakila.actor where actor_ID = 2;\').execute()\n", "1 row"),
                ("session.sql(\"update sakila.actor set first_name = \'Updated\' where actor_ID = 2;\").execute()\n",
                 "Query OK"),
                ("session.sql(\"select * from sakila.actor where first_name = \'Updated\';\").execute()\n", "1 row"),
                ("session.rollback()\n", "Query OK"),
                ("session.sql(\"select * from sakila.actor where first_name = \'Updated\';\").execute()\n", "Empty set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_5_03_1(self):
      '''[4.5.003]:1 JS Transaction with Commit: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic','--schema=sakila', '--js']
      x_cmds = [("session.startTransaction();\n", "Query OK"),
                ("session.runSql(\'select * from sakila.actor where actor_ID = 2;\');\n","1 row"),
                ("session.runSql(\"update sakila.actor set first_name = \'Updated45031\' where actor_ID = 2;\");\n","Query OK"),
                ("session.runSql(\"select * from sakila.actor where first_name = \'Updated45031\';\");\n","1 row"),
                ("session.commit();\n", "Query OK"),
                ("session.runSql(\"select * from sakila.actor where first_name = \'Updated45031\';\");\n","1 row")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_5_03_2(self):
      '''[4.5.003]:2 JS Transaction with Commit: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--js']
      x_cmds = [("session.startTransaction();\n", "Query OK"),
                ("session.sql(\'select * from sakila.actor where actor_ID = 2;\').execute();\n","1 row"),
                ("session.sql(\"update sakila.actor set first_name = \'Updated45032\' where actor_ID = 2;\").execute();\n","Query OK"),
                ("session.sql(\"select * from sakila.actor where first_name = \'Updated45032\';\").execute();\n","1 row"),
                ("session.commit();\n", "Query OK"),
                ("session.sql(\"select * from sakila.actor where first_name = \'Updated45032\';\").execute();\n","1 row")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_5_04_1(self):
      '''[4.5.004]:1 PY Transaction with Commit: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--schema=sakila', '--py']
      x_cmds = [("session.start_transaction()\n", "Query OK"),
                ("session.run_sql(\'select * from sakila.actor where actor_ID = 2;\')\n", "1 row"),
                ("session.run_sql(\"update sakila.actor set first_name = \'Updated45041\' where actor_ID = 2;\")\n",
                 "Query OK"),
                ("session.run_sql(\"select * from sakila.actor where first_name = \'Updated45041\';\")\n", "1 row"),
                ("session.commit()\n", "Query OK"),
                ("session.run_sql(\"select * from sakila.actor where first_name = \'Updated45041\';\")\n", "1 row")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_5_04_2(self):
      '''[4.5.004]:2 PY Transaction with Commit: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--schema=sakila', '--py']
      x_cmds = [("session.start_transaction()\n", "Query OK"),
                ("session.sql(\'select * from sakila.actor where actor_ID = 2;\').execute()\n", "1 row"),
                (
                "session.sql(\"update sakila.actor set first_name = \'Updated45042\' where actor_ID = 2;\").execute()\n",
                "Query OK"),
                ("session.sql(\"select * from sakila.actor where first_name = \'Updated45042\';\").execute()\n",
                 "1 row"),
                ("session.commit()\n", "Query OK"),
                (
                "session.sql(\"select * from sakila.actor where first_name = \'Updated45042\';\").execute()\n", "1 row")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_6_01_1(self):
      '''[4.6.001]:1 Create a collection with node session: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--js']

      x_cmds = [("session.dropCollection(\"sakila\",\"test_collection_js\");\n", "mysql-js>"),
                ("session.getSchema(\'sakila\').createCollection(\"test_collection_js\");\n", "mysql-js>"),
                ("session.getSchema(\'sakila\').getCollection(\"test_collection_js\")\n","<Collection:test_collection_js"),
                ("\\py\n","mysql-py>"),
                ("session.drop_collection(\"sakila\",\"test_collection_py\")\n", "mysql-py>"),
                ("session.get_schema(\'sakila\').create_collection(\"test_collection_py\")\n", "mysql-py>"),
                ("session.get_schema(\'sakila\').get_collection(\"test_collection_py\")\n","<Collection:test_collection_py"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_6_02_1(self):
      '''[4.6.002]:1 JS PY Ensure collection exists in a database with node session: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--js']

      x_cmds = [("session.dropCollection(\"sakila\",\"test_collection_js\");\n", "mysql-js>"),
                ("session.getSchema(\'sakila\').createCollection(\"test_collection_js\");\n", "mysql-js>"),
                ("session.getSchema(\'sakila\').getCollection(\"test_collection_js\").existsInDatabase()\n","true"),
                ("\\py\n","mysql-py>"),
                ("session.drop_collection(\"sakila\",\"test_collection_py\")\n", "mysql-py>"),
                ("session.get_schema(\'sakila\').create_collection(\"test_collection_py\")\n", "mysql-py>"),
                ("session.get_schema(\'sakila\').get_collection(\"test_collection_py\").exists_in_database()\n","true"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_6_03_1(self):
      '''[4.6.003]:1 JS PY Add Documents to a collection with node session: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--js']

      x_cmds = [("session.dropCollection('sakila','test_collection_js');\n", "mysql-js>"),
                  ("session.getSchema('sakila').createCollection('test_collection_js');\n", "mysql-js>"),
                ("session.getSchema('sakila').getCollection('test_collection_js').existsInDatabase();\n","true"),
                ("var myColl = session.getSchema('sakila').getCollection('test_collection_js');\n","mysql-js>"),
                ("myColl.add({ name: 'Test1', lastname:'lastname1'});\n","Query OK"),
                ("myColl.add({ name: 'Test2', lastname:'lastname2'});\n","Query OK"),
                ("session.getSchema('sakila').getCollectionAsTable('test_collection_js').select();\n","2 rows"),
                ("\\py\n","mysql-py>"),
                ("session.drop_collection(\"sakila\",\"test_collection_py\")\n", "mysql-py>"),
                ("session.get_schema(\'sakila\').create_collection(\"test_collection_py\")\n", "mysql-py>"),
                ("session.get_schema('sakila').get_collection(\"test_collection_py\").exists_in_database()\n","true"),
                ("myColl2 = session.get_schema(\'sakila\').get_collection(\"test_collection_py\")\n","mysql-py>"),
                ("myColl2.add([{ \"name\": \"TestPy2\", \"lastname\":\"lastnamePy2\"},{ \"name\": \"TestPy3\", \"lastname\":\"lastnamePy3\"}])\n","Query OK"),
                ("myColl2.add({ \"name\": \'TestPy1\', \"lastname\":\'lastnamePy1\'})\n","Query OK"),
                ("session.get_schema(\'sakila\').get_collection_as_table(\'test_collection_py\').select()\n","3 rows"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_6_04_1(self):
      '''[4.6.004] JS PY Find documents from Database using node session: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--js']

      x_cmds = [("session.getSchema('world_x').getCollection('countryinfo').existsInDatabase();\n","true"),
                ("var myColl = session.getSchema(\'world_x\').getCollection(\"countryinfo\");\n","mysql-js>"),
                ("myColl.find(\"Name = \'Mexico\'\").fields([\'_id\', \'Name\',\'geography.Region\',\'geography.Continent\']);\n","1 document"),
                ("myColl.find(\"geography.Region = \'Central America\'\").fields([\'_id\', \'Name\','geography.Region\',\'geography.Continent\']).limit(4);\n","4 documents"),
                ("\\py\n","mysql-py>"),
                ("myColl2 = session.get_schema(\'world_x\').get_collection(\"countryinfo\")\n","mysql-py>"),
                ("myColl2.find(\"Name = \'Mexico\'\").fields([\'_id\', \'Name\',\'geography.Region\',\'geography.Continent\'])\n","1 document"),
                ("myColl2.find(\"geography.Region = \'Central America\'\").fields([\'_id\', \'Name\','geography.Region\',\'geography.Continent\']).limit(4)\n","4 documents"),
                    ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_6_05_1(self):
      '''[4.6.005] JS Modify document with Set and Unset with node session: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--js']

      x_cmds = [("session.getSchema(\'world_x\').getCollection(\"countryinfo\").existsInDatabase();\n","true"),
                ("var myColl = session.getSchema(\'world_x\').getCollection(\"countryinfo\");\n","mysql-js>"),
                ("myColl.modify(\"Name = :country\").set(\'Soccer_World_Championships\',\'3\').bind(\'country\',\'Argentina\');\n","Query OK, 1 item affected"),
                ("myColl.modify(\"Name = :country\").unset(\'Soccer_World_Championships\').bind(\'country\',\'Argentina\');\n","Query OK, 1 item affected"),
                # ("\\py\n","mysql-py>"),
                # ("myColl2 = session.getSchema(\'world_x\').getCollection(\"countryinfo\")\n","mysql-py>"),
                # ("myColl2.modify(\"Name = :country\").set(\'Soccer_World_Championships\',\'6\').bind(\'country\',\'Argentina\')\n","Query OK, 1 item affected"),
                # ("myColl2.modify(\"Name = :country\").unset(\'Soccer_World_Championships\').bind(\'country\',\'Argentina\')\n","Query OK, 1 item affected"),
                    ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_6_06_1(self):
      '''[4.6.006] JS Modify document with Merge and Array with node session: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--js']

      x_cmds = [("session.dropCollection(\"sakila\",\"test_merge_js\");\n", "mysql-js>"),
                ("session.getSchema(\'sakila\').createCollection(\"test_merge_js\");\n", "mysql-js>"),
                ("session.getSchema(\'sakila\').getCollection(\"test_merge_js\").existsInDatabase();\n","true"),
                ("var myColl = session.getSchema(\'sakila\').getCollection(\"test_merge_js\");\n","mysql-js>"),
                ("myColl.add({ nombre: \'Test1\', apellido:\'lastname1\'});\n","Query OK"),
                ("myColl.add({ nombre: \'Test2\', apellido:\'lastname2\'});\n","Query OK"),
                ("myColl.modify().merge({idioma: \'spanish\'}).execute();\n","Query OK, 2 items affected"),
                ("myColl.modify(\'nombre =: Name\').arrayAppend(\'apellido\', 'aburto').bind(\'Name\',\'Test1\');\n","Query OK, 1 item affected"),
                # ----------------------------------------------------------------
                # ("\\py\n","mysql-py>"),
                # ("session.dropCollection(\"sakila\",\"test_merge_py\")\n", "mysql-py>"),
                # ("session.getSchema(\'sakila\').createCollection(\"test_merge_py\")\n", "mysql-py>"),
                # ("session.getSchema(\'sakila\').getCollection(\"test_merge_py\").existsInDatabase()\n","true"),
                # ("myColl2 = session.getSchema(\'sakila\').getCollection(\"test_merge_py\")\n","mysql-py>"),
                # ("myColl2.add([{ \"nombre\": \"TestPy2\", \"apellido\":\"lastnamePy2\"},{ \"nombre\": \"TestPy3\", \"apellido\":\"lastnamePy3\"}])\n","Query OK"),
                # ("myColl2.modify().merge({\'idioma\': \'spanish\'}).execute()\n","Query OK, 2 items affected"),
                # ("myColl2.modify(\'nombre =: Name\').arrayAppend(\'apellido\', 'aburto').bind(\'Name\',\'TestPy2\')\n","Query OK, 1 item affected"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_6_07_1(self):
      '''[4.6.007] PY Modify document with Set and Unset with node session: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--js']

      x_cmds = [("session.getSchema(\'world_x\').getCollection(\"countryinfo\").existsInDatabase();\n","true"),
                # ("var myColl = session.getSchema(\'world_x\').getCollection(\"countryinfo\");\n","mysql-js>"),
                # ("myColl.modify(\"Name = :country\").set(\'Soccer_World_Championships\',\'3\').bind(\'country\',\'Argentina\');\n","Query OK, 1 item affected"),
                # ("myColl.modify(\"Name = :country\").unset(\'Soccer_World_Championships\').bind(\'country\',\'Argentina\');\n","Query OK, 1 item affected"),
                ("\\py\n","mysql-py>"),
                ("myColl2 = session.get_schema(\'world_x\').get_collection(\"countryinfo\")\n","mysql-py>"),
                ("myColl2.modify(\"Name = :country\").set(\'Soccer_World_Championships\',\'6\').bind(\'country\',\'Argentina\')\n","Query OK, 1 item affected"),
                ("myColl2.modify(\"Name = :country\").unset(\'Soccer_World_Championships\').bind(\'country\',\'Argentina\')\n","Query OK, 1 item affected"),
                    ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_6_08_1(self):
      '''[4.6.008] PY Modify document with Merge and Array with node session: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--schema=sakila', '--js']

      x_cmds = [  # ("session.drop_collection(\"sakila\",\"test_merge_js\");\n", "mysql-js>"),
          #           ("session.get_schema(\'sakila\').create_collection(\"test_merge_js\");\n", "mysql-js>"),
          #           ("session.get_schema(\'sakila\').get_collection(\"test_merge_js\").exists_in_database();\n","true"),
          #           ("var myColl = session.get_schema(\'sakila\').get_collection(\"test_merge_js\");\n","mysql-js>"),
          #           ("myColl.add({ nombre: \'Test1\', apellido:\'lastname1\'});\n","Query OK"),
          #           ("myColl.add({ nombre: \'Test2\', apellido:\'lastname2\'});\n","Query OK"),
          #           ("myColl.modify().merge({idioma: \'spanish\'}).execute();\n","Query OK, 2 items affected"),
          #           ("myColl.modify(\'nombre =: Name\').array_append(\'apellido\', 'aburto').bind(\'Name\',\'Test1\');\n","Query OK, 1 item affected"),
          # ----------------------------------------------------------------
          ("\\py\n", "mysql-py>"),
          ("session.drop_collection(\"sakila\",\"test_merge_py\")\n", "mysql-py>"),
          ("session.get_schema(\'sakila\').create_collection(\"test_merge_py\")\n", "mysql-py>"),
          ("session.get_schema(\'sakila\').get_collection(\"test_merge_py\").exists_in_database()\n", "true"),
          ("myColl2 = session.get_schema(\'sakila\').get_collection(\"test_merge_py\")\n", "mysql-py>"),
          (
          "myColl2.add([{ \"nombre\": \"TestPy2\", \"apellido\":\"lastnamePy2\"},{ \"nombre\": \"TestPy3\", \"apellido\":\"lastnamePy3\"}])\n",
          "Query OK"),
          ("myColl2.modify().merge({\'idioma\': \'spanish\'}).execute()\n", "Query OK, 2 items affected"),
          ("myColl2.modify(\'nombre =: Name\').array_append(\'apellido\', 'aburto').bind(\'Name\',\'TestPy2\')\n",
           "Query OK, 1 item affected"),
      ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_7_01_1(self):
      '''[4.7.001]   Retrieve with Table Output Format with classic session: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic','--schema=sakila', '--sqlc','--table']

      x_cmds = [("select actor_id from actor limit 5;\n","| actor_id |"),
                ]

      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_7_03_1(self):
      '''[4.7.003] Retrieve with JSON raw Format with classic session: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic','--schema=sakila', '--sqlc', '--json=raw']

      x_cmds = [("select actor_id from actor limit 5;\n","\"rows\":[{\"actor_id\":58}"),
                ]

      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_7_03_2(self):
      '''[4.7.003] Retrieve with JSON raw Format with classic session: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic','--schema=sakila', '--sqlc', '--json=pretty']

      x_cmds = [("select actor_id from actor limit 5;\n","\"rows\": [" + os.linesep + ""),
                ]

      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_7_04_1(self):
      '''[4.7.001]   Retrieve with Table Output Format with classic session: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--sql','--table']

      x_cmds = [("select actor_id from actor limit 5;\n","| actor_id |"),
                ]

      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_7_06_1(self):
      '''[4.7.003] Retrieve with JSON raw Format with classic session: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--sql', '--json=raw']

      x_cmds = [("select actor_id from actor limit 5;\n","\"rows\":[{\"actor_id\":58}"),
                ]

      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_7_06_2(self):
      '''[4.7.003] Retrieve with JSON raw Format with classic session: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--sql', '--json=pretty']

      x_cmds = [("select actor_id from actor limit 5;\n","\"rows\": [" + os.linesep + ""),
                ]

      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_7_07_1(self):
      '''[4.7.003] Retrieve with JSON raw Format with classic session: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var mySession = mysqlx.getSession('"+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+"');\n","mysql-js>"),
                ("mySession;\n","<XSession:"+LOCALHOST.user+"@"+LOCALHOST.host+""),
                ("var result = mySession.getSchema('world_x').getCollection('Countryinfo').find().execute();\n","mysql-js>"),
                ("var record = result.fetchOne();\n","mysql-js>"),
                ("print(record);\n","\"government\": {" + os.linesep + ""),
                ]

      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_7_07_2(self):
      '''[4.7.003] Retrieve with JSON raw Format with classic session: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var mySession = mysqlx.getSession('"+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+"');\n","mysql-js>"),
                ("var result = mySession.getSchema('world_x').getCollection('Countryinfo').find().execute();\n","mysql-js>"),
                ("var record = result.fetchAll();\n","mysql-js>"),
                ("print(record);\n","IndepYear\":"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  # #FAILING........
  # @unittest.skip("eho not being succeed in code script")
  # def test_4_10_01_01(self):
  #     '''[3.1.009]:3 Check that STATUS command [ \status, \s ] works: node session \status'''
  #     results = ''
  #     init_command = [MYSQL_SHELL, '--interactive=full', '--classic','--schema=sakila',
  #                     '--sqlc','--uri', '{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,REMOTEHOST.port) ]
  #     cmd_echo = subprocess.Popen(['echo','select * from sakila.actor limit 3;'], stdout=subprocess.PIPE, shell=True)
  #     #p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin =os.system('echo select * from sakila.actor limit 3;'))
  #     p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin =cmd_echo.stdout )
  #     #p.stdin.flush()
  #     cmd_echo.stdout.close()
  #     stdin,stdout = p.communicate()
  #     if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
  #       self.assertEqual(stdin, 'PASS')
  #     if stdin.find(bytearray("Creating a Node Session to","ascii"),0,len(stdin))> -1 and stdin.find(bytearray("mysql-sql>","ascii"),0,len(stdin))> -1:
  #       results = 'PASS'
  #     self.assertEqual(results, 'PASS')



  def test_4_9_01_1(self):
      '''[4.9.002] Create a Stored Session'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n","mysql-js>"),
                ("shell.storedSessions.add('classic_session', '"+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"');\n","mysql-js>"),
                ("shell.storedSessions;\n","    \"classic_session\": {" + os.linesep + "        \"dbPassword\": \"**********\", " + os.linesep + "        \"dbUser\": \""+LOCALHOST.user+"\", " + os.linesep + "        \"host\": \""+LOCALHOST.host+"\", " + os.linesep + "        \"port\": "+LOCALHOST.port+"" + os.linesep + "    }"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')



  def test_4_9_01_2(self):
      '''[4.9.002] Create a Stored Session: store port'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn node_session\n","mysql-js>"),
                ("shell.storedSessions.add('node_session', '"+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.xprotocol_port+"/sakila');\n","mysql-js>"),
                ("shell.storedSessions;\n","    \"node_session\": {" + os.linesep + "        \"dbPassword\": \"**********\", " + os.linesep + "        \"dbUser\": \""+LOCALHOST.user+"\", " + os.linesep + "        \"host\": \""+LOCALHOST.host+"\", " + os.linesep + "        \"port\": "+LOCALHOST.xprotocol_port+", " + os.linesep + "        \"schema\": \"sakila\"" + os.linesep + "    }"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_9_01_3(self):
      '''[4.9.002] Create a Stored Session: schema store'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n","mysql-js>"),
                ("shell.storedSessions.add('classic_session', '"+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila');\n","mysql-js>"),
                ("shell.storedSessions;\n","    \"classic_session\": {" + os.linesep + "        \"dbPassword\": \"**********\", " + os.linesep + "        \"dbUser\": \""+LOCALHOST.user+"\", " + os.linesep + "        \"host\": \""+LOCALHOST.host+"\", " + os.linesep + "        \"port\": "+LOCALHOST.port+", " + os.linesep + "        \"schema\": \"sakila\"" + os.linesep + "    }"),
                ("\\connect -c $classic_session\n","Creating a Classic Session to "+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_9_01_4(self):
      '''[4.9.002] Create a Stored Session: schema store'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn app_session\n","mysql-js>"),
                ("shell.storedSessions.add('app_session', '"+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.xprotocol_port+"/sakila');\n","mysql-js>"),
                ("\\connect $app_session\n","Creating an X Session to "+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.xprotocol_port+"/sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_9_01_5(self):
      '''[4.9.002] Create a Stored Session: using saveconn '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n","mysql-js>"),
                ("\\saveconn classic_session "+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila\n","mysql-js>"),
                ("shell.storedSessions;\n","    \"classic_session\": {" + os.linesep + "        \"dbPassword\": \"**********\", " + os.linesep + "        \"dbUser\": \""+LOCALHOST.user+"\", " + os.linesep + "        \"host\": \""+LOCALHOST.host+"\", " + os.linesep + "        \"port\": "+LOCALHOST.port+", " + os.linesep + "        \"schema\": \"sakila\"" + os.linesep + "    }"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_9_02_1(self):
      '''[4.9.002] Update a Stored Session: using savec -f to override current value  '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n", "mysql-js>"),
                (
                "\\savec classic_session " + LOCALHOST.user + ":" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.port + "/sakila\n",
                "mysql-js>"),
                ("shell.storedSessions;\n", "\"classic_session\": {" + os.linesep + ""),
                (
                "\\savec -f classic_session dummy:" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.port + "/sakila\n",
                "mysql-js>"),
                ("shell.storedSessions;\n", "\"dbUser\": \"dummy\", " + os.linesep + ""),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_9_02_2(self):
      '''[4.9.002] Update a Stored Session: using saveconn '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n", "mysql-js>"),
                (
                "\\saveconn classic_session " + LOCALHOST.user + ":" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.port + "/sakila\n",
                "mysql-js>"),
                ("shell.storedSessions;\n", "\"classic_session\": {" + os.linesep + ""),
                (
                "shell.storedSessions.update(\"classic_session\", \"dummy:" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.port + "/sakila\")\n",
                "mysql-js>"),
                ("shell.storedSessions;\n", "\"dbUser\": \"dummy\", " + os.linesep + ""),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_9_03_1(self):
      '''[4.9.002] remove a Stored Session: using savec'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n", "mysql-js>"),
                (
                "\\savec classic_session " + LOCALHOST.user + ":" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.port + "/sakila\n",
                "mysql-js>"),
                ("shell.storedSessions;\n", "\"classic_session\": {" + os.linesep + ""),
                ("\\rmconn classic_session\n", "mysql-js>"),
                (
                "shell.storedSessions.update(\"classic_session\", \"dummy:" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.port + "/sakila\")\n",
                "does not exist"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_9_03_2(self):
      '''[4.9.002] remove a Stored Session: using saveconn'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n", "mysql-js>"),
                (
                "\\saveconn classic_session " + LOCALHOST.user + ":" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.port + "/sakila\n",
                "mysql-js>"),
                ("shell.storedSessions;\n", "\"classic_session\": {" + os.linesep + ""),
                ("shell.storedSessions.remove(\"classic_session\");", "true"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_4_9_04_1(self):
      '''[4.9.002] remove a Stored Session: using savec '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n", "mysql-js>"),
                ("\\savec classic_session " + LOCALHOST.user + ":" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.port + "/sakila\n",
                "mysql-js>"),
                ("shell.storedSessions;\n", "\"classic_session\": {" + os.linesep + ""),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #@unittest.skip("Format displayed incorrectly related to bug https://jira.oraclecorp.com/jira/browse/MYS-538")
  def test_4_9_04_2(self):
      '''[4.9.002] remove a Stored Session: using saveconn '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n","mysql-js>"),
                ("\\rmconn classic_session1\n", "mysql-js>"),
                ("\\saveconn classic_session "+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila\n","mysql-js>"),
                ("\\saveconn classic_session1 "+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila\n","mysql-js>"),
                ("\\lsc\n","classic_session : "+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila" + os.linesep + "classic_session1"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #@unittest.skip("Format displayed incorrectly related to bug https://jira.oraclecorp.com/jira/browse/MYS-538")
  def test_4_9_04_3(self):
      '''[4.9.002] remove a Stored Session: using savec '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\rmconn classic_session\n", "mysql-js>"),
                ("\\rmconn classic_session1\n", "mysql-js>"),
                ("\\savec classic_session " + LOCALHOST.user + ":" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.port + "/sakila\n","mysql-js>"),
                ("\\savec classic_session1 " + LOCALHOST.user + ":" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.port + "/sakila\n","mysql-js>"),
                ("\\lsconn\n","classic_session : " + LOCALHOST.user + "@" + LOCALHOST.host + ":" + LOCALHOST.port + "/sakila" + os.linesep + "classic_session1"),
                ]

  def test_examples_1_1(self):
      '''[2.0.07]:3 Connect local Server on PY mode: APP SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_session('" + LOCALHOST.user + ":" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.xprotocol_port + "')\n",
                 "mysql-py>"),
                ("myTable = session.get_schema('sakila').get_table('actor')\n", "mysql-py>"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute()\n", "Query OK"),
                (
                "myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute()\n",
                "mysql-py>"),
                (
                "myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute()\n",
                "mysql-py>"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute()\n", "2 rows in set"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute()\n", "Query OK"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute()\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_examples_1_2(self):
      '''[2.0.07]:3 Connect local Server on PY mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session('" + LOCALHOST.user + ":" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.xprotocol_port + "')\n",
                 "mysql-py>"),
                ("myTable = session.get_schema('sakila').get_table('actor')\n", "mysql-py>"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute()\n", "Query OK"),
                (
                "myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute()\n",
                "mysql-py>"),
                (
                "myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute()\n",
                "mysql-py>"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute()\n", "2 rows in set"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute()\n", "Query OK"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute()\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_examples_1_3(self):
      '''[2.0.07]:3 Connect local Server on PY mode: APP SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_session('" + LOCALHOST.user + ":" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.xprotocol_port + "')\n",
                 "mysql-py>"),
                ("myTable = session.get_schema('sakila').get_table('actor')\n", "mysql-py>"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute()\n", "Query OK"),
                (
                "myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute()\n",
                "mysql-py>"),
                (
                "myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute()\n",
                "mysql-py>"),
                ("myTable.select().where('first_name like : name').bind('name','testFN').execute()\n", "2 rows in set"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute()\n", "Query OK"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute()\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_examples_1_4(self):
      '''[2.0.07]:3 Connect local Server on PY mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session('" + LOCALHOST.user + ":" + LOCALHOST.password + "@" + LOCALHOST.host + ":" + LOCALHOST.xprotocol_port + "')\n",
                 "mysql-py>"),
                ("myTable = session.get_schema('sakila').get_table('actor')\n", "mysql-py>"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute()\n", "Query OK"),
                (
                "myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute()\n",
                "mysql-py>"),
                (
                "myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute()\n",
                "mysql-py>"),
                ("myTable.select().where('first_name like : name').bind('name','testFN').execute()\n", "2 rows in set"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute()\n", "Query OK"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute()\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_examples_1_5(self):
      '''[2.0.07]:3 Connect local Server on JS mode: APP SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--js']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getSession({'host': '" + LOCALHOST.host + "', 'dbUser': '"
                 + LOCALHOST.user + "', 'port': " + LOCALHOST.xprotocol_port + ", 'dbPassword': '" + LOCALHOST.password + "'}).getSchema('sakila');\n", "mysql-js>"),
                ("var myTable = session.getTable('actor');\n", "mysql-js>"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute();\n", "Query OK"),
                ("myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute();\n", "mysql-js>"),
                ("myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute();\n", "mysql-js>"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute();\n", "2 rows in set"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute();\n", "Query OK"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute();\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_examples_1_6(self):
      '''[2.0.07]:3 Connect local Server on JS mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--js']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession({'host': '" + LOCALHOST.host + "', 'dbUser': '"
                 + LOCALHOST.user + "', 'port': " + LOCALHOST.xprotocol_port + ", 'dbPassword': '" + LOCALHOST.password + "'}).getSchema('sakila');\n", "mysql-js>"),
                ("var myTable = session.getTable('actor');\n", "mysql-js>"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute();\n", "Query OK"),
                ("myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute();\n", "mysql-js>"),
                ("myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute();\n", "mysql-js>"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute();\n", "2 rows in set"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute();\n", "Query OK"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute();\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_examples_1_7(self):
      '''[2.0.07]:3 Connect local Server on JS mode: APP SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--js']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getSession({'host': '" + LOCALHOST.host + "', 'dbUser': '"
                 + LOCALHOST.user + "', 'port': " + LOCALHOST.xprotocol_port + ", 'dbPassword': '" + LOCALHOST.password + "'}).getSchema('sakila');\n", "mysql-js>"),
                ("var myTable = session.getTable('actor');\n", "mysql-js>"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute();\n", "Query OK"),
                ("myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute();\n", "mysql-js>"),
                ("myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute();\n", "mysql-js>"),
                ("myTable.select().where('first_name  like : name').bind('name','testFN').execute();\n", "2 rows in set"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute();\n", "Query OK"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute();\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_examples_1_8(self):
      '''[2.0.07]:3 Connect local Server on JS mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--js']
      x_cmds = [("var mysqlx=require('mysqlx');\n","mysql-js>"),
                ("var session=mysqlx.getNodeSession({'host': '" + LOCALHOST.host + "', 'dbUser': '"
                 + LOCALHOST.user + "', 'port': " + LOCALHOST.xprotocol_port + ", 'dbPassword': '" + LOCALHOST.password + "'}).getSchema('sakila');\n", "mysql-js>"),
                ("var myTable = session.getTable('actor');\n", "mysql-js>"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute();\n", "Query OK"),
                ("myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute();\n", "mysql-js>"),
                ("myTable.insert({ 'first_name': 'testFN', 'last_name':'testLN','last_update': '2006-02-15 04:34:33' }).execute();\n", "mysql-js>"),
                ("myTable.select().where('first_name like : name').bind('name','testFN').execute();\n", "2 rows in set"),
                ("myTable.delete().where(\"first_name like 'testFN%'\").execute();\n", "Query OK"),
                ("myTable.select().where('first_name =: name').bind('name','testFN').execute();\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  # Javascript based with Big Data
  # Be aware to update the BigCreate_Classic, BigCreate_Node and BigCreate_Coll_Node files,
  # in order to create the required number of rows, based on the "jsRowsNum_Test" value
  # JS Create Non collections
  def test_4_10_00_01(self):
     '''JS Exec Batch with huge data in Classic mode, Create and Insert:  --file= BigCreate_Classic.js'''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--file=' + Exec_files_location + 'BigCreate_Classic.js']
     p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE )
     stdin,stdout = p.communicate()
     if stdin.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
         results="FAIL"
     else:
         results = "PASS"
     self.assertEqual(results, 'PASS')

  def test_4_10_00_02(self):
     '''JS Exec Batch with huge data in Node mode, Create and Insert:  --file= BigCreate_Node.js'''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--file=' + Exec_files_location + 'BigCreate_Node.js']
     p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE )
     stdin,stdout = p.communicate()
     if stdin.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
         results="FAIL"
     else:
         results = "PASS"
     self.assertEqual(results, 'PASS')

  # JS Create Collections
  def test_4_10_00_03(self):
     '''JS Exec Batch with huge data in Node mode, Create and Add:  --file= BigCreate_Coll_Node.js'''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--file=' + Exec_files_location + 'BigCreate_Coll_Node.js']
     p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE )
     stdin,stdout = p.communicate()
     if stdin.find(bytearray("ERROR", "ascii"), 0, len(stdin)) > -1:
         results = "FAIL"
     else:
         results = "PASS"
     self.assertEqual(results, 'PASS')

  # JS Read Non collections
  def test_4_10_00_04(self):
     '''JS Exec a select with huge limit in Classic mode, Read'''
     jsRowsNum_Test = 1000
     results = ''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                   '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--js']
     x_cmds = [("session.runSql(\"use world_x;\");\n","Query OK"),
               ("session.runSql(\"SELECT * FROM world_x.big_data_classic_js where geometryCol is not null limit " + str(jsRowsNum_Test) + ";\")\n", str(jsRowsNum_Test) + " rows in set")
              ]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  def test_4_10_00_05(self):
     '''JS Exec a select with huge limit in Node mode, Read'''
     jsRowsNum_Test = 1000
     results = ''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                   '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--js']
     x_cmds = [("var Table = session.getSchema(\'world_x\').getTable(\'big_data_node_js\')\n", ""),
               ("Table.select().where(\"stringCol like :likeFilter\").limit(" + str(jsRowsNum_Test) + ").bind(\"likeFilter\",\'Node\%\').execute()\n", str(jsRowsNum_Test) + " rows in set")
              ]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  # JS Read Collections
  def test_4_10_00_06(self):
     '''JS Exec a select with huge limit in Node mode for collection, Read'''
     jsRowsNum_Test = 1000
     results = ''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                   '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--js']
     x_cmds = [("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
               ("myColl.find(\"Name = \'Mexico\'\").fields([\'_id\', \'Name\','geography.Region\',\'geography.Continent\']).limit(" + str(jsRowsNum_Test) + ")\n", str(jsRowsNum_Test) + " documents in set")
              ]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  # JS Update Non collections
  def test_4_10_00_07(self):
     '''JS Exec an update clause to a huge number of rows in Classic mode, Update'''
     jsRowsNum_Test = 1000
     results = ''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                   '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--js']
     x_cmds = [("session.runSql(\"use world_x;\");\n", "Query OK"),
               ("session.runSql(\"update big_data_classic_js set datetimeCol = now() where stringCol like \'Classic\%\' and blobCol is not null limit " + str(jsRowsNum_Test) + " ;\");\n", "Query OK, " + str(jsRowsNum_Test) + " rows affected")
              ]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  def test_4_10_00_08(self):
     '''JS Exec an update clause to huge number of rows in Node mode, Update'''
     jsRowsNum_Test = 1000
     results = ''
     CurrentTime = datetime.datetime.now()
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                   '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--js']
     x_cmds = [("var Table = session.getSchema('world_x').getTable('big_data_node_js')\n", ""),
               ("Table.update().set(\'datetimeCol\',\'" + str(CurrentTime) + "\').where(\"stringCol like :likeFilter\").limit(" + str(jsRowsNum_Test) + ").bind(\"likeFilter\",\'Node\%\').execute()\n", "Query OK, " + str(jsRowsNum_Test) + " items affected")
              ]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  # JS Update Collections
  def test_4_10_00_09(self):
      '''JS Exec an update clause to huge number of document rows in Node mode, using Set '''
      jsRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [("session.sql(\"use world_x;\");\n", "Query OK"),
                ("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
                ("myColl.modify(\"Name = :country\").set(\'Soccer_World_Championships\',\'0\').limit(" + str(jsRowsNum_Test) + ").bind(\'country\',\'Mexico\').execute();\n","Query OK, " + str(jsRowsNum_Test) + " items affected")
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_10_00_10(self):
      '''JS Exec an update clause to huge number of document rows in Node mode, using Unset '''
      jsRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [("session.sql(\"use world_x;\");\n", "Query OK"),
                ("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
                ("myColl.modify(\"Name = :country\").unset(\'Soccer_World_Championships\').limit(" + str(jsRowsNum_Test) + ").bind(\'country\',\'Mexico\').execute();\n","Query OK, " + str(jsRowsNum_Test) + " items affected")
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_10_00_11(self):
      '''JS Exec an update clause to huge number of document rows in Node mode, using Merge '''
      jsRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']

      x_cmds = [("session.sql(\"use world_x;\");\n", "Query OK"),
                ("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
                ("myColl.modify().merge({Language: \'Spanish\', Extra_Info:[\'Extra info TBD\']}).limit(" + str(jsRowsNum_Test) + ").execute();\n", "Query OK, " + str(jsRowsNum_Test) + " items affected"),
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_10_00_12(self):
      '''JS Exec an update clause to huge number of document rows in Node mode, using Array '''
      jsRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']

      x_cmds = [("session.sql(\"use world_x;\");\n", "Query OK"),
                ("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
                ("myColl.modify().arrayAppend(\'Language\', \'Spanish_mexico\').arrayAppend(\'Extra_Info\', \'Extra info TBD 2\').limit(" + str(jsRowsNum_Test) + ").execute();\n", "Query OK, " + str(jsRowsNum_Test) + " items affected"),
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  # JS Delete from Non collections
  def test_4_10_00_13(self):
      '''JS Exec a delete clause to huge number of rows in Classic mode, Delete '''
      jsRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--js']
      x_cmds = [("session.runSql(\"use world_x;\");\n", "Query OK"),
                ("session.runSql(\"DELETE FROM big_data_classic_js where stringCol like \'Classic\%\' limit " + str(jsRowsNum_Test) + ";\");\n", "Query OK, " + str(jsRowsNum_Test) + " rows affected"),
                ("session.runSql(\"DROP TABLE big_data_classic_js;\");\n", "Query OK, 0 rows affected"),
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_10_00_14(self):
     '''JS Exec a delete clause to huge number of rows in Node mode, Delete'''
     jsRowsNum_Test = 1000
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--js']
     x_cmds = [("var Table = session.getSchema(\'world_x\').getTable(\'big_data_node_js\')\n", ""),
               ("Table.delete().where(\'stringCol like :likeFilter\').limit(" + str(jsRowsNum_Test) + ").bind(\'likeFilter\', \'Node\%\').execute();\n", "Query OK, " + str(jsRowsNum_Test) + " items affected"),
               ("session.dropTable(\'world_x\', \'big_data_node_js\');\n", "Query OK"),
              ]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  # JS Delete from Collections
  def test_4_10_00_15(self):
      '''JS Exec a delete clause to huge number of document rows in Node mode, Delete'''
      jsRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
                ("myColl.remove(\'Name=:country\').limit(" + str(jsRowsNum_Test) + ").bind(\'country\',\'Mexico\').execute();\n", "Query OK, " + str(jsRowsNum_Test) + " items affected"),
                ("session.dropCollection(\'world_x\', \'big_coll_node_js\');\n", "Query OK"),
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  # Python based with Big Data
  # Be aware to update the BigCreate_Classic, BigCreate_Node and BigCreate_Coll_Node files,
  # in order to create the required number of rows, based on the "pyRowsNum_Test" value
  # Py Create Non collections
  # @unittest.skip("To avoid execution 4_10_01_01, because of issue https://jira.oraclecorp.com/jira/browse/MYS-398")
  def test_4_10_01_01(self):
      '''PY Exec Batch with huge data in Classic mode, Create and Insert:  --file= BigCreate_Classic.py'''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--py',
                      '--file=' + Exec_files_location + 'BigCreate_Classic.py']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      if stdout.find(bytearray("Error", "ascii"), 0, len(stdin)) > -1:
          self.assertEqual(stdin, 'PASS', str(stdout))

  def test_4_10_01_02(self):
      '''PY Exec Batch with huge data in Node mode, Create and Insert:  --file= BigCreate_Node.py'''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py',
                      '--file=' + Exec_files_location + 'BigCreate_Node.py']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      if stdout.find(bytearray("Error", "ascii"), 0, len(stdin)) > -1:
          self.assertEqual(stdin, 'PASS', str(stdout))

          # Py Create Collections

  def test_4_10_01_03(self):
      '''PY Exec Batch with huge data in Node mode, Create and Add:  --file= BigCreate_Coll_Node.py'''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py',
                      '--file=' + Exec_files_location + 'BigCreate_Coll_Node.py']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      if stdout.find(bytearray("Error", "ascii"), 0, len(stdin)) > -1:
          self.assertEqual(stdin, 'PASS', str(stdout))

          # Py Read Non collections

  def test_4_10_01_04(self):
      '''PY Exec a select with huge limit in Classic mode, Read'''
      pyRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--py']
      x_cmds = [("session.run_sql(\"use world_x;\");\n", "Query OK"),
                (
                "session.run_sql(\"SELECT * FROM world_x.big_data_classic_py where geometryCol is not null limit " + str(
                    pyRowsNum_Test) + ";\")\n", str(pyRowsNum_Test) + " rows in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_10_01_05(self):
      '''PY Exec a select with huge limit in Node mode, Read'''
      pyRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("Table = session.get_schema(\"world_x\").get_table(\"big_data_node_py\")\n", ""),
                ("Table.select().where(\"stringCol like :likeFilter\").limit(" + str(
                    pyRowsNum_Test) + ").bind(\"likeFilter\",\"Node%\").execute()\n",
                 str(pyRowsNum_Test) + " rows in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

      # Py Read Collections

  def test_4_10_01_06(self):
      '''PY Exec a select with huge limit in Node mode for collection, Read'''
      pyRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("myColl = session.get_schema(\"world_x\").get_collection(\"big_coll_node_py\");\n", ""),
                (
                "myColl.find(\"Name = \'Mexico\'\").fields([\"_id\", \"Name\",\"geography.Region\",\"geography.Continent\"]).limit(" + str(
                    pyRowsNum_Test) + ")\n", str(pyRowsNum_Test) + " documents in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

      # Py Update Non collections

  def test_4_10_01_07(self):
      '''PY Exec an update clause to a huge number of rows in Classic mode, Update'''
      pyRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--py']
      x_cmds = [("session.run_sql(\"use world_x;\");\n", "Query OK"),
                (
                "session.run_sql(\"update big_data_classic_py set datetimeCol = now() where stringCol like \'Classic%\' and blobCol is not null limit " + str(
                    pyRowsNum_Test) + ";\");\n", "Query OK, " + str(pyRowsNum_Test) + " rows affected")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_10_01_08(self):
      '''PY Exec an update clause to huge number of rows in Node mode, Update'''
      pyRowsNum_Test = 1000
      results = ''
      CurrentTime = datetime.datetime.now()
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("Table = session.get_schema('world_x').get_table('big_data_node_py')\n", ""),
                ("Table.update().set(\'datetimeCol\',\'" + str(
                    CurrentTime) + "\').where(\"stringCol like :likeFilter\").limit(" + str(
                    pyRowsNum_Test) + ").bind(\"likeFilter\",\'Node%\').execute()\n",
                 "Query OK, " + str(pyRowsNum_Test) + " items affected")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

      # Py Update Collections

  def test_4_10_01_09(self):
      '''PY Exec an update clause to huge number of document rows in Node mode, using Set '''
      pyRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [("session.sql(\"use world_x;\");\n", "Query OK"),
                ("myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_py\");\n", ""),
                ("myColl.modify(\"Name = :country\").set(\'Soccer_World_Championships\',\'0\').limit(" + str(
                    pyRowsNum_Test) + ").bind(\'country\',\'Mexico\').execute();\n",
                 "Query OK, " + str(pyRowsNum_Test) + " items affected")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_10_01_10(self):
      '''PY Exec an update clause to huge number of document rows in Node mode, using Unset '''
      pyRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("session.sql(\"use world_x;\");\n", "Query OK"),
                ("myColl = session.get_schema(\'world_x\').get_collection(\"big_coll_node_py\");\n", ""),
                ("myColl.modify(\"Name = :country\").unset(\'Soccer_World_Championships\').limit(" + str(
                    pyRowsNum_Test) + ").bind(\'country\',\'Mexico\').execute();\n",
                 "Query OK, " + str(pyRowsNum_Test) + " items affected")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_10_01_11(self):
      '''PY Exec an update clause to huge number of document rows in Node mode, using Merge '''
      pyRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']

      x_cmds = [("session.sql(\"use world_x;\")\n", "Query OK"),
                ("myColl = session.get_schema(\'world_x\').get_collection(\"big_coll_node_py\")\n", ""),
                ("myColl.modify().merge({\'Language\': \"Spanish\", \'Extra_Info\':[\"Extra info TBD\"]}).limit(" + str(
                    pyRowsNum_Test) + ").execute()\n", "Query OK, " + str(pyRowsNum_Test) + " items affected"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_10_01_12(self):
      '''PY Exec an update clause to huge number of document rows in Node mode, using Array '''
      pyRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']

      x_cmds = [("session.sql(\"use world_x;\")\n", "Query OK"),
                ("myColl = session.get_schema(\'world_x\').get_collection(\"big_coll_node_py\")\n", ""),
                (
                "myColl.modify().array_append(\"Language\", \"Spanish_mexico\").array_append(\"Extra_Info\", \"Extra info TBD 2\").limit(" + str(
                    pyRowsNum_Test) + ").execute()\n", "Query OK, " + str(pyRowsNum_Test) + " items affected"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

      # Py Delete from Non collections

  def test_4_10_01_13(self):
      '''PY Exec a delete clause to huge number of rows in Classic mode, Delete '''
      pyRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--py']
      x_cmds = [("session.run_sql(\"use world_x;\")\n", "Query OK"),
                ("session.run_sql(\"DELETE FROM big_data_classic_py where stringCol like \'Classic%\' limit " + str(
                    pyRowsNum_Test) + ";\")\n", "Query OK, " + str(pyRowsNum_Test) + " rows affected"),
                ("session.run_sql(\"DROP TABLE big_data_classic_py;\")\n", "Query OK, 0 rows affected"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_10_01_14(self):
      '''PY Exec a delete clause to huge number of rows in Node mode, Delete'''
      pyRowsNum_Test = 1000
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("Table = session.get_schema(\'world_x\').get_table(\'big_data_node_py\')\n", ""),
                ("Table.delete().where(\"stringCol like :likeFilter\").limit(" + str(
                    pyRowsNum_Test) + ").bind(\"likeFilter\", \"Node%\").execute()\n",
                 "Query OK, " + str(pyRowsNum_Test) + " items affected"),
                ("session.drop_table(\'world_x\', \'big_data_node_py\')\n", "Query OK"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

      # Py Delete from Collections

  def test_4_10_01_15(self):
      '''PY Exec a delete clause to huge number of document rows in Node mode, Delete'''
      pyRowsNum_Test = 1000
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("myColl = session.get_schema(\"world_x\").get_collection(\"big_coll_node_py\")\n", ""),
                ("myColl.remove(\"Name=:country\").limit(" + str(
                    pyRowsNum_Test) + ").bind(\"country\",\"Mexico\").execute()\n",
                 "Query OK, " + str(pyRowsNum_Test) + " items affected"),
                ("session.drop_collection(\"world_x\", \"big_coll_node_py\")\n", "Query OK"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  # SQL based with Big Data
  # Be aware to update the BigCreate_SQL, BigCreate_SQL_Coll files,
  # in order to create the required number of rows, based on the "sqlRowsNum_Test" value

  #SQL Non Collections Create
  def test_4_10_02_01(self):
     '''SQL Exec Batch with huge data in Classic mode, Create and Insert:  --file= BigCreate_SQL.py'''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--sqlc', '--file=' + Exec_files_location + 'BigCreate_SQL.sql']
     p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
     stdin,stdout = p.communicate()
     if stdout.find(bytearray("Error","ascii"),0,len(stdin))> -1:
       self.assertEqual(stdin, 'PASS', str(stdout))

  #SQL Create Collections
  def test_4_10_02_02(self):
     '''SQL Exec Batch with huge data in Classic mode for collection, Create and Insert:  --file= BigCreate_Coll_SQL.sql'''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                     '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--sqlc', '--file=' + Exec_files_location + 'BigCreate_Coll_SQL.sql']
     p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
     stdin,stdout = p.communicate()
     if stdout.find(bytearray("Error","ascii"),0,len(stdin))> -1:
       self.assertEqual(stdin, 'PASS', str(stdout))

  #SQL Read Non Collections
  def test_4_10_02_03(self):
     '''SQL Exec a select with huge limit in Classic mode, Read'''
     sqlRowsNum_Test = 1000
     results = ''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                   '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--sqlc']
     x_cmds = [("SELECT * FROM world_x.bigdata_sql where stringCol like \'SQL%\' limit " + str(sqlRowsNum_Test) + ";\n", str(sqlRowsNum_Test) + " rows in set")]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  #SQL Read Collections
  def test_4_10_02_04(self):
     '''SQL Exec a select with huge limit in Classic mode for collection, Read'''
     sqlRowsNum_Test = 1000
     results = ''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                   '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--sqlc']
     x_cmds = [("SELECT * FROM world_x.bigdata_coll_sql where _id < " + str(sqlRowsNum_Test+1) + ";\n", str(sqlRowsNum_Test) + " rows in set")]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  #SQL Update Non Collections
  def test_4_10_02_05(self):
     '''SQL Exec an update to the complete table in Classic mode for non collection, Read'''
     sqlRowsNum_Test = 1000
     results = ''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                   '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--sqlc']
     x_cmds = [("update world_x.bigdata_sql set datetimeCol = now() where stringCol like 'SQL%';\n", "Rows matched: " + str(sqlRowsNum_Test) + "  Changed: " + str(sqlRowsNum_Test) + "  Warnings: 0")]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  #SQL Update Collections
  def test_4_10_02_06(self):
     '''SQL Exec an update to the complete table in Classic mode for collection, Read'''
     sqlRowsNum_Test = 1000
     results = ''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                   '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--sqlc']
     x_cmds = [("update world_x.bigdata_coll_sql set doc = \'{\"GNP\" : 414972,\"IndepYear\" : 1810,\"Name\" : \"Mexico\",\"_id\" : \"9001\"}\';\n", "Rows matched: " + str(sqlRowsNum_Test) + "  Changed: " + str(sqlRowsNum_Test) + "  Warnings: 0")]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  #SQL Delete Non Collections
  def test_4_10_02_07(self):
     '''SQL Exec a delete to the complete table in Classic mode for non collection, Read'''
     sqlRowsNum_Test = 1000
     results = ''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                   '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--sqlc']
     x_cmds = [("DELETE FROM world_x.bigdata_sql where blobCol is not null;\n", str(sqlRowsNum_Test) + " rows affected"),
               ("DROP PROCEDURE world_x.InsertInfoSQL;\n", "0 rows affected"),
               ("DROP TABLE world_x.bigdata_sql;\n", "0 rows affected")
               ]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  #SQL Delete Collections
  def test_4_10_02_08(self):
     '''SQL Exec a delete to the complete table in Classic mode for collection, Read'''
     sqlRowsNum_Test = 1000
     results = ''
     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                   '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--sqlc']
     x_cmds = [("DELETE FROM world_x.bigdata_coll_sql where _id > 0;\n", str(sqlRowsNum_Test) + " rows affected"),
               ("DROP PROCEDURE world_x.InsertInfoSQLColl;\n", "0 rows affected"),
               ("DROP TABLE world_x.bigdata_coll_sql;\n", "0 rows affected")
               ]
     results = exec_xshell_commands(init_command, x_cmds)
     self.assertEqual(results, 'PASS')

  def test_4_11_01(self):
      """ CreateIndex function """
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", 'mysql-js>'),
                ("var mysqlx = require('mysqlx');\n", "mysql-js>"),
                ("var session = mysqlx.getSession('"+ LOCALHOST.user +":"+ LOCALHOST.password +"@"+ LOCALHOST.host +":"+ LOCALHOST.xprotocol_port +"');\n", "mysql-js>"),
                ("var schema = session.getSchema('sakila_x');\n", "mysql-js>"),
                ("var coll = session.getSchema('sakila_x').getCollection('movies');\n", "mysql-js>"),
                ("coll.createIndex('rating_index').field('rating', 'text(5)', true).execute();\n", "Query OK"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_11_02(self):
      """ dropIndex fucntion """
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", 'mysql-js>'),
                ("var mysqlx = require('mysqlx');\n", "mysql-js>"),
                ("var session = mysqlx.getSession('"+ LOCALHOST.user +":"+ LOCALHOST.password +"@"+ LOCALHOST.host +":"+ LOCALHOST.xprotocol_port +"');\n", "mysql-js>"),
                ("var schema = session.getSchema('sakila_x');\n", "mysql-js>"),
                ("var coll = session.getSchema('sakila_x').getCollection('movies');\n", "mysql-js>"),
                ("coll.dropIndex('rating_index').execute();\n", "Query OK"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_11_3(self):
      ''' using  getLastDocumentId() function'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--js']

      x_cmds = [("session.dropCollection('sakila','my_collection');\n", "mysql-js>"),
                ("session.getSchema('sakila').createCollection('my_collection');\n", "mysql-js>"),
                ("var myColl = session.getSchema('sakila').getCollection('my_collection');\n","mysql-js>"),
                ("myColl.add( { _id: '12345', a : 1 } );\n","Query OK"),
                ("var result = myColl.add( { _id: '54321', a : 2 } ).execute();\n","mysql-js>"),
                ("result.getLastDocumentId();\n","54321"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_CHLOG_1_0_2_5_1A(self):
      '''[CHLOG 1.0.2.5_1_1] Session type shortcut [--classic] :  --sql/--js/--py '''
      sessMode = ['-sql', '-js', '-py']
      for w in sessMode:
          results = ''
          init_command = [MYSQL_SHELL, '--interactive=full', "-"+w, '-u' + LOCALHOST.user,
                          '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                          '--schema=sakila','--classic']
          p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
          stdin,stdout = p.communicate()
          if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1 or stdout != '':
            results="FAIL"
            break
          if stdin.find(bytearray("Creating a Classic Session to","ascii"),0,len(stdin))> -1 and stdin.find(bytearray("mysql"+w+">","ascii"),0,len(stdin))> -1 :
            results = 'PASS'
      self.assertEqual(results, 'PASS')

  def test_CHLOG_1_0_2_5_1B(self):
      '''[CHLOG 1.0.2.5_1_2] Session type shortcut [--node] :  --sql/--js/--py '''
      sessMode = ['-sql', '-js', '-py']
      for w in sessMode:
          results = ''
          init_command = [MYSQL_SHELL, '--interactive=full', "-"+w, '-u' + LOCALHOST.user,
                          '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                          '--schema=sakila','--node']
          p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
          stdin,stdout = p.communicate()
          if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1 or stdout != '':
            results="FAIL"
            break
          if stdin.find(bytearray("Creating a Node Session to","ascii"),0,len(stdin))> -1 and stdin.find(bytearray("mysql"+w+">","ascii"),0,len(stdin))> -1 :
            results = 'PASS'
      self.assertEqual(results, 'PASS')

  def test_CHLOG_1_0_2_5_1C(self):
      '''[CHLOG 1.0.2.5_1_3] Session type shortcut [--x] :  --sql/--js/--py '''
      sessMode = ['-sql', '-js', '-py']
      for w in sessMode:
          results = ''
          init_command = [MYSQL_SHELL, '--interactive=full', "-"+w, '-u' + LOCALHOST.user,
                          '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                          '--schema=sakila','--x']
          p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
          stdin,stdout = p.communicate()
          if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1 or stdout != '':
            results="FAIL"
            break
          if stdin.find(bytearray("Creating an X Session to","ascii"),0,len(stdin))> -1 and stdin.find(bytearray("mysql"+w+">","ascii"),0,len(stdin))> -1 :
            results = 'PASS'
      self.assertEqual(results, 'PASS')


  def test_CHLOG_1_0_2_5_2A(self):
      '''[CHLOG 1.0.2.5_2] Different password command line args'''
      sessMode = ['-p', '--password=', '--dbpassword=']
      for w in sessMode:
          results = ''
          init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, w +  LOCALHOST.password ,
                          '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x']
          p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
          #p.stdin.write(bytearray(LOCALHOST.password+"\n", 'ascii'))
          p.stdin.flush()
          stdin,stdout = p.communicate()
          if stdin.find(bytearray("mysql-js>","ascii"), 0, len(stdin)) > 0:
              results="PASS"
          else:
              results="FAIL"
              break
      self.assertEqual(results, 'PASS')

  def test_CHLOG_1_0_2_5_2B(self):
      '''[CHLOG 1.0.2.5_2] Different password command line args'''
      sessMode = ['-p', '--password', '--dbpassword']
      for w in sessMode:
          results = ''
          init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, w,'--passwords-from-stdin',
                          '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x']
          p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
          p.stdin.write(bytearray(LOCALHOST.password+"\n", 'ascii'))
          p.stdin.flush()
          stdin,stdout = p.communicate()
          if stdin.find(bytearray("mysql-js>","ascii"), 0, len(stdin)) > 0:
              results="PASS"
          else:
              results="FAIL"
              break
      self.assertEqual(results, 'PASS')



  def test_CHLOG_0_0_2_ALPHA_11(self):
      '''[CHLOG 1.0.2.5_2] Different password command line args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--version' ]
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("MySQL Shell Version","ascii"), 0, len(stdin)) >= 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_CHLOG_1_0_2_6_1(self):
      '''[CHLOG 1.0.2.5_2] Different password command line args'''
      results = ''
      #init_command = [MYSQL_SHELL, '--interactive=full', '--version' ]
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js', '--schema=sakila',
                      '--execute=print(dir(session))']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("\"close\",","ascii"), 0, len(stdin)) >= 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')




# ########################      M  Y  S _XXX  ##########################################################
#  ############################################################################################
#  ############################################################################################


  def test_MYS_60(self):
      '''[3.1.009]:1 Check that STATUS command [ \status, \s ] works: app session \status'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), '--x', '--js']
      x_cmds = [("\\s\n", "Current user:                 "+ LOCALHOST.user +"@localhost")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_193_00(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-193 with classic session"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--js']
      x_cmds = [(";\n", 'mysql-js>'),
                ("session\n", "<ClassicSession:" + LOCALHOST.user + "@" + LOCALHOST.host + ":" + LOCALHOST.port + ">")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_193_01(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-193 with node session"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [(";\n", 'mysql-js>'),
                ("session\n", "<NodeSession:" + LOCALHOST.user + "@" + LOCALHOST.host + ":" + LOCALHOST.xprotocol_port + ">")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_193_02(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-193 with app session"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x', '--js']
      x_cmds = [(";\n", 'mysql-js>'),
                ("session\n", "<XSession:" + LOCALHOST.user + "@" + LOCALHOST.host + ":" + LOCALHOST.xprotocol_port + ">")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_200_00(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-200 with classic session"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--py']
      x_cmds = [(";\n", 'mysql-py>'),
                (
                "session.run_sql(\"CREATE TABLE world_x.TextMYS200classic (  sTiny TINYTEXT NULL,  sText TEXT NULL,  sMediumText MEDIUMTEXT NULL, sLongText LONGTEXT NULL);\")\n",
                "Query OK"),
                (
                "session.run_sql(\"INSERT INTO world_x.TextMYS200classic VALUES(\'IamTiny\',\'IamAText\',\'IAmMediumText\',\'IAmLongText\');\")\n",
                "Query OK"),
                ("session.run_sql(\'SELECT * FROM world_x.TextMYS200classic;\')\n", "1 row in set"),
                (
                "session.run_sql(\"SELECT DATA_TYPE FROM INFORMATION_SCHEMA.COLUMNS WHERE table_name = \'TextMYS200classic\' and DATA_TYPE = \'longtext\';\")\n",
                "1 row in set"),
                ("session.run_sql(\"drop table world_x.TextMYS200classic;\")\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_200_01(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-200 with node session"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [(";\n", 'mysql-py>'),
                (
                "session.sql(\"CREATE TABLE world_x.TextMYS200node (  sTiny TINYTEXT NULL,  sText TEXT NULL,  sMediumText MEDIUMTEXT NULL, sLongText LONGTEXT NULL);\")\n",
                "Query OK"),
                ("Table = session.get_schema(\'world_x\').get_table(\'TextMYS200node\')\n", ""),
                ("Table.insert().values(\'IamTiny\',\'IamAText\',\'IAmMediumText\',\'IAmLongText\')\n", "Query OK"),
                ("Table.select()\n", "1 row in set"),
                (
                "session.sql(\"SELECT DATA_TYPE FROM INFORMATION_SCHEMA.COLUMNS WHERE table_name = \'TextMYS200node\' and DATA_TYPE = \'longtext\';\")\n",
                "1 row in set"),
                ("session.sql(\"drop table world_x.TextMYS200node;\")\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_224_00(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-224 with node session and json=raw"""
      results = ''
      error = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--py', '--json=raw']
      x_cmds = [("\n", 'mysql-py>'),
                ("session\n", '{\"result\":{\"class\":\"NodeSession\",\"connected\":true,\"uri\":\"' + LOCALHOST.user + '@' + LOCALHOST.host + ':' + LOCALHOST.xprotocol_port + '\"}}'),
                ("\\sql\n", "mysql-sql>"),
                ("use world_x;\n", "warningCount\":0,\"warnings\":[],\"rows\":[],\"hasData\":false,\"affectedRowCount\":0,\"autoIncrementValue\":-1}"),
                ("create table test_classic (variable varchar(10));\n", "\"warningCount\":0,\"warnings\":[],\"rows\":[],\"hasData\":false,\"affectedRowCount\":0,\"autoIncrementValue\":-1}"),
                ("select * from test_classic;\n","\"warningCount\":0,\"warnings\":[],\"rows\":[],\"hasData\":true,\"affectedRowCount\":0,\"autoIncrementValue\":-1}"),
                ("drop table world_x.test_classic;\n", "\"warningCount\":0,\"warnings\":[],\"rows\":[],\"hasData\":false,\"affectedRowCount\":0,\"autoIncrementValue\":-1}")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_224_01(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-224 with node session and json=pretty"""
      results = ''
      error = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--py', '--json=pretty']
      x_cmds = [("\n", 'mysql-py>'),
                ("session\n", '\"uri\": \"' + LOCALHOST.user + '@' + LOCALHOST.host + ':' + LOCALHOST.xprotocol_port + '\"'),
                ("\\sql\n", "mysql-sql>"),
                ("use world_x;\n", "\"rows\": []"),
                ("create table test_pretty (variable varchar(10));\n", "\"rows\": []"),
                ("select * from test_pretty;\n", "\"rows\": []"),
                ("drop table world_x.test_pretty;\n", "\"rows\": []")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_225_00(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-225 with classic session"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--py']
      x_cmds = [(";\n", 'mysql-py>'),
                (
                "session.run_sql(\'CREATE TABLE world_x.TestMYS225classic (Value INT NOT NULL, ValueDecimal FLOAT NOT NULL);\')\n",
                "Query OK"),
                (
                "session.run_sql(\'INSERT INTO world_x.TestMYS225classic VALUES (1,1.1),(2,2.1),(3,3.1),(4,4.1),(5,5.1),(6,6.1),(7,7.1),(8,8.1),(9,9.1),(10,10.1);\')\n",
                "Query OK, 10 rows affected"),
                ("session.run_sql(\'SELECT sum(value),sum(valuedecimal) FROM world_x.TestMYS225classic;\')\n",
                 "1 row in set"),
                ("session.run_sql(\'SELECT avg(value),avg(valuedecimal) FROM world_x.TestMYS225classic;\')\n",
                 "1 row in set"),
                ("session.run_sql(\'DROP TABLE world_x.TestMYS225classic;\')\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_225_01(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-225 with node session"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [(";\n", 'mysql-py>'),
                (
                "session.sql(\'CREATE TABLE world_x.TestMYS225node (Value INT NOT NULL, valuedecimal FLOAT NOT NULL);\')\n",
                "Query OK"),
                ("Table = session.get_schema(\'world_x\').get_table(\'TestMYS225node\')\n", ""),
                (
                "Table.insert().values(1,1.1).values(2,2.2).values(3,3.3).values(4,4.4).values(5,5.5).values(6,6.6).values(7,7.7).values(8,8.8).values(9,9.9).values(10,10.10)\n",
                "Query OK, 10 items affected"),
                ("Table.select([\'sum(value)\',\'sum(valuedecimal)\'])\n", "1 row in set"),
                ("Table.select([\'avg(value)\',\'avg(valuedecimal)\'])\n", "1 row in set"),
                ("session.sql(\'DROP TABLE world_x.TestMYS225node;\')\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_230_00(self):
      '''[4.0.002]:1 Batch Exec - Loading code from file:  < createtable.js'''
      init_command = [MYSQL_SHELL, '--interactive=full']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location+'Hello.js'))
      stdout,stderr = p.communicate()
      if stdout.find(bytearray("I am executed in batch mode, Hello","ascii"),0,len(stdout))> -1:
          results = 'PASS'
      else:
          results = 'FAIL'
      self.assertEqual(results, 'PASS')


  def test_MYS_230_01(self):
      '''[4.0.002]:1 Batch Exec - Loading code from file:  < createtable.js'''
      init_command = [MYSQL_SHELL, '--interactive=full','--js']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=open(Exec_files_location+'Hello.js'))
      stdout,stderr = p.communicate()
      if stdout.find(bytearray("I am executed in batch mode, Hello","ascii"),0,len(stdout))> -1:
          results = 'PASS'
      else:
          results = 'FAIL'
      self.assertEqual(results, 'PASS')



  def test_MYS_286_00(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-286 with classic session"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>'),
                ("create table world_x.MYS286 (date datetime);\n", "Query OK"),
                ("insert into world_x.MYS286 values (now());\n", "Query OK, 1 row affected"),
                ("select * from world_x.MYS286;\n", "1 row in set"),
                ("DROP TABLE world_x.MYS286;\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_286_01(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-286 with classic session"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [(";\n", 'mysql-js>'),
                ("session.sql(\'create table world_x.mys286 (date datetime);\')\n", "Query OK"),
                ("Table = session.getSchema(\'world_x\').getTable(\'mys286\')\n", "<Table:mys286>"),
                ("Table.insert().values('2016-03-14 12:36:37')\n", "Query OK, 1 item affected"),
                ("Table.select()\n", "2016-04-14 12:36:37"),
                ("session.sql(\'DROP TABLE world_x.mys286;\')\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_290_00(self):
      '''Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-290 with --file'''
      results = ''
      init_command = [MYSQL_SHELL, '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node',
                      '--file=' + Exec_files_location + 'JavaScript_Error.js']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      if stdout.find(bytearray("Invalid object member getdatabase", "ascii")):
          results = 'PASS'
      else:
          results = 'FAIL'
      self.assertEqual(results, 'PASS', str(stdout))

  def test_MYS_290_01(self):
      '''Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-290 with --interactive=full --file '''
      results = ''
      init_command = [MYSQL_SHELL, '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--interactive=full',
                      '--file=' + Exec_files_location + 'JavaScript_Error.js']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      stdin, stdout = p.communicate()
      if stdout.find(bytearray("Invalid object member getdatabase", "ascii")):
          results = 'PASS'
      else:
          results = 'FAIL'
      self.assertEqual(results, 'PASS', str(stdout))
  
  #FAILING........
  @unittest.skip("SSL is not creating the connection, related issue: https://jira.oraclecorp.com/jira/browse/MYS-488")
  def test_MYS_291(self):
      '''SSL Support '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js']
      x_cmds = [("var mysqlx=require('mysqlx');\n", "mysql-js>"),
                ("var session=mysqlx.getNodeSession({host: '"+LOCALHOST.host+"', dbUser: '"+LOCALHOST.user+"', port: '"+LOCALHOST.xprotocol_port+
                 "', dbPassword: '"+LOCALHOST.password+"', ssl-ca: '"+ Exec_files_location + "ca.pem', ssl-cert: '"+
                 Exec_files_location+"client-cert.pem', ssl-key: '"+Exec_files_location+"client-key.pem'});\n","mysql-js>" ),
                ("session;\n", "NodeSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_296(self):
      '''[4.1.002] SQL Create a table using STDIN batch process: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node','--sql','--schema=sakila',
                      '--file='+ Exec_files_location +'CreateTable_SQL.sql']

      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE )
      stdin,stdout = p.communicate()
      if stdout.find(bytearray("ERROR","ascii"),0,len(stdin))> -1:
        self.assertEqual(stdin, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show tables like \'example_SQLTABLE\';\n","1 row in set"),
                ("drop table if exists example_SQLTABLE;\n","Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_296_00(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-296 --file preference over < . Hello.js executed, HelloAgain.js not executed """
      results = ''
      expectedValue = 'I am executed in batch mode, Hello'
      target_vm =r'"%s"' % MYSQL_SHELL
      init_command_str = target_vm + ' --file=' + Exec_files_location + 'Hello.js' + ' < ' + Exec_files_location + 'HelloAgain.js'
      #init_command_str = MYSQL_SHELL + ' --file=' + Exec_files_location + 'Hello.js' + ' < ' + Exec_files_location + 'HelloAgain.js'
      p = subprocess.Popen(init_command_str, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, shell=True)
      stdin,stdout = p.communicate()
      if str(stdin) == expectedValue:
          results = 'PASS'
      else:
          results = 'FAIL'
      self.assertEqual(results, 'PASS', str(stdout))

  def test_MYS_296_01(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-296 --file preference over < . Hello.js executed, HelloAgain.js not executed """
      results = ''
      expectedValue = 'I am executed in batch mode, Hello'
      target_vm =r'"%s"' % MYSQL_SHELL
      init_command_str = target_vm + ' < ' + Exec_files_location + 'HelloAgain.js' + ' --file=' + Exec_files_location + 'Hello.js'
      p = subprocess.Popen(init_command_str, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, shell=True)
      stdin,stdout = p.communicate()
      if str(stdin) == expectedValue:
          results = 'PASS'
      else:
          results = 'FAIL'
      self.assertEqual(results, 'PASS', str(stdout))

  def test_MYS_303_00(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-303 with --help """
      results = 'FAIL'
      expectedValue = '  --help                   Display this help and exit.'
      target_vm =r'"%s"' % MYSQL_SHELL
      init_command_str = target_vm + ' --help'
      #init_command_str = MYSQL_SHELL + ' --help'
      p = subprocess.Popen(init_command_str, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, shell=True)
      stdin, stdout = p.communicate()
      stdin_splitted = stdin.splitlines()
      for line in stdin_splitted:
          if str(line) == expectedValue:
            results = 'PASS'
            break
      self.assertEqual(results, 'PASS', str(stdout))

  def test_MYS_309_00(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-309 with classic session and - as part of schema name"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--py']
      x_cmds = [(";\n", 'mysql-py>'),
                ("session.create_schema(\'my-Classic\')\n", "<ClassicSchema:my-Classic>"),
                ("session.drop_schema(\'my-Classic\')\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_309_01(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-309 with node session and - as part of schema name"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [(";\n", 'mysql-py>'),
                ("session.create_schema(\'my-Node\')\n", "<Schema:my-Node>"),
                ("session.drop_schema(\'my-Node\')\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  #FAILING........
  @unittest.skip("issues MYS320 , delimiter in js is not recongnized")
  def test_MYS_320(self):
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect -n {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("DROP PROCEDURE IF EXISTS get_actors;\n","mysql-sql>"),
                ("delimiter #\n","mysql-sql>"),
                ("create procedure get_actors()\n",""),
                ("begin\n",""),
                ("select first_name from sakila.actor;\n",""),
                ("end#\n","mysql-sql>"),
                #("\n","mysql-sql>"),
                ("delimiter ;\n","mysql-sql>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_323_00(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-323"""
      results = 'FAIL'
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [('\\saveconn  -f myNConn {0}:{1}@{2}:{3}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                   LOCALHOST.xprotocol_port), "Successfully stored"),
                ('\\connect -n $myNConn\n',
                 'Using \'myNConn\' stored connection' + os.linesep + 'Creating a Node Session'),
                ('\\saveconn  -f myXConn {0}:{1}@{2}:{3}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                   LOCALHOST.xprotocol_port), "Successfully stored"),
                ('\\connect -x $myXConn\n',
                 'Using \'myXConn\' stored connection' + os.linesep + 'Creating an X Session'),
                ('\\saveconn  -f myCConn {0}:{1}@{2}:{3}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                   LOCALHOST.port), "Successfully stored"),
                ('\\connect -c $myCConn\n',
                 'Using \'myCConn\' stored connection' + os.linesep + 'Creating a Classic Session'),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_325_1(self):
      '''[2.0.01]:1 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x', '--py','--js' ,  '--sql']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(";\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-sql>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_MYS_325_2(self):
      '''[2.0.01]:1 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x',  '--sql','--js', '--py']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray("\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-py>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_MYS_325_3(self):
      '''[2.0.01]:1 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x', '--py' ,  '--sql','--js']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(";\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_MYS_326(self):
      ''' Error displayed as json instead of common error format for sql mode, more info on
      https://jira.oraclecorp.com/jira/browse/MYS-471 and
      https://jira.oraclecorp.com/jira/browse/MYS-326 '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--sqlc']
      x_cmds = [("foo\"AnyText\";\n", "ERROR: 1064 (42000): You have an error in your SQL syntax")]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_335(self):
      '''[CHLOG 1.0.2.5_2] Different password command line args'''
      results = ''
      #init_command = [MYSQL_SHELL, '--interactive=full', '--version' ]
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--sqlc']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.flush()
      #stdin,stdout = p.communicate()
      p.stdin.write(bytearray("use sakas;\n" , 'ascii'))
      p.stdin.flush()
      stdout,stderr = p.communicate()
      if stderr.find(bytearray("ERROR:","ascii"), 0, len(stderr)) >= 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_MYS_338_01(self):
      '''[2.0.14]:4 Connect remote Server inside mysqlshell FAILOVER: \connect -c  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect -c {0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, "wrongpass", REMOTEHOST.host, REMOTEHOST.port), "mysql-js>"),
                ("print(session)\n", "Undefined"),
                ("session\n", "Undefined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_338_02(self):
      '''[2.0.14]:4 Connect remote Server inside mysqlshell FAILOVER: \connect -c  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect -c {0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, "wrongpass", REMOTEHOST.host, REMOTEHOST.port), "mysql-js>"),
                ("print(db)\n", "Undefined"),
                ("db\n", "Undefined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_338_03(self):
      '''[2.0.14]:4 Connect remote Server inside mysqlshell FAILOVER: \connect -c  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect -c {0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, "wrongpass", REMOTEHOST.host, REMOTEHOST.port), "mysql-js>"),
                ("db.name\n", "The db variable is not set, establish a global session first."),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("SESSION.URI DISPLAY WRONG MENU DATA TO THE USER: ISSUE MYS-542")
  def test_MYS_338_04(self):
      '''[2.0.14]:4 Connect remote Server inside mysqlshell FAILOVER: \connect -c  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("session.uri\n", "establish a session?"),
                ("y\n", "Please specify the session type:"),
                ("1\n", "MySQL server URI (or $alias)"),
                ("{0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host, REMOTEHOST.xprotocol_port), "mysql-js>"),
                ("session.uri\n", "{0}@{1}:{2}".format(REMOTEHOST.user,  REMOTEHOST.host, REMOTEHOST.xprotocol_port)),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  @unittest.skip("SESSION.URI DISPLAY WRONG MENU DATA TO THE USER: ISSUE MYS-542")
  def test_MYS_338_05(self):
      '''[2.0.14]:4 Connect remote Server inside mysqlshell FAILOVER: \connect -c  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("session.uri\n", "establish a session?"),
                ("y\n", "Please specify the session type:"),
                ("1\n", "MySQL server URI (or $alias)"),
                ("{0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host, REMOTEHOST.xprotocol_port), "mysql-js>"),
                ("session.uri\n", "{0}@{1}:{2}".format(REMOTEHOST.user,  REMOTEHOST.host, REMOTEHOST.xprotocol_port)),
                ("db.name\n", "want to set the active schema?"),
                ("y\n", "Please specify the schema:"),
                ("sakila\n", "mysql-js>"),
                ("db.name\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_339(self):
      '''[4.9.002] Update a Stored Session: '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n","mysql-js>"),
                ("\\saveconn classic_session "+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila\n","mysql-js>"),
                ("shell.storedSessions;\n","\"classic_session\": {" + os.linesep + ""),
                ("\\rmconn classic_session\n","mysql-js>"),
                ("\\saveconn classic_session dummy:"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila\n","mysql-js>"),
                ("shell.storedSessions;\n","\"dbUser\": \"dummy\", " + os.linesep + ""),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_341_00(self):
      '''[3.1.009]:1 Check that STATUS command [ \status, \s ] works: app session \status'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), '--x', '--js']
      x_cmds = [("\\status\n", "Current user:                 "+LOCALHOST.user+"@"+LOCALHOST.host),
                ("\\s\n", "Current user:                 "+LOCALHOST.user+"@"+LOCALHOST.host),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_341_01(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-341 with classic session and py custom prompt"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--py']
      x_cmds = [(";\n", 'mysql-py>'),
                ("def custom_prompt(): return \'--mypy--prompt-->\'\n", ""),
                ("shell.custom_prompt = custom_prompt\n", "--mypy--prompt-->"),
                ("\\js\n", "mysql-js>"),
                ("\\py\n", "--mypy--prompt-->")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_341_02(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-341 with node session and js custom prompt"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [(";\n", 'mysql-js>'),
                ("function custom_prompt(){ return session.uri + \'>>\'; }\n", ""),
                ("shell.custom_prompt = custom_prompt\n", LOCALHOST.user + "@" + LOCALHOST.host + ":" + LOCALHOST.xprotocol_port + ">>"),
                ("\\py\n", "mysql-py>"),
                ("\\js\n", LOCALHOST.user + "@" + LOCALHOST.host + ":" + LOCALHOST.xprotocol_port + ">>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')



  def test_MYS_348(self):
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
           '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--sql']
      x_cmds = [("use sakila;\n", "mysql-sql>"),
                ("drop table if exists funwithdates;\n", "Query OK"),
                ("CREATE TABLE funwithdates ( col_a date DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1;\n","mysql-sql>" ),
                ( "INSERT INTO funwithdates (col_a) VALUES ('1000-02-01 0:00:00');\n", "mysql-sql>"),
                ( "select * from funwithdates;\n", "mysql-sql>"),
                ( "\\js\n", "mysql-js>"),
                ( "row = session.getSchema('sakila').funwithdates.select().execute().fetchOne();\n", "mysql-js>"),
                ( "type(row.col_a);\n","Date"),
                #( "row.col_a;\n","mysql-js>"),
                #( "row.col_a.getYear();\n","1000"),
                ( "row.col_a.getFullYear();\n","1000"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_353(self):
      '''[CHLOG 1.0.2.5_2] Different password command line args'''
      sessMode = ['-p', '--password=', '--dbpassword=']
      for w in sessMode:
          results = ''
          init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, w+LOCALHOST.password,
                          '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--x']
          p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
          p.stdin.write(bytearray(LOCALHOST.password+"\n", 'ascii'))
          p.stdin.flush()
          stdin,stdout = p.communicate()
          if stdin.find(bytearray("Using a password on the command line interface can be insecure","ascii"), 0, len(stdin)) > 0:
              results="PASS"
          else:
              results="FAIL"
              break
      self.assertEqual(results, 'PASS')

  def test_MYS_354(self):
      '''[4.1.002] SQL Create a table using STDIN batch process: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sqlc', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic','--schema=sakila']
      x_cmds = [("SELECT * FROM INFORMATION_SCHEMA.PROCESSLIST WHERE USER ='"+ LOCALHOST.user +"';\n","| "+ LOCALHOST.user +" |")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_360(self):
      ''' DB.TABLES DOESN'T UPDATE CACHE WHEN CALLED TWICE'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila']

      x_cmds = [("\sql\n", "mysql-sql>"),
                ("drop table if exists sakila.tables;\n", "mysql-sql>"),
                ("create table tables ( id int );\n", "Query OK"),
                ("\js\n", "mysql-js>"),
                ("db.getTables();\n", "tables"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_361_1(self):
      ''' DB.TABLENAME.SELECT() DOESN'T WORK IF TABLENAME IS "TABLES" OR "COLLECTIONS"'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--sql']

      x_cmds = [("drop table if exists sakila.tables;\n", "Query OK"),
                ("CREATE TABLE `tables` (\n", "..."),
                ("  `character_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,\n", "..."),
                ("  `name` varchar(30) NOT NULL,\n", "..."),
                ("  `age` smallint(4) unsigned NOT NULL,\n", "..."),
                ("  `gender` enum('male', 'female') DEFAULT 'male' NOT NULL,\n", "..."),
                ("  `from` varchar(30) DEFAULT '' NOT NULL,\n", "..."),
                ("  `universe` varchar(30) NOT NULL,\n", "..."),
                ("  `base` bool DEFAULT false NOT NULL,\n", "..."),
                ("  PRIMARY KEY (`character_id`),\n", "..."),
                ("  KEY `idx_name` (`name`),\n", "..."),
                ("  KEY `idx_base` (`base`)\n", "..."),
                (") ENGINE=InnoDB DEFAULT CHARSET=utf8;\n", "Query OK, 0 rows affected"),
                ("\\js\n", "mysql-js>"),
                ("var table = db.getTable('tables');\n", "mysql-js>"),
                ("db.getTables();\n", "tables"),
                ("table.insert().values(28, 'Garrus Vakarian', 30, 'male', '', 'Mass Effect', 0).values(29, 'Liara TSoni', 109, 'female', '', 'Mass Effect', 0).execute();\n", "Query OK, 2 items affected"),
                ("table.select();\n", "2 rows in set"),
                ("table.delete().where('NOT base').execute();\n", "2 items affected"),
                ("table.select();\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_361_2(self):
      ''' DB.TABLENAME.SELECT() DOESN'T WORK IF TABLENAME IS "TABLES" OR "COLLECTIONS"'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--sql']

      x_cmds = [("drop table if exists sakila.tables;\n", "Query OK"),
                ("CREATE TABLE `collections` (\n", "..."),
                ("  `character_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,\n", "..."),
                ("  `name` varchar(30) NOT NULL,\n", "..."),
                ("  `age` smallint(4) unsigned NOT NULL,\n", "..."),
                ("  `gender` enum('male', 'female') DEFAULT 'male' NOT NULL,\n", "..."),
                ("  `from` varchar(30) DEFAULT '' NOT NULL,\n", "..."),
                ("  `universe` varchar(30) NOT NULL,\n", "..."),
                ("  `base` bool DEFAULT false NOT NULL,\n", "..."),
                ("  PRIMARY KEY (`character_id`),\n", "..."),
                ("  KEY `idx_name` (`name`),\n", "..."),
                ("  KEY `idx_base` (`base`)\n", "..."),
                (") ENGINE=InnoDB DEFAULT CHARSET=utf8;\n", "Query OK, 0 rows affected"),
                ("\\js\n", "mysql-js>"),
                ("var table = db.getTable('collections');\n", "mysql-js>"),
                ("db.getTables();\n", "collections"),
                ("table.insert().values(28, 'Garrus Vakarian', 30, 'male', '', 'Mass Effect', 0).values(29, 'Liara TSoni', 109, 'female', '', 'Mass Effect', 0).execute();\n", "Query OK, 2 items affected"),
                ("table.select();\n", "2 rows in set"),
                ("table.delete().where('NOT base').execute();\n", "2 items affected"),
                ("table.select();\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_365_01(self):
      ''' Schema names not available directly as session.schema and get_schema('uri') must work for classic session'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic', '--py']
      x_cmds = [("session.create_schema('uri')\n", ""),
                ("session.get_schema('uri')\n", ""),
                ("session.drop_schema('uri')\n", "Query OK")
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_365_02(self):
      ''' Schema names not available directly as session.schema and getSchema('uri') must work for node session'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [("session.createSchema('uri')\n", ""),
                ("session.getSchema('uri')\n", ""),
                ("session.dropSchema('uri')\n", "Query OK")
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_365_03(self):
      ''' Schema names not available directly as session.schema and getSchema('uri') must work for node session'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--x', '--js']
      x_cmds = [("session.createSchema('uri')\n", ""),
                ("session.getSchema('uri')\n", ""),
                ("session.dropSchema('uri')\n", "Query OK")
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_366_00(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-366 with node session """
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [(";\n", 'mysql-py>'),
                ("session.drop_collection('world_x','MyBindColl')\n", "mysql-py>"),
                ("coll = session.get_schema('world_x').create_collection('MyBindColl')\n", "mysql-py>"),
                ("coll.add({'name': ['jhon', 'Test'], 'pages': ['Default'], 'hobbies': ['default'], 'lastname': ['TestLastName']})\n", "Query OK, 1 item affected"),
                # Since parameter for where is [\"jhon\", \"WrongValue\"] and there is no name field with this array, nothing is updated
                ("coll.modify('name = :nameclause').array_append('name','UpdateName').bind('nameclause',['jhon', 'WrongValue'])\n", "Query OK, 0 items affected"),
                # Since parameter for where is [\"jhon\", \"Test\"] and there is a name field with this array, Append is applied having then [\"jhon\", \"Test\", \"UpdateName\"]
                ("coll.modify('name = :nameclause').array_append('name','UpdateName').bind('nameclause',['jhon', 'Test'])\n", "Query OK, 1 item affected "),
                ("session.drop_collection('world_x','MyBindColl')\n", "")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_373_1(self):
      '''[4.1.002] SQL Create a table using STDIN batch process: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila']
      x_cmds = [("print(session);\n","NodeSession:" + LOCALHOST.user + "@localhost:33060/sakila")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_373_2(self):
      '''[4.1.002] SQL Create a table using STDIN batch process: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--js', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--x','--schema=sakila']
      x_cmds = [("print(session);\n","XSession:"+ LOCALHOST.user +"@localhost:33060/sakila")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_373_3(self):
      '''[4.1.002] SQL Create a table using STDIN batch process: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sqlc', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--classic','--schema=sakila']
      x_cmds = [("\\s\n","Session type:                 Classic")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_378(self):
      '''show the default user if its not provided as argument : Creating a Node Session to XXXXXX@localhost:33060'''
      results = ''
      user = os.path.split(os.path.expanduser('~'))[-1]
      init_command = [MYSQL_SHELL, '--interactive=full', '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,'--schema=sakila','--sql',
                      '--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(LOCALHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("Creating a Node Session to " + user + "@", "ascii"), 0, len(stdin)) >= 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')



  def test_MYS_379(self):
      '''show the default user if its not provided as argument : Creating a Node Session to XXXXXX@localhost:33060'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,'--schema=sakila','--sql',
                      '--dbuser='+LOCALHOST.user,'--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray(LOCALHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("Creating a Node Session to ","ascii"), 0, len(stdin)) >= 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')



  def test_MYS_385(self):
      '''[CHLOG 1.0.2.5_2] Different password command line args'''
      results = ''
      #init_command = [MYSQL_SHELL, '--interactive=full', '--version' ]
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js', '--schema=sakila',
                      '-e print(dir(session))']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("\"close\",","ascii"), 0, len(stdin)) >= 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')


  def test_MYS_399(self):
      """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-224 with node session and json=raw"""
      results = ''
      error = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--py', '--json=raw']
      x_cmds = [("\n", 'mysql-py>'),
                ("session\n", '{\"result\":{\"class\":\"NodeSession\",\"connected\":true,\"uri\":\"' + LOCALHOST.user + '@' + LOCALHOST.host + ':' + LOCALHOST.xprotocol_port + '\"}}'),
                ("\\sql\n", "mysql-sql>"),
                ("use world_x;\n", "warningCount\":0,\"warnings\":[],\"rows\":[],\"hasData\":false,\"affectedRowCount\":0,\"autoIncrementValue\":-1}"),
                ("create table test_classic (variable varchar(10));\n", "\"warningCount\":0,\"warnings\":[],\"rows\":[],\"hasData\":false,\"affectedRowCount\":0,\"autoIncrementValue\":-1}"),
                ("select * from test_classic;\n","\"warningCount\":0,\"warnings\":[],\"rows\":[],\"hasData\":true,\"affectedRowCount\":0,\"autoIncrementValue\":-1}"),
                ("drop table world_x.test_classic;\n", "\"warningCount\":0,\"warnings\":[],\"rows\":[],\"hasData\":false,\"affectedRowCount\":0,\"autoIncrementValue\":-1}")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_400_01(self):
      ''' using  getDocumentId() and getDocumentIds() functions based in js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [("DocumentIDsColl = session.getSchema('sakila_x').createCollection('colldocumentids');\n", "<Collection:colldocumentids>"),
                ("res = DocumentIDsColl.add({ _id: '1', name: 'Rubens', lastname: 'Morquecho'}).add({ _id: '2', name: 'Omar', lastname: 'Mendez'}).execute()\n", "Query OK, 2 items affected"),
                # Validate getDocumentIds() with chaining add() and user-supplied document IDs
                ("res.getLastDocumentIds()\n", "\"1\""),
                ("res.getLastDocumentIds()\n", "\"2\""),
                # Validate getDocumentId() not allowed with chaining add()
                ("res.getLastDocumentId()\n", "mysql-js>"),
                ("res = DocumentIDsColl.add({ _id: '3', name: 'Armando', lastname: 'Lopez'}).execute()\n", "Query OK, 1 item affected"),
                # Validate getDocumentId() for single add()
                ("res.getLastDocumentId()\n", "3"),
                # Validate getDocumentIds() without chaining
                ("res.getLastDocumentIds()\n", "\"3\""),
                ("session.dropCollection('sakila_x','colldocumentids');\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_400_02(self):
      ''' using  getDocumentId() and getDocumentIds() functions based in py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("DocumentIDsColl = session.get_schema('sakila_x').create_collection('colldocumentids');\n", "mysql-py>"),
                ("res = DocumentIDsColl.add({ '_id': '1', 'name': 'Rubens', 'lastname': 'Morquecho'}).add({ '_id': '2', 'name': 'Omar', 'lastname': 'Mendez'}).execute()\n", "mysql-py>"),
                # Validate getDocumentIds() with chaining add() and user-supplied document IDs
                ("res.get_last_document_ids()\n", "\"1\""),
                ("res.get_last_document_ids()\n", "\"2\""),
                # Validate getDocumentId() not allowed with chaining add()
                ("res.get_last_document_id()\n", "mysql-py>"),
                ("res = DocumentIDsColl.add({ '_id': '3', 'name': 'Armando', 'lastname': 'Lopez'}).execute()\n", "mysql-py>"),
                # Validate getDocumentId() for single add()
                ("res.get_last_document_id()\n", "3"),
                # Validate getDocumentIds() without chaining
                ("res.get_last_document_ids()\n", "\"3\""),
                ("session.drop_collection('sakila_x','colldocumentids');\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_401_1(self):
      ''' View support (without DDL)'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']

      x_cmds = [("var db = session.getSchema('sakila');\n", "mysql-js>"),
                ("var table = db.getTable('actor');\n", "mysql-js>"),
                ("table.isView();\n", "false"),
                ("view = db.getTable('actor_info');\n","<Table:actor_info>"),
                ("view.isView();\n","true"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_401_2(self):
      ''' View support (without DDL)'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']

      x_cmds = [("var db = session.getSchema('sakila');\n", "mysql-js>"),
                ("var table = db.getTable('actor');\n", "mysql-js>"),
                ("table.isView();\n", "false"),
                ("view = db.getTable('actor_info');\n","<Table:actor_info>"),
                ("view.isView();\n","true"),
                ("view.update().set('last_name','GUINESSE').where('actor_id=1').execute();\n","The target table actor_info of the UPDATE is not updatable"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_401_3(self):
      ''' View support (without DDL)'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']

      x_cmds = [("var db = session.getSchema('sakila');\n", "mysql-js>"),
                ("var table = db.getTable('actor');\n", "mysql-js>"),
                ("table.isView();\n", "false"),
                ("view = db.getTable('actor_info');\n","<Table:actor_info>"),
                ("view.isView();\n","true"),
                ("view.delete().where('actor_id = 1').execute();\n","The target table actor_info of the DELETE is not updatable"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_401_4(self):
      ''' View support (without DDL)'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']

      x_cmds = [("var db = session.getSchema('sakila');\n", "mysql-js>"),
                ("var table = db.getTable('actor');\n", "mysql-js>"),
                ("table.isView();\n", "false"),
                ("view = db.getTable('actor_info');\n","<Table:actor_info>"),
                ("view.isView();\n","true"),
                ("view.insert().values(203,'JOHN','CENA', 'Action: The Marine').execute();\n","The target table actor_info of the INSERT is not insertable-into"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_401_5(self):
      ''' View support (without DDL)'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--sql', '--schema=sakila']

      x_cmds = [("create view actor_list as select actor_id as id, first_name as name, last_name as lname from actor;\n", "Query OK"),
                ("\\js\n", "mysql-js>"),
                ("view = db.getTable('actor_list');\n", "<Table:actor_list>"),
                ("view.isView();\n", "true"),
                ("view.insert().values(201, 'JOHN', 'SENA').execute();\n","Query OK, 1 item affected"),
                ("view.update().set('lname', 'CENA').where('id=201').execute();\n","Query OK, 1 item affected"),
                ("view.delete().where('id=201').execute();\n","Query OK, 1 item affected"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  #FAILING........
  @unittest.skip("connecting to store session without $, shows the password: ISSUE MYS-402")
  def test_MYS_402(self):
      '''[4.9.002] Create a Stored Session: schema store'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n","mysql-js>"),
                ("shell.storedSessions.add('classic_session', '"+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"\sakila');\n","mysql-js>"),
                ("\\connect classic_session\n","Creating an X Session to root@localhost:33060"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_403(self):
      '''[4.9.002] Create a Stored Session: schema store'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session\n","mysql-js>"),
                ("shell.storedSessions.add('classic_session', '"+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila');\n","mysql-js>"),
                ("\\connect $classic_session\n","Creating an X Session to "+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.port),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_427(self):
      '''[MYS_427] Warning is not longer displayed when password is not provided in URI connection '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      '{0}:@{1}:{2}'.format(LOCALHOST.user, LOCALHOST.host, LOCALHOST.xprotocol_port), '--node', '--js']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin =subprocess.PIPE )
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("[Warning]","ascii"),0,len(stdin))> -1:
        results = 'FAIL'
      else:
        results = 'PASS'
      self.assertEqual(results, 'PASS')

  def test_MYS_441(self):
      ''' db.tables and db.views should be removed'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--js']

      x_cmds = [("\\warnings\n", "mysql-js>"),
                ("db.tables();\n", "" + os.linesep + os.linesep + ""),
                ("db.views();\n",  "" + os.linesep + os.linesep + ""),
                ("db.getTables();\n","actor"),
                ("db.getViews();\n","actor_info"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_442_01(self):
      '''JS In node mode check isView() function to identify whether the underlying object is a View or not, return bool '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      '{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), '--node', '--js']
      x_cmds = [("table = session.getSchema('sakila').getTable('actor')\n", "mysql-js>"),
                ("table.isView()\n", "false"),
                ("view = session.getSchema('sakila').getTable('actor_info')\n", "mysql-js>"),
                ("view.isView()\n", "true")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_442_02(self):
      '''JS In classic mode check isView() function to identify whether the underlying object is a View or not, return bool '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      '{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.port), '--classic', '--js']
      x_cmds = [("table = session.getSchema('sakila').getTable('actor')\n", "mysql-js>"),
                ("table.isView()\n", "false"),
                ("view = session.getSchema('sakila').getTable('actor_info')\n", "mysql-js>"),
                ("view.isView()\n", "true")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_442_03(self):
      '''PY In node mode check is_view() function to identify whether the underlying object is a View or not, return bool '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      '{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), '--node', '--py']
      x_cmds = [("table = session.get_schema('sakila').get_table('actor')\n", ""),
                ("table.is_view()\n", "false"),
                ("view = session.get_schema('sakila').get_table('actor_info')\n", ""),
                ("view.is_view()\n", "true")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_442_04(self):
      '''PY In classic mode check is_view() function to identify whether the underlying object is a View or not, return bool '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      '{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.port), '--classic', '--py']
      x_cmds = [("table = session.get_schema('sakila').get_table('actor')\n", ""),
                ("table.is_view()\n", "false"),
                ("view = session.get_schema('sakila').get_table('actor_info')\n", ""),
                ("view.is_view()\n", "true")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_442_05(self):
      '''View select all response '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      '{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), '--node', '--js']
      x_cmds = [("view = session.getSchema('sakila').getTable('actor_info')\n", "mysql-js>"),
                ("view.select().execute()\n", "rows in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_442_06(self):
      '''Error displayed when try to update the view '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      '{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), '--node', '--js']
      x_cmds = [("view = session.getSchema('sakila').getTable('actor_info')\n", "mysql-js>"),
                ("view.update().set('last_name','GUINESSE').where('actor_id=1').execute()\n", "MySQL Error (1288): The target table actor_info of the UPDATE is not updatable")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_442_07(self):
      '''Vies displayed as part of getTables() function '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      '{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), '--node', '--js']
      x_cmds = [("session.getSchema('sakila').getTables()\n", "<Table:actor_info>,"),
                ("session.getSchema('sakila').getTables()\n", "<Table:actor_list>,")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_442_08(self):
      '''JS node For a view to be updatable, there must be a one-to-one relationship between the rows in the view and the rows in the underlying table
       therefore a new view is created following sakila.actor so update, insert and delete rows works'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      '{0}:{1}@{2}:{3}/{4}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port, "sakila"), '--node', '--js']
      x_cmds = [("session.sql(\"create view actor_list2 as select actor_id as id, first_name as name, last_name as lname from actor;\").execute()\n", "Query OK"),
                ("view = session.getSchema('sakila').getTable('actor_list2')\n", ""),
                ("view.insert().values(250, 'XShellName','XShellLastName').execute()\n", "Query OK, 1 item affected"),
                ("view.update().set('lname','XShellUpd').where('id=250').execute()\n", "Query OK, 1 item affected"),
                ("view.delete().where('id=250').execute()\n", "Query OK, 1 item affected"),
                ("session.dropView('sakila','actor_list2')\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_442_09(self):
      '''PY node For a view to be updatable, there must be a one-to-one relationship between the rows in the view and the rows in the underlying table
       therefore a new view is created following sakila.actor so update, insert and delete rows works'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      '{0}:{1}@{2}:{3}/{4}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port, "sakila"), '--node', '--py']
      x_cmds = [("session.sql(\"create view actor_list2 as select actor_id as id, first_name as name, last_name as lname from actor;\").execute()\n", "Query OK"),
                ("view = session.get_schema('sakila').get_table('actor_list2')\n", ""),
                ("view.insert().values(250, 'XShellName','XShellLastName').execute()\n", "Query OK, 1 item affected"),
                ("view.update().set('lname','XShellUpd').where('id=250').execute()\n", "Query OK, 1 item affected"),
                ("view.delete().where('id=250').execute()\n", "Query OK, 1 item affected"),
                ("session.drop_view('sakila','actor_list2')\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("issues MYS432 , Add support for log level names")
  def test_MYS_432_1(self):
      '''Add support for log level names'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=none','--js']
      x_cmds = [(";\n", "mysql-js>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("issues MYS432 , Add support for log level names")
  def test_MYS_432_2(self):
      '''Add support for log level names'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=internal','--js']
      x_cmds = [(";\n", "mysql-js>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("issues MYS432 , Add support for log level names")
  def test_MYS_432_3(self):
      '''Add support for log level names'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=error','--js']
      x_cmds = [(";\n", "mysql-js>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("issues MYS432 , Add support for log level names")
  def test_MYS_432_4(self):
      '''Add support for log level names'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=warning','--js']
      x_cmds = [(";\n", "mysql-js>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("issues MYS432 , Add support for log level names")
  def test_MYS_432_5(self):
      '''Add support for log level names'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=info','--js']
      x_cmds = [(";\n", "mysql-js>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("issues MYS432 , Add support for log level names")
  def test_MYS_432_6(self):
      '''Add support for log level names'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=debug','--js']
      x_cmds = [(";\n", "mysql-js>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("issues MYS432 , Add support for log level names")
  def test_MYS_432_7(self):
      '''Add support for log level names'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=debug2','--js']
      x_cmds = [(";\n", "mysql-js>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("issues MYS432 , Add support for log level names")
  def test_MYS_432_8(self):
      '''Add support for log level names'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=debug3','--js']
      x_cmds = [(";\n", "mysql-js>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_437_1(self):
      ''' NOT AND LIKE OPERATORS ARE NOT ACCEPTED IN UPPERCASE'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--sql']

      x_cmds = [("drop table if exists sakila.character;\n", "Query OK"),
                ("CREATE TABLE `character` (\n", "..."),
                ("  `character_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,\n", "..."),
                ("  `name` varchar(30) NOT NULL,\n", "..."),
                ("  `age` smallint(4) unsigned NOT NULL,\n", "..."),
                ("  `gender` enum('male', 'female') DEFAULT 'male' NOT NULL,\n", "..."),
                ("  `from` varchar(30) DEFAULT '' NOT NULL,\n", "..."),
                ("  `universe` varchar(30) NOT NULL,\n", "..."),
                ("  `base` bool DEFAULT false NOT NULL,\n", "..."),
                ("  PRIMARY KEY (`character_id`),\n", "..."),
                ("  KEY `idx_name` (`name`),\n", "..."),
                ("  KEY `idx_base` (`base`)\n", "..."),
                (") ENGINE=InnoDB DEFAULT CHARSET=utf8;\n", "Query OK, 0 rows affected"),
                ("\\js\n", "mysql-js>"),
                ("var table = db.getTable('character');\n", "mysql-js>"),
                ("table.insert().values(28, 'Garrus Vakarian', 30, 'male', '', 'Mass Effect', 0).values(29, 'Liara TSoni', 109, 'female', '', 'Mass Effect', 0).execute();\n", "Query OK, 2 items affected"),
                ("table.select();\n", "2 rows in set"),
                ("table.delete().where('NOT base').execute();\n", "2 items affected"),
                ("table.select();\n", "Empty set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_437_2(self):
      ''' NOT AND LIKE OPERATORS ARE NOT ACCEPTED IN UPPERCASE'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--sql']

      x_cmds = [("drop table if exists sakila.character;\n", "Query OK"),
                ("CREATE TABLE `character` (\n", "..."),
                ("  `character_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,\n", "..."),
                ("  `name` varchar(30) NOT NULL,\n", "..."),
                ("  `age` smallint(4) unsigned NOT NULL,\n", "..."),
                ("  `gender` enum('male', 'female') DEFAULT 'male' NOT NULL,\n", "..."),
                ("  `from` varchar(30) DEFAULT '' NOT NULL,\n", "..."),
                ("  `universe` varchar(30) NOT NULL,\n", "..."),
                ("  `base` bool DEFAULT false NOT NULL,\n", "..."),
                ("  PRIMARY KEY (`character_id`),\n", "..."),
                ("  KEY `idx_name` (`name`),\n", "..."),
                ("  KEY `idx_base` (`base`)\n", "..."),
                (") ENGINE=InnoDB DEFAULT CHARSET=utf8;\n", "Query OK, 0 rows affected"),
                ("\\js\n", "mysql-js>"),
                ("var table = db.getTable('character');\n", "mysql-js>"),
                ("table.insert().values(28, 'Garrus Vakarian', 30, 'male', '', 'Mass Effect', 0).values(29, 'Liara TSoni', 109, 'female', '', 'Mass Effect', 0).execute();\n", "Query OK, 2 items affected"),
                ("table.select();\n", "2 rows in set"),
                ("table.update().set('universe', 'Mass Effect 3').where('name LIKE :param1').bind('param1', '%Vaka%').execute();\n", "1 item affected"),
                ("table.select().where('name LIKE :param1').bind('param1', '%Vaka%').execute();\n", "Mass Effect 3"),
                ("table.update().set('universe', 'Mass Effect 3').where('name like :param1').bind('param1', 'Liara%').execute();\n", "1 item affected"),
                ("table.select().where('name LIKE :param1').bind('param1', 'Liara%').execute();\n", "Mass Effect 3"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_438(self):
      ''' TRUE OR FALSE NOT RECOGNIZED AS AVAILABLE BOOL CONSTANTS'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=sakila', '--sql']

      x_cmds = [("drop table if exists sakila.character;\n", "Query OK"),
                ("CREATE TABLE `character` (\n", "..."),
                ("  `character_id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,\n", "..."),
                ("  `name` varchar(30) NOT NULL,\n", "..."),
                ("  `age` smallint(4) unsigned NOT NULL,\n", "..."),
                ("  `gender` enum('male', 'female') DEFAULT 'male' NOT NULL,\n", "..."),
                ("  `from` varchar(30) DEFAULT '' NOT NULL,\n", "..."),
                ("  `universe` varchar(30) NOT NULL,\n", "..."),
                ("  `base` bool DEFAULT false NOT NULL,\n", "..."),
                ("  PRIMARY KEY (`character_id`),\n", "..."),
                ("  KEY `idx_name` (`name`),\n", "..."),
                ("  KEY `idx_base` (`base`)\n", "..."),
                (") ENGINE=InnoDB DEFAULT CHARSET=utf8;\n", "Query OK, 0 rows affected"),
                ("\\js\n", "mysql-js>"),
                ("var table = db.getTable('character');\n", "mysql-js>"),
                ("table.insert().values(28, 'Garrus Vakarian', 30, 'male', '', 'Mass Effect', 0).values(29, 'Liara TSoni', 109, 'female', '', 'Mass Effect', 1).execute();\n", "Query OK, 2 items affected"),
                ("table.insert().values(30, 'Garrus Vakarian', 30, 'male', '', 'Mass Effect', false).values(31, 'Liara TSoni', 109, 'female', '', 'Mass Effect', true).execute();\n", "Query OK, 2 items affected"),
                ("table.select();\n", "4 rows in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_388(self):
      """ AFTER CREATING SCHEMA IN PY SESSION, get_schemaS DOESN'T REFRESH\SHOW SUCH SCHEMA"""
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--py']
      x_cmds = [(";\n", 'mysql-py>'),
                ("session.run_sql('DROP DATABASE IF EXISTS schema_test;')\n", ""),
                ("session.run_sql('CREATE SCHEMA schema_test;')\n", "Query OK"),
                ("session.get_schemas()\n", "schema_test"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_444(self):
      """ "DB.COLLECTIONS" SHOWS COLLECTION AFTER DROPPING VIA MYSQL CLIENT"""
      results = ''
      error = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--sql']
      x_cmds = [("drop database if exists collections;\n", 'Query OK'),
                ("create database collections;\n", "Query OK"),
                ("\\py\n", "mysql-py>"),
                ("db=session.get_schema('collections')\n", "mysql-py>"),
                ("db.create_collection('flags')\n", "<Collection:flags>"),
                ("\\sql\n", "mysql-sql>"),
                ("use collections;\n", "Query OK"),
                ("show tables;\n", "flags"),
                ("\\py\n", "mysql-py>"),
                ("db.get_collections()\n", "<Collection:flags>"),
                ("\\sql\n", "mysql-sql>"),
                ("use collections;\n", "Query OK"),
                ("drop table flags;\n", "Query OK"),
                ("\\py\n", "mysql-py>"),
                ("db.get_collections()\n", "[" + os.linesep + "]"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_446(self):
      ''' How should Collection.add([]).execute() behave? Error is not displayed, nothing added '''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("testCollection = session.get_schema('sakila_x').create_collection('testcoll');\n", "mysql-py>"),
                ("res = testCollection.add([]).execute();\n", ""),
                ("session.sql(\"select * from sakila_x.testcoll;\").execute();\n", "Empty set"),
                ("session.drop_collection('sakila_x','testcoll');\n", "Query OK")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_451(self):
      '''MYSQL SHELL HELP OUTPUT IS WRONG ABOUT MYSH'''
      results = ''
      var = "===== Global Commands ====="+os.linesep +\
            "\\help       (\\?,\\h)    Print this help."+os.linesep +\
            "\\sql                   Switch to SQL processing mode."+os.linesep +\
            "\\js                    Switch to JavaScript processing mode."+os.linesep +\
            "\\py                    Switch to Python processing mode."+os.linesep +\
            "\\source     (\\.)       Execute a script file. Takes a file name as an argument."+os.linesep +\
            "\\                      Start multi-line input when in SQL mode."+os.linesep +\
            "\\quit       (\\q,\\exit) Quit MySQL Shell."+os.linesep +\
            "\\connect    (\\c)       Connect to a server."+os.linesep +\
            "\\warnings   (\\W)       Show warnings after every statement."+os.linesep +\
            "\\nowarnings (\\w)       Don't show warnings after every statement."+os.linesep +\
            "\\status     (\\s)       Print information about the current global connection."+os.linesep +\
            "\\use        (\\u)       Set the current schema for the global session."+os.linesep +\
            "\\saveconn   (\\savec)   Store a session configuration."+os.linesep +\
            "\\rmconn     (\\rmc)     Remove the stored session configuration."+os.linesep +\
            "\\lsconn     (\\lsc)     List stored session configurations."
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\help\n", var)
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_470_1(self):
      '''Enable named parameters in python for mysqlx.get_session() and mysqlx.get_node_session()'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_session(host= '" + LOCALHOST.host + "', dbUser= '"
                 + LOCALHOST.user + "', dbPassword= '" + LOCALHOST.password + "')\n", "mysql-py>"),
                ("session\n",
                 "<XSession:" + LOCALHOST.user + "@" + LOCALHOST.host + ":" + LOCALHOST.xprotocol_port + ">"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_470_2(self):
      '''Enable named parameters in python for mysqlx.getSession() and mysqlx.get_node_session()'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.get_node_session(host= '" + LOCALHOST.host + "', dbUser= '"
                 + LOCALHOST.user + "', dbPassword= '" + LOCALHOST.password + "')\n", "mysql-py>"),
                ("session\n",
                 "<NodeSession:" + LOCALHOST.user + "@" + LOCALHOST.host + ":" + LOCALHOST.xprotocol_port + ">"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_496(self):
      '''MySQL Shell prints Undefined on JSON column (Classic Session)'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.port, '--classic', '--sqlc', '--schema=sakila_x']
      x_cmds = [("select * from users limit 2;\n", '{\"_id\": \"'),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_MYS_497(self):
      '''No output if missing ";" on the -e option or redirecting to STDIN'''
      results = ''
      #init_command = [MYSQL_SHELL, '--interactive=full', '--version' ]
      init_command = [MYSQL_SHELL, '--interactive=full','-e show databases;', '--sql', '--uri',  LOCALHOST.user+ ':' + LOCALHOST.password +
                      '@' + LOCALHOST.host     ]
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("| sakila_x","ascii"), 0, len(stdin)) >= 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')



  def test_MYS_500_01(self):
      '''Add println function for JavaScript'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--sql']
      x_cmds = [("\\connect {0}:{1}@{2}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),"Creating an X Session"),
                ("\\js\n", "mysql-js>"),
                ("println(session);\n", "<XSession:"+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.xprotocol_port+">" + os.linesep + ""),
                ("\\use sakila\n", "mysql-js>"),
                ("db;\n", "<Schema:sakila>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_500_02(self):
      '''Add println function for JavaScript'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--sql']
      x_cmds = [("\\connect -n {0}:{1}@{2}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "Creating a Node Session"),
                ("\\js\n", "mysql-js>"),
                ("println(session);\n", "<NodeSession:"+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.xprotocol_port+">" + os.linesep + ""),
                ("\\use sakila\n", "mysql-js>"),
                ("session.getCurrentSchema();\n", "<Schema:sakila>"),
                ("db;\n", "<Schema:sakila>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_500_03(self):
      '''Add println function for JavaScript'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--sql']
      x_cmds = [("\\connect -c {0}:{1}@{2}:{3}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.port),"Creating a Classic Session"),
                ("\\js\n", "mysql-js>"),
                ("println(session);\n", "<ClassicSession:"+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.port+">" + os.linesep + ""),
                ("\\use sakila\n", "mysql-js>"),
                ("session.getCurrentSchema();\n", "<ClassicSchema:sakila>"),
                ("db;\n", "<ClassicSchema:sakila>"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')



  def test_MYS_502(self):
      '''Add println function for JavaScript'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--sql']
      x_cmds = [("\\connect {0}:{1}@{2}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),"Creating an X Session"),
                ("\\js\n", "mysql-js>"),
                ("println(session);\n", "<XSession:"+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.xprotocol_port+">" + os.linesep + "")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_518_01(self):
      ''' Session object Bool isOpen() function in js mode for node session'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [("session.isOpen()\n", "true"),
                ("session.close()\n", "mysql-js>"),
                ("session.isOpen()\n", "false"),
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_518_02(self):
      ''' Session object Bool is_open() function in py mode for node session'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("session.is_open()\n", "true"),
                ("session.close()\n", "mysql-py>"),
                ("session.is_open()\n", "false")
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_518_03(self):
      ''' Session object Bool isOpen() function in js mode for classic session'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--js']
      x_cmds = [("session.isOpen()\n", "true"),
                ("session.close()\n", "mysql-js>"),
                ("session.isOpen()\n", "false"),
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_518_04(self):
      ''' Session object Bool is_open() function in py mode for classic session'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--py']
      x_cmds = [("session.is_open()\n", "true"),
                ("session.close()\n", "mysql-py>"),
                ("session.is_open()\n", "false")
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_518_05(self):
      ''' pasreUri function in js mode for node session'''
      results = ''
      Sschema = "world_x"
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--js']
      x_cmds = [("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                  LOCALHOST.port,  Sschema), "\"dbPassword\": \"" + LOCALHOST.password + "\""),
                ("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                  LOCALHOST.port,  Sschema), "\"dbUser\": \"" + LOCALHOST.user + "\""),
                ("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                  LOCALHOST.port,  Sschema), "\"host\": \"" + LOCALHOST.host + "\""),
                #("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                #                                                  LOCALHOST.port,  Sschema), "\"port\": " + LOCALHOST.xprotocol_port),
                ("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                  LOCALHOST.port,  Sschema), "\"schema\": \"" + Sschema + "\"")
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_518_06(self):
      ''' pasreUri function in py mode for node session'''
      results = ''
      Sschema = "world_x"
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--node', '--py']
      x_cmds = [("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                  LOCALHOST.port,  Sschema), "\"dbPassword\": \"" + LOCALHOST.password + "\""),
                ("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                  LOCALHOST.port,  Sschema), "\"dbUser\": \"" + LOCALHOST.user + "\""),
                ("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                  LOCALHOST.port,  Sschema), "\"host\": \"" + LOCALHOST.host + "\""),
                #("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                #                                                  LOCALHOST.port,  Sschema), "\"port\": " + LOCALHOST.xprotocol_port),
                ("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                  LOCALHOST.port,  Sschema), "\"schema\": \"" + Sschema + "\"")
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_536(self):
      '''[CHLOG 1.0.2.5_2] enabledXProtocol arg'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--dba','enableXProtocol']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("X Protocol plugin is already enabled and listening for connections","ascii"), 0, len(stdin)) >= 0:
          results="PASS"
      else:
          results="FAIL"
          self.assertEqual(results, 'PASS')
      results = ''
      #mysqlsh -uroot -pguidev! -hlocalhost -P3578 --sqlc -e "uninstall plugin mysqlx"
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--sqlc', '-e uninstall plugin mysqlx;']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("Query OK","ascii"), 0, len(stdin)) >= 0:
          results="PASS"
      else:
          results="FAIL"
          self.assertEqual(results, 'PASS')
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--classic', '--dba','enableXProtocol']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytearray("X Protocol plugin is already enabled and listening for connections","ascii"), 0, len(stdin)) >= 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')


  def test_MYS_538_1(self):
      '''WRONG FORMAT DISPLAYED TO USER FOR \LSC OR \LSCONN'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session1\n","mysql-js>"),
                ("\\rmconn classic_session2\n","mysql-js>"),
                ("\\rmconn classic_session3\n","mysql-js>"),
                ("\\saveconn classic_session1 "+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila\n","mysql-js>"),
                ("\\saveconn classic_session2 "+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/test\n","mysql-js>"),
                ("\\saveconn classic_session3 "+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/test2\n","mysql-js>"),
                ("\\lsc\n","classic_session1 : "+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila"+os.linesep+\
                 "classic_session2 : "+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/test"+os.linesep+\
                 "classic_session3 : "+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/test2" + os.linesep),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_538_2(self):
      '''WRONG FORMAT DISPLAYED TO USER FOR \LSC OR \LSCONN'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']

      x_cmds = [("\\rmconn classic_session1\n","mysql-js>"),
                ("\\rmconn classic_session2\n","mysql-js>"),
                ("\\rmconn classic_session3\n","mysql-js>"),
                ("\\saveconn classic_session1 "+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila\n","mysql-js>"),
                ("\\saveconn classic_session2 "+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/test\n","mysql-js>"),
                ("\\saveconn classic_session3 "+LOCALHOST.user+":"+LOCALHOST.password+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/test2\n","mysql-js>"),
                ("\\lsconn\n","classic_session1 : "+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/sakila"+os.linesep+\
                 "classic_session2 : "+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/test"+os.linesep+\
                 "classic_session3 : "+LOCALHOST.user+"@"+LOCALHOST.host+":"+LOCALHOST.port+"/test2" + os.linesep),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_539(self):
      ''' Unable to add documents to collection'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                       '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node','--schema=world_x', '--js']
      var = "{ GNP: .6, IndepYear: 1967, Name: \"Sealand\", _id: \"SEA\""+\
            "demographics: { LifeExpectancy: 79, Population: 27},"+\
            "geography: { Continent: \"Europe\", Region: \"British Islands\", SurfaceArea: 193},"+\
            "government: { GovernmentForm: \"Monarchy\", HeadOfState: \"Michael Bates\"}}"

      x_cmds = [("var myColl = session.getSchema('world_x').getCollection('countryinfo');\n","mysql-js>"),
                ("var result = myColl.add("+var+" ).execute();\n","mysql-js>"),
                ("result.getLastDocumentId();\n","SEA"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_MYS_540(self):
      if sys.platform == 'win32':
          results = "PASS"
          self.assertEqual(results, 'PASS')
      else:
          '''Running scripts from command line including'''
          inputfilename = Exec_files_location + "test_540.txt"
          content = "#!"+MYSQL_SHELL+" -f"+os.linesep + "print(\"Hello World\");"+os.linesep
          f = open(inputfilename, 'w')
          f.write(content)
          f.close()
          p = subprocess.Popen(["chmod","777", inputfilename ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
          stdin, stdout = p.communicate()
          results = ''
          expectedValue = 'Hello World'
          init_command = [Exec_files_location + './test_540.txt']
          p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
          stdin, stdout = p.communicate()
          found = stdin.find(bytearray(expectedValue, "ascii"), 0, len(stdin))
          if found == -1:
              results = "FAIL \n\r" + stdin.decode("ascii")
          else:
              results = "PASS"
          self.assertEqual(results, 'PASS')



  # ----------------------------------------------------------------------


if __name__ == '__main__':
  unittest.main( testRunner=xmlrunner.XMLTestRunner(file(XMLReportFilePath,"w")))
