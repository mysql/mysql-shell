// Use the method shell.enablePager() with the shell.options["pager"] not set, Pager must not be used to print the output. Then set a valid command to the shell.options["pager"] option, the Pager specified must start to print the output.

//@ set pager to an empty string
||

//@ run command before pager is enabled in JS mode
|1111|

//@ call enablePager()
||

//@ run command while pager is enabled in JS mode and pager is not set
|2222|

//@ set pager to an external command
// output of this command will be redirected to the pager, no output here
||

//@ run command while pager is enabled in JS mode and pager is set to a valid command
||

//@ call disablePager()
||

//@<OUT> verify if external command received output when pager was enabled
Running with arguments: <<<__pager.cmd>>> --append

<<<__pager.cmd>>> --append
3333

done
