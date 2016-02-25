import subprocess
import time
import shlex
import datetime
import sys
import os
import threading
import functools
import cProfile
import unittest
import json
import xmlrunner

###############   Retrieve variables from configuration file    ##########################
class MYSQL_HOST:
    user = ""
    password = ""
    host = ""
    port = ""
class MYSQLX_HOST:
    user = ""
    password = ""
    host = ""
    xprotocol_port = ""

    # **** LOCAL EXECUTION ****
config = json.load(open('D:\\XShell_Performance\\config_performance.json'))
MYSQL = str(config["general"]["mysql_path"])
MYSQL_SHELL = str(config["general"]["xshell_path"])
Exec_files_location = str(config["general"]["aux_files_path"])
XMLReportFilePath = "D:\\XShell_Performance\\xshell_perf_test.xml"

# **** JENKINS EXECUTION ****
#config_path = os.environ['CONFIG_PATH']
#config=json.load(open(config_path))
#MYSQL_SHELL = os.environ['MYSQLX_PATH']
#Exec_files_location = os.environ['AUX_FILES_PATH']
#XSHELL_QA_TEST_ROOT = os.environ['XSHELL_QA_TEST_ROOT']
#XMLReportFilePath = XSHELL_QA_TEST_ROOT+"/xshell_qa_test.xml"

#########################################################################

MYSQL_HOST.user = str(config["mysql_host"]["user"])
MYSQL_HOST.password = str(config["mysql_host"]["password"])
MYSQL_HOST.host = str(config["mysql_host"]["host"])
MYSQL_HOST.port = str(config["mysql_host"]["port"])

MYSQLX_HOST.user = str(config["mysqlx_host"]["user"])
MYSQLX_HOST.password = str(config["mysqlx_host"]["password"])
MYSQLX_HOST.host = str(config["mysqlx_host"]["host"])
MYSQLX_HOST.xprotocol_port = str(config["mysqlx_host"]["xprotocol_port"])

###########################################################################################

# Timer definition and execution
def timefunc(f):
    def f_timer(*args, **kwargs):
        start = time.time()
        result = f(*args, **kwargs)
        end = time.time()
        print f.__name__, 'took', end - start, 'seconds to execute'
        return result
    return f_timer

###########################################################################################
# Be aware you need to run first the Setup_Script.sql in order to create the database and stored procedure required

@timefunc
def setup_script_mysql():
    """ Setup required database and store procedure:  --file= Setup_Script.sql """
    init_command_str = MYSQL + ' --no-defaults -u' + MYSQL_HOST.user + ' --password=' + MYSQL_HOST.password + ' -h' + \
                       MYSQL_HOST.host + ' -P' + MYSQL_HOST.port + ' information_schema < ' + Exec_files_location + \
                       'setup_script.sql &'
    p = subprocess.Popen(init_command_str, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         stdin=subprocess.PIPE, shell=True)
    stdin, stdout = p.communicate()
    if stdout.find(bytearray("ERROR", "ascii")) > -1:
        print 'Error:', stdout
    try:
        p.kill()
    except ValueError as e:
        print 'Error: Process created for setup_script_mysql test was not able to close:', e


# MYSQL in SQL (common server)
@timefunc
def test_mysql_sql_create():
    """ SQL common Create and Insert:  --file= Creation_SQL.sql """
    init_command_str = MYSQL + ' --no-defaults -u' + MYSQL_HOST.user + ' --password=' + MYSQL_HOST.password + ' -h' + \
                       MYSQL_HOST.host + ' -P' + MYSQL_HOST.port + ' information_schema < ' + Exec_files_location + \
                       'creation_sql.sql &'
    p = subprocess.Popen(init_command_str, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         stdin=subprocess.PIPE, shell=True)
    stdin, stdout = p.communicate()
    if stdout.find(bytearray("ERROR", "ascii")) > -1:
        print 'Error:', stdout
    try:
        p.kill()
    except ValueError as e:
        print 'Error: Process created for test_mysql_sql_create test was not able to close:', e

# MYSQL X in Javascript with Node mode
@timefunc
def test_mysqlx_js_node_create():
    """ Javascript with Node Mode, Create and Insert:  --file= Creation_NodeMode.js """
    init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + MYSQLX_HOST.user, '--password=' +
                    MYSQLX_HOST.password, '-h' + MYSQLX_HOST.host, '-P' + MYSQLX_HOST.xprotocol_port,
                    '--session-type=node', '--js', '--file=' + Exec_files_location + 'Creation_NodeMode.js']
    p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    stdin, stdout = p.communicate()
    if stdout.find(bytearray("Error", "ascii"), 0, len(stdin)) > -1:
        print stdout
    try:
        p.kill()
    except ValueError as e:
        print 'Error: Process created for test_mysqlx_js_node_create test was not able to close:', e

# MYSQL X in Javascript with Classic mode
@timefunc
def test_mysqlx_js_classic_create():
    """ Javascript with Node Mode, Create and Insert:  --file= Creation_NodeMode.js """
    init_command = [MYSQL_SHELL, '--interactive=full', '--log-level=7', '-u' + MYSQL_HOST.user, '--password=' +
                    MYSQL_HOST.password, '-h' + MYSQL_HOST.host, '-P' + MYSQL_HOST.port,
                    '--session-type=classic', '--js', '--file=' + Exec_files_location + 'Creation_ClassicMode.js']
    p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
    stdin, stdout = p.communicate()
    if stdout.find(bytearray("Error", "ascii"), 0, len(stdin)) > -1:
        print stdout
    try:
        p.kill()
    except ValueError as e:
        print 'Error: Process created for test_mysqlx_js_classic_create test was not able to close:', e
# ----------------------------------------------------------------------

print 'Test Start: ', datetime.datetime.now()
setup_script_mysql()
result = test_mysql_sql_create()
result = test_mysqlx_js_node_create()
result = test_mysqlx_js_classic_create()
print 'Test End: ', datetime.datetime.now()



