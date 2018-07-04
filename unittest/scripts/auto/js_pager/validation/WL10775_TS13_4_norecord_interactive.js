// Use the method shell.enablePager(). Then use shell.disablePager(), validate that the command set to the shell.options["pager"] option is not in use to print the output and the shell.options["pager"] option must have no changes in its value.

//@ set pager to an external command
|<<<__pager.cmd>>> --append|

//@ run command before pager is enabled in JS mode
|1111|

//@ call enablePager()
||

//@ run command while pager is enabled in JS mode
||

//@ call disablePager()
||

//@ pager should remain the same after calling disablePager()
|<<<__pager.cmd>>> --append|

//@ run command after pager has been disabled in JS mode
|3333|

//@<OUT> verify if external command received output when pager was enabled
Running with arguments: <<<__pager.cmd>>> --append

2222

done
