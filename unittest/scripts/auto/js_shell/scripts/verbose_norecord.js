
//@ Tests verbose output of the shell (level 0)
shell.options.verbose=0;
// shouldn't appear:
shell.log("error", "==error");
shell.log("warning", "==warning");
shell.log("info", "==info");
shell.log("debug", "==debug");
shell.log("debug2", "==debug2");
shell.log("debug3", "==debug3");

//@ Tests verbose output of the shell (level 1)
shell.options.verbose=1;
shell.log("error", "==error");
shell.log("warning", "==warning");
shell.log("info", "==info");
// shouldn't appear:
shell.log("debug", "==debug");
shell.log("debug2", "==debug2");
shell.log("debug3", "==debug3");

//@ Tests verbose output of the shell (level 2)
shell.options.verbose=2;
shell.log("error", "==error");
shell.log("warning", "==warning");
shell.log("info", "==info");
shell.log("debug", "==debug");
// shouldn't appear:
shell.log("debug2", "==debug2");
shell.log("debug3", "==debug3");

//@ Tests verbose output of the shell (level 3)
shell.options.verbose=3;
shell.log("error", "==error");
shell.log("warning", "==warning");
shell.log("info", "==info");
shell.log("debug", "==debug");
shell.log("debug2", "==debug2");
// shouldn't appear:
shell.log("debug3", "==debug3");

//@ Tests verbose output of the shell (level 4)
shell.options.verbose=4;
shell.log("error", "==error");
shell.log("warning", "==warning");
shell.log("info", "==info");
shell.log("debug", "==debug");
shell.log("debug2", "==debug2");
shell.log("debug3", "==debug3");

//@ Check default value of --verbose on startup
testutil.callMysqlsh(["--js", "-e", "println('verbose=',shell.options.verbose)"]);
testutil.callMysqlsh(["--js", "--verbose=1", "-e", "println('verbose=',shell.options.verbose)"]);
testutil.callMysqlsh(["--js", "--verbose=2", "-e", "println('verbose=',shell.options.verbose)"]);