// Use the method shell.enablePager() with the shell.options["pager"] not set, Pager must not be used to print the output. Then set a valid command to the shell.options["pager"] option, the Pager specified must start to print the output.

//@ set pager to an external command
|<<<__pager.cmd>>> --append|

//@ call enablePager()
||

//@ run command while pager is enabled in JS mode
||

//@ change pager
// output of this command will be redirected to the new pager, no output here
||

//@ run command while pager is enabled in JS mode, new pager should be used
||

//@ call disablePager()
||

//@<OUT> verify if external command received output when pager was enabled
Running with arguments: <<<__pager.cmd>>> --append

2222
Pager has been set to '<<<__pager.cmd>>> --append --new-pager'.
Running with arguments: <<<__pager.cmd>>> --append --new-pager

<<<__pager.cmd>>> --append --new-pager
3333

done
