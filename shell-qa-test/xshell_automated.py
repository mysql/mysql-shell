import subprocess
import time
import sys
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
    data = []
    line = ""
    t = time.time()
    # while line != text  and proc.poll() == None:
    while line.find(text,0,len(line))< 0  and proc.poll() == None:
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
            return "FAIL: " +stdout.decode("ascii")
            break
        expectbefore = lookup
        commandbefore =command
    # p.stdin.write(bytearray(commandbefore, 'ascii'))
    p.stdin.write(bytearray('', 'ascii'))
    p.stdin.flush()
    stdin,stdout = p.communicate()
    found = stdout.find(bytearray(expectbefore,"ascii"), 0, len(stdout))
    if found == -1 :
            found = stdin.find(bytearray(expectbefore,"ascii"), 0, len(stdin))
            if found == -1 :
                return "FAIL:  " + stdout.decode("ascii")
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

config=json.load(open('config.json'))

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

MYSQL_SHELL = str(config["general"]["xshell_path"])
Exec_files_location = str(config["general"]["aux_files_path"])
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
                                                        LOCALHOST.port),'--session-type=classic', '--js']
      x_cmds = [(";\n", 'mysql-js>')
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

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

  def test_2_0_13_02(self):
      '''[2.0.13]:2 Connect local Server inside mysqlshell FAILOVER: \connect  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect {0}:{1}@{2}\n".format(LOCALHOST.user, "wronpassw", LOCALHOST.host), "mysql-js>"),
                ("print(session)\n", "ReferenceError: session is not defined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_13_03(self):
      '''[2.0.13]:3 Connect local Server inside mysqlshell FAILOVER: \connect_node  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect_node {0}:{1}@{2}\n".format(LOCALHOST.user, "wrongpassw", LOCALHOST.host), "mysql-js>"),
                ("print(session)\n", "ReferenceError: session is not defined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_13_04(self):
      '''[2.0.13]:4 Connect local Server inside mysqlshell FAILOVER: \connect_classic  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect_classic {0}:{1}@{2}:{3}\n".format(LOCALHOST.user, "wrongpass", LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
                ("print(session)\n", "ReferenceError: session is not defined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_14_02(self):
      '''[2.0.14]:2 Connect remote Server inside mysqlshell FAILOVER: \connect  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect {0}:{1}@{2}\n".format(REMOTEHOST.user, "wronpassw", REMOTEHOST.host), "mysql-js>"),
                ("print(session)\n", "ReferenceError: session is not defined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_14_03(self):
      '''[2.0.14]:3 Connect remote Server inside mysqlshell FAILOVER: \connect_node  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect_node {0}:{1}@{2}\n".format(REMOTEHOST.user, "wrongpassw", REMOTEHOST.host), "mysql-js>"),
                ("print(session)\n", "ReferenceError: session is not defined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_14_04(self):
      '''[2.0.14]:4 Connect remote Server inside mysqlshell FAILOVER: \connect_classic  wrong password'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [(";\n", "mysql-js>"),
                ("\\connect_classic {0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, "wrongpass", REMOTEHOST.host, REMOTEHOST.port), "mysql-js>"),
                ("print(session)\n", "ReferenceError: session is not defined"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_2_0_15_01(self):
      '''[2.0.15]:1 Connect local Server w/Init Exec mode: --[sql/js/py] FAILOVER: wrong Exec mode --sqxx'''
      results = ''
      p = subprocess.Popen([MYSQL_SHELL, '--interactive=full', '--uri', 'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, "wrongpass", LOCALHOST.host, LOCALHOST.xprotocol_port),
                          '--session-type=app', '--sqx'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
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
                          '--session-type=app', '--sqx'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
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
      x_cmds = [("\\help connect\n", "Connect to server")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_02_02(self):
      '''[3.1.002]:2 Check that help command with parameter  works: \h connect'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\h connect\n", "Connect to server")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_02_03(self):
      '''[3.1.002]:3 Check that help command with parameter  works: \? connect'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\? connect\n", "Connect to server")
                ]
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

  @unittest.skip("not catching the Bye! message")
  def test_3_1_04_01(self):
      '''[3.1.004]:1 Check that command [ \quit, \q, \exit ] works: \quit'''
      results = ''
      p = subprocess.Popen([MYSQL_SHELL, '--interactive=full'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray('\\quit', 'ascii'))
      p.stdin.flush()
      p.stdin.write(bytearray('', 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      found = stdout.find(bytearray("Bye!","ascii"), 0, len(stdout))
      if found == -1:
          results= "FAIL \n\r" + stdout.decode("ascii")
      else:
          results= "PASS"
      self.assertEqual(results, 'PASS')

  @unittest.skip("not catching the Bye! message")
  def test_3_1_04_02(self):
      '''[3.1.004]:2 Check that command [ \quit, \q, \exit ] works: \q '''
      results = ''
      p = subprocess.Popen([MYSQL_SHELL, '--interactive=full'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray('\\q', 'ascii'))
      p.stdin.flush()
      p.stdin.write(bytearray('', 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      found = stdout.find(bytearray("Bye!","ascii"), 0, len(stdout))
      if found == -1:
          results= "FAIL \n\r" + stdout.decode("ascii")
      else:
          results= "PASS"
      self.assertEqual(results, 'PASS')

  @unittest.skip("not catching the Bye! message")
  def test_3_1_04_03(self):
      '''[3.1.004]:3 Check that command [ \quit, \q, \exit ] works: \exit'''
      results = ''
      p = subprocess.Popen([MYSQL_SHELL, '--interactive=full'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
      p.stdin.write(bytearray('\\exit', 'ascii'))
      p.stdin.flush()
      p.stdin.write(bytearray('', 'ascii'))
      p.stdin.flush()
      stdin,stdout = p.communicate()
      found = stdout.find(bytearray("Bye!","ascii"), 0, len(stdout))
      if found == -1:
          results= "FAIL \n\r" + stdout.decode("ascii")
      else:
          results= "PASS"
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
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), '--session-type=app', '--js']
      x_cmds = [("\\status\n", "Current user:                 root@localhost")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_09_02(self):
      '''[3.1.009]:2 Check that STATUS command [ \status, \s ] works: classic session \status'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,LOCALHOST.port), '--session-type=classic', '--js']
      x_cmds = [("\\status\n", "Current user:                 root@localhost")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_09_03(self):
      '''[3.1.009]:3 Check that STATUS command [ \status, \s ] works: node session \status'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                      'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,REMOTEHOST.xprotocol_port), '--session-type=node', '--sql']
      x_cmds = [("\\status\n", "Current user:                 root@localhost")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_10_1(self):
      '''[3.1.010]:1 Check that EXECUTE SCRIPT FILE command [ \source, \. ] works: node session \source select_actor_10.sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [("\\connect_node {0}:{1}@{2}\n".format(LOCALHOST.user, LOCALHOST.password,LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("\\source {0}select_actor_10.sql\n".format(Exec_files_location),"rows in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_3_1_10_2(self):
      '''[3.1.010]:2 Check that EXECUTE SCRIPT FILE command [ \source, \. ] works: node session \. select_actor_10.sql'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect_node {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("\\. {0}select_actor_10.sql\n".format(Exec_files_location),"rows in set"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("not leaving the multiline mode with empty line")
  def test_3_1_11_1(self):
      '''[3.1.011]:1 Check that MULTI LINE MODE command [ \ ] works'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect_node {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("DROP PROCEDURE IF EXISTS get_actors;\n","mysql-sql>"),
                ("delimiter #\n","mysql-sql>"),
                ("create procedure get_actors()\n",""),
                ("begin\n",""),
                ("select first_name from sakila.actor;\n",""),
                ("end\n",""),
                ("\n","mysql-sql>"),
                ("delimiter :\n","mysql-sql>"),
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
      init_command = [MYSQL_SHELL, '--interactive=full' , '--file=' + Exec_files_location + 'CreateTable.js' ]
      x_cmds = [('\n', "mysql-js>")
               ]
      results = exec_xshell_commands(init_command, x_cmds)

      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect_node {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show tables like \'testdb\';\n","1 row in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("not reading the  < batch  pipe when using  inside script")
  def test_4_0_02_01(self):
      '''[4.0.002]:1 Batch Exec - Redirecting code to standard input: < createtable.js'''
      results = ""
      init_command = [MYSQL_SHELL, '--interactive=full' , '< '  + Exec_files_location + 'CreateTable2.js' ]
      x_cmds = [('\n', "mysql-js>")
               ]
      results = exec_xshell_commands(init_command, x_cmds)

      # check that table was created successfully
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect_node {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show tables like \'testdb2\';\n","1 row in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_1_01_01(self):
      '''[4.1.001]:1 SQL Create a table: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--session-type=node']
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
                      '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--session-type=node','--sql','--schema=sakila',
                      '--file='+ Exec_files_location +'CreateTable_SQL.sql']
      x_cmds = [('\n', "mysql-js>")
               ]
      results = exec_xshell_commands(init_command, x_cmds)
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full']
      x_cmds = [('\\connect_node {0}:{1}@{2}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host), "mysql-js>"),
                ("\\sql\n","mysql-sql>"),
                ("use sakila;\n","mysql-sql>"),
                ("show tables like \'example_SQLTABLE\';\n","1 row in set"),
                ("drop table if exists \'example_SQLTABLE\';\n","1 row in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_2_16_1(self):
      '''[4.2.016]:1 PY Read executing the stored procedure: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7','--py']
      x_cmds = [("import mysqlx\n", "mysql-py>"),
                ("session=mysqlx.getNodeSession(\'{0}:{1}@{2}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                 LOCALHOST.host ), "mysql-py>"),
                ("session.sql(\'use sakila;\').execute()\n","Query OK"),
                ("session.sql(\"drop procedure if exists get_actors;\").execute()\n","Query OK"),
                ("session.sql(\"CREATE PROCEDURE get_actors() BEGIN  "
                 "select first_name from actor where actor_id < 5 ; END;\").execute()\n","Query OK"),
                ("session.sql(\'call get_actors();\').execute()\n","rows in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_2_16_2(self):
      '''[4.2.016]:2 PY Read executing the stored procedure: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7','--py']
      x_cmds = [("import mysql\n", "mysql-py>"),
                ("session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                LOCALHOST.host,LOCALHOST.port ), "mysql-py>"),
                ("session.runSql(\'use sakila;\')\n","Query OK"),
                ("session.runSql(\'drop procedure if exists get_actors;\')\n","Query OK"),
                ("session.runSql(\'CREATE PROCEDURE get_actors() BEGIN  "
                 "select first_name from actor where actor_id < 5 ; END;\')\n","Query OK"),
                ("session.runSql(\'call get_actors();\')\n","rows in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("muyltiline not catched on script")
  def test_4_3_1_1(self):
      '''[4.3.001]:1 SQL Update table using multiline mode:CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--session-type=classic','--sqlc']
      x_cmds = [("\\\n","..."),
                ("use sakila;\n","       ..."),
                ("Update actor set last_name =\'Test Last Name\', last_update = now() where actor_id = 2;\n","       ..."),
                ("\n","rows in set"),
                ("select last_name from actor where actor_id = 2;\n","Test Last Name")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("not reading the  < batch  pipe when using  inside script")
  def test_4_3_2_1(self):
      '''[4.3.002]:1 SQL Update table using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--session-type=classic','< UpdateTable_SQL.sql ']
      x_cmds = [(";", "mysql-sql>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("not reading the  < batch  pipe when using  inside script")
  def test_4_3_2_2(self):
      '''[4.3.002]:2 SQL Update table using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--session-type=node','<UpdateTable_SQL.sql ']
      x_cmds = [(";", "mysql-sql>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("muyltiline not catched on script")
  def test_4_3_3_1(self):
      '''[4.3.003]:1 SQL Update database using multiline mode: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--session-type=classic','--sqlc']
      x_cmds = [("create schema if not exists AUTOMATION;\n","mysql-sql>"),
                ("\\\n","..."),
                ("ALTER SCHEMA \'AUTOMATION\' DEFAULT COLLATE utf8_general_ci ;\n","..."),
                ("\n","mysql-sql>"),
                ("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = \'AUTOMATION' LIMIT 1;\n","utf8_general")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("not reading the  < batch  pipe when using  inside script")
  def test_4_3_4_1(self):
      '''[4.3.004]:1 SQL Update database using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sqlc', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--session-type=classic','< SchemaDatabaseUpdate_SQL.sql']
      x_cmds = [(";", "mysql-sql>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("not reading the  < batch  pipe when using  inside script")
  def test_4_3_4_2(self):
      '''[4.3.004]:2 SQL Update database using STDIN batch code'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                      '--schema=sakila','--session-type=node','< SchemaDatabaseUpdate_SQL.sql']
      x_cmds = [(";", "mysql-sql>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_5_1(self):
      '''[4.3.005]:1 SQL Update Alter view using multiline mode'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--session-type=classic','--sqlc']
      x_cmds = [("use sakila;\n","mysql-sql>"),
                ("DROP VIEW IF EXISTS sql_viewtest;\n","mysql-sql>"),
                ("create view sql_viewtest as select * from actor where first_name like \'%as%\';\n","mysql-sql>"),
                ("\\\n","       ..."),
                ("alter view sql_viewtest as select count(*) as ActorQuantity from actor;\n","       ..."),
                ("\n","mysql-sql>"),
                ("select * from sql_viewtest;\n","row in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("not reading the  < batch  pipe when using  inside script")
  def test_4_3_6_1(self):
      '''[4.3.006]:1 SQL Update Alter view using STDIN batch code: CLASSIC SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--session-type=classic','< AlterView_SQL.sql']
      x_cmds = [(";", "mysql-sql>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  @unittest.skip("not reading the  < batch  pipe when using  inside script")
  def test_4_3_6_2(self):
      '''[4.3.006]:2 SQL Update Alter view using STDIN batch code: NODE SESSION'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--schema=sakila','--session-type=node','< AlterView_SQL.sql']
      x_cmds = [(";", "mysql-sql>")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

  def test_4_3_7_1(self):
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                      '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                      '--session-type=classic','--sqlc']
      x_cmds = [("use sakila;\n","mysql-sql>"),
                ("DROP procedure IF EXISTS sql_sptest;\n","mysql-sql>"),
                ("\\\n","..."),
                ("DELIMITER $$\n","       ..."),
                ("create procedure sql_sptest\n","       ..."),
                ("BEGIN\n","       ..."),
                ("SELECT count(*) FROM country;\n","       ..."),
                ("END$$\n","       ..."),
                ("\n","mysql-sql>"),
                ("DELIMITER ;\n","mysql-sql>"),
                ("call  sql_sptest;\n","rows in set")
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

 # ----------------------------------------------------------------------
# ----------------------------------------------------------------------
# tc_4_3_7_1("[4.3.007]:1 SQL Update Alter stored procedure using multiline mode")
# tc_4_3_8_1("[4.3.008]:1 SQL Update Alter stored procedure using STDIN batch code: CLASSIC SESSION")
# tc_4_3_8_2("[4.3.008]:2 SQL Update Alter stored procedure using STDIN batch code: NODE SESSION")
# tc_4_3_9_1("[4.3.009]:1 JS Update table using session object: CLASSIC SESSION")
# tc_4_3_9_2("[4.3.009]:2 JS Update table using session object: NODE SESSION")
# tc_4_3_10_1("[4.3.010]:1 JS Update table using multiline mode: CLASSIC SESSION")
# tc_4_3_10_2("[4.3.010]:2 JS Update table using multiline mode: NODE SESSION")
# tc_4_3_11_1("[4.3.011]:1 JS Update table using STDIN batch code: CLASSIC SESSION")
# tc_4_3_11_2("[4.3.011]:2 JS Update table using STDIN batch code: NODE SESSION")
# tc_4_3_12_1("[4.3.012]:1 JS Update database using session object: CLASSIC SESSION")
# tc_4_3_12_2("[4.3.012]:2 JS Update database using session object: NODE SESSION")
# tc_4_3_13_1("[4.3.013]:1 JS Update database using multiline mode: CLASSIC SESSION")
# tc_4_3_13_2("[4.3.013]:2 JS Update database using multiline mode: NODE SESSION")
# tc_4_3_14_1("[4.3.014]:1 JS Update database using STDIN batch code: CLASSIC SESSION")
# tc_4_3_14_2("[4.3.014]:2 JS Update database using STDIN batch code: NODE SESSION")
# tc_4_3_15_1("[4.3.015]:1 JS Update alter view using session object: CLASSIC SESSION")
# tc_4_3_15_2("[4.3.015]:2 JS Update alter view using session object: NODE SESSION")
# tc_4_3_16_1("[4.3.016]:1 JS Update alter view using multiline mode: CLASSIC SESSION")
# tc_4_3_16_2("[4.3.016]:2 JS Update alter view using multiline mode: NODE SESSION")
# tc_4_3_17_1("[4.3.017]:1 JS Update alter view using STDIN batch code: CLASSIC SESSION")
# tc_4_3_17_2("[4.3.017]:2 JS Update alter view using STDIN batch code: NODE SESSION")
# tc_4_3_18_1("[4.3.018]:1 JS Update alter stored procedure using session object: CLASSIC SESSION")
# tc_4_3_18_2("[4.3.018]:2 JS Update alter stored procedure using session object: NODE SESSION")
# tc_4_3_19_1("[4.3.019]:1 JS Update alter stored procedure using multiline mode: CLASSIC SESSION")
# tc_4_3_19_2("[4.3.019]:2 JS Update alter stored procedure using multiline mode: NODE SESSION")
# tc_4_3_20_1("[4.3.020]:1 JS Update alter stored procedure using STDIN batch code: CLASSIC SESSION")
# tc_4_3_20_2("[4.3.020]:2 JS Update alter stored procedure using STDIN batch code: NODE SESSION")
# tc_4_3_21_1("[4.3.021]:1 PY Update table using session object: CLASSIC SESSION")
# tc_4_3_21_2("[4.3.021]:2 PY Update table using session object: NODE SESSION")
# tc_4_3_22_1("[4.3.022]:1 PY Update table using multiline mode: CLASSIC SESSION")
# tc_4_3_22_2("[4.3.022]:2 PY Update table using multiline mode: NODE SESSION")
# tc_4_3_23_1("[4.3.023]:1 PY Update table using STDIN batch code: CLASSIC SESSION")
# tc_4_3_23_2("[4.3.023]:2 PY Update table using STDIN batch code: NODE SESSION")
# tc_4_3_24_1("[4.3.024]:1 PY Update database using session object: CLASSIC SESSION")
# tc_4_3_24_2("[4.3.024]:2 PY Update database using session object: NODE SESSION")
# tc_4_3_25_1("[4.3.025]:1 PY Update database using multiline mode: CLASSIC SESSION")
# tc_4_3_25_2("[4.3.025]:2 PY Update database using multiline mode: NODE SESSION")
# tc_4_3_26_1("[4.3.026]:1 PY Update database using STDIN batch code: CLASSIC SESSION")
# tc_4_3_26_2("[4.3.026]:2 PY Update database using STDIN batch code: NODE SESSION")
# tc_4_3_27_1("[4.3.027]:1 PY Update alter view using session object: CLASSIC SESSION")
# tc_4_3_27_2("[4.3.027]:2 PY Update alter view using session object: NODE SESSION")
# tc_4_3_28_1("[4.3.028]:1 PY Update alter view using multiline mode: CLASSIC SESSION")
# tc_4_3_28_2("[4.3.028]:2 PY Update alter view using multiline mode: NODE SESSION")
# tc_4_3_29_1("[4.3.029]:1 PY Update alter view using STDIN batch code: CLASSIC SESSION")
# tc_4_3_29_2("[4.3.029]:2 PY Update alter view using STDIN batch code: NODE SESSION")
# tc_4_3_30_1("[4.3.030]:1 PY Update alter stored procedure using session object: CLASSIC SESSION")
# tc_4_3_30_2("[4.3.030]:2 PY Update alter stored procedure using session object: NODE SESSION")
# tc_4_3_31_1("[4.3.031]:1 PY Update alter stored procedure using multiline mode: CLASSIC SESSION")
# tc_4_3_31_2("[4.3.031]:2 PY Update alter stored procedure using multiline mode: NODE SESSION")
# tc_4_3_32_1("[4.3.032]:1 PY Update alter stored procedure using STDIN batch code: CLASSIC SESSION")
# tc_4_3_32_2("[4.3.032]:2 PY Update alter stored procedure using STDIN batch code: NODE SESSION")
# tc_4_4_1_1("[4.4.001]:1 SQL Delete table using multiline mode: CLASSIC SESSION")
# tc_4_4_1_2("[4.4.001]:2 SQL Delete table using multiline mode: NODE SESSION")
# tc_4_4_2_1("[4.4.002]:1 SQL Delete table using STDIN batch code: CLASSIC SESSION")
# tc_4_4_2_2("[4.4.002]:2 SQL Delete table using STDIN batch code: NODE SESSION")
# tc_4_4_3_1("[4.4.003]:1 SQL Delete database using multiline mode: CLASSIC SESSION")
# tc_4_4_3_2("[4.4.003]:2 SQL Delete database using multiline mode: NODE SESSION")
# tc_4_4_4_1("[4.4.004]:1 SQL Delete database using STDIN batch code: CLASSIC SESSION")
# tc_4_4_4_2("[4.4.004]:2 SQL Delete database using STDIN batch code: NODE SESSION")
# tc_4_4_5_1("[4.4.005]:1 SQL Delete view using multiline mode: CLASSIC SESSION")
# tc_4_4_5_2("[4.4.005]:2 SQL Delete view using multiline mode: NODE SESSION")
# tc_4_4_6_1("[4.4.006]:1 SQL Delete view using STDIN batch code: CLASSIC SESSION")
# tc_4_4_6_2("[4.4.006]:2 SQL Delete view using STDIN batch code: NODE SESSION")
# tc_4_4_7_1("[4.4.007]:1 SQL Delete stored procedure using multiline mode: CLASSIC SESSION")
# tc_4_4_7_2("[4.4.007]:2 SQL Delete stored procedure using multiline mode: NODE SESSION")
# tc_4_4_8_1("[4.4.008]:1 SQL Delete stored procedure using STDIN batch code: CLASSIC SESSION")
# tc_4_4_8_2("[4.4.008]:2 SQL Delete stored procedure using STDIN batch code: NODE SESSION")
# tc_4_4_9_1("[4.4.009]:1 JS Delete table using session object: CLASSIC SESSION")
# tc_4_4_9_2("[4.4.009]:2 JS Delete table using session object: NODE SESSION")
# tc_4_4_10_1("[4.4.010]:1 JS Delete table using multiline mode: CLASSIC SESSION")
# tc_4_4_10_2("[4.4.010]:2 JS Delete table using multiline modet: NODE SESSION")
# tc_4_4_11_1("[4.4.011]:1 JS Delete table using STDIN batch code: CLASSIC SESSION")
# tc_4_4_11_2("[4.4.011]:2 JS Delete table using STDIN batch code: NODE SESSION")
# tc_4_4_12_1("[4.4.012]:1 JS Delete database using session object: CLASSIC SESSION")
# tc_4_4_12_2("[4.4.012]:2 JS Delete database using session object: NODE SESSION")
# tc_4_4_13_1("[4.4.013]:1 JS Delete database using multiline mode: CLASSIC SESSION")
# tc_4_4_13_2("[4.4.013]:2 JS Delete database using multiline mode: NODE SESSION")
# tc_4_4_14_1("[4.4.014]:1 JS Delete database using STDIN batch code: CLASSIC SESSION")
# tc_4_4_14_2("[4.4.014]:2 JS Delete database using STDIN batch code: NODE SESSION")
# tc_4_4_15_1("[4.4.015]:1 JS Delete view using session object: CLASSIC SESSION")
# tc_4_4_15_2("[4.4.015]:2 JS Delete view using session object: NODE SESSION")
# tc_4_4_16_1("[4.3.016]:1 JS Update alter view using multiline mode: CLASSIC SESSION")
# tc_4_4_16_2("[4.3.016]:2 JS Update alter view using multiline mode: NODE SESSION")
# tc_4_4_17_1("[4.4.017]:1 JS Delete view using STDIN batch code: CLASSIC SESSION")
# tc_4_4_17_2("[4.4.017]:2 JS Delete view using STDIN batch code: NODE SESSION")
# tc_4_4_18_1("[4.4.018]:1 JS Delete stored procedure using session object: CLASSIC SESSION")
# tc_4_4_18_2("[4.4.018]:2 JS Delete stored procedure using session object: NODE SESSION")
# tc_4_4_19_1("[4.4.019]:1 JS Delete stored procedure using multiline mode: CLASSIC SESSION")
# tc_4_4_19_2("[4.4.019]:2 JS Delete stored procedure using multiline mode: NODE SESSION")
# tc_4_4_20_1("[4.4.020]:1 JS Delete stored procedure using STDIN batch code: CLASSIC SESSION")
# tc_4_4_20_2("[4.4.020]:2 JS Delete stored procedure using STDIN batch code: NODE SESSION")
# tc_4_4_21_1("[4.4.021]:1 PY Delete table using session object: CLASSIC SESSION")
# tc_4_4_21_2("[4.4.021]:2 PY Delete table using session object: NODE SESSION")
# tc_4_4_22_1("[4.4.022]:1 PY Delete table using multiline mode: CLASSIC SESSION")
# tc_4_4_22_2("[4.4.022]:2 PY Delete table using multiline mode: NODE SESSION")
# tc_4_4_23_1("[4.4.023]:1 PY Delete table using STDIN batch code: CLASSIC SESSION")
# tc_4_4_23_2("[4.4.023]:2 PY Delete table using STDIN batch code: NODE SESSION")
# tc_4_4_24_1("[4.4.024]:1 PY Delete database using session object: CLASSIC SESSION")
# tc_4_4_24_2("[4.4.024]:2 PY Delete database using session object: NODE SESSION")
# tc_4_4_25_1("[4.4.025]:1 PY Delete database using multiline mode: CLASSIC SESSION")
# tc_4_4_25_2("[4.4.025]:2 PY Delete database using multiline mode: NODE SESSION")
# tc_4_4_26_1("[4.4.026]:1 PY Delete database using STDIN batch code: CLASSIC SESSION")
# tc_4_4_26_2("[4.4.026]:2 PY Delete database using STDIN batch code: NODE SESSION")
# tc_4_4_27_1("[4.4.027]:1 PY Delete view using session object: CLASSIC SESSION")
# tc_4_4_27_2("[4.4.027]:2 PY Delete view using session object: NODE SESSION")
# tc_4_4_28_1("[4.4.028]:1 PY Delete view using multiline mode: CLASSIC SESSION")
# tc_4_4_28_2("[4.4.028]:2 PY Delete view using multiline mode: NODE SESSION")
# tc_4_4_29_1("[4.4.029]:1 PY Delete view using STDIN batch code: CLASSIC SESSION")
# tc_4_4_29_2("[4.4.029]:2 PY Delete view using STDIN batch code: NODE SESSION")
# tc_4_4_30_1("[4.4.030]:1 PY Delete stored procedure using session object: CLASSIC SESSION")
# tc_4_4_30_2("[4.4.030]:2 PY Delete stored procedure using session object: NODE SESSION")
# tc_4_4_31_1("[4.4.031]:1 PY Delete stored procedure using multiline mode: CLASSIC SESSION")
# tc_4_4_31_2("[4.4.031]:2 PY Delete stored procedure using multiline mode: NODE SESSION")
# tc_4_4_32_1("[4.4.032]:1 PY Delete stored procedure using STDIN batch code: CLASSIC SESSION")
# tc_4_4_32_2("[4.4.032]:2 PY Delete stored procedure using STDIN batch code: NODE SESSION")
# tc_4_5_01_1("[4.5.001]:1 JS Transaction with Rollback: CLASSIC SESSION")
# tc_4_5_01_2("[4.5.001]:2 JS Transaction with Rollback: NODE SESSION")
# tc_4_5_02_1("[4.5.002]:1 PY Transaction with Rollback: CLASSIC SESSION")
# tc_4_5_02_2("[4.5.002]:2 PY Transaction with Rollback: NODE SESSION")
# tc_4_5_03_1("[4.5.003]:1 JS Transaction with Commit: CLASSIC SESSION")
# tc_4_5_03_2("[4.5.003]:2 JS Transaction with Commit: NODE SESSION")
# tc_4_5_04_1("[4.5.004]:1 PY Transaction with Commit: CLASSIC SESSION")
# tc_4_5_04_2("[4.5.004]:2 PY Transaction with Commit: NODE SESSION")
# tc_4_6_01_1("[4.6.001]:1 Create a collection with node session: NODE SESSION")
# tc_4_6_02_1("[4.6.002] JS PY Ensure collection exists in a database with node session: NODE SESSION")
# tc_4_6_03_1("[4.6.003] JS PY Add Documents to a collection with node session: NODE SESSION")
# tc_4_6_04_1("[4.6.004] JS PY Find documents from Database using node session: NODE SESSION")
# tc_4_6_05_1("[4.6.005] JS Modify document with Set and Unset with node session: NODE SESSION")
# tc_4_6_06_1("[4.6.006] JS Modify document with Merge and Array with node session: NODE SESSION")
# tc_4_6_07_1("[4.6.007] PY Modify document with Set and Unset with node session: NODE SESSION")
# tc_4_6_08_1("[4.6.008] PY Modify document with Merge and Array with node session: NODE SESSION")

if __name__ == '__main__':
 unittest.main( testRunner=xmlrunner.XMLTestRunner(file("xshell_qa_test.xml","w")))
