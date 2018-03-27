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

    if (actual.length == 0){
        print("No additional functions are available\n")
    }
    else{
        print("Extra Functions:", actual);
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

function validateMember(memberList, member){
	if (memberList.indexOf(member) != -1){
		print(member + ": OK\n");
	}
	else{
		print(member + ": Missing\n");
	}
}

function validateNotMember(memberList, member){
	if (memberList.indexOf(member) != -1){
		print(member + ": Unexpected\n");
	}
	else{
		print(member + ": OK\n");
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

// Check if the instance (sandbox) on the given port has SSL enabled and update
// the global SSL values used by the test scripts accordingly.
function update_have_ssl(port) {
    // Check the instance SSL support
    var res = false;
    var connected = connect_to_sandbox([port]);
    if (connected) {
        var result = session.runSql("SELECT @@GLOBAL.have_ssl;");
        var row = result.fetchOne();
        var value = row[0];
        if (value == 'YES') {
            res = true;
        }
        session.close();
    }
    println("Value of 'have_ssl' for instance on " + port + ": " + value);
    // Update SSL global values used by test scripts
    __have_ssl = res;
    if (__have_ssl) {
        add_instance_extra_opts = {memberSslMode: 'REQUIRED'};
        __ssl_mode = 'REQUIRED';
    } else {
        add_instance_extra_opts = {memberSslMode: 'DISABLED'};
        __ssl_mode = 'DISABLED';
    }
}


function set_sysvar(session, variable, value) {
    session.runSql("SET GLOBAL "+variable+" = ?", [value]);
}

function get_sysvar(session, variable) {
    return session.runSql("SHOW GLOBAL VARIABLES LIKE ?", [variable]).fetchOne()[1];
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


function ensure_plugin_enabled(plugin_name) {
  // For function signature simplicity I use `plugin_name` also as shared library name.
  var sess = mysql.getClassicSession(__uripwd);
  var rs = sess.runSql("SELECT COUNT(1) FROM INFORMATION_SCHEMA.PLUGINS WHERE PLUGIN_NAME like '" + plugin_name + "';");
  var is_installed = rs.fetchOne()[0];

  if (!is_installed) {
    var os = sess.runSql('select @@version_compile_os').fetchOne()[0];
    if (os == "Win32" || os == "Win64") {
      sess.runSql("INSTALL PLUGIN " + plugin_name + " SONAME '" + plugin_name + ".dll';");
    }
    else {
      sess.runSql("INSTALL PLUGIN " + plugin_name + " SONAME '" + plugin_name + ".so';");
    }
  }
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
