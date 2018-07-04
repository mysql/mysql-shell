// Display the help for shell.options and verify that the pager option is present.

//@ set pager to an external command
|<<<__pager.cmd>>>|

//@ invoke \help shell.options, there should be no output here
||

//@ help should contain information about pager
||
