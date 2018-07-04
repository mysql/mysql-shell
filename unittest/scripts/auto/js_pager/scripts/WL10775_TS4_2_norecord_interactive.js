// Access the shell.options['pager'] option to verify you can see the value assigned. Validate the value is an empty string if the options has not been set yet.

//@ check if unassigned option defaults to an empty string
print(shell.options.pager);

//@ set pager to an external command
shell.options.pager = __pager.cmd;

//@ validate if option has the assigned value
print(shell.options.pager);
