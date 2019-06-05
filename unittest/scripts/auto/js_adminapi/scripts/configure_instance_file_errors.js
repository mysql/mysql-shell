
// These tests verify handling of cases where the config file does not exist,
// is not readable or not writable, assuming the file has to be written

// During check:
// does not exist
// is not readable (no permission, bad path)
//
// During update:
// does not exist - should create
// is not readable
// is not writable
// directory is not readable
// directory is not writable

// TODO - also add the same for configureLocalInstance, for after instance is in group

function diff_text(expected, actual) {
  var exp = expected.split("\n").sort();
  var act = actual.split("\n").sort();

  // Ignore empty lines (remove from the lists of strings to compare).
  for(var i = exp.length-1; i--;){
    if (exp[i] == '') exp.splice(i, 1);
  }
  for(var i = act.length-1; i--;){
    if (act[i] == '') act.splice(i, 1);
  }

  var diff = [];
  for (var i = 0; i < exp.length; i++) {
    var k = act.indexOf(exp[i]);
    if (k < 0) {
      diff.push("missing: " + exp[i]);
    } else {
      act.splice(k, 1);
    }
  }
  for (var i = 0; i < act.length; i++) {
    diff.push("unexpected: " + act[i]);
  }
  return diff;
}

function EXPECT_MYCNF_UNCHANGED(path) {
  var expected = os.load_text_file(bad_mycnf_path);
  var actual = os.load_text_file(path);
  var diff = diff_text(expected, actual);
  if (diff.length > 0) {
    println("Differences:");
    println(diff.join("\n"));
    testutil.fail("<red>Test check failed:</red> my.cnf file was changed but it was not supposed to");
  }
}

function EXPECT_MYCNF_FIXED(path) {
  var expected = os.load_text_file(good_mycnf_path);
  var actual = os.load_text_file(path);
  var diff = diff_text(expected, actual);
  if (diff.length > 0) {
    println("Differences:");
    println(diff.join("\n"));
    testutil.fail("<red>Test check failed:</red> my.cnf file was not fixed as expected");
  }
}


var SCEN_FILE_MISSING = 0;
var SCEN_FILE_EXISTS = 1;
var SCEN_FILE_NO_READ = 2;
var SCEN_FILE_NO_WRITE = 4;
var SCEN_FILE_DIR_NO_READ = 8;
var SCEN_FILE_DIR_NO_WRITE = 16;

function dirname(path) {
  var i = path.lastIndexOf("/");
  var j = path.lastIndexOf("\\");
  if (i > j)
    return path.substr(0, i);
  else
    return path.substr(0, j);
}

function setup_scenario(flags) {
  try {testutil.rmfile(mycnf_path);} catch (err) {}
  try {testutil.rmfile(alt_mycnf_path);} catch (err) {}

  session.runSql("set global binlog_format=mixed");

  if (flags & SCEN_FILE_EXISTS) {
    testutil.cpfile(bad_mycnf_path, mycnf_path);
  }

  var fmod = 0600;
  if (flags & SCEN_FILE_NO_READ) {
    fmod &= ~0400;
  }
  if (flags & SCEN_FILE_NO_WRITE) {
    fmod &= ~0200;
  }
  if (fmod != 0600)
    testutil.chmod(mycnf_path, fmod);

  fmod = 0700;
  if (flags & SCEN_FILE_DIR_NO_READ) {
    fmod &= 0400;
  }
  if (flags & SCEN_FILE_DIR_NO_WRITE) {
    fmod &= 0200;
  }
  if (fmod != 0700)
    testutil.chmod(dirname(mycnf_path), fmod);
}

// Init

testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
var mycnf_path = testutil.getSandboxConfPath(__mysql_sandbox_port1);
var alt_mycnf_path = "mytmp.cnf";
var good_mycnf_path = "mygood.cnf";
var bad_mycnf_path = "mybad.cnf";
try {testutil.rmfile(alt_mycnf_path);} catch (err) {}
try {testutil.rmfile(good_mycnf_path);} catch (err) {}
try {testutil.rmfile(bad_mycnf_path);} catch (err) {}

// Configure it once to get things that don't matter for this test out of the way
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path, clusterAdmin: "root@'%'", restart:false});

testutil.restartSandbox(__mysql_sandbox_port1);
// Add a bogus option, to ensure it's not lost when the file is rewritten
testutil.changeSandboxConf(__mysql_sandbox_port1, "test_option", "somevalue");

// BEGIN WORKAROUND
// This shouldn't be needed, but is a workaround for a strange behaviour in mysqlprovision
// where calling check on a config file with something to change generates
// extra entries for group_replication and group_replication_start_on_boot,
// while calling it without anything to fix doesn't.
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_format", "MIXED");
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path});
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_format", "ROW");
// END WORKAROUND

// Make a backup of the fixed config file
testutil.cpfile(mycnf_path, good_mycnf_path);

// Make a single change to require the config file to be updated
testutil.changeSandboxConf(__mysql_sandbox_port1, "binlog_format", "mixed");
shell.connect(__sandbox_uri1);

// Make a backup of the modified config file to be used in all tests
testutil.cpfile(mycnf_path, bad_mycnf_path);

// Positive cases
//===========================================
//@<> File exists and is writable
setup_scenario(SCEN_FILE_EXISTS);
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path});
EXPECT_MYCNF_FIXED(mycnf_path);

// Let the next test reuse the fixed config file from this one
//@<> File isn't writable, but we don't need to write to it either - pass
testutil.makeFileReadOnly(mycnf_path);
dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path});

// Remove the read only
testutil.chmod(mycnf_path, 0777);

//@<> File exists, outputPath given, file is created
setup_scenario(SCEN_FILE_EXISTS);

dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path, outputMycnfPath: alt_mycnf_path});

EXPECT_MYCNF_UNCHANGED(mycnf_path);
EXPECT_MYCNF_FIXED(alt_mycnf_path);

// Errors during check
//===========================================

//@<> File does not exist, error
// Regression for BUG#27702439- WRONG MESSAGES DISPLAYED WHEN USING 'MYCNFPATH' AND 'OUTPUTMYCNFPATH' PARAMETERS
setup_scenario(SCEN_FILE_MISSING);

error_msg = "Dba.configureInstance: File '" + mycnf_path + "' does not exist.";
EXPECT_THROWS(function(){dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path})}, error_msg);
EXPECT_OUTPUT_CONTAINS("ERROR: The specified MySQL option file does not exist. A valid path is required for the mycnfPath option.\nPlease provide a valid path for the mycnfPath option or leave it empty if you which to skip the verification of the MySQL option file.");

//@<> File does not exist, interactive: true, fail
// Regression for BUG#27702439- WRONG MESSAGES DISPLAYED WHEN USING 'MYCNFPATH' AND 'OUTPUTMYCNFPATH' PARAMETERS
setup_scenario(SCEN_FILE_MISSING);
EXPECT_THROWS(function(){dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf_path, interactive:true})}, error_msg);
EXPECT_OUTPUT_CONTAINS("ERROR: The specified MySQL option file does not exist. A valid path is required for the mycnfPath option.\nPlease provide a valid path for the mycnfPath option or leave it empty if you which to skip the verification of the MySQL option file.");


//@<> File does not exist, no interactive, fail

//@<> File exists but not readable, fail

// Errors during update
//===========================================

//@<> File does not exist, allow create, success

//@<> File does not exist, allow create, directory not writable, fail

//@<> File does not exist, allow create, invalid path, fail

//@<> File exists, not readable, fail

//@<> File exists, not writable, fail

//@<> File exists, file writable but directory not writable, fail backup

//@<> File exists, directory is not readable, ??

//@<> File exists, outputPath given, target dir not writable, fail

//@<> File exists, outputPath given, target path invalid, fail

//@<> File exists, outputPath given, writable target file exists but can't be backed up, fail backup


//===========================================
// Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
