// Access the shell.options['pager'] option to verify you can see the value assigned. Validate the value is an empty string if the options has not been set yet.

//@ check if unassigned option defaults to an empty string
||

//@ set pager to an external command
|<<<__pager.cmd>>>|

//@ validate if option has the assigned value
|<<<__pager.cmd>>>|
