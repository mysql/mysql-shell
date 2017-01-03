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

def kill_process(instance, cluster_Path, MYSQL_SHELL):
    results="PASS"
    home = os.path.expanduser("~")
    try:
        init_command = [MYSQL_SHELL, '--interactive=full']
        if os.path.isdir(os.path.join(home,"mysql-sandboxes",instance)):
            x_cmds = [
                      ("dba.killSandboxInstance(" + instance + ");\n", "successfully killed."),
                      ("dba.deleteSandboxInstance(" + instance + ");\n", "successfully deleted."),
                      ]
            results = exec_xshell_commands(init_command, x_cmds)
        elif os.path.isdir(os.path.join(cluster_Path,instance)):
            x_cmds = [
                      ("dba.killSandboxInstance(" + instance + ", { sandboxDir: \"" + cluster_Path + "\"});\n", "successfully killed."),
                      ("dba.deleteSandboxInstance(" + instance + ", { sandboxDir: \"" + cluster_Path + "\"});\n", "successfully deleted."),
                     ]
            results = exec_xshell_commands(init_command, x_cmds)
    except Exception, e:
        # kill instance failed
        print("kill instance"+instance+"Failed, "+e)
    return results


@timeout(240)
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
        #found = read_til_getShell(p, p.stdout, lookup)
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



class globalvar:
    last_found=""
    last_search=""

###########################################################################################

