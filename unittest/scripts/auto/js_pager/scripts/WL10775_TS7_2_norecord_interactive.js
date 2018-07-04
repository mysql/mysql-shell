// Set a value to the shell.options["pager"] option using the \pager or \P command. Validate that the option has the value set.

//@ check if unassigned option defaults to an empty string
print(shell.options.pager);

//@ set pager to a dummy command
\pager dummy command

//@ validate if option has the assigned value
print(shell.options.pager);
