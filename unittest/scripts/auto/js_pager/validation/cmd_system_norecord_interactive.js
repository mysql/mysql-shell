//@ set pager to an external command which will capture the output to a file
|<<<__pager.cmd>>> --append|

//@ enable the pager
||

//@ run \system command, output should not be captured by pager
||

//@ disable the pager
||

//@ check if pager didn't get the output from the system command
||
