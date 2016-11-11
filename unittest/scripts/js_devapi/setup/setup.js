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
  while(!condition() && waiting < timeout) {
    os.sleep(wait_interval);
    waiting = waiting + 1;
  }
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

var recov_cluster;
var recov_master_uri;
var recov_slave_uri;
function wait_slave_online() {
  var full_status = recov_cluster.status();
  var slave_status = full_status.defaultReplicaSet.topology[recov_master_uri].leaves[recov_slave_uri].status;

  println("--->" + recov_slave_uri + ": " + slave_status);
  return slave_status == "ONLINE";
}

function check_slave_online(cluster, master_uri, slave_uri) {
  recov_cluster = cluster;
  recov_master_uri = master_uri;
  recov_slave_uri = slave_uri;

  wait(60, 1, wait_slave_online);

  recov_cluster = null;
}