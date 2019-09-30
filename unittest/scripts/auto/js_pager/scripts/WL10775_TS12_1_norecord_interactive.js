// Set the shell.options["pager"] to a valid external command and perform an action which should use the Pager. The external command must receive the output of performed action.The command should have a functionality that help us to identify that the Pager is printing the output instead of Shell printing the output.

//@ disable the pager
shell.options.pager = '';

//@ invoke \help mysql
\help mysql

//@ set pager to an external command
shell.options.pager = __pager.cmd;

//@ invoke \help mysql, there should be no output here
\help mysql

//@ check if pager got all the output from \help mysql
os.loadTextFile(__pager.file);
