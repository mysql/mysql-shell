import subprocess
import time
import sys
import os
import threading
import functools
import configparser
import unittest

def timeout(timeout):
    def deco(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            # res = [Exception('function [%s] timeout [%s seconds] exceeded!' % (func.__name__, timeout))]
            res = [Exception('FAILED timeout [%s seconds] exceeded!' % ( timeout))]
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
    t = time.time()
    while (new_byte != b'\n'):
        try:
            new_byte = fd.read(1)
            if new_byte == '' and proc.poll() != None:
                break
            elif new_byte:
                # data += new_byte
                data += str(new_byte, encoding='utf-8')
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
    data = []
    line = ""
    t = time.time()
    while line != text  and proc.poll() == None:
        try:
            line = read_line(proc, fd, text)

            if line:
                data.append(line)
            elif proc.poll() is not None:
                break
        except ValueError:
            # timeout occurred
            print("read_line_timeout")
            break
    return "".join(data)

@timeout(10)
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
        # p.stdin.write(bytes(command + "\n", 'ascii'))
        p.stdin.write(bytes(command , 'ascii'))
        p.stdin.flush()
        # stdin,stdout = p.communicate()
        found = read_til_getShell(p, p.stdout, expectbefore)
        if found.find(expectbefore, 0, len(found)) == -1:
            stdin,stdout = p.communicate()
            # return "FAIL \n\r"+stdin.decode("ascii") +stdout.decode("ascii")
            RESULTS="FAILED"
            return "FAIL: " +stdout.decode("ascii")
            break
        expectbefore = lookup
        commandbefore =command
    # p.stdin.write(bytes(commandbefore, 'ascii'))
    p.stdin.write(bytes('', 'ascii'))
    p.stdin.flush()
    stdin,stdout = p.communicate()
    found = stdout.find(bytes(expectbefore,"ascii"), 0, len(stdout))
    if found == -1 :
            found = stdin.find(bytes(expectbefore,"ascii"), 0, len(stdin))
            if found == -1 :
                return "FAIL:  " + stdout.decode("ascii")
            else :
                return "PASS"
    else:
        return "PASS"

###############   Retrieve variables fron config.ini file    ##########################
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
config = configparser.ConfigParser()
config.read("config.ini")

LOCALHOST.user = config.get("local", "user")
LOCALHOST.password = config.get("local", "password")
LOCALHOST.host = config.get("local", "host")
LOCALHOST.xprotocol_port = config.get("local", "xprotocol_port")
LOCALHOST.port = config.get("local", "port")

REMOTEHOST.user = config.get("remote", "user")
REMOTEHOST.password = config.get("remote", "password")
REMOTEHOST.host = config.get("remote", "host")
REMOTEHOST.xprotocol_port = config.get("remote", "xprotocol_port")
REMOTEHOST.port = config.get("remote", "port")

MYSQL_SHELL = config.get("general","xshell_path")
Exec_files_location = config.get("general","aux_files_path")
###########################################################################################

class LocalConnection(unittest.TestCase):

  def test_2_0_01_01(self):
      '''[2.0.01]:1 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=app', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_02(self):
      '''[2.0.01]:2 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '-p','--passwords-from-stdin',
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=app']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytes(LOCALHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytes("mysql-js>","ascii"), 0, len(stdin)) > 0:
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
      p.stdin.write(bytes(LOCALHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytes("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_01_04(self):
      '''[2.0.01]:4 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                        LOCALHOST.xprotocol_port), '--session-type=app', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_05(self):
      '''[2.0.01]:5 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),
                      '--session-type=app', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_06(self):
      '''[2.0.01]:6 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
           '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_07(self):
      '''[2.0.01]:7 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '-p', '-h' + LOCALHOST.host, '--session-type=node',
                      '--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytes(LOCALHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytes("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_01_08(self):
      '''[2.0.01]:8 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                        LOCALHOST.xprotocol_port), '--session-type=node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_09(self):
      '''[2.0.01]:9 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),
                      '--session-type=node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_10(self):
      '''[2.0.01]:10 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--session-type=classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_01_11(self):
      '''[2.0.01]:11 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '-p', '-P'+LOCALHOST.port,'-h' + LOCALHOST.host, '--session-type=classic',
                      '--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytes(LOCALHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytes("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_01_13(self):
      '''[2.0.01]:13 Connect local Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                        LOCALHOST.port),'--session-type=classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

class RemoteConnection(unittest.TestCase):
  def test_2_0_02_01(self):
      '''[2.0.02]:1 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--session-type=app', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_02(self):
      '''[2.0.02]:2 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p', '-h' + REMOTEHOST.host, '--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytes(REMOTEHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytes("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_02_03(self):
      '''[2.0.02]:3 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p', '-h' + REMOTEHOST.host, '--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytes(REMOTEHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytes("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_02_04(self):
      '''[2.0.02]:4 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                        REMOTEHOST.xprotocol_port), '--session-type=app', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_05(self):
      '''[2.0.02]:5 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host),
                      '--session-type=app', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_06(self):
      '''[2.0.02]:6 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host, '-P' + REMOTEHOST.xprotocol_port, '--session-type=node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_07(self):
      '''[2.0.02]:7 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p', '-h' + REMOTEHOST.host,
                      '--session-type=node', '--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytes(REMOTEHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytes("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_02_08(self):
      '''[2.0.02]:8 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                        REMOTEHOST.xprotocol_port), '--session-type=node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_09(self):
      '''[2.0.02]:9 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host),
                      '--session-type=node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_10(self):
      '''[2.0.02]:10 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.port, '--session-type=classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_11(self):
      '''[2.0.02]:11 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p', '-h' + REMOTEHOST.host,
                      '--session-type=classic', '-P' + REMOTEHOST.port,'--passwords-from-stdin']
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytes(REMOTEHOST.password+"\n", 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      if stdin.find(bytes("mysql-js>","ascii"), 0, len(stdin)) > 0:
          results="PASS"
      else:
          results="FAIL"
      self.assertEqual(results, 'PASS')

  def test_2_0_02_12(self):
      '''[2.0.02]:12 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                        REMOTEHOST.port), '--session-type=classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_02_13(self):
      '''[2.0.02]:13 Connect remote Server w/Command Line Args'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.port), '--session-type=classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

class LocalConnection_SQLMode(unittest.TestCase):

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
      x_cmds = [("\\connect_node {0}:{1}@{2}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),
                 "Creating a Node Session"),
                ("print(session);\n", "NodeSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_03_05(self):
      '''[2.0.03]:5 Connect local Server on SQL mode: NODE SESSION WITH PORT'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect_node {0}:{1}@{2}:{3};\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                    LOCALHOST.xprotocol_port),"Creating a Node Session"),
                ("print(session);\n", "NodeSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def tc_2_0_03_06(self):
      '''[2.0.03]:6 Connect local Server on SQL mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect_classic {0}:{1}@{2}:{3}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.port),"Creating Classic Session"),
                ("print(session);\n", "ClassicSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

class RemoteConnection_SQLMode(unittest.TestCase):

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
      x_cmds = [("\\connect_node {0}:{1}@{2}\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host),
                 "Creating a Node Session"),
                ("print(session);\n", "NodeSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_04_05(self):
      '''[2.0.04]:5 Connect remote Server on SQL mode: NODE SESSION WITH PORT'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect_node {0}:{1}@{2}:{3};\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                                    REMOTEHOST.xprotocol_port),"Creating a Node Session"),
                ("print(session);\n", "NodeSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def tc_2_0_04_06(self):
      '''[2.0.04]:6 Connect remote Server on SQL mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect_classic {0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.port),"Creating Classic Session"),
                ("print(session);\n", "ClassicSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

class LocalConnection_JSMode(unittest.TestCase):

  def test_2_0_05_02(self):
      '''[2.0.05]:2 Connect local Server on JS mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\').mysqlx;\n","mysql-js>"),
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
      x_cmds = [("var mysqlx=require(\'mysqlx\').mysqlx;\n","mysql-js>"),
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
      x_cmds = [("var mysql=require(\'mysql\').mysql;\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                            LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("var schemaList = session.getSchemas();\n", "mysql-js>"),
                ("print(schemaList);\n", "sakila")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

class RemoteConnection_JSMode(unittest.TestCase):

  def test_2_0_06_02(self):
      '''[2.0.06]:2 Connect remote Server on JS mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("var mysqlx=require(\'mysqlx\').mysqlx;\n","mysql-js>"),
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
      x_cmds = [("var mysqlx=require(\'mysqlx\').mysqlx;\n","mysql-js>"),
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
      x_cmds = [("var mysql=require(\'mysql\').mysql;\n","mysql-js>"),
                ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(REMOTEHOST.user, REMOTEHOST.password,
                                                                            REMOTEHOST.host, REMOTEHOST.port), "mysql-js>"),
                ("var schemaList = session.getSchemas();\n", "mysql-js>"),
                ("print(schemaList);\n", "sakila")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

class LocalConnection_PYMode(unittest.TestCase):

  def test_2_0_07_02(self):
      '''[2.0.07]:2 Connect local Server on PY mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysqlx\n","mysql-py>"),
                ("session=mysqlx.getNodeSession(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host), "mysql-py>"),
                ("schemaList = session.getSchemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_07_03(self):
      '''[2.0.07]:3 Connect local Server on PY mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysqlx\n","mysql-py>"),
                ("session=mysqlx.getNodeSession({\'host\': \'" + LOCALHOST.host + "\', \'dbUser\': \'"
                 + LOCALHOST.user +  "\', \'dbPassword\': \'" + LOCALHOST.password + "\'})\n", "mysql-py>"),
                ("schemaList = session.getSchemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_07_04(self):
      '''[2.0.07]:4 Connect local Server on PY mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysql\n","mysql-py>"),
                ("session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                            LOCALHOST.host, LOCALHOST.port), "mysql-py>"),
                ("schemaList = session.getSchemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

class RemoteConnection_PYMode(unittest.TestCase):

  def test_2_0_08_02(self):
      '''[2.0.08]:2 Connect remote Server on PY mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysqlx\n","mysql-py>"),
                ("session=mysqlx.getNodeSession(\'{0}:{1}@{2}\')\n".format(REMOTEHOST.user, REMOTEHOST.password,
                                                                                REMOTEHOST.host), "mysql-py>"),
                ("schemaList = session.getSchemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_08_03(self):
      '''[2.0.08]:3 Connect remote Server on PY mode: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysqlx\n","mysql-py>"),
                ("session=mysqlx.getNodeSession({\'host\': \'" + REMOTEHOST.host + "\', \'dbUser\': \'"
                 + REMOTEHOST.user +  "\', \'dbPassword\': \'" + REMOTEHOST.password + "\'})\n", "mysql-py>"),
                ("schemaList = session.getSchemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_2_0_08_04(self):
      '''[2.0.08]:4 Connect remote Server on PY mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--py']
      x_cmds = [("import mysql\n","mysql-py>"),
                ("session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\')\n".format(REMOTEHOST.user, REMOTEHOST.password,
                                                                            REMOTEHOST.host, REMOTEHOST.port), "mysql-py>"),
                ("schemaList = session.getSchemas()\n", "mysql-py>"),
                ("schemaList\n", "sakila")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

class LocalConnection_InitExecMode(unittest.TestCase):

  def test_2_0_09_01(self):
      '''[2.0.09]:1 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--session-type=classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_02(self):
      '''[2.0.09]:2 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--session-type=classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_03(self):
      '''[2.0.09]:3 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--session-type=classic', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_04(self):
      '''4 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.port), '--session-type=classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_05(self):
      '''[2.0.09]:5 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.port), '--session-type=classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_06(self):
      '''[2.0.09]:6 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.port), '--session-type=classic', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_07(self):
      '''[2.0.09]:7 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_08(self):
      '''[2.0.09]:8 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=node', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_09(self):
      '''[2.0.09]:9 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=node', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_10(self):
      '''[2.0.09]:10 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--session-type=node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_11(self):
      '''[2.0.09]:11 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--session-type=node', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_12(self):
      '''[2.0.09]:12 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--session-type=node', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_13(self):
      '''[2.0.09]:13 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=app', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_14(self):
      '''[2.0.09]:14 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=app', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')


  def test_2_0_09_15(self):
      '''[2.0.09]:15 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=app', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_16(self):
      '''[2.0.09]:16 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--session-type=app', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_17(self):
      '''[2.0.09]:17 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--session-type=app', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_09_18(self):
      '''[2.0.09]:18 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                              LOCALHOST.xprotocol_port), '--session-type=app', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

class RemoteConnection_InitExecMode(unittest.TestCase):

  def test_2_0_10_01(self):
      '''[2.0.10]:1 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.port, '--session-type=classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_02(self):
      '''[2.0.10]:2 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.port, '--session-type=classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_03(self):
      '''[2.0.10]:3 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.port, '--session-type=classic', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_04(self):
      '''[2.0.10]:4 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.port), '--session-type=classic', '--sqlc']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_05(self):
      '''[2.0.10]:5 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.port), '--session-type=classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_06(self):
      '''[2.0.10]:6 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.port), '--session-type=classic', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_07(self):
      '''[2.0.10]:7 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--session-type=node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_08(self):
      '''[2.0.10]:8 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--session-type=node', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_09(self):
      '''[2.0.10]:9 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--session-type=node', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_10(self):
      '''[2.0.10]:10 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--session-type=node', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_11(self):
      '''[2.0.10]:11 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--session-type=node', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_12(self):
      '''[2.0.10]:12 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--session-type=node', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_13(self):
      '''[2.0.10]:13 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--session-type=app', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_14(self):
      '''[2.0.10]:14 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--session-type=app', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_15(self):
      '''[2.0.10]:15 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--session-type=app', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_16(self):
      '''[2.0.10]:16 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--session-type=app', '--sql']
      x_cmds = [(";\n", 'mysql-sql>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_17(self):
      '''[2.0.10]:17 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --js'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--session-type=app', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_10_18(self):
      '''[2.0.10]:18 Connect remote Server w/Init Exec mode: --[sql/js/py]: CLASSIC APPLICATION --uri --py'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                            'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                              REMOTEHOST.xprotocol_port), '--session-type=app', '--py']
      x_cmds = [("\n", 'mysql-py>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

class LocalConnection_FailOver(unittest.TestCase):

  def test_2_0_11_01(self):
      '''[2.0.11]:1 Connect local Server w/Command Line Args FAILOVER: Wrong Password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=g' + LOCALHOST.password,
                      '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=app', '--sql']
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
                      '-P' + LOCALHOST.xprotocol_port, '--session-type=app', '--sql']
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
                                                        LOCALHOST.xprotocol_port), '--session-type=app', '--sql']
      x_cmds = [(";\n", "Invalid user or password")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find("Invalid user or password", 0, len(results))>0:
        results="PASS"
      self.assertEqual(results, 'PASS')

class RemoteConnection_FailOver(unittest.TestCase):

  def test_2_0_12_01(self):
      '''[2.0.11]:1 Connect remote Server w/Command Line Args FAILOVER: Wrong Password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=g' + REMOTEHOST.password,
                      '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--session-type=app', '--sql']
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
                      '-P' + REMOTEHOST.xprotocol_port, '--session-type=app', '--sql']
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
                                                        REMOTEHOST.xprotocol_port), '--session-type=app', '--sql']
      x_cmds = [(";\n", "Invalid user or password"),

                ]
      results = exec_xshell_commands(init_command, x_cmds)
      if results.find("Invalid user or password", 0, len(results))>0:
        results="PASS"
      self.assertEqual(results, 'PASS')


if __name__ == '__main__':
     unittest.main(verbosity=2)