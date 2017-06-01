xshell_automated 
===============
(script on Python created by WexQA team in order to speed up/automate  MySQL Shell Testing Effort)
----------------------
Prerequisites:
MySQL Server 5.7.12+
Python 2.7
xmlrunner
------------------------------------------
GENERALS
It uses subprocess in order to open xshell session:
      p = subprocess.Popen(init_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
It uses xmlrunner to generate the xml reports
      unittest.main( testRunner=xmlrunner.XMLTestRunner(file(XMLReportFilePath,"w")))
----------------------------------------------------
exec_xshell_commands()
It uses  exec_xshell_commands(init_command, x_cmds) function in order to execute all the commands for a use case inside the xshell, where:

init_command: set of arguments for opening a xshell session
init_command example:
     init_command = [MYSQL_SHELL, '--interactive=full', '-u' + LOCALHOST.user, '--password=' + LOCALHOST.password,'-h' + LOCALHOST.host,'-P' + LOCALHOST.xprotocol_port, '--node', '--sql']

x_cmds: set of commands to be executed in xshell session and its expected results
x_cmds example:
      x_cmds = [("session;", '<Session:root@localhost:33060>')]

this tupla contains:
      x_cmds = [(string to be executed in shell, expected string results]
and could be passes several tuplas in one time to execute a usecase:
      x_cmds = [(string 1 to be executed in shell, expected string 1 results),
                (string 2 to be executed in shell, expected string 2 results)
                (string 3 to be executed in shell, expected string 3 results)]
--------------------------------------------------------
