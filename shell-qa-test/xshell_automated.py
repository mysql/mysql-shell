# Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

def timeout(timeout):
    def deco(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            # res = [Exception('function [%s] timeout [%s seconds] exceeded!' % (func.__name__, timeout))]
            #res = [Exception('FAILED timeout [%s seconds] exceeded! ' % ( timeout))]
            #globales = func.func_globals
            res = [Exception('FAILED timeout [%s seconds] exceeded! \n\r SEARCHED: [ %s ]  \n\r FOUND: [ %s ] ' % (timeout,globalvar.last_search,globalvar.last_found))]
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


def read_line(p):
    data = ""
    new_byte = b''
    #t = time.time()
    while (new_byte != b'\n'):
        try:
            new_byte = p.stdout.read(1)
            if new_byte == '' and p.poll() != None:
                break
            elif new_byte:
                # data += new_byte
                data += str(new_byte) ##, encoding='utf-8')
                #if any(n in data for n in xPrompts.prompts):
                for matches in xPrompts.prompts:
                    if matches==data:
                    #if re.match('\A'+matches,data):
                        return data
                        #break;
            elif p.poll() is not None:
                break
        except ValueError:
            break
    return data

@timeout(15)
def read_til_getShell(p):
    globalvar.last_found=""
    data = []
    line = ""
    shell_found= False
    while not shell_found:
    #while not any(n in '\A'+line for n in xPrompts.prompts):
        try:
            line = read_line(p)
            globalvar.last_found = globalvar.last_found + line
            if line:
                data.append(line)
            elif p.poll() is not None:
                shell_found = True
                break
        except ValueError:
            print("read_line_timeout")
            shell_found = True
            break
        for matches in xPrompts.prompts:        ##
            if matches==line:
            #if re.match('\A' + matches, line):   ##
                return  "".join(data)            ##

    return "".join(data)

def exec_xshell_commands(init_cmdLine,x_cmds):
    RESULTS="PASS"
    globalvar.last_search = ""
    if "--sql"  in init_cmdLine:
        sessionType = "mysql-sql>"
    elif "--sqlc"  in init_cmdLine:
        sessionType = "mysql-sql>"
    elif "--py" in init_cmdLine:
        sessionType = "mysql-py>"
    elif "--js" in init_cmdLine:
        sessionType = "mysql-js>"
    else:
        sessionType = "mysql-js>"
    allXshellSession = ""
    allFound = []
    p = subprocess.Popen(init_cmdLine, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
    globalvar.last_search = sessionType
    found = str(read_til_getShell(p))
    allFound.append(found)
    if found.find(sessionType, 0, len(found)) == -1 and found.find("FAILED", 0, len(found)) >= 0:
        # RESULTS = "FAILED"
        if os.getenv("TEST_DEBUG"):
            print "\n".join(allFound)
            import traceback
            traceback.print_stack()
        return found #, RESULTS
    for x in range(0, x_cmds.__len__()-1):
        command=x_cmds[x]
        #for command in x_cmds:
        if os.getenv("TEST_DEBUG"):
            print "EXEC", command
        p.stdin.write(bytearray(command[0], 'ascii'))
        p.stdin.flush()
        globalvar.last_search = str(command[1])
        allXshellSession = allXshellSession + globalvar.last_search
        found = str(read_til_getShell(p)).replace('\r\n', '\n')
        allFound.append(found)
        ###  I JUST CHANGE THE AND INSTEAD OR
        if found.find(command[1].replace('\r\n', '\n'), 0, len(found)) == -1 or found.find("FAILED", 0, len(found)) >= 0:  ### changed and/or
            if os.getenv("TEST_DEBUG"):
                print "\n".join(allFound)
                import traceback
                traceback.print_stack()
                print "NOT FOUND: [ "+str(command[1])+" ]"

            # RESULTS = "FAILED"
            return found + "\n\r NOT FOUND: [ "+str(command[1])+" ]"
    ####   execute last iteration
    command=x_cmds[x_cmds.__len__()-1]
    p.stdin.write(bytearray(command[0], 'ascii'))
    p.stdin.flush()
    globalvar.last_search = str(command[1])
    allXshellSession = allXshellSession+ globalvar.last_search
    stdout,stderr = p.communicate()
    allFound.append(stdout)
    allFound.append(stderr)
    if p.returncode < 0:
        return "FAILED: mysqlsh exited with %s" % p.returncode
    #found = str(read_til_getShell(p, p.stdout, command[1]))
    #if found.find(command[1], 0, len(found)) == -1 or found.find("FAILED", 0, len(found)) >= 0:
    #    return found + "\n\r NOT FOUND: [ "+str(command[1])+" ]"
    #return RESULTS
    found = stdout.replace('\r\n', '\n').find(bytearray(command[1].replace('\r\n', '\n'), "ascii"), 0, len(stdout))
    allFound.append(found)
    if found == -1 and x_cmds.__len__() != 0 :
        #return "FAILED SEARCHING: "+globalvar.last_search+" , STDOUT: "+ str(stdout)+", STDERR: "+ str(stderr)
        if os.getenv("TEST_DEBUG"):
            print "\n".join([str(x) for x in allFound if x])
            import traceback
            traceback.print_stack()
            print "FAILED SEARCHING: "+globalvar.last_search+" ,\n STDOUT: "+ str(stdout)+",\n STDERR: "+ str(stderr)+"\n ALL XSHELL SESSION:"+ allXshellSession
        return "FAILED SEARCHING: "+globalvar.last_search+" , STDOUT: "+ str(stdout)+", STDERR: "+ str(stderr)+" ALL XSHELL SESSION:"+ allXshellSession
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
    config = json.load(open('config_local.json'))
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

# Force default theme
os.environ['MYSQLSH_PROMPT_THEME'] = Exec_files_location+"/../prompt_classic.json"
# disable credential store
os.environ['MYSQLSH_CREDENTIAL_STORE_HELPER'] = '<disabled>'

class globalvar:
    last_found=""
    last_search=""

class prompt:
    def __init__(self):
        self.prompts = []
        self.re_prompts = []

    def add(self, x):
        self.prompts.append(x)
    def remove_last(self):
        self.prompts.pop(-1)

xPrompts= prompt()
xPrompts.add("mysql-js>")
xPrompts.add("mysql-sql>")
xPrompts.add("mysql-py>")
xPrompts.add("       ...")
xPrompts.add("      ...")


###########################################################################################

class XShell_TestCases(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        if os.environ.has_key('skip_qa_data'):
          print "Warning: skip_qa_data environment variable found, skipping data setup..."
          return;

        # install xplugin
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--dba', 'enableXProtocol']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
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
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdin, stdout = p.communicate()
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [
            ('\\connect --mx {0}:{1}@{2}:{3}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), "mysql-js>"),
            ("\\sql\n", "mysql-sql>"),
            ("use world_x;\n", "mysql-sql>"),
            ("show tables ;\n", "4 rows in set"),
            ]
        results = exec_xshell_commands(init_command, x_cmds)
        if results != "PASS":
            print results
            raise ValueError("FAILED initializing schema world_x")

        # create sakila and sakila-data
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--mysql',
                        '--file=' + Exec_files_location + 'sakila-schema.sql']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdin, stdout = p.communicate()
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--sqlc', '--mysql',
                        '--file=' + Exec_files_location + 'sakila-data-5712.sql']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdin, stdout = p.communicate()
        # if stdout.find(bytearray("ERROR","ascii"),0,len(stdout))> -1:
        #  self.assertEqual(stdin, 'PASS')
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [
            ('\\connect --mx {0}:{1}@{2}:{3}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), "mysql-js>"),
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
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdin, stdout = p.communicate()
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [
            ('\\connect --mx {0}:{1}@{2}:{3}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), "mysql-js>"),
            ("\\sql\n", "mysql-sql>"),
            ("use sakila_x;\n", "mysql-sql>"),
            ("select count(*) from movies;\n", "1 row in set"),
            ("select count(*) from users;\n", "1 row in set"),
            ]
        results = exec_xshell_commands(init_command, x_cmds)
        if results != "PASS":
            raise ValueError("FAILED initializing schema sakila_x")

    def test_2_0_01_10(self):
        '''[2.0.01]:10 Connect local Server w/Command Line Args'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--sqlc']
        x_cmds = [(";\n", 'mysql-sql>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')


    def test_2_0_07_02(self):
        '''[2.0.07]:2 Connect local Server on PY mode: NODE SESSION'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--py']
        x_cmds = [#("import mysqlx\n", "mysql-py>"),
                  ("session=mysqlx.get_session(\'{0}:{1}@{2}:{3}\')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                               LOCALHOST.host, LOCALHOST.xprotocol_port), "mysql-py>"),
                  ("schemaList = session.get_schemas()\n", "mysql-py>"),
                  ("schemaList\n", "sakila"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_07_03(self):
        '''[2.0.07]:3 Connect local Server on PY mode: NODE SESSION'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--py']
        x_cmds = [#("import mysqlx\n", "mysql-py>"),
                  ("session=mysqlx.get_session({\'host\': \'" + LOCALHOST.host + "\', \'dbUser\': \'"
                   + LOCALHOST.user + "\', \'dbPassword\': \'" + LOCALHOST.password + "\', \'port\': " + LOCALHOST.xprotocol_port + "})\n", "mysql-py>"),
                  ("schemaList = session.get_schemas()\n", "mysql-py>"),
                  ("schemaList\n", "sakila"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # Connection test should be let out if this test and just ensure that the --[sql/js/py]
    # flags open the shell in the correct mode, even there are tests that imply this works
    # there are no specific tests for this.
    #
    # This comment applies to all the test_2_0_09* tests using LOCALHOST, but all the combinations are completely
    # unnecessary, just one test for js, py and sql should be enough
    #
    # Tests using REMOTEHOST are innecessary, duplicated
    def test_2_0_09_01(self):
        '''[2.0.09]:1 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --sql'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--sqlc']
        x_cmds = [(";\n", 'mysql-sql>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_09_02(self):
        '''[2.0.09]:2 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --js'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--js']
        x_cmds = [(";\n", 'mysql-js>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_09_03(self):
        '''[2.0.09]:3 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --py'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--py']
        x_cmds = [("\n", 'mysql-py>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_09_04(self):
        '''4 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --sql'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                        'mysql://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                         LOCALHOST.port), '--mysql', '--sqlc']
        x_cmds = [(";\n", 'mysql-sql>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_09_05(self):
        '''[2.0.09]:5 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --js'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                        'mysql://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                         LOCALHOST.port), '--mysql', '--js']
        x_cmds = [(";\n", 'mysql-js>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_09_06(self):
        '''[2.0.09]:6 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC SESSION --uri --py'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                        'mysql://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                         LOCALHOST.port), '--mysql', '--py']
        x_cmds = [("\n", 'mysql-py>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_09_07(self):
        '''[2.0.09]:7 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --sql'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--sql']
        x_cmds = [(";\n", 'mysql-sql>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_09_08(self):
        '''[2.0.09]:8 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --js'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
        x_cmds = [(";\n", 'mysql-js>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_09_09(self):
        '''[2.0.09]:9 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --py'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']
        x_cmds = [("\n", 'mysql-py>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_09_10(self):
        '''[2.0.09]:10 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --sql'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                        'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                          LOCALHOST.xprotocol_port), '--mysqlx', '--sql']
        x_cmds = [(";\n", 'mysql-sql>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_09_11(self):
        '''[2.0.09]:11 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --js'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                        'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                          LOCALHOST.xprotocol_port), '--mysqlx', '--js']
        x_cmds = [(";\n", 'mysql-js>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_2_0_09_12(self):
        '''[2.0.09]:12 Connect local Server w/Init Exec mode: --[sql/js/py]: CLASSIC NODE --uri --py'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                        'mysqlx://{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                          LOCALHOST.xprotocol_port), '--mysqlx', '--py']
        x_cmds = [("\n", 'mysql-py>')
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # This is not a connection test but an options validation tests, should be coded as that
    def test_2_0_11_02(self):
        '''[2.0.11]:2 Connect local Server w/Command Line Args FAILOVER: unknown option'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-v' + LOCALHOST.user, '--password=', '-h' + LOCALHOST.host,
                        '-P' + LOCALHOST.xprotocol_port, '--x', '--sql']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
        stdout, stderr = p.communicate()
        found = stdout.find(bytearray("unknown option -v", "ascii"), 0, len(stdout))
        if found == -1:
            results = "FAIL \n\r" + stdout.decode("ascii")
        else:
            results = "PASS"
        self.assertEqual(results, 'PASS')

        #if results.find("unknown option -v", 0, len(results)) > 0:
        #    results = "PASS"
        #self.assertEqual(results, 'PASS')

    def test_3_1_04_01(self):
        '''[3.1.004]:1 Check that command [ \quit, \q, \exit ] works: \quit'''
        results = ''
        p = subprocess.Popen([MYSQL_SHELL, '--interactive=full'], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             stdin=subprocess.PIPE)
        p.stdin.write(bytearray('\\quit\n', 'ascii'))
        p.stdin.flush()
        stdout, stderr = p.communicate()
        found = stderr.find(bytearray("Error", "ascii"), 0, len(stdout))
        if found == -1:
            results = "PASS"
        else:
            results = "FAIL \n\r" + stdout.decode("ascii")
        self.assertEqual(results, 'PASS')

    def test_3_1_04_02(self):
        '''[3.1.004]:2 Check that command [ \quit, \q, \exit ] works: \q '''
        results = ''
        p = subprocess.Popen([MYSQL_SHELL, '--interactive=full'], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             stdin=subprocess.PIPE)
        p.stdin.write(bytearray('\\q\n', 'ascii'))
        p.stdin.flush()
        stdout, stderr = p.communicate()
        found = stderr.find(bytearray("Error", "ascii"), 0, len(stdout))
        if found == -1:
            results = "PASS"
        else:
            results = "FAIL \n\r" + stdout.decode("ascii")
        self.assertEqual(results, 'PASS')

    def test_3_1_04_03(self):
        '''[3.1.004]:3 Check that command [ \quit, \q, \exit ] works: \exit'''
        results = ''
        p = subprocess.Popen([MYSQL_SHELL, '--interactive=full'], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             stdin=subprocess.PIPE)
        p.stdin.write(bytearray('\\exit\n', 'ascii'))
        p.stdin.flush()
        stdout, stderr = p.communicate()
        found = stderr.find(bytearray("Error", "ascii"), 0, len(stdout))
        if found == -1:
            results = "PASS"
        else:
            results = "FAIL \n\r" + stdout.decode("ascii")
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

    # Suggested location: interactive_shell_t.cc
    def test_3_1_10_1(self):
        '''[3.1.010]:1 Check that EXECUTE SCRIPT FILE command [ \source, \. ] works: node session \source select_actor_10.sql'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [
            ("\\connect --mx {0}:{1}@{2}:{3}\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), "mysql-js>"),
            ("\\sql\n", "mysql-sql>"),
            ("\\source {0}select_actor_10.sql\n".format(Exec_files_location), "rows in set"),
            ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # Suggested location: interactive_shell_t.cc
    def test_3_1_10_2(self):
        '''[3.1.010]:2 Check that EXECUTE SCRIPT FILE command [ \source, \. ] works: node session \. select_actor_10.sql'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [
            ('\\connect --mx {0}:{1}@{2}:{3}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), "mysql-js>"),
            ("\\sql\n", "mysql-sql>"),
            ("\\. {0}select_actor_10.sql\n".format(Exec_files_location), "rows in set"),
            ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_0_01_01(self):
        '''[4.0.001]:1 Batch Exec - Loading code from file:  --file= createtable.js'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx',
                        '--file=' + Exec_files_location + 'CreateTable.js']

        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
        stdout, stderr = p.communicate()
        if stdout.find(bytearray("FAIL", "ascii"), 0, len(stdout)) > -1:
            self.assertEqual(stdout, 'PASS')
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [
            ('\\connect --mx {0}:{1}@{2}:{3}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), "mysql-js>"),
            ("\\sql\n", "mysql-sql>"),
            ("use sakila;\n", "mysql-sql>"),
            ("show tables like \'testdb\';\n", "1 row in set"),
            ("drop table if exists testdb;\n", "Query OK"),
            ("show tables like 'testdb';\n", "Empty set"),
            ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')


    def test_4_0_02_01(self):
        '''[4.0.002]:1 Batch Exec - Loading code from file:  < createtable.js'''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             stdin=open(Exec_files_location + 'CreateTable.js'))
        stdin, stdout = p.communicate()
        if stdout.find(bytearray("ERROR", "ascii"), 0, len(stdin)) > -1:
            self.assertEqual(stdin, 'PASS')
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [
            ('\\connect --mx {0}:{1}@{2}:{3}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), "mysql-js>"),
            ("\\sql\n", "mysql-sql>"),
            ("use sakila;\n", "mysql-sql>"),
            ("show tables like 'testdb';\n", "1 row in set"),
            ("drop table if exists testdb;\n", "Query OK"),
            ("show tables like 'testdb';\n", "Empty set"),
            ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_2_16_2(self):
        '''[4.2.016]:2 PY Read executing the stored procedure: CLASSIC SESSION'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '--py']
        x_cmds = [("from mysqlsh import mysql\n", "mysql-py>"),
                  ("session=mysql.get_classic_session('{0}:{1}@{2}:{3}')\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                   LOCALHOST.host, LOCALHOST.port),
                   "mysql-py>"),
                  ("session.run_sql('use sakila;')\n", "Query OK"),
                  ("session.run_sql('drop procedure if exists get_actors;')\n", "Query OK"),
                  ("session.run_sql('CREATE PROCEDURE get_actors() BEGIN  "
                   "select first_name from actor where actor_id < 5 ; END;')\n", "Query OK"),
                  ("session.run_sql('call get_actors();')\n", "rows in set")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_3_10_1(self):
        '''[4.3.010]:1 JS Update table using multiline mode: CLASSIC SESSION'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [("var mysql=require('mysql');\n", "mysql-js>"),
                  (
                  "var session=mysql.getClassicSession('{0}:{1}@{2}:{3}');\n".format(LOCALHOST.user, LOCALHOST.password,
                                                                                     LOCALHOST.host, LOCALHOST.port),
                  "mysql-js>"),
                  ("session.runSql('use sakila;');\n", "Query OK"),
                  ("session.runSql('drop table if exists sakila.friends;');\n", "Query OK"),
                  ("session.runSql('create table sakila.friends (name varchar(50), last_name varchar(50), "
                   "age integer, gender varchar(20));');\n", "Query OK"),
                  #("var table = session.sakila.friends;\n", "mysql-js>"),
                  ("session.\n", "    ..."),
                  (
                  "runSql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES ('jack','black', 17, 'male');\");\n",
                  "    ..."),
                  ("\n", "Query OK, 1 row affected"),
                  ("session.\n", "     ..."),
                  (
                  "runSql(\"INSERT INTO sakila.friends (name,last_name,age,gender) VALUES ('ruben','morquecho', 40, 'male');\");\n",
                  "    ..."),
                  ("\n", "Query OK, 1 row affected"),
                  ("session.runSql('select name from sakila.friends;');\n", "ruben"),
                  ("session.runSql('select name from sakila.friends;');\n", "jack"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_7_01_1(self):
        '''[4.7.001]   Retrieve with Table Output Format with classic session: CLASSIC SESSION'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--schema=sakila', '--sqlc',
                        '--table']

        x_cmds = [("select actor_id from actor limit 5;\n", "| actor_id |"),
                  ]

        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_7_03_1(self):
        '''[4.7.003] Retrieve with JSON raw Format with classic session: CLASSIC SESSION'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--schema=sakila', '--sqlc',
                        '--json=raw']

        x_cmds = [("select actor_id from actor limit 5;\n", "\"rows\":[{\"actor_id\":58}"),
                  ]
        #'{"info":"mysql-sql> "}
        xPrompts.add("{\"info\":\"mysql-sql> \"}")
        results = exec_xshell_commands(init_command, x_cmds)
        xPrompts.remove_last()

        self.assertEqual(results, 'PASS')

    def test_4_7_03_2(self):
        '''[4.7.003] Retrieve with JSON raw Format with classic session: CLASSIC SESSION'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--schema=sakila', '--sqlc',
                        '--json=pretty']

        x_cmds = [("select actor_id from actor limit 5;\n", "\"rows\": [" + os.linesep + ""),
                  ]
        xPrompts.add("    \"info\": \"mysql-sql>")
        results = exec_xshell_commands(init_command, x_cmds)
        xPrompts.remove_last()
        self.assertEqual(results, 'PASS')

    # Letting the BIG DATA tests for now, later we decide what is kept and what is not
    # Javascript based with Big Data
    # Be aware to update the BigCreate_Classic, BigCreate_Node and BigCreate_Coll_Node files,
    # in order to create the required number of rows, based on the "jsRowsNum_Test" value
    # JS Create Non collections
    def test_4_10_00_01(self):
        '''JS Exec Batch with huge data in Classic mode, Create and Insert:  --file= BigCreate_Classic.js'''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql',
                        '--file=' + Exec_files_location + 'BigCreate_Classic.js']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdin, stdout = p.communicate()
        if stdin.find(bytearray("ERROR", "ascii"), 0, len(stdin)) > -1:
            results = "FAIL"
        else:
            results = "PASS"
        self.assertEqual(results, 'PASS')

    def test_4_10_00_02(self):
        '''JS Exec Batch with huge data in Node mode, Create and Insert:  --file= BigCreate_Node.js'''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx',
                        '--file=' + Exec_files_location + 'BigCreate_Node.js']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdin, stdout = p.communicate()
        if stdin.find(bytearray("ERROR", "ascii"), 0, len(stdin)) > -1:
            results = "FAIL"
        else:
            results = "PASS"
        self.assertEqual(results, 'PASS')

    # JS Create Collections
    def test_4_10_00_03(self):
        '''JS Exec Batch with huge data in Node mode, Create and Add:  --file= BigCreate_Coll_Node.js'''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx',
                        '--file=' + Exec_files_location + 'BigCreate_Coll_Node.js']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdin, stdout = p.communicate()
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
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--js']
        x_cmds = [("session.runSql(\"use world_x;\");\n", "Query OK"),
                  (
                  "session.runSql(\"SELECT * FROM world_x.big_data_classic_js where geometryCol is not null limit " + str(
                      jsRowsNum_Test) + ";\")\n", str(jsRowsNum_Test) + " rows in set")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_10_00_05(self):
        '''JS Exec a select with huge limit in Node mode, Read'''
        jsRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
        x_cmds = [("var Table = session.getSchema(\'world_x\').getTable(\'big_data_node_js\')\n", ""),
                  ("Table.select().where(\"stringCol like :likeFilter\").limit(" + str(
                      jsRowsNum_Test) + ").bind(\"likeFilter\",\'Node\%\').execute()\n",
                   str(jsRowsNum_Test) + " rows in set")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # JS Read Collections
    def test_4_10_00_06(self):
        '''JS Exec a select with huge limit in Node mode for collection, Read'''
        jsRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
        x_cmds = [("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
                  (
                  "myColl.find(\"Name = \'Mexico\'\").fields([\'_id\', \'Name\','geography.Region\',\'geography.Continent\']).limit(" + str(
                      jsRowsNum_Test) + ")\n", str(jsRowsNum_Test) + " documents in set")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # JS Update Non collections
    def test_4_10_00_07(self):
        '''JS Exec an update clause to a huge number of rows in Classic mode, Update'''
        jsRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--js']
        x_cmds = [("session.runSql(\"use world_x;\");\n", "Query OK"),
                  (
                  "session.runSql(\"update big_data_classic_js set datetimeCol = now() where stringCol like \'Classic\%\' and blobCol is not null limit " + str(
                      jsRowsNum_Test) + " ;\");\n", "Query OK, " + str(jsRowsNum_Test) + " rows affected")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_10_00_08(self):
        '''JS Exec an update clause to huge number of rows in Node mode, Update'''
        jsRowsNum_Test = 1000
        results = ''
        CurrentTime = datetime.datetime.now()
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
        x_cmds = [("var Table = session.getSchema('world_x').getTable('big_data_node_js')\n", ""),
                  ("Table.update().set(\'datetimeCol\',\'" + str(
                      CurrentTime) + "\').where(\"stringCol like :likeFilter\").limit(" + str(
                      jsRowsNum_Test) + ").bind(\"likeFilter\",\'Node\%\').execute()\n",
                   "Query OK, " + str(jsRowsNum_Test) + " items affected")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # JS Update Collections
    def test_4_10_00_09(self):
        '''JS Exec an update clause to huge number of document rows in Node mode, using Set '''
        jsRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
        x_cmds = [("session.sql(\"use world_x;\");\n", "Query OK"),
                  ("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
                  ("myColl.modify(\"Name = :country\").set(\'Soccer_World_Championships\',\'0\').limit(" + str(
                      jsRowsNum_Test) + ").bind(\'country\',\'Mexico\').execute();\n",
                   "Query OK, " + str(jsRowsNum_Test) + " items affected")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_10_00_10(self):
        '''JS Exec an update clause to huge number of document rows in Node mode, using Unset '''
        jsRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
        x_cmds = [("session.sql(\"use world_x;\");\n", "Query OK"),
                  ("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
                  ("myColl.modify(\"Name = :country\").unset(\'Soccer_World_Championships\').limit(" + str(
                      jsRowsNum_Test) + ").bind(\'country\',\'Mexico\').execute();\n",
                   "Query OK, " + str(jsRowsNum_Test) + " items affected")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_10_00_11(self):
        '''JS Exec an update clause to huge number of document rows in Node mode, using Merge '''
        jsRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']

        x_cmds = [("session.sql(\"use world_x;\");\n", "Query OK"),
                  ("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
                  ("myColl.modify(\'true\').merge({Language: \'Spanish\', Extra_Info:[\'Extra info TBD\']}).limit(" + str(
                      jsRowsNum_Test) + ").execute();\n", "Query OK, " + str(jsRowsNum_Test) + " items affected"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_10_00_12(self):
        '''JS Exec an update clause to huge number of document rows in Node mode, using Array '''
        jsRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']

        x_cmds = [("session.sql(\"use world_x;\");\n", "Query OK"),
                  ("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
                  (
                  "myColl.modify(\'true\').arrayAppend(\'Language\', \'Spanish_mexico\').arrayAppend(\'Extra_Info\', \'Extra info TBD 2\').limit(" + str(
                      jsRowsNum_Test) + ").execute();\n", "Query OK, " + str(jsRowsNum_Test) + " items affected"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # JS Delete from Non collections
    def test_4_10_00_13(self):
        '''JS Exec a delete clause to huge number of rows in Classic mode, Delete '''
        jsRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--js']
        x_cmds = [("session.runSql(\"use world_x;\");\n", "Query OK"),
                  ("session.runSql(\"DELETE FROM big_data_classic_js where stringCol like \'Classic\%\' limit " + str(
                      jsRowsNum_Test) + ";\");\n", "Query OK, " + str(jsRowsNum_Test) + " rows affected"),
                  ("session.runSql(\"DROP TABLE big_data_classic_js;\");\n", "Query OK, 0 rows affected"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_10_00_14(self):
        '''JS Exec a delete clause to huge number of rows in Node mode, Delete'''
        jsRowsNum_Test = 1000
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
        x_cmds = [("var Table = session.getSchema(\'world_x\').getTable(\'big_data_node_js\')\n", ""),
                  ("Table.delete().where(\'stringCol like :likeFilter\').limit(" + str(
                      jsRowsNum_Test) + ").bind(\'likeFilter\', \'Node\%\').execute();\n",
                   "Query OK, " + str(jsRowsNum_Test) + " items affected"),
                  ("session.getSchema(\'world_x\').dropTable(\'big_data_node_js\');\n", ""),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # JS Delete from Collections
    def test_4_10_00_15(self):
        '''JS Exec a delete clause to huge number of document rows in Node mode, Delete'''
        jsRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
        x_cmds = [("var myColl = session.getSchema(\'world_x\').getCollection(\"big_coll_node_js\");\n", ""),
                  ("myColl.remove(\'Name=:country\').limit(" + str(
                      jsRowsNum_Test) + ").bind(\'country\',\'Mexico\').execute();\n",
                   "Query OK, " + str(jsRowsNum_Test) + " items affected"),
                  ("session.getSchema(\'world_x\').dropCollection(\'big_coll_node_js\');\n", ""),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # Not sure if there's any value on repeating the big data tests both in JS/PYTHON
    # Python based with Big Data
    # Be aware to update the BigCreate_Classic, BigCreate_Node and BigCreate_Coll_Node files,
    # in order to create the required number of rows, based on the "pyRowsNum_Test" value
    # Py Create Non collections
    # @unittest.skip("To avoid execution 4_10_01_01, because of issue https://jira.oraclecorp.com/jira/browse/MYS-398")
    def test_4_10_01_01(self):
        '''PY Exec Batch with huge data in Classic mode, Create and Insert:  --file= BigCreate_Classic.py'''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--py',
                        '--file=' + Exec_files_location + 'BigCreate_Classic.py']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdin, stdout = p.communicate()
        if stdout.find(bytearray("Error", "ascii"), 0, len(stdin)) > -1:
            self.assertEqual(stdin, 'PASS', str(stdout))

    def test_4_10_01_02(self):
        '''PY Exec Batch with huge data in Node mode, Create and Insert:  --file= BigCreate_Node.py'''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py',
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py',
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--py']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--py']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']

        x_cmds = [("session.sql(\"use world_x;\")\n", "Query OK"),
                  ("myColl = session.get_schema(\'world_x\').get_collection(\"big_coll_node_py\")\n", ""),
                  (
                  "myColl.modify(\'true\').merge({\'Language\': \"Spanish\", \'Extra_Info\':[\"Extra info TBD\"]}).limit(" + str(
                      pyRowsNum_Test) + ").execute()\n", "Query OK, " + str(pyRowsNum_Test) + " items affected"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_4_10_01_12(self):
        '''PY Exec an update clause to huge number of document rows in Node mode, using Array '''
        pyRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']

        x_cmds = [("session.sql(\"use world_x;\")\n", "Query OK"),
                  ("myColl = session.get_schema(\'world_x\').get_collection(\"big_coll_node_py\")\n", ""),
                  (
                      "myColl.modify(\'true\').array_append(\"Language\", \"Spanish_mexico\").array_append(\"Extra_Info\", \"Extra info TBD 2\").limit(" + str(
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--py']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']
        x_cmds = [("Table = session.get_schema(\'world_x\').get_table(\'big_data_node_py\')\n", ""),
                  ("Table.delete().where(\"stringCol like :likeFilter\").limit(" + str(
                      pyRowsNum_Test) + ").bind(\"likeFilter\", \"Node%\").execute()\n",
                   "Query OK, " + str(pyRowsNum_Test) + " items affected"),
                  ("session.get_schema(\'world_x\').drop_table(\'big_data_node_py\')\n", ""),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

        # Py Delete from Collections

    def test_4_10_01_15(self):
        '''PY Exec a delete clause to huge number of document rows in Node mode, Delete'''
        pyRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']
        x_cmds = [("myColl = session.get_schema(\"world_x\").get_collection(\"big_coll_node_py\")\n", ""),
                  ("myColl.remove(\"Name=:country\").limit(" + str(
                      pyRowsNum_Test) + ").bind(\"country\",\"Mexico\").execute()\n",
                   "Query OK, " + str(pyRowsNum_Test) + " items affected"),
                  ("session.get_schema(\"world_x\").drop_collection(\"big_coll_node_py\")\n", ""),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # Again, not sure if there's any value on testing BIG DATA tests with SQL as well
    # SQL based with Big Data
    # Be aware to update the BigCreate_SQL, BigCreate_SQL_Coll files,
    # in order to create the required number of rows, based on the "sqlRowsNum_Test" value

    # SQL Non Collections Create
    def test_4_10_02_01(self):
        '''SQL Exec Batch with huge data in Classic mode, Create and Insert:  --file= BigCreate_SQL.py'''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--sqlc',
                        '--file=' + Exec_files_location + 'BigCreate_SQL.sql']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdin, stdout = p.communicate()
        if stdout.find(bytearray("Error", "ascii"), 0, len(stdin)) > -1:
            self.assertEqual(stdin, 'PASS', str(stdout))

    # SQL Create Collections
    def test_4_10_02_02(self):
        '''SQL Exec Batch with huge data in Classic mode for collection, Create and Insert:  --file= BigCreate_Coll_SQL.sql'''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--sqlc',
                        '--file=' + Exec_files_location + 'BigCreate_Coll_SQL.sql']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdin, stdout = p.communicate()
        if stdout.find(bytearray("Error", "ascii"), 0, len(stdin)) > -1:
            self.assertEqual(stdin, 'PASS', str(stdout))

    # SQL Read Non Collections
    def test_4_10_02_03(self):
        '''SQL Exec a select with huge limit in Classic mode, Read'''
        sqlRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--sqlc']
        x_cmds = [("SELECT * FROM world_x.bigdata_sql where stringCol like \'SQL%\' limit " + str(
            sqlRowsNum_Test) + ";\n", str(sqlRowsNum_Test) + " rows in set")]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # SQL Read Collections
    def test_4_10_02_04(self):
        '''SQL Exec a select with huge limit in Classic mode for collection, Read'''
        sqlRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--sqlc']
        x_cmds = [("SELECT * FROM world_x.bigdata_coll_sql where _id < " + str(sqlRowsNum_Test + 1) + ";\n",
                   str(sqlRowsNum_Test) + " rows in set")]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # SQL Update Collections
    def test_4_10_02_06(self):
        '''SQL Exec an update to the complete table in Classic mode for collection, Read'''
        sqlRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--sqlc']
        x_cmds = [(
                  "update world_x.bigdata_coll_sql set doc = \'{\"GNP\" : 414972,\"IndepYear\" : 1810,\"Name\" : \"Mexico\",\"_id\" : \"9001\"}\';\n",
                  "Rows matched: " + str(sqlRowsNum_Test) + "  Changed: " + str(sqlRowsNum_Test) + "  Warnings: 0")]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # SQL Delete Non Collections
    def test_4_10_02_07(self):
        '''SQL Exec a delete to the complete table in Classic mode for non collection, Read'''
        sqlRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--sqlc']
        x_cmds = [
            ("DELETE FROM world_x.bigdata_sql where blobCol is not null;\n", str(sqlRowsNum_Test) + " rows affected"),
            ("DROP PROCEDURE world_x.InsertInfoSQL;\n", "0 rows affected"),
            ("DROP TABLE world_x.bigdata_sql;\n", "0 rows affected")
            ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # SQL Delete Collections
    def test_4_10_02_08(self):
        '''SQL Exec a delete to the complete table in Classic mode for collection, Read'''
        sqlRowsNum_Test = 1000
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--sqlc']
        x_cmds = [("DELETE FROM world_x.bigdata_coll_sql where _id > 0;\n", str(sqlRowsNum_Test) + " rows affected"),
                  ("DROP PROCEDURE world_x.InsertInfoSQLColl;\n", "0 rows affected"),
                  ("DROP TABLE world_x.bigdata_coll_sql;\n", "0 rows affected")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_225_00(self):
        """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-225 with classic session"""
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--py']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']
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
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             stdin=open(Exec_files_location + 'Hello.js'))
        stdout, stderr = p.communicate()
        if stdout.find(bytearray("I am executed in batch mode, Hello", "ascii"), 0, len(stdout)) > -1:
            results = 'PASS'
        else:
            results = 'FAIL'
        self.assertEqual(results, 'PASS')

    def test_MYS_230_01(self):
        '''[4.0.002]:1 Batch Exec - Loading code from file:  < createtable.js'''
        init_command = [MYSQL_SHELL, '--interactive=full', '--js']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             stdin=open(Exec_files_location + 'Hello.js'))
        stdout, stderr = p.communicate()
        if stdout.find(bytearray("I am executed in batch mode, Hello", "ascii"), 0, len(stdout)) > -1:
            results = 'PASS'
        else:
            results = 'FAIL'
        self.assertEqual(results, 'PASS')

    def test_MYS_286_00(self):
        """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-286 with classic session"""
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--sqlc']
        x_cmds = [(";\n", 'mysql-sql>'),
                  ("create table world_x.MYS286 (date datetime);\n", "Query OK"),
                  ("insert into world_x.MYS286 values (now());\n", "Query OK, 1 row affected"),
                  ("select * from world_x.MYS286;\n", "1 row in set"),
                  ("DROP TABLE world_x.MYS286;\n", "Query OK")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_286_01(self):
        """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-286 with classic session
        Affected by bug: https://clustra.no.oracle.com/orabugs/bug.php?id=26552838 """
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
        x_cmds = [(";\n", 'mysql-js>'),
                  ("session.sql(\'create table world_x.mys286 (date datetime);\')\n", "Query OK"),
                  ("Table = session.getSchema(\'world_x\').getTable(\'mys286\')\n", "<Table:mys286>"),
                  ("Table.insert().values('2016-03-14 12:36:37')\n", "Query OK, 1 item affected"),
                  ("Table.select()\n", "2016-03-14 12:36:37"),
                  ("Table.insert().values('2016-03-14 12:36:37')\n", "Query OK")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_290_00(self):
        '''Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-290 with --file'''
        results = ''
        init_command = [MYSQL_SHELL, '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx',
                        '--file=' + Exec_files_location + 'JavaScript_Error.js']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdoutdata, stderrordata = p.communicate()
        if stderrordata.find(bytearray("Invalid object member getdatabase", "ascii")) >= 0:
            results = 'PASS'
        else:
            results = 'FAIL'
        self.assertEqual(results, 'PASS', str(stderrordata))

    def test_MYS_290_01(self):
        '''Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-290 with --interactive=full --file '''
        results = ''
        init_command = [MYSQL_SHELL, '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--interactive=full',
                        '--file=' + Exec_files_location + 'JavaScript_Error.js']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdoutdata, stderrordata = p.communicate()
        if stderrordata.find(bytearray("Invalid object member getdatabase", "ascii")) >= 0:
            results = 'PASS'
        else:
            results = 'FAIL'
        self.assertEqual(results, 'PASS', str(stderrordata))

    # FAILING........
    def test_MYS_296(self):
        '''[4.1.002] SQL Create a table using STDIN batch process: NODE SESSION'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--sql', '--schema=sakila',
                        '--file=' + Exec_files_location + 'CreateTable_SQL.sql']

        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdin, stdout = p.communicate()
        if stdout.find(bytearray("ERROR", "ascii"), 0, len(stdin)) > -1:
            self.assertEqual(stdin, 'PASS')
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [
            ('\\connect --mx {0}:{1}@{2}:{3}\n'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port), "mysql-js>"),
            ("\\sql\n", "mysql-sql>"),
            ("use sakila;\n", "mysql-sql>"),
            ("show tables like \'example_SQLTABLE\';\n", "1 row in set"),
            ("drop table if exists example_SQLTABLE;\n", "Query OK")
            ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_296_00(self):
        """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-296 --file preference over < . Hello.js executed, HelloAgain.js not executed """
        results = ''
        expectedValue = 'I am executed in batch mode, Hello'
        target_vm = r'"%s"' % MYSQL_SHELL
        init_command_str = target_vm + ' --file=' + Exec_files_location + 'Hello.js' + ' < ' + Exec_files_location + 'HelloAgain.js'
        # init_command_str = MYSQL_SHELL + ' --file=' + Exec_files_location + 'Hello.js' + ' < ' + Exec_files_location + 'HelloAgain.js'
        p = subprocess.Popen(init_command_str, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE,
                             shell=True)
        stdin, stdout = p.communicate()
        if str(stdin) == expectedValue:
            results = 'PASS'
        else:
            results = 'FAIL'
        self.assertEqual(results, 'PASS', str(stdout))

    def test_MYS_296_01(self):
        """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-296 --file preference over < . Hello.js executed, HelloAgain.js not executed """
        results = ''
        expectedValue = 'I am executed in batch mode, Hello'
        target_vm = r'"%s"' % MYSQL_SHELL
        init_command_str = target_vm + ' < ' + Exec_files_location + 'HelloAgain.js' + ' --file=' + Exec_files_location + 'Hello.js'
        p = subprocess.Popen(init_command_str, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE,
                             shell=True)
        stdin, stdout = p.communicate()
        if str(stdin) == expectedValue:
            results = 'PASS'
        else:
            results = 'FAIL'
        self.assertEqual(results, 'PASS', str(stdout))

    def test_MYS_309_01(self):
        """ Verify the bug https://jira.oraclecorp.com/jira/browse/MYS-309 with node session and - as part of schema name"""
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']
        x_cmds = [#(";\n", 'mysql-py>'),
                  ("session.create_schema('my-Node')\n", "<Schema:my-Node>"),
                  ("session.drop_schema('my-Node')\n", "mysql-py>")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_334(self):
        '''Shell --json output cannot be easily processed from a script calling the shell'''
        results = 'PASS'
        init_command = [MYSQL_SHELL, '--interactive=full', '--sql', '--database=mysql', '--json', '--mysqlx', '-u' +
                        LOCALHOST.user, '-h' + LOCALHOST.host, '--password=' + LOCALHOST.password]
        expectedResult = ["{",
                          "\"warning\": \"WARNING: Using a password on the command line interface can be insecure.\"",
                          "}",
                          "{",
                          "\"info\": \"Creating an X protocol session to '{0}@{1}/{2}'\"".format(LOCALHOST.user,
                                                                                          LOCALHOST.host,
                                                                                          "mysql"),
                          "}",
                          "{",
                          "\"info\": \"Session successfully established. Default schema `mysql` accessible through db.\"",
                          "}"]
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        p.stdin.flush()
        stdoutdata, stderrdata = p.communicate()
        stdoutsplitted = stdoutdata.splitlines()
        for expResult in expectedResult:
            count = 1
            for line in stdoutsplitted:
                found = line.find(expResult, 0, len(line))
                if found == -1 and count > len(stdoutsplitted):
                    results = "FAIL"
                    break
                elif found != -1:
                    results = "PASS"
                    stdoutsplitted.remove(line)
                    break
            if results == "FAIL":
                break
        self.assertEqual(results, 'PASS')

    def test_MYS_338_01(self):
        '''[2.0.14]:4 Connect remote Server inside mysqlshell FAILOVER: \connect --mc  wrong password'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [(";\n", "mysql-js>"),
                  ("\\connect --mc {0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, "wrongpass", REMOTEHOST.host,
                                                           REMOTEHOST.port), "mysql-js>"),
                  ("println(session)\n", "null")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_338_02(self):
        '''[2.0.14]:4 Connect remote Server inside mysqlshell FAILOVER: \connect --mc  wrong password'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [(";\n", "mysql-js>"),
                  ("\\connect --mc {0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, "wrongpass", REMOTEHOST.host,
                                                           REMOTEHOST.port), "mysql-js>"),
                  ("println(db)\n", "null")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_338_03(self):
        '''[2.0.14]:4 Connect remote Server inside mysqlshell FAILOVER: \connect --mc  wrong password'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full']
        x_cmds = [(";\n", "mysql-js>"),
                  ("\\connect --mc {0}:{1}@{2}:{3}\n".format(REMOTEHOST.user, "wrongpass", REMOTEHOST.host,
                                                           REMOTEHOST.port), "mysql-js>"),
                  ("db.name\n", "Cannot read property 'name' of null"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_348(self):
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--sql']
        x_cmds = [("use sakila;\n", "mysql-sql>"),
                  ("drop table if exists funwithdates;\n", "Query OK"),
                  ("CREATE TABLE funwithdates ( col_a date DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1;\n",
                   "mysql-sql>"),
                  ("INSERT INTO funwithdates (col_a) VALUES ('1000-02-01 0:00:00');\n", "mysql-sql>"),
                  ("select * from funwithdates;\n", "mysql-sql>"),
                  ("\\js\n", "mysql-js>"),
                  ("row = session.getSchema('sakila').funwithdates.select().execute().fetchOne();\n", "mysql-js>"),
                  ("type(row.col_a);\n", "Date"),
                  ("row.col_a.getFullYear();\n", "1000"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # This is NOT testing what it is suppossed to test, but the test should exist
    # which implies result consumption (automatic result printing)
    # and availability after that
    def test_MYS_349(self):
        '''Valid DevAPI JavaScript code breaks in Shell when using copy & paste'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port]
        x_cmds = [("var db = session.getSchema(\"sakila\");\n", "mysql-js>"),
                  ("var res = db.actor.select().where('actor_id=1').execute();\n", "mysql-js>"),
                  ("res.fetchOne();\n", "1,")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_378(self):
        '''show the default user if its not provided as argument : Creating an X protocol session to XXXXXX@localhost:#####'''
        results = ''
        user = os.path.split(os.path.expanduser('~'))[-1]
        init_command = [MYSQL_SHELL, '--interactive=full', '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                        '--schema=sakila', '--sql',
                        '--passwords-from-stdin']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        p.stdin.write(bytearray(LOCALHOST.password + "\n", 'ascii'))
        p.stdin.flush()
        stdoutdata, stderrordata = p.communicate()
        if stdoutdata.find(bytearray("Creating a session to '" + user + "@", "ascii"), 0, len(stdoutdata)) >= 0:
            results = "PASS"
        else:
            results = "FAIL"
        self.assertEqual(results, 'PASS')

    def test_MYS_379(self):
        '''show the default user if its not provided as argument : Creating an X protocol session to XXXXXX@localhost:#####'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port,
                        '--schema=sakila', '--sql',
                        '--dbuser=' + LOCALHOST.user, '--passwords-from-stdin']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        p.stdin.write(bytearray(LOCALHOST.password + "\n", 'ascii'))
        p.stdin.flush()
        stdoutdata, stderrordata = p.communicate()
        if stdoutdata.find(bytearray("Creating a session to ", "ascii"), 0, len(stdoutdata)) >= 0:
            results = "PASS"
        else:
            results = "FAIL"
        self.assertEqual(results, 'PASS')

    def test_MYS_385(self):
        '''[CHLOG 1.0.2.5_2] Different password command line args'''
        results = ''
        # init_command = [MYSQL_SHELL, '--interactive=full', '--version' ]
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js', '--schema=sakila',
                        '-e print(dir(session))']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        p.stdin.flush()
        stdin, stdout = p.communicate()
        if stdin.find(bytearray("\"close\",", "ascii"), 0, len(stdin)) >= 0:
            results = "PASS"
        else:
            results = "FAIL"
        self.assertEqual(results, 'PASS')

    def test_MYS_401_2(self):
        ''' View support (without DDL)'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']

        x_cmds = [("var db = session.getSchema('sakila');\n", "mysql-js>"),
                  ("var table = db.getTable('actor');\n", "mysql-js>"),
                  ("table.isView();\n", "false"),
                  ("view = db.getTable('actor_info');\n", "<Table:actor_info>"),
                  ("view.isView();\n", "true"),
                  ("view.update().set('last_name','GUINESSE').where('actor_id=1').execute();\n",
                   "The target table actor_info of the UPDATE is not updatable"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_401_3(self):
        ''' View support (without DDL)'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']

        x_cmds = [("var db = session.getSchema('sakila');\n", "mysql-js>"),
                  ("var table = db.getTable('actor');\n", "mysql-js>"),
                  ("table.isView();\n", "false"),
                  ("view = db.getTable('actor_info');\n", "<Table:actor_info>"),
                  ("view.isView();\n", "true"),
                  ("view.delete().where('actor_id = 1').execute();\n",
                   "The target table actor_info of the DELETE is not updatable"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_401_4(self):
        ''' View support (without DDL)'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']

        x_cmds = [("var db = session.getSchema('sakila');\n", "mysql-js>"),
                  ("var table = db.getTable('actor');\n", "mysql-js>"),
                  ("table.isView();\n", "false"),
                  ("view = db.getTable('actor_info');\n", "<Table:actor_info>"),
                  ("view.isView();\n", "true"),
                  ("view.insert().values(203,'JOHN','CENA', 'Action: The Marine').execute();\n",
                   "The target table actor_info of the INSERT is not insertable-into"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_401_5(self):
        ''' View support (without DDL)'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--sql', '--schema=sakila']

        x_cmds = [(
                  "create view actor_list as select actor_id as id, first_name as name, last_name as lname from actor;\n",
                  "Query OK"),
                  ("\\js\n", "mysql-js>"),
                  ("view = db.getTable('actor_list');\n", "<Table:actor_list>"),
                  ("view.isView();\n", "true"),
                  ("view.insert().values(201, 'JOHN', 'SENA').execute();\n", "Query OK, 1 item affected"),
                  ("view.update().set('lname', 'CENA').where('id=201').execute();\n", "Query OK, 1 item affected"),
                  ("view.delete().where('id=201').execute();\n", "Query OK, 1 item affected"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_420(self):
        '''[MYS-420]: Help in command prompt with space blank behaves different (add trim() function)'''
        results = 'PASS'
        init_command = [MYSQL_SHELL, '--interactive=full', '--py']
        x_cmds = [("\\connect\n", "\connect [--mx|--mysqlx|--mc|--mysql] <URI>"),
                  ("\\connect      \n", "\connect [--mx|--mysqlx|--mc|--mysql] <URI>"),
                  ("       \\connect      \n", "\connect [--mx|--mysqlx|--mc|--mysql] <URI>"),
                  ("\\py\n", "mysql-py>"),
                  ("\\connect\n", "\connect [--mx|--mysqlx|--mc|--mysql] <URI>"),
                  ("\\connect      \n", "\connect [--mx|--mysqlx|--mc|--mysql] <URI>"),
                  ("       \\connect      \n", "\connect [--mx|--mysqlx|--mc|--mysql] <URI>")
                  ]
        for command, expectedResult in x_cmds:
            p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
            p.stdin.write(command)
            p.stdin.flush()
            stdoutdata, stderrdata = p.communicate()
            found = stdoutdata.find(expectedResult, 0, len(stdoutdata))
            if found == -1:
                found = stderrdata.find(expectedResult, 0, len(stderrdata))
                if found == -1:
                    results = "FAIL"
                    break
        self.assertEqual(results, 'PASS')

    def test_MYS_438(self):
        ''' TRUE OR FALSE NOT RECOGNIZED AS AVAILABLE BOOL CONSTANTS'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--schema=sakila', '--sql']

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
                  (
                  "table.insert().values(28, 'Garrus Vakarian', 30, 'male', '', 'Mass Effect', 0).values(29, 'Liara TSoni', 109, 'female', '', 'Mass Effect', 1).execute();\n",
                  "Query OK, 2 items affected"),
                  (
                  "table.insert().values(30, 'Garrus Vakarian', 30, 'male', '', 'Mass Effect', false).values(31, 'Liara TSoni', 109, 'female', '', 'Mass Effect', true).execute();\n",
                  "Query OK, 2 items affected"),
                  ("table.select();\n", "4 rows in set"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # Part is covered and part is not, tests should exist for both tables and views
    def test_MYS_442_01(self):
        '''JS In node mode check isView() function to identify whether the underlying object is a View or not, return bool '''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                        '{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                 LOCALHOST.xprotocol_port), '--mysqlx', '--js']
        x_cmds = [("table = session.getSchema('sakila').getTable('actor')\n", "mysql-js>"),
                  ("table.isView()\n", "false"),
                  ("view = session.getSchema('sakila').getTable('actor_info')\n", "mysql-js>"),
                  ("view.isView()\n", "true")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # Part is covered and part is not, tests should exist for both tables and views
    def test_MYS_442_03(self):
        '''PY In node mode check is_view() function to identify whether the underlying object is a View or not, return bool '''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                        '{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                 LOCALHOST.xprotocol_port), '--mysqlx', '--py']
        x_cmds = [("table = session.get_schema('sakila').get_table('actor')\n", ""),
                  ("table.is_view()\n", "false"),
                  ("view = session.get_schema('sakila').get_table('actor_info')\n", ""),
                  ("view.is_view()\n", "true")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_442_06(self):
        '''Error displayed when try to update the view '''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                        '{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                 LOCALHOST.xprotocol_port), '--mysqlx', '--js']
        x_cmds = [("view = session.getSchema('sakila').getTable('actor_info')\n", "mysql-js>"),
                  ("view.update().set('last_name','GUINESSE').where('actor_id=1').execute()\n",
                   "The target table actor_info of the UPDATE is not updatable")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    # This should be embedded into DevAPI tests at mysqlx_schema.js
    def test_MYS_442_07(self):
        '''Vies displayed as part of getTables() function '''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri',
                        '{0}:{1}@{2}:{3}'.format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                 LOCALHOST.xprotocol_port), '--mysqlx', '--js']
        x_cmds = [("session.getSchema('sakila').getTables()\n", "<Table:actor_info>,"),
                  ("session.getSchema('sakila').getTables()\n", "<Table:actor_list>,")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_470_2(self):
        '''Enable named parameters in python for mysqlx.getSession() and mysqlx.get_session()'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--py']
        x_cmds = [("from mysqlsh import mysqlx\n", "mysql-py>"),
                  ("session=mysqlx.get_session(host= '" + LOCALHOST.host + "', port=" + LOCALHOST.xprotocol_port + ", dbUser= '"
                   + LOCALHOST.user + "', dbPassword= '" + LOCALHOST.password + "')\n", "mysql-py>"),
                  ("session\n",
                   "<Session:" + LOCALHOST.user + "@" + LOCALHOST.host + ":" + LOCALHOST.xprotocol_port + ">"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_492(self):
        '''MYSQLSH SHOWS TIME COLUMNS AS BOOLEAN VALUES'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--schema=sakila']
        x_cmds = [('\\sql\n', "mysql-sql>"),
                  ("use sakila;\n", "mysql-sql>"),
                  ("DROP TABLE IF EXISTS t1;\n", "mysql-sql>"),
                  ("create table t1 (id int not null primary key, t time);\n", "Query OK"),
                  ("insert into t1 values (1, '10:05:30');\n", "Query OK"),
                  ('\\js\n', "Switching to JavaScript mode"),
                  ("db.getTable('t1').select();", '10:05:30'),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_495(self):
        '''MYSQLSH SHOWS TIME COLUMNS AS BOOLEAN VALUES'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--schema=sakila']
        x_cmds = [('\\sql\n', "mysql-sql>"),
                  ("\\status\n", " sec" + os.linesep + os.linesep + os.linesep + "mysql-sql>"),
                  ("rollback release;\n", "mysql-sql>"),
                  ("\\status;\n", "mysql-sql>"),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_496(self):
        '''MySQL Shell prints Undefined on JSON column (Classic Session)'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + REMOTEHOST.user, '--password=' + REMOTEHOST.password,
                        '-h' + REMOTEHOST.host, '-P' + REMOTEHOST.port, '--mysql', '--sqlc', '--schema=sakila_x']
        x_cmds = [("select * from users limit 2;\n", '{\"_id\": \"'),
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_518_01(self):
        ''' Session object Bool isOpen() function in js mode for node session'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--js']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.port, '--mysql', '--py']
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
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--js']
        x_cmds = [("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                    LOCALHOST.port, Sschema),
                   "\"password\": \"" + LOCALHOST.password + "\""),
                  ("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                    LOCALHOST.port, Sschema),
                   "\"user\": \"" + LOCALHOST.user + "\""),
                  ("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                    LOCALHOST.port, Sschema),
                   "\"host\": \"" + LOCALHOST.host + "\""),
                  # ("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                  #                                                  LOCALHOST.port,  Sschema), "\"port\": " + LOCALHOST.xprotocol_port),
                  ("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                    LOCALHOST.port, Sschema),
                   "\"schema\": \"" + Sschema + "\"")
                  ]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_518_06(self):
        ''' pasreUri function in py mode for node session'''
        results = ''
        Sschema = "world_x"
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,
                        '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port, '--mysqlx', '--py']
        x_cmds = [("shell.parse_uri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                    LOCALHOST.port, Sschema),
                   "\"password\": \"" + LOCALHOST.password + "\""),
                  ("shell.parse_uri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                    LOCALHOST.port, Sschema),
                   "\"user\": \"" + LOCALHOST.user + "\""),
                  ("shell.parse_uri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                    LOCALHOST.port, Sschema),
                   "\"host\": \"" + LOCALHOST.host + "\""),
                  # ("shell.parseUri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                  #                                                  LOCALHOST.port,  Sschema), "\"port\": " + LOCALHOST.xprotocol_port),
                  ("shell.parse_uri('{0}:{1}@{2}:{3}/{4}')\n".format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host,
                                                                    LOCALHOST.port, Sschema),
                   "\"schema\": \"" + Sschema + "\"")
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
            content = "#!" + MYSQL_SHELL + " -f" + os.linesep + "print(\"Hello World\");" + os.linesep
            f = open(inputfilename, 'w')
            f.write(content)
            f.close()
            p = subprocess.Popen(["chmod", "777", inputfilename], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                 stdin=subprocess.PIPE)
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

    def test_MYS_583(self):
        '''[MYS-583]: https://jira.oraclecorp.com/jira/browse/MYS-583
      URI parsing does not decode PCT before passing to other systems
      Affected by bug: https://clustra.no.oracle.com/orabugs/bug.php?id=26422790 '''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--mysql', '--sqlc', '--uri={0}:{1}@{2}:{3}'.
            format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.port), '-e',
                        'CREATE USER \'omar!#$&()*+,/:;=?@[]\'@\'localhost\' IDENTIFIED BY \'guidev!\';']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        p.stdin.flush()
        stdoutdata, stderrdata = p.communicate()
        if stderrdata.find(bytearray("\"ERROR\",", "ascii"), 0, len(stderrdata)) == -1:
            results = "PASS"
        else:
            results = "FAIL"
        self.assertEqual(results, 'PASS')
        init_command = [MYSQL_SHELL, '--interactive=full', '--mysql', '--sqlc', '--uri={0}:{1}@{2}:{3}'.
            format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.port), '-e',
                        'GRANT ALL PRIVILEGES ON *.* TO \'omar!#$&()*+,/:;=?@[]\'@\'localhost\' WITH GRANT OPTION;']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        p.stdin.flush()
        stdoutdata, stderrdata = p.communicate()
        if stderrdata.find(bytearray("\"ERROR\",", "ascii"), 0, len(stderrdata)) == -1:
            results = "PASS"
        else:
            results = "FAIL"
        self.assertEqual(results, 'PASS')
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri={0}:{1}@{2}:{3}'.
            format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port)]
        x_cmds = [("\\c --mc omar%21%23%24%26%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D:" + "{0}@{1}:{2}"
                   .format(LOCALHOST.password, LOCALHOST.host, LOCALHOST.port) + "\n", "omar!#$&()*+,/:;=?@[]"),
                  ("\\c --mx omar%21%23%24%26%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D:" + "{0}@{1}:{2}"
                   .format(LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port) + "\n",
                   "omar!%23$&()*+,%2F%3A;=%3F%40%5B%5D@localhost:" + LOCALHOST.xprotocol_port)]
        for command, expectedResult in x_cmds:
            count = 1
            p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
            p.stdin.write(command)
            p.stdin.flush()
            stdoutdata, stderrdata = p.communicate()
            if expectedResult not in stdoutdata and expectedResult not in stderrdata:
                print "CAN'T FIND EXPECTED RESULT", expectedResult
                print stdoutdata, stderrdata
                results = "FAIL"
                break
        self.assertEqual(results, 'PASS')
        init_command = [MYSQL_SHELL, '--interactive=full', '--mysql', '--sqlc', '--uri={0}:{1}@{2}:{3}'.
            format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.port), '-e',
                        'DROP USER \'omar!#$&()*+,/:;=?@[]\'@\'localhost\';']
        p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        p.stdin.flush()
        stdoutdata, stderrdata = p.communicate()
        if stderrdata.find(bytearray("\"ERROR\",", "ascii"), 0, len(stderrdata)) == -1:
            results = "PASS"
        else:
            results = "FAIL"
        self.assertEqual(results, 'PASS')

    def test_MYS_598(self):
        '''MYS-598 Row object overwrites values if two columns have the same name'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full',
                        "--uri={0}:{1}@{2}:{3}/sakila".format(LOCALHOST.user, LOCALHOST.password,
                                                              LOCALHOST.host, LOCALHOST.port)]
        x_cmds = [("\\sql\n", "mysql-sql>"),
                  ("CREATE TABLE mys598 (TestCol int(11) NOT NULL, TestCol2 int(11) NOT NULL, "
                   "PRIMARY KEY (TestCol)) ENGINE=InnoDB DEFAULT CHARSET=latin1;\n", "Query OK"),
                  ("INSERT INTO mys598 (TestCol,TestCol2) VALUES(1,2);\n", "Query OK"),
                  ("select * from mys598;\n", "| TestCol | TestCol2 |" + os.linesep + "+---------+----------+" +
                   os.linesep + "|       1 |        2 |" + os.linesep),
                  ("select TestCol as Test, TestCol2 as Test from mys598;\n", "| Test | Test |" + os.linesep +
                   "+------+------+" + os.linesep + "|    1 |    2 |" + os.linesep),
                  ("drop table sakila.mys598;\n", "Query OK")]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_697(self):
        '''[MYS-697]: https://jira.oraclecorp.com/jira/browse/MYS-697
      Unexpected behavior while executing SQL statements containing comments'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--sqlc', '--uri={0}:{1}@{2}:{3}'.
            format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.port)]
        x_cmds = [("SELECT * from /* this is an in-line comment */ sakila.actor limit 1;\n", "1 row in set"),
                  ("\\js\n", "mysql-js>"),
                  ("session.runSql(\"SELECT * from /* this is an in-line comment */ sakila.actor limit 1;\")\n",
                   "1 row in set"),
                  ("\\py\n", "mysql-py>"),
                  ("session.run_sql(\"SELECT * from /* this is an in-line comment */ sakila.actor limit 1;\")\n",
                   "1 row in set")]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_MYS_730(self):
        '''Accept named log level on startup as command line argument'''
        results = 'PASS'
        responces = ["mysql-js>"]
        p = subprocess.Popen(
            [MYSQL_SHELL, '--interactive=full', '--log-level=debug3'], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            stdin=subprocess.PIPE)
        stdout, stderr = p.communicate()
        for responce in responces:
            found = stdout.find(bytearray(responce, "ascii"), 0, len(stdout))
            if found == -1:
                results = "FAIL \n\r" + stdout.decode("ascii")
                break
        self.assertEqual(results, 'PASS')

    def test_MYS_816(self):
        '''[MYS-816]: https://jira.oraclecorp.com/jira/browse/MYS-816
        Default Session Type Should be Node instead of X'''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user,
                        '--password=' + LOCALHOST.password, '-h' + LOCALHOST.host, '-P' + LOCALHOST.xprotocol_port]
        x_cmds = [("session\n", "<Session:")]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

    def test_Bug_26402917(self):
        ''' https://clustra.no.oracle.com/orabugs/bug.php?id=26402917
        Test bug behavior db.<table>.select() validate expected data in rows '''
        results = ''
        init_command = [MYSQL_SHELL, '--interactive=full', '--uri={0}:{1}@{2}:{3}/world_x'.
            format(LOCALHOST.user, LOCALHOST.password, LOCALHOST.host, LOCALHOST.xprotocol_port)]
        expectedDataInFirst20Rows = \
            "+------+-----------------------------------+-------------+----------------------+--------------------------+" + os.linesep + \
            "| ID   | Name                              | CountryCode | District             | Info                     |" + os.linesep + \
            "+------+-----------------------------------+-------------+----------------------+--------------------------+" + os.linesep + \
            "|    1 | Kabul                             | AFG         | Kabol                | {\"Population\": 1780000}  |" + os.linesep + \
            "|    2 | Qandahar                          | AFG         | Qandahar             | {\"Population\": 237500}   |" + os.linesep + \
            "|    3 | Herat                             | AFG         | Herat                | {\"Population\": 186800}   |" + os.linesep + \
            "|    4 | Mazar-e-Sharif                    | AFG         | Balkh                | {\"Population\": 127800}   |" + os.linesep + \
            "|    5 | Amsterdam                         | NLD         | Noord-Holland        | {\"Population\": 731200}   |" + os.linesep + \
            "|    6 | Rotterdam                         | NLD         | Zuid-Holland         | {\"Population\": 593321}   |" + os.linesep + \
            "|    7 | Haag                              | NLD         | Zuid-Holland         | {\"Population\": 440900}   |" + os.linesep + \
            "|    8 | Utrecht                           | NLD         | Utrecht              | {\"Population\": 234323}   |" + os.linesep + \
            "|    9 | Eindhoven                         | NLD         | Noord-Brabant        | {\"Population\": 201843}   |" + os.linesep + \
            "|   10 | Tilburg                           | NLD         | Noord-Brabant        | {\"Population\": 193238}   |" + os.linesep + \
            "|   11 | Groningen                         | NLD         | Groningen            | {\"Population\": 172701}   |" + os.linesep + \
            "|   12 | Breda                             | NLD         | Noord-Brabant        | {\"Population\": 160398}   |" + os.linesep + \
            "|   13 | Apeldoorn                         | NLD         | Gelderland           | {\"Population\": 153491}   |" + os.linesep + \
            "|   14 | Nijmegen                          | NLD         | Gelderland           | {\"Population\": 152463}   |" + os.linesep + \
            "|   15 | Enschede                          | NLD         | Overijssel           | {\"Population\": 149544}   |" + os.linesep + \
            "|   16 | Haarlem                           | NLD         | Noord-Holland        | {\"Population\": 148772}   |" + os.linesep + \
            "|   17 | Almere                            | NLD         | Flevoland            | {\"Population\": 142465}   |" + os.linesep + \
            "|   18 | Arnhem                            | NLD         | Gelderland           | {\"Population\": 138020}   |" + os.linesep + \
            "|   19 | Zaanstad                          | NLD         | Noord-Holland        | {\"Population\": 135621}   |"
        expectedDataInLast20Rows = \
            "| 4060 | Santa Monica                      | USA         | California           | {\"Population\": 91084}    |" + os.linesep + \
            "| 4061 | Fall River                        | USA         | Massachusetts        | {\"Population\": 90555}    |" + os.linesep + \
            "| 4062 | Kenosha                           | USA         | Wisconsin            | {\"Population\": 89447}    |" + os.linesep + \
            "| 4063 | Elgin                             | USA         | Illinois             | {\"Population\": 89408}    |" + os.linesep + \
            "| 4064 | Odessa                            | USA         | Texas                | {\"Population\": 89293}    |" + os.linesep + \
            "| 4065 | Carson                            | USA         | California           | {\"Population\": 89089}    |" + os.linesep + \
            "| 4066 | Charleston                        | USA         | South Carolina       | {\"Population\": 89063}    |" + os.linesep + \
            "| 4067 | Charlotte Amalie                  | VIR         | St Thomas            | {\"Population\": 13000}    |" + os.linesep + \
            "| 4068 | Harare                            | ZWE         | Harare               | {\"Population\": 1410000}  |" + os.linesep + \
            "| 4069 | Bulawayo                          | ZWE         | Bulawayo             | {\"Population\": 621742}   |" + os.linesep + \
            "| 4070 | Chitungwiza                       | ZWE         | Harare               | {\"Population\": 274912}   |" + os.linesep + \
            "| 4071 | Mount Darwin                      | ZWE         | Harare               | {\"Population\": 164362}   |" + os.linesep + \
            "| 4072 | Mutare                            | ZWE         | Manicaland           | {\"Population\": 131367}   |" + os.linesep + \
            "| 4073 | Gweru                             | ZWE         | Midlands             | {\"Population\": 128037}   |" + os.linesep + \
            "| 4074 | Gaza                              | PSE         | Gaza                 | {\"Population\": 353632}   |" + os.linesep + \
            "| 4075 | Khan Yunis                        | PSE         | Khan Yunis           | {\"Population\": 123175}   |" + os.linesep + \
            "| 4076 | Hebron                            | PSE         | Hebron               | {\"Population\": 119401}   |" + os.linesep + \
            "| 4077 | Jabaliya                          | PSE         | North Gaza           | {\"Population\": 113901}   |" + os.linesep + \
            "| 4078 | Nablus                            | PSE         | Nablus               | {\"Population\": 100231}   |" + os.linesep + \
            "| 4079 | Rafah                             | PSE         | Rafah                | {\"Population\": 92020}    |" + os.linesep + \
            "+------+-----------------------------------+-------------+----------------------+--------------------------+" + os.linesep + \
            "4079 rows in set"
        x_cmds = [("city = db.getTable('City')\n", "mysql-js>"),
                  ("city.select()\n", expectedDataInFirst20Rows),
                  ("city.select()\n", expectedDataInLast20Rows)]
        results = exec_xshell_commands(init_command, x_cmds)
        self.assertEqual(results, 'PASS')

# ----------------------------------------------------------------------

class PB2TextTestResult(unittest.result.TestResult):
    separator1 = '=' * 70
    separator2 = '-' * 70

    def __init__(self, stream, descriptions, verbosity):
        super(PB2TextTestResult, self).__init__(stream, descriptions, verbosity)
        self.stream = stream
        self.showAll = verbosity > 1
        self.dots = verbosity == 1
        self.descriptions = descriptions

    def getDescription(self, test):
        doc_first_line = test.shortDescription()
        if self.descriptions and doc_first_line:
            return ' '.join((str(test), doc_first_line))
        else:
            return str(test)

    def startTest(self, test):
        super(PB2TextTestResult, self).startTest(test)
        self.stream.writeln("[ RUN      ] " + self.getDescription(test))
        self.stream.flush()

    def addSuccess(self, test):
        super(PB2TextTestResult, self).addSuccess(test)
        self.stream.writeln("[       OK ] " + self.getDescription(test))

    def addError(self, test, err):
        super(PB2TextTestResult, self).addError(test, err)
        # self.stream.write("[  ERROR   ] ")
        self.stream.writeln("[  FAILED  ] " + self.getDescription(test))

    def addFailure(self, test, err):
        super(PB2TextTestResult, self).addFailure(test, err)
        self.stream.writeln("[  FAILED  ] " + self.getDescription(test))

    def addSkip(self, test, reason):
        super(PB2TextTestResult, self).addSkip(test, reason)
        self.stream.writeln("[  SKIPPED ] " + self.getDescription(test) + " {0!r}".format(reason))

    def addExpectedFailure(self, test, err):
        super(PB2TextTestResult, self).addExpectedFailure(test, err)
        self.stream.writeln("expected failure")

    def addUnexpectedSuccess(self, test):
        super(PB2TextTestResult, self).addUnexpectedSuccess(test)
        self.stream.writeln("unexpected success")

    def printErrors(self):
        self.stream.writeln()
        self.printErrorList('ERROR', self.errors)
        self.printErrorList('FAIL', self.failures)

    def printErrorList(self, flavour, errors):
        for test, err in errors:
            self.stream.writeln(self.separator1)
            self.stream.writeln("%s: %s" % (flavour,self.getDescription(test)))
            self.stream.writeln(self.separator2)
            self.stream.writeln("%s" % err)

if __name__ == '__main__':
    runner = unittest.TextTestRunner(resultclass=PB2TextTestResult, verbosity=2)
    unittest. main(testRunner=runner)
