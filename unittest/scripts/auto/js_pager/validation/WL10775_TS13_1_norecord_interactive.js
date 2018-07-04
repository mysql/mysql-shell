// Display the help for the shell object, verify the methods enablePager() and disablePager() are present.

//@ set pager to an external command
|<<<__pager.cmd>>>|

//@ invoke \help shell, there should be no output here
||

//@ load output from pager
||

//@ help should contain information about enablePager() method
||

//@ help should contain information about disablePager() method
||
