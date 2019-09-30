// Use the method shell.enablePager(). Then use shell.disablePager(), validate that the command set to the shell.options["pager"] option is not in use to print the output and the shell.options["pager"] option must have no changes in its value.

//@ set pager to an external command
shell.options.pager = __pager.cmd + ' --append';

//@ run command before pager is enabled in JS mode
1111

//@ call enablePager()
shell.enablePager();

//@ run command while pager is enabled in JS mode
2222

//@ call disablePager()
shell.disablePager();

//@ pager should remain the same after calling disablePager()
println(shell.options.pager);

//@ run command after pager has been disabled in JS mode
3333

//@ verify if external command received output when pager was enabled
os.loadTextFile(__pager.file);
println('done');
