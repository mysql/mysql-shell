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
      '''[2.0.03]:2 Connect local Server on SQL mode: APPLICATION SESSION W/O PORT'''
      results = ''
      init_command = [MYSQL_SHELL, '--interactive=full','--sql']
      x_cmds = [("\\connect {0}:{1}@{2}:{3};\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,LOCALHOST.xprotocol_port),
                 "Creating Application Session"),
                ("\\js\n", "mysql-js"),
                ("print(session);\n", "XSession:"),
                ]
      results = exec_xshell_commands(init_command, x_cmds)
      self.assertEqual(results, 'PASS')

# tc_2_0_03_2("[2.0.03]:2 Connect local Server on SQL mode: APPLICATION SESSION W/O PORT")
# tc_2_0_03_3("[2.0.03]:3 Connect local Server on SQL mode: APPLICATION SESSION WITH PORT")
# tc_2_0_03_4("[2.0.03]:4 Connect local Server on SQL mode: NODE SESSION W/O PORT")
# tc_2_0_03_5("[2.0.03]:5 Connect local Server on SQL mode: NODE SESSION WITH PORT")
# tc_2_0_03_6("[2.0.03]:6 Connect local Server on SQL mode: CLASSIC SESSION")
# tc_2_0_04_2("[2.0.03]:2 Connect remote Server on SQL mode: APPLICATION SESSION W/O PORT")
# tc_2_0_04_3("[2.0.03]:3 Connect remote Server on SQL mode: APPLICATION SESSION WITH PORT")
# tc_2_0_04_4("[2.0.03]:4 Connect remote Server on SQL mode: NODE SESSION W/O PORT")
# tc_2_0_04_5("[2.0.03]:5 Connect remote Server on SQL mode: NODE SESSION WITH PORT")
# tc_2_0_04_6("[2.0.03]:6 Connect remote Server on SQL mode: CLASSIC SESSION")
# tc_2_0_05_2("[2.0.05]:2 Connect local Server on JS mode: NODE SESSION")
# tc_2_0_05_3("[2.0.05]:3 Connect local Server on JS mode: NODE SESSION")
# tc_2_0_05_4("[2.0.05]:4 Connect local Server on JS mode: CLASSIC SESSION")
# tc_2_0_06_2("[2.0.06]:2 Connect remote Server on JS mode: NODE SESSION")
# tc_2_0_06_3("[2.0.06]:3 Connect remote Server on JS mode: NODE SESSION")
# tc_2_0_06_4("[2.0.06]:4 Connect remote Server on JS mode: CLASSIC SESSION")
# tc_2_0_07_2("[2.0.07]:2 Connect local Server on PY mode: NODE SESSION")
# tc_2_0_07_3("[2.0.07]:3 Connect local Server on PY mode: NODE SESSION")
# tc_2_0_07_4("[2.0.07]:4 Connect local Server on PY mode: CLASSIC SESSION")
# tc_2_0_08_2("[2.0.08]:2 Connect remote Server on PY mode: NODE SESSION")
# tc_2_0_08_3("[2.0.08]:3 Connect remote Server on PY mode: NODE SESSION")
# tc_2_0_08_4("[2.0.08]:4 Connect remote Server on PY mode: CLASSIC SESSION")

if __name__ == '__main__':
     unittest.main(verbosity=2)