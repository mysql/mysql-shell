// Change the value of the shell.options['pager'] using shell.set or shell.setPersist and verify that the value of the property is the one assigned.

//@ check if unassigned option defaults to an empty string
print(shell.options.pager);

//@ set pager to an external command
shell.options.set('pager', __pager.cmd);

//@ validate if option has the assigned value
print(shell.options.pager);
