// On SQL mode execute a SELECT statement, verify that the output is forwarded to the pager specified by shell.options["pager"] option.

//@ set pager to an external command
shell.options.pager = __pager.cmd;

//@ connect to a server, so we can run queries in SQL mode
shell.connect(__uripwd);

//@ switch to SQL mode
\sql

//@ run SQL query, there should be no output here
SELECT 1;

//@ switch back to JS mode
\js

//@ check if pager got all the output from SQL query
os.load_text_file(__pager.file);

//@ close the connection
session.close();
