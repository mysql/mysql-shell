// On SQL mode execute a SELECT statement, verify that the output is forwarded to the pager specified by shell.options["pager"] option.

//@ set pager to an external command
|<<<__pager.cmd>>>|

//@ connect to a server, so we can run queries in SQL mode
|<Session:<<<__uri>>>>|

//@ switch to SQL mode
|Switching to SQL mode... Commands end with ;|

//@ run SQL query, there should be no output here
||

//@ switch back to JS mode
|Switching to JavaScript mode...|

//@<OUT> check if pager got all the output from SQL query
Running with arguments: <<<__pager.cmd>>>

+---+
| 1 |
+---+
| 1 |
+---+
1 row in set ([[*]] sec)

//@ close the connection
||
