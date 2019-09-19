// Display the help for MySQL Shell (mysqlsh --help) and verify that the option --pager is present

//@ help should contain information about --pager option
|  --pager=<value>               Pager used to display text output of statements|
|                                executed in SQL mode as well as some other|
|                                selected commands. Pager can be manually|
|                                enabled in scripting modes. If you don't supply|
|                                an option, the default pager is taken from your|
|                                ENV variable PAGER. This option only works in|
|                                interactive mode. This option is disabled by|
|                                default.|
