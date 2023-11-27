if (__mysqluripwd) {
  var s = mysql.getSession(__mysqluripwd);
  var r = s.runSql("SELECT @@hostname, @@report_host").fetchOne();
  var __mysql_hostname = r[0];
  var __mysql_report_host = r[1];
  var __mysql_host = __mysql_report_host ? __mysql_report_host : __mysql_hostname;

  // Address that appear in pre-configured sandboxes that set report_host to 127.0.0.1
  var __address1 = "127.0.0.1:" + __mysql_sandbox_port1;
  var __address2 = "127.0.0.1:" + __mysql_sandbox_port2;
  var __address3 = "127.0.0.1:" + __mysql_sandbox_port3;

  // Address that appear in raw sandboxes, that show the real hostname
  var __address1r = __mysql_host + ":" + __mysql_sandbox_port1;
  var __address2r = __mysql_host + ":" + __mysql_sandbox_port2;
  var __address3r = __mysql_host + ":" + __mysql_sandbox_port3;
}

function validate_crud_functions(crud, expected)
{
    var actual = dir(crud);

    // Ensures expected functions are on the actual list
    var missing = [];
    for(exp_funct of expected){
        var pos = actual.indexOf(exp_funct);
        if(pos == -1){
            missing.push(exp_funct);
        }
        else{
            actual.splice(pos, 1);
        }
    }

    if(missing.length == 0){
        print ("All expected functions are available\n");
    }
    else{
        print("Missing Functions:", missing);
    }

    // help is ignored cuz it's always available
    if (actual.length == 0 || (actual.length == 1 && actual[0] == "help")){
        print("No additional functions are available\n")
    }
    else{
        print("Extra Functions:", actual);
    }
}

function validateMembers(obj, expected)
{
    var actual = dir(obj);

    // Ensures expected members are on the actual list
    var missing = [];
    for(exp of expected){
        var pos = actual.indexOf(exp);
        if(pos == -1){
            missing.push(exp);
        }
        else{
            actual.splice(pos, 1);
        }
    }

    var errors = []

    if(missing.length) {
        errors.push("Missing Members: " + missing.join(", "));
    }

    // help is ignored cuz it's always available
    if (actual.length > 1  || (actual.length == 1 && actual[0] != 'help')) {
      errors.push("Extra Members: " + actual.join(", "));
    }

    if (errors.length) {
      testutil.fail(errors.join("\n"))
    }
}

function ensure_schema_does_not_exist(session, name) {
  try {
    var schema = session.getSchema(name);
    session.dropSchema(name);
  } catch(err) {
    // Nothing happens, it means the schema did not exist
  }
}

function getSchemaFromList(schemas, name){
  for(index in schemas){
    if(schemas[index].name == name)
      return schemas[index];
  }

  return null;
}

function wait(timeout, wait_interval, condition){
  waiting = 0;
  res = condition();
  while(!res && waiting < timeout) {
    os.sleep(wait_interval);
    waiting = waiting + 1;
    res = condition();
  }
  return res;
}


// cb_params is an array with whatever parameters required by the
// called callback, this way we can pass params to that function
// right from the caller of retry
function retry(attempts, wait_interval, callback, cb_params){
  attempt = 1;
  res = callback(cb_params);
  while(!res && attempt < attempts) {
    println("Attempt " + attempt + " of " + attempts + " failed, retrying...");

    os.sleep(wait_interval);
    attempt = attempt + 1;
    res = callback(cb_params);
  }

  return res;
}

var ro_session;
var ro_module = require('mysql');
function wait_super_read_only_done() {
  var super_read_only = ro_session.runSql('select @@super_read_only').fetchOne()[0];

  println("---> Super Read Only = " + super_read_only);

  return super_read_only == "0";
}

function check_super_read_only_done(connection) {
  ro_session = ro_module.getClassicSession(connection);
  wait(60, 1, wait_super_read_only_done);
  ro_session.close();
}

function connect_to_sandbox(params) {
  var port = params[0];
  var connected = false;
  try {
    shell.connect({scheme: 'mysql', user:'root', password:'root', host:'localhost', port:port})
    connected = true;
    println('connected to sandbox at ' + port);
  } catch (err) {
    println('failed connecting to sandbox at ' + port + ': ' + err.message);
  }

  return connected;
}

//Function to delete sandbox (only succeed after full server shutdown).
function try_delete_sandbox(port, sandbox_dir) {
    var deleted = false;

    options = {}
    if (sandbox_dir != '')
        options['sandboxDir'] = sandbox_dir;

    print('Try deleting sandbox at: ' + port);
    started = wait(10, 1, function() {
        try {
            dba.deleteSandboxInstance(port, options);

            println(' succeeded');
            return true;
        } catch (err) {
            println(' failed: ' + err.message);
            return false;
        }
    });
    if (deleted) {
        println('Delete succeeded at: ' + port);
    } else {
        println('Delete failed at: ' + port);
    }
}

function set_sysvar(session, variable, value) {
    session.runSql("SET GLOBAL "+variable+" = ?", [value]);
}

function get_sysvar(session, variable, type) {
  var close_session = false;
  var pos = 0;
  var query = "";
  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    session = shell.connect("mysql://root:root@localhost:" + session);
    close_session = true;
  }

  if (type == undefined)
    type = "GLOBAL";

  if (type == "GLOBAL") {
    query = "SELECT @@GLOBAL." + variable;
  } else if (type == "PERSISTED") {
    query = "SELECT * from performance_schema.persisted_variables WHERE Variable_name like '%" + variable + "%'";
    pos = 1;
  }

  var row = session.runSql(query).fetchOne();

  // Close the session if established in this function
  if (close_session)
    session.close();

  if (row == null)
    return "";

  return row[pos];
}

function enable_auto_rejoin(session, port) {
  var close_session = false;

  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    var port = session;
    session = shell.connect("mysql://root:root@localhost:" + session);
    close_session = true;
  }

   testutil.changeSandboxConf(port, "group_replication_start_on_boot", "ON");

  if (__version_num > 80011)
    session.runSql("RESET PERSIST group_replication_start_on_boot");

  // Close the session if established in this function
  if (close_session)
    session.close();
}

function disable_auto_rejoin(session, port) {
  var close_session = false;

  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    var port = session;
    session = shell.connect("mysql://root:root@localhost:" + session);
    close_session = true;
  }

  testutil.changeSandboxConf(port, "group_replication_start_on_boot", "OFF");

  if (__version_num > 80011)
    session.runSql("RESET PERSIST IF EXISTS group_replication_start_on_boot");

  // Close the session if established in this function
  if (close_session)
    session.close();
}

function disable_expel_timeout(session) {
  // in 8.0.21, default expel_timeout was set to 5 seconds from 0
  if (__version_num >= 80021) return;

  var close_session = false;

  // Check if the variable session is an int, if so it's the member_port and we need to establish a session
  if (typeof session == "number") {
    var port = session;
    session = shell.connect("mysql://root:root@localhost:" + session);
    close_session = true;
  }

  session.runSql("SET PERSIST group_replication_expel_timeout=0");

  // Close the session if established in this function
  if (close_session)
    session.close();
}

function create_root_from_anywhere(session, clear_super_read_only) {
    var super_read_only = get_sysvar(session, "super_read_only");
    var super_read_only_enabled = super_read_only === "ON";
    session.runSql("SET SQL_LOG_BIN=0");
    if (clear_super_read_only && super_read_only_enabled)
        set_sysvar(session, "super_read_only", 0);
    session.runSql("CREATE USER root@'%' IDENTIFIED BY 'root'");
    session.runSql("GRANT ALL ON *.* to root@'%' WITH GRANT OPTION");
    if (clear_super_read_only && super_read_only_enabled)
        set_sysvar(session, "super_read_only", 1);
    session.runSql("SET SQL_LOG_BIN=1");
}


function ensure_plugin_enabled(plugin_name, session, plugin_soname) {
  // For function signature simplicity I use `plugin_name` also as shared library name.
  var sess = session === undefined ? mysql.getClassicSession(__uripwd) : session;
  var rs = sess.runSql("SELECT COUNT(1) FROM INFORMATION_SCHEMA.PLUGINS WHERE PLUGIN_NAME like '" + plugin_name + "';");
  var is_installed = rs.fetchOne()[0];

  if (plugin_soname === undefined)
    plugin_soname = plugin_name;

  if (!is_installed) {
    var os = sess.runSql('select @@version_compile_os').fetchOne()[0];
    if (os == "Win32" || os == "Win64") {
      sess.runSql("INSTALL PLUGIN " + plugin_name + " SONAME '" + plugin_soname + ".dll';");
    }
    else {
      sess.runSql("INSTALL PLUGIN " + plugin_name + " SONAME '" + plugin_soname + ".so';");
    }
  }
  if (session === undefined)
    sess.close();
}

function ensure_plugin_disabled(plugin_name) {
  var sess = mysql.getClassicSession(__uripwd);
  var rs = sess.runSql("SELECT COUNT(1) FROM INFORMATION_SCHEMA.PLUGINS WHERE PLUGIN_NAME like '" + plugin_name + "';");
  var is_installed = rs.fetchOne()[0];

  if (is_installed) {
    sess.runSql("UNINSTALL PLUGIN " + plugin_name + ";");
  }
  sess.close();
}

// Check the server compatibility with the specified version numbers.
// Return true if the server version is >= than the one specified one, otherwise
// false.
function check_server_version(major, minor, patch) {
    var result = session.runSql("SELECT sys.version_major(), sys.version_minor(), sys.version_patch()");
    var row = result.fetchOne();
    var srv_major = row[0];
    var srv_minor = row[1];
    var srv_patch = row[2];
    return (srv_major > major ||
        (srv_major == major &&
            (srv_minor > minor || (srv_minor == minor && srv_patch >= patch))));
}

// Check if the instance exists in the Metadata schema
function exist_in_metadata_schema() {
  var result = session.runSql(
      'SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances where CAST(mysql_server_uuid AS CHAR CHARACTER SET ASCII) = CAST(@@server_uuid AS CHAR CHARACTER SET ASCII);');
  var row = result.fetchOne();
  return row[0] != 0;
}

// -------- Test Expectations

function EXPECT_EQ(expected, actual, note) {
  if (note == undefined)
    note = "";
  if (repr(expected) != repr(actual)) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Tested values don't match as expected:</red> "+note+"\n\t<yellow>Actual:</yellow> " + repr(actual) + "\n\t<yellow>Expected:</yellow> " + repr(expected);
    testutil.fail(context);
  }
}

function EXPECT_STDOUT_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (out.indexOf(text) < 0) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

function WARNING_SKIPPED_TEST(reason) {
  return false;
}

function EXPECT_THROWS(func, etext) {
  if (typeof(etext) != "string" && typeof(etext) != "object") {
      testutil.fail("EXPECT_THROWS expects string or array, " +typeof(etext) + " given");
  }
  try {
    func();
    testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Missing expected exception throw like " + etext + "</red>");
  } catch (err) {
    testutil.dprint("Got exception as expected: " + JSON.stringify(err));
    if (typeof(etext) === "string") {
        if (err.message.indexOf(etext) < 0) {
          testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Exception expected:</red> " + etext + "\n\t<yellow>Actual:</yellow> " + err.message);
        }
    } else if (typeof(etext) === "object") {
        var found = false;
        for (i in etext) {
            if (err.message.indexOf(etext[i]) >= 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            var msg = "<b>Context:</b> " + __test_context + "\n<red>One of the exceptions expected:</red>\n";
            for (i in etext) {
                msg += etext[i] + "\n";
            }
            msg += "<yellow>Actual:</yellow> " + err.message;
            testutil.fail(msg);
        }
    }
  }
}

function EXPECT_THROWS_TYPE(func, etext, type) {
  try {
    func();
    testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Missing expected exception throw like " + etext + "</red>");
  } catch (err) {
    testutil.dprint("Caught exception: " + JSON.stringify(err));
    if (err.message.indexOf(etext) < 0) {
      testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Exception expected:</red> " + etext + "\n\t<yellow>Actual:</yellow> " + err.message);
    }
    if (err.type  !== type) {
      testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Exception type expected:</red> " + type + "\n\t<yellow>Actual:</yellow> " + err.type);
    }
  }
}

function EXPECT_NO_THROWS(func, context) {
  try {
    func();
  } catch (err) {
    testutil.dprint("Unexpected exception: " + JSON.stringify(err));
    testutil.fail("<b>Context:</b> " + __test_context + "\n<red>Unexpected exception thrown (" + context + "): " + err.message + "</red>");
  }
}

function EXPECT_OUTPUT_CONTAINS(text) {
  var out = testutil.fetchCapturedStdout(false);
  var err = testutil.fetchCapturedStderr(false);
  if (out.indexOf(text) < 0 && err.indexOf(text) < 0) {
    var context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text + "\n<yellow>Actual stdout:</yellow> " + out + "\n<yellow>Actual stderr:</yellow> " + err;
    testutil.fail(context);
  }
}

// Starting 8.0.24 the client lib started reporting connection error using
// host:port format, previous versions used just the host.
//
// This function is used to format the host description accordingly.
function libmysql_host_description(hostname, port) {
  if (testutil.versionCheck(__libmysql_version_id, ">", "8.0.23")) {
    return hostname + ":" + port;
  }

  return hostname;
}


function __split_trim_join(text) {
  const needle = '\n';
  const s = text.split(needle);
  s.forEach(function (item, index) { this[index] = item.trimEnd(); }, s);
  return {'str': s.join(needle), 'array': s};
}

function __check_wildcard_match(expected, actual) {
  if (0 === expected.length) {
    return expected == actual;
  }

  const needle = '[[*]]';
  const strings = expected.split(needle);
  var start = 0;

  for (const str of strings) {
    const idx = actual.indexOf(str, start);

    if (idx < 0) {
      return false;
    }

    start = idx + str.length;
  }

  if (strings[strings.length - 1] === '') {
    // expected ends with wildcard
    start = actual.length;
  }

  return start === actual.length;
}

function __multi_value_compare(expected, actual) {
  const start = expected.indexOf('{{');

  if (start < 0) {
    return __check_wildcard_match(expected, actual);
  } else {
     const end = expected.indexOf('}}');
     const pre = expected.substring(0, start);
     const post = expected.substring(end + 2);
     const opts = expected.substring(start + 2, end);

     for (const item of opts.split('|')) {
       if (__check_wildcard_match(pre + item + post, actual)) {
         return true;
       }
     }

     return false;
  }
}

function __find_line_matching(expected, actual_lines, start_at = 0) {
  var actual_index = start_at;

  while (actual_index < actual_lines.length) {
    if (__multi_value_compare(expected, actual_lines[actual_index])) {
      break;
    } else {
      ++actual_index;
    }
  }

  return actual_index;
}

function __diff_with_error(arr, error, idx) {
  const needle = '\n';
  const copy = [...arr];
  copy[idx] += error;
  return copy.join(needle);
}

function __check_multiline_expect(expected_lines, actual_lines) {
  var diff = '';
  var matches = true;
  const needle = '\n';

  while ('' === expected_lines[0] || '[[*]]' === expected_lines[0]) {
    expected_lines.shift();
  }

  var actual_index = __find_line_matching(expected_lines[0], actual_lines);

  if (actual_index < actual_lines.length) {
    for (var expected_index = 0; expected_index < expected_lines.length; ++expected_index) {
      if ('[[*]]' === expected_lines[expected_index]) {
        if (expected_index == expected_lines.length - 1) {
          continue;  // Ignores [[*]] if at the end of the expectation
        } else {
          ++expected_index;

          actual_index = __find_line_matching(expected_lines[expected_index], actual_lines, actual_index + expected_index - 1);

          if (actual_index < actual_lines.length) {
            actual_index -= expected_index;
          } else {
            matches = false;
            diff = __diff_with_error(expected_lines, '<yellow><------ INCONSISTENCY</yellow>', expected_index);
            break;
          }
        }
      }

      if ((actual_index + expected_index) >= actual_lines.length) {
        matches = false;
        diff = __diff_with_error(expected_lines, '<yellow><------ MISSING</yellow>', expected_index);
        break;
      }

      if (!__multi_value_compare(expected_lines[expected_index], actual_lines[actual_index + expected_index])) {
        matches = false;
        diff = __diff_with_error(expected_lines, '<yellow><------ INCONSISTENCY</yellow>', expected_index);
        break;
      }
    }
  } else {
    matches = false;
    diff = __diff_with_error(expected_lines, '<yellow><------ INCONSISTENCY</yellow>', 0);
  }

  return {'matches': matches, 'diff': diff};
}

function EXPECT_OUTPUT_CONTAINS_MULTILINE(t) {
  const out = __split_trim_join(testutil.fetchCapturedStdout(false));
  const err = __split_trim_join(testutil.fetchCapturedStderr(false));
  const text = __split_trim_join(t);
  const out_result = __check_multiline_expect(text.array, out.array);
  const err_result = __check_multiline_expect(text.array, err.array);

  if (!out_result.matches && !err_result.matches) {
    const context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text.str + "\n<yellow>Actual stdout:</yellow> " + out.str + "\n<yellow>Actual stderr:</yellow> " + err.str + "\n<yellow>Diff with stdout:</yellow>\n" + out_result.diff + "\n<yellow>Diff with stderr:</yellow>\n" + err_result.diff;
    testutil.fail(context);
  }
}

function EXPECT_STDOUT_CONTAINS_MULTILINE(t) {
  const out = __split_trim_join(testutil.fetchCapturedStdout(false));
  const err = __split_trim_join(testutil.fetchCapturedStderr(false));
  const text = __split_trim_join(t);
  const out_result = __check_multiline_expect(text.array, out.array);

  if (!out_result.matches) {
    const context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text.str + "\n<yellow>Actual stdout:</yellow> " + out.str + "\n<yellow>Actual stderr:</yellow> " + err.str + "\n<yellow>Diff with stdout:</yellow>\n" + out_result.diff;
    testutil.fail(context);
  }
}

function EXPECT_STDERR_CONTAINS_MULTILINE(t) {
  const out = __split_trim_join(testutil.fetchCapturedStdout(false));
  const err = __split_trim_join(testutil.fetchCapturedStderr(false));
  const text = __split_trim_join(t);
  const err_result = __check_multiline_expect(text.array, err.array);

  if (!err_result.matches) {
    const context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text.str + "\n<yellow>Actual stdout:</yellow> " + out.str + "\n<yellow>Actual stderr:</yellow> " + err.str + "\n<yellow>Diff with stderr:</yellow>\n" + err_result.diff;
    testutil.fail(context);
  }
}
