// Display the general help for MySQL Shell (\h), verify that the command \pager (\P) is present.

//@ set pager to an external command
|<<<__pager.cmd>>>|

//@ invoke \help Shell Commands, there should be no output here
||

//@ help should contain information about pager commands
||
