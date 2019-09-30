// Use the method shell.enablePager(). Change the value of the command set to shell.options["pager"] option. The Pager must use the new command set to print the output.

//@ set pager to an external command
shell.options.pager = __pager.cmd + ' --append';

//@ call enablePager()
shell.enablePager();

//@ run command while pager is enabled in JS mode
2222

//@ change pager
// output of this command will be redirected to the new pager, no output here
shell.options.pager = __pager.cmd + ' --append --new-pager';

//@ run command while pager is enabled in JS mode, new pager should be used
3333

//@ call disablePager()
shell.disablePager();

//@ verify if external command received output when pager was enabled
os.loadTextFile(__pager.file);
println('done');
