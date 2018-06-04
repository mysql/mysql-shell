//@ Generic Help
\?

//@ Help Contents
\? contents

//@ Generic Help in SQL mode
\sql
\?

//@ Help Contents in SQL mode
\? contents
\js

//@ Help on Admin API Category
\? adminapi

//@ Help on shell commands
\? shell commands

//@ Help on ShellAPI Category
\? shellapi

//@ Help on X DevAPI Category
\? x devapi

//@ Help on unknown topic
\? unknown

//@ Help on topic with several matches
\? session

//@ Help for sandbox operations
\? *sandbox*

//@ Help for SQL, with classic session, multiple matches
shell.connect(__mysqluripwd)
\? select

//@ Switching to SQL mode, same test gives results
\sql
\? select

//@ Switching back to JS, help for SQL Syntax
\js
\? SQL Syntax
session.close()

//@ Help for SQL Syntax, with x session
shell.connect(__uripwd)
\? SQL Syntax
session.close()

//@ Help for SQL Syntax, no connection
\? SQL Syntax
