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

function ensure_schema_does_not_exist(session, name){
	try{
		var schema = session.getSchema(name);
		session.dropSchema(name);
	}
	catch(err){
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

function cleanup_sandbox(port) {
    println ('Stopping the sandbox at ' + port + ' to delete it...');
    try {
      stop_options = {}
      stop_options['password'] = 'root';
      if (__sandbox_dir != '')
        stop_options['sandboxDir'] = __sandbox_dir;

      dba.stopSandboxInstance(port, stop_options);
    } catch (err) {
      println(err.message);
    }

    options = {}
    if (__sandbox_dir != '')
      options['sandboxDir'] = __sandbox_dir;

    var deleted = false;

    print('Try deleting sandbox at: ' + port);
    deleted = wait(10, 1, function() {
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
    session.runSql("RESET PERSIST group_replication_start_on_boot");

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
      'SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances where CAST(mysql_server_uuid AS char ascii) = CAST(@@server_uuid AS char ascii);');
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
