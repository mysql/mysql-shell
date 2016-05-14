import subprocess
import time
import sys
import os
import threading
import functools
from utils import *

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

def read_line(proc, fd, end_string, timeout=0):
    data = ""
    new_byte = b''
    t = time.time()
    while (new_byte != b'\n') and (time.time() - t < timeout):
        try:
            new_byte = fd.read(1)
            if new_byte == '' and proc.poll() != None:
                break
            elif new_byte:
                # data += new_byte
                data += new_byte
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

def read_til_getShell(proc, fd, text, timeout=0, debug=False):
    data = []
    line = ""
    t = time.time()
    while line != text and (time.time() - t < timeout) and proc.poll() == None:
        try:
            line = read_line(proc, fd, text, timeout)
            if debug:
                print("FROM SHELL: %s"%line)
            if line:
                data.append(line)
            elif proc.poll() is not None:
                break
        except ValueError:
            # timeout occurred
            print("read_line_timeout")
            break

    return "".join(data)
    # return ''.join(map(str, data))

@timeout(4)
def exec_xshell_commands(init_cmdLine, commandList, expectbefore=None):
    RESULTS = "FAILED"
    commandbefore = ""
    if expectbefore is None:
        if "--sql" in init_cmdLine:
            expectbefore = "mysql-sql>"
        elif "--py" in init_cmdLine:
            expectbefore = "mysql-py>"
        elif "--js" in init_cmdLine:
            expectbefore = "mysql-js>"
        else:
            expectbefore = "mysql-js>"
    debug = False
    if os.getenv("MYSQLX_TEST_DEBUG"):
      debug = True
      print(init_cmdLine)
    p = subprocess.Popen(init_cmdLine, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
    for command, lookup in commandList:
        if debug:
            print("TO SHELL: %s" % command)
        p.stdin.write(command+"\n")
        p.stdin.flush()
        if debug:
            print("EXPECT: %s" % expectbefore)
        found = read_til_getShell(p, p.stdout, expectbefore, output_timeout, debug)
        if found.find(expectbefore, 0, len(found)) == -1:
            stdout,stderr = p.communicate()
            print("After sending %s" % command)
            print("Expected output '%s' NOT FOUND"%expectbefore)
            print("Actual output:")
            print(found)
            print(stdout)
            return "FAIL"
        expectbefore = lookup
        commandbefore =command
    p.stdin.write(commandbefore + "\n")
    p.stdin.flush()
    stdout,stderr = p.communicate()
    found = stdout.find(expectbefore, 0, len(stdout))
    if found == -1:
        print()
        print("Expected output '%s' NOT FOUND"%expectbefore)
        print("Actual output:")
        print(stdout)
        return "FAIL"
    return "PASS"


###############   G L O B A L   V A R I A B L E S   ############################33
MYSQL_SHELL = mysqlx_path
output_timeout = 600

class LOCALHOST:
    if os.getenv("MYSQL_URI"):
        uri = os.getenv("MYSQL_URI")
        user, _, rest = uri.partition("@")
        if ':' in user:
            user, _, password = user.partition(":")
        else:
            password = ''
        if ':' in rest:
            host, _, xprotocol_port = rest.partition(":")
        else:
            host = rest
            xprotocol_port = '33060'
        port = os.getenv("MYSQL_PORT") or "3306"
    else:
        user = 'root'
        password = 'guidev!'
        host = 'localhost'
        xprotocol_port = '33060'
        port = "3578"

class REMOTEHOST:
    if os.getenv("MYSQL_REMOTE_URI"):
        uri = os.getenv("MYSQL_REMOTE_URI")
        user, _, rest = uri.partition("@")
        if ':' in user:
            user, _, password = user.partition(":")
        else:
            password = ''
        if ':' in rest:
            host, _, xprotocol_port = rest.partition(":")
        else:
            host = rest
            xprotocol_port = '33060'
        port = '3306'
    else:
        user = 'root'
        password = 'guidev!'
        # host = '10.161.157.198'
        host = '10.172.166.248'
        xprotocol_port = '33060'
        port = '3357'


###############     testcases functions ######################################
#
# def testcase_init():
#     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7']
#     x_cmds = [("var mysql=require(\'mysql\').mysql;\n", 'mysql-js>'),
#               ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user,LOCALHOST.password,LOCALHOST.host, LOCALHOST.port) , 'mysql-js>'),
#               ("session.sql(\"use sakila;\");\n" , 'Query OK'),
#               ("session.sql(\"select * from actor_info where actor_id <10 order by actor_id desc limit 2 offset 3\");\n" , 'rows in set')
#               ]
#
#     print("Executing [%s]... " % ("TESTCASE [1.0.1]"))
#     Resultado = exec_xshell_commands(init_command, x_cmds)
#     print(Resultado)
#
# def testcase_init2():
#     init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7']
#     x_cmds = [("var mysql=require(\'mysql\').mysql;\n", 'mysql-js>'),
#               ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user,LOCALHOST.password,LOCALHOST.host, LOCALHOST.port) , 'mysql-js>'),
#               ("session.sql(\"use sakila;\");\n" , 'Query OK'),
#               ("session.sql(\"select * from actor_info where actor_id <10 order by actor_id desc limit 2 offset 3\");\n" , 'rows in set')
#               ]
#     print("Executing [%s]... " % ("TESTCASE [1.0.2]"))
#     Resultado = exec_xshell_commands(init_command, x_cmds)
#     print(Resultado)

def tc_2_0_01_1(self):
    tc_name = "[2.0.01]:1 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                    '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=app', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_01_2(self):
    tc_name = "[2.0.01]:2 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '-p', '--passwords-from-stdin',
                    '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=app']
    x_cmds = [(LOCALHOST.password+"\n", 'mysql-js>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:   ISSUES SETTING PASSWORD IN CONSOLE

def tc_2_0_01_3(self):
    tc_name = "[2.0.01]:3 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '-p','--passwords-from-stdin',
                    '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port]
    x_cmds = [(LOCALHOST.password+"\n", 'mysql-js>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:   ISSUES SETTING PASSWORD IN CONSOLE

def tc_2_0_01_4(self):
    tc_name = "[2.0.01]:4 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                    'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                      LOCALHOST.xprotocol_port), '--session-type=app', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_01_5(self):
    tc_name = "[2.0.01]:5 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                    'mysqlx://{0}:{1}@{2}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),
                    '--session-type=app', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")


def tc_2_0_01_6(self):
    tc_name = "[2.0.01]:6 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
         '-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--session-type=node', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_01_7(self):
    tc_name = "[2.0.01]:7 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '-i', '-u' + LOCALHOST.user, '-p', '--passwords-from-stdin', '-h' + LOCALHOST.host, '--session-type=node']
    x_cmds = [(LOCALHOST.password+"\n", 'mysql-js>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #   NOTE:   ISSUES SETTING PASSWORD IN CONSOLE

def tc_2_0_01_8(self):
    tc_name = "[2.0.01]:8 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                    'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                      LOCALHOST.xprotocol_port), '--session-type=node', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_01_9(self):
    tc_name = "[2.0.01]:9 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                    'mysqlx://{0}:{1}@{2}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),
                    '--session-type=node', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_01_10(self):
    tc_name = "[2.0.01]:10 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                    '-h' + LOCALHOST.host,'-P' + LOCALHOST.port, '--session-type=classic', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_01_11(self):
    tc_name = "[2.0.01]:11 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '-i', '-u' + LOCALHOST.user,'-p', '--passwords-from-stdin', '-h' + LOCALHOST.host,'--session-type=classic']
    x_cmds = [(";\n", 'mysql-js>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #   NOTE:   ISSUES SETTING PASSWORD IN CONSOLE

def tc_2_0_01_12(self):
    tc_name = "[2.0.01]:12 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                    'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                      LOCALHOST.port), '--session-type=classic', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_01_13(self):
    tc_name = "[2.0.01]:13 Connect local Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                    'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                      LOCALHOST.port),'--session-type=classic', '--js']
    x_cmds = [(";\n", 'mysql-js>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_02_1(self):
    tc_name = "[2.0.02]:1 Connect remote Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                    '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.xprotocol_port, '--session-type=app', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_02_2(self):
    tc_name = "[2.0.02]:2 Connect remote Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p', '--passwords-from-stdin','-h' + REMOTEHOST.host]
    x_cmds = [(";\n", 'mysql-js>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:   ISSUES SETTING PASSWORD IN CONSOLE

def tc_2_0_02_3(self):
    tc_name = "[2.0.02]:3 Connect remote Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p','--passwords-from-stdin', '-h' + REMOTEHOST.host]
    x_cmds = [(";\n", 'mysql-js>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:   ISSUES SETTING PASSWORD IN CONSOLE

def tc_2_0_02_4(self):
    tc_name = "[2.0.02]:4 Connect remote Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                    'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                      REMOTEHOST.xprotocol_port), '--session-type=app', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_02_5(self):
    tc_name = "[2.0.02]:5 Connect remote Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                    'mysqlx://{0}:{1}@{2}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host),
                    '--session-type=app', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_02_6(self):
    tc_name = "[2.0.02]:6 Connect remote Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                    '-h' + REMOTEHOST.host, '-P' + REMOTEHOST.xprotocol_port, '--session-type=node', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_02_7(self):
    tc_name = "[2.0.02]:7 Connect remote Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p', '--passwords-from-stdin','-h' + REMOTEHOST.host,'--session-type=node']
    x_cmds = [(";\n", 'mysql-js>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:   ISSUES SETTING PASSWORD IN CONSOLE

def tc_2_0_02_8(self):
    tc_name = "[2.0.02]:8 Connect remote Server w/Command Line Args"
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                    'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                      REMOTEHOST.xprotocol_port), '--session-type=node', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_02_9(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                    'mysqlx://{0}:{1}@{2}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host),
                    '--session-type=node', '--sql']
    x_cmds = [(";\n", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_02_10(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                    '-h' + REMOTEHOST.host,'-P' + REMOTEHOST.port, '--session-type=classic', '--sql']
    x_cmds = [(";", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_02_11(self, tc_name):
    self.__doc__ = tc_name
    init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user,'-p', '--passwords-from-stdin','-h' + REMOTEHOST.host,'--session-type=classic']
    x_cmds = [(REMOTEHOST.password, 'mysql-js>')
              ]
    results = exec_xshell_commands(init_command, x_cmds, "Enter password:")
    self.assertEqual(results, "PASS")

def tc_2_0_02_12(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                    'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                      REMOTEHOST.port), '--session-type=classic', '--sql']
    x_cmds = [(";", 'mysql-sql>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_02_13(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                          'mysqlx://{0}:{1}@{2}:{3}'.format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                            REMOTEHOST.port), '--session-type=classic', '--js']
    x_cmds = [(";\n", 'mysql-js>')
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_03_2(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full','--sql', '--passwords-from-stdin']
    x_cmds = [("\\connect {0}:{1}@{2}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),"Creating Application Session"),
              ("\\js\n", "mysql-js>"),
              ("print(session);\n", "XSession:"),
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_03_3(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full','--sql','--passwords-from-stdin']
    x_cmds = [("\\connect {0}:{1}@{2}:{3};\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,LOCALHOST.xprotocol_port),
               "Creating Application Session"),
              ("\\js\n", "mysql-js"),
              ("print(session);\n", "XSession:"),
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_03_4(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full','--passwords-from-stdin']
    x_cmds = [("\\connect -n {0}:{1}@{2}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host),
               "Creating Node Session"),
              ("print(session);\n", "NodeSession:"),
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_03_5(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full','--passwords-from-stdin']
    x_cmds = [("\\connect -n {0}:{1}@{2}:{3};\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                  LOCALHOST.xprotocol_port),"Creating Node Session"),
              ("print(session);\n", "NodeSession:"),
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")


def tc_2_0_03_6(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full','--passwords-from-stdin']
    x_cmds = [("\\connect -c {0}:{1}@{2}:{3}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                            LOCALHOST.port),"Creating Classic Session"),
              ("print(session);\n", "ClassicSession:"),
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")


def tc_2_0_04_2(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full','--sql','--passwords-from-stdin']
    x_cmds = [("\\connect {0}:{1}@{2}\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host),"Creating Application Session"),
              ("\\js\n", "mysql-js>"),
              ("print(session);\n", "XSession:"),
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_04_3(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full','--sql','--passwords-from-stdin']
    x_cmds = [("\\connect {0}:{1}@{2}:{3};\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,REMOTEHOST.xprotocol_port),
               "Creating Application Session"),
              ("\\js\n", "mysql-js"),
              ("print(session);\n", "XSession:"),
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_04_4(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full','--passwords-from-stdin']
    x_cmds = [("\\connect -n {0}:{1}@{2}\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host),
               "Creating Node Session"),
              ("print(session);\n", "NodeSession:"),
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_04_5(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full','--passwords-from-stdin']
    x_cmds = [("\\connect -n {0}:{1}@{2}:{3};\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                                  REMOTEHOST.xprotocol_port),"Creating Node Session"),
              ("print(session);\n", "NodeSession:"),
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_2_0_04_6(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full','--passwords-from-stdin']
    x_cmds = [("\\connect -c {0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, REMOTEHOST.password, REMOTEHOST.host,
                                                            REMOTEHOST.port),"Creating Classic Session"),
              ("print(session);\n", "ClassicSession:"),
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

#  -------------------------------------------------------------------------------------
#  -------------------------------------------------------------------------------------

#  -------------------------------------------------------------------------------------
#  -------------------------------------------------------------------------------------
#  -------------------------------------------------------------------------------------

def tc_4_2_16_1(self, tc_name):
    self.__doc__ = tc_name
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
    self.assertEqual(results, "PASS")

def tc_4_2_16_2(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7','--py']
    x_cmds = [("import mysql\n", "mysql-py>"),
              ("session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                               LOCALHOST.host,LOCALHOST.port ), "mysql-py>"),
              ("session.sql(\'use sakila;\')\n","Query OK"),
              ("session.sql(\"drop procedure if exists get_actors;\")\n","Query OK"),
              ("session.sql(\"CREATE PROCEDURE get_actors() BEGIN  "
               "select first_name from actor where actor_id < 5 ; END;\")\n","Query OK"),
              ("session.sql(\'call get_actors();\')\n","rows in set")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_4_3_1_1(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                    '--session-type=classic','--sql']
    x_cmds = [("\\\n","..."),
              ("use sakila;\n","..."),
              ("Update actor set last_name =\'Test Last Name\', last_update = now() where actor_id = 2;\n","..."),
              ("\n","rows in set"),
              ("select last_name from actor where actor_id = 2;\n","Test Last Name")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_4_3_2_1(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                    '--schema=sakila','--session-type=classic','< UpdateTable_SQL.sql ']
    x_cmds = [(";", "mysql-sql>")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:  ISSUES USING BATCH FILES

def tc_4_3_2_2(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                    '--schema=sakila','--session-type=node','<UpdateTable_SQL.sql ']
    x_cmds = [(";", "mysql-sql>")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:  ISSUES USING BATCH FILES

def tc_4_3_3_1(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                    '--session-type=classic','--sql']
    x_cmds = [("create schema if not exists AUTOMATION;\n","mysql-sql>"),
              ("\\\n","..."),
              ("ALTER SCHEMA \'AUTOMATION\' DEFAULT COLLATE utf8_general_ci ;\n","..."),
              ("\n","mysql-sql>"),
              ("SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = \'AUTOMATION' LIMIT 1;\n","utf8_general")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:   ISSUES MULTILINE FROM XSHELL SIDE

def tc_4_3_4_1(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                    '--schema=sakila','--session-type=classic','< SchemaDatabaseUpdate_SQL.sql']
    x_cmds = [(";", "mysql-sql>")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:  ISSUES USING BATCH FILES

def tc_4_3_4_2(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                    '--schema=sakila','--session-type=node','< SchemaDatabaseUpdate_SQL.sql']
    x_cmds = [(";", "mysql-sql>")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:  ISSUES USING BATCH FILES

def tc_4_3_5_1(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                    '--session-type=classic','--sql']
    x_cmds = [("use sakila;\n","mysql-sql>"),
              ("DROP VIEW IF EXISTS sql_viewtest;\n","mysql-sql>"),
              ("create view sql_viewtest as select * from actor where first_name like \'%as%\';\n","mysql-sql>"),
              ("\\\n","..."),
              ("alter view sql_viewtest as select count(*) as ActorQuantity from actor;\n","..."),
              ("\n","mysql-sql>"),
              ("select * from sql_viewtest;\n","rows in set")
              ]

    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_4_3_6_1(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                    '--schema=sakila','--session-type=classic','< AlterView_SQL.sql']
    x_cmds = [(";", "mysql-sql>")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:  ISSUES USING BATCH FILES

def tc_4_3_6_2(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                    '--schema=sakila','--session-type=node','< AlterView_SQL.sql']
    x_cmds = [(";", "mysql-sql>")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:  ISSUES USING BATCH FILES

def tc_4_3_7_1(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                    '--session-type=classic','--sql']
    x_cmds = [("use sakila;\n","mysql-sql>"),
              ("DROP procedure IF EXISTS sql_sptest;\n","mysql-sql>"),
              ("\\\n","..."),
              ("DELIMITER $$\n","..."),
              ("create procedure sql_sptest;\n","..."),
              ("BEGIN\n","..."),
              ("SELECT count(*) FROM country;\n","..."),
              ("END$$\n","..."),
              ("\n","mysql-sql>"),
              ("DELIMITER ;\n","mysql-sql>"),
              ("call  sql_sptest;\n","rows in set")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_4_3_8_1(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                    '--schema=sakila','--session-type=classic','< AlterStoreProcedure_SQL.sql']
    x_cmds = [(";", "mysql-sql>")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:  ISSUES USING BATCH FILES

def tc_4_3_8_2(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                    '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host, '-P' + LOCALHOST.port,
                    '--schema=sakila','--session-type=node','< AlterStoreProcedure_SQL.sql']
    x_cmds = [(";", "mysql-sql>")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")
    #  NOTE:  ISSUES USING BATCH FILES

def tc_4_3_9_1(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full']
    x_cmds = [("var mysql=require(\'mysql\').mysql;\n","mysql-js>"),
              ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                              LOCALHOST.host, LOCALHOST.port),"mysql-js>"),
              ("session.executeSql(\"use sakila;\");\n","Query OK"),
              ("session.executeSql(\"drop table if exists sakila.friends;\");\n","Query OK"),
              ("session.executeSql(\'create table sakila.friends (name varchar(50), last_name varchar(50), "
               "age integer, gender varchar(20));\');\n","Query OK"),
              ("session.executeSql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES (\'jack\',"
               "\'black\', 17, \'male\');\");\n","mysql-js>"),
              ("session.executeSql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES (\'ruben\',"
               "\'morquecho\', 40, \'male\');\");\n","mysql-js>"),
              ("session.executeSql(\"UPDATE friends SET name=\'ruben dario\' where name =  '\ruben\';\");\n","mysql-js>"),
              ("session.executeSql(\"SELECT * from friends where name LIKE '\%ruben%\';\");\n","ruben dario")
              ]

    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_4_3_9_2(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full']
    x_cmds = [("var mysqlx=require(\'mysqlx\').mysqlx;\n","mysql-js>"),
              ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                              LOCALHOST.host),"mysql-js>"),
              ("session.sql(\"use sakila;\").execute();\n","Query OK"),
              ("session.sql(\"drop table if exists sakila.friends;\").execute();\n","Query OK"),
              ("session.sql(\'create table sakila.friends (name varchar(50), last_name varchar(50), "
               "age integer, gender varchar(20));\').execute();\n","Query OK"),
              ("var table = session.sakila.friends;\n","mysql-js>"),
              ("table.insert(\'name\',\'last_name\',\'age\',\'gender\').values(\'jack\',\'black\', 17, \'male\');\n","Query OK"),
              ("table.insert(\'name\',\'last_name\',\'age\',\'gender\').values(\'ruben\',\'morquecho\', 40, \'male\');\n","Query OK"),
              ("var res_ruben = table.update().set(\'name\',\'ruben dario\').set(\'age\',42).where(\'name=\"ruben\"\').execute();\n","mysql-js>"),
              ("print(table.select());\n","mysql-js>")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_4_3_10_1(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full']
    x_cmds = [("var mysql=require(\'mysql\').mysql;\n","mysql-js>"),
              ("var session=mysql.getClassicSession(\'{0}:{1}@{2}:{3}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                    LOCALHOST.host, LOCALHOST.port), "mysql-js>"),
              ("session.executeSql(\"use sakila;\");\n","Query OK"),
              ("session.executeSql(\"drop table if exists sakila.friends;\");\n","Query OK"),
              ("session.executeSql(\'create table sakila.friends (name varchar(50), last_name varchar(50), "
               "age integer, gender varchar(20));\');\n","Query OK"),
              ("session.executeSql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES (\'jack\',"
               "\'black\', 17, \'male\');\");\n","mysql-js>"),
              ("session.executeSql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES (\'ruben\',"
               "\'morquecho\', 40, \'male\');\");\n","mysql-js>"),
              ("\\\n","..."),
              ("session.executeSql(\"UPDATE friends SET name=\'ruben dario\' where name =  '\ruben\';\");\n","..."),
              ("session.executeSql(\"UPDATE friends SET name=\'jackie chan\' where name =  '\jack\';\");\n","..."),
              ("\n","mysql-js>"),
              ("session.executeSql(\"SELECT * from friends where name LIKE '\%ruben%\';\");\n","ruben dario"),
              ("session.executeSql(\"SELECT * from friends where name LIKE '\%jackie chan%\';\");\n","jackie chan")
              ]

    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")

def tc_4_3_10_2(self, tc_name):
    self.__doc__ = tc_name
    results = ''
    init_command = [MYSQL_SHELL, '--interactive=full']
    x_cmds = [("var mysqlx=require(\'mysqlx\').mysqlx;\n","mysql-js>"),
              ("var session=mysqlx.getNodeSession(\'{0}:{1}@{2}\');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                              LOCALHOST.host),"mysql-js>"),
              ("session.sql(\"use sakila;\").execute();\n","Query OK"),
              ("session.sql(\"drop table if exists sakila.friends;\").execute();\n","Query OK"),
              ("session.sql(\'create table sakila.friends (name varchar(50), last_name varchar(50), "
               "age integer, gender varchar(20));\').execute();\n","Query OK"),
              ("var table = session.sakila.friends;\n","mysql-js>"),
              ("table.insert(\'name\',\'last_name\',\'age\',\'gender\').values(\'jack\',\'black\', 17, \'male\');\n","Query OK"),
              ("table.insert(\'name\',\'last_name\',\'age\',\'gender\').values(\'ruben\',\'morquecho\', 40, \'male\');\n","Query OK"),
              ("\\\n","..."),
              ("var res_ruben = table.update().set(\'name\',\'ruben dario\').set(\'age\',42).where(\'name=\"ruben\"\').execute();\n","..."),
              ("var res_jack = table.update().set(\'name\',\'jackie chan\').set(\'age\',18).where(\'name=\"jack\"\').execute();\n","..."),
              ("\n","..."),
              ("print(table.select());\n","ruben dario"),
              ("print(table.select());\n","ruben dario")
              ]
    results = exec_xshell_commands(init_command, x_cmds)
    self.assertEqual(results, "PASS")


#  NOTE:   ISSUES SETTING PASSWORD IN CONSOLE
# ## ------------------------------------------------------------------------------------------------------------- ###
# ## --------    T E TS T C A S E S     E X E C U T I O N  ------------------------------------------------------ ###
# ## ------------------------------------------------------------------------------------------------------------- ###

class QAShellTests(ShellTestCase):
# testcase_init()
# testcase_init2()
# tc_2_0_01_1()
# tc_2_0_01_2()
# tc_2_0_01_3()
# tc_2_0_01_4()
# tc_2_0_01_5()
# tc_2_0_01_6()
# tc_2_0_01_7()
# tc_2_0_01_8()
# tc_2_0_01_9()
# tc_2_0_01_10()
# tc_2_0_01_11()
# tc_2_0_01_12()
# tc_2_0_01_13()
# tc_2_0_02_1()
# tc_2_0_02_2()
# tc_2_0_02_3()
# tc_2_0_02_4()
# tc_2_0_02_5()
# tc_2_0_02_6()
# tc_2_0_02_7()
# tc_2_0_02_8()
    def test_2_0_02_9(self):
        tc_2_0_02_9(self, "[2.0.02]:9 Connect remote Server w/Command Line Args")

    def test_2_0_02_10(self):
        tc_2_0_02_10(self, "[2.0.02]:10 Connect remote Server w/Command Line Args")

    def test_2_0_02_11(self):
        tc_2_0_02_11(self, "[2.0.02]:11 Connect remote Server w/Command Line Args")

    def test_2_0_02_12(self):
        tc_2_0_02_12(self, "[2.0.02]:12 Connect remote Server w/Command Line Args")

    def test_2_0_02_13(self):
        tc_2_0_02_13(self, "[2.0.02]:13 Connect remote Server w/Command Line Args")

    def test_2_0_03_2(self):
        tc_2_0_03_2(self, "[2.0.03]:2 Connect local Server on SQL mode: APPLICATION SESSION W/O PORT")

    def test_2_0_03_3(self):
        tc_2_0_03_3(self, "[2.0.03]:3 Connect local Server on SQL mode: APPLICATION SESSION WITH PORT")

    def test_2_0_03_4(self):
        tc_2_0_03_4(self, "[2.0.03]:4 Connect local Server on SQL mode: NODE SESSION W/O PORT")

    def test_2_0_03_5(self):
        tc_2_0_03_5(self, "[2.0.03]:5 Connect local Server on SQL mode: NODE SESSION WITH PORT")

    def test_2_0_03_6(self):
        tc_2_0_03_6(self, "[2.0.03]:6 Connect local Server on SQL mode: CLASSIC SESSION")

    def test_2_0_04_2(self):
        tc_2_0_04_2(self, "[2.0.03]:2 Connect remote Server on SQL mode: APPLICATION SESSION W/O PORT")

    def test_2_0_04_3(self):
        tc_2_0_04_3(self, "[2.0.03]:3 Connect remote Server on SQL mode: APPLICATION SESSION WITH PORT")

    def test_2_0_04_4(self):
        tc_2_0_04_4(self, "[2.0.03]:4 Connect remote Server on SQL mode: NODE SESSION W/O PORT")

    def test_2_0_04_5(self):
        tc_2_0_04_5(self, "[2.0.03]:5 Connect remote Server on SQL mode: NODE SESSION WITH PORT")

    def test_2_0_04_6(self):
        tc_2_0_04_6(self, "[2.0.03]:6 Connect remote Server on SQL mode: CLASSIC SESSION")
# ----------------------------------------------------------------------
# ----------------------------------------------------------------------
# tc_4_2_16_1("[4.2.016]:1 PY Read executing the stored procedure")
# tc_4_2_16_2("[4.2.016]:2 PY Read executing the stored procedure")
# tc_4_3_1_1("[4.3.001]:1 SQL Update table using multiline mode")
# tc_4_3_2_1("[4.3.002]:1 SQL Update table using STDIN batch code")
# tc_4_3_2_2("[4.3.002]:2 SQL Update table using STDIN batch code")
# tc_4_3_3_1("[4.3.003]:1 SQL Update database using multiline mode")
# tc_4_3_4_1("[4.3.004]:1 SQL Update database using STDIN batch code")
# tc_4_3_4_2("[4.3.004]:2 SQL Update database using STDIN batch code")
# tc_4_3_5_1("[4.3.005]:1 SQL Update Alter view using multiline mode")
# tc_4_3_6_1("[4.3.006]:1 SQL Update Alter view using STDIN batch code: CLASSIC SESSION")
# tc_4_3_6_2("[4.3.006]:2 SQL Update Alter view using STDIN batch code: NODE SESSION")
# tc_4_3_7_1("[4.3.007]:1 SQL Update Alter stored procedure using multiline mode")
# tc_4_3_8_1("[4.3.008]:1 SQL Update Alter stored procedure using STDIN batch code: CLASSIC SESSION")
# tc_4_3_8_2("[4.3.008]:2 SQL Update Alter stored procedure using STDIN batch code: NODE SESSION")
# tc_4_3_9_1("[4.3.009]:1 JS Update table using session object: CLASSIC SESSION")
# tc_4_3_9_2("[4.3.009]:2 JS Update table using session object: NODE SESSION")
# tc_4_3_10_1("[4.3.010]:1 JS Update table using multiline mode: CLASSIC SESSION")
# tc_4_3_10_2("[4.3.010]:2 JS Update table using multiline mode: NODE SESSION")


# ## ------------------------------------------------------------------------------------------------------------- ###
# ## ------------------------------------------------------------------------------------------------------------- ###
