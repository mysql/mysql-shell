////////////////////////////////////////////////////////////////////////////////

//@<> Setup
const __hostname_ip_uri1 = __hostname_uri1.replace(hostname, hostname_ip);
const __hostname_ip_uri2 = __hostname_uri2.replace(hostname, hostname_ip);
const __hostname_ip_uri3 = __hostname_uri3.replace(hostname, hostname_ip);

const __hostname_ip_x_uri1 = __hostname_ip_uri1.replace('mysql', 'mysqlx').replace(__mysql_sandbox_port1, __mysql_sandbox_x_port1);
const __hostname_ip_x_uri2 = __hostname_ip_uri2.replace('mysql', 'mysqlx').replace(__mysql_sandbox_port2, __mysql_sandbox_x_port2);
const __hostname_ip_x_uri3 = __hostname_ip_uri3.replace('mysql', 'mysqlx').replace(__mysql_sandbox_port3, __mysql_sandbox_x_port3);

const wizards_backup = shell.options.useWizards;

testutil.deploySandbox(__mysql_sandbox_port1, 'root', {'report_host': hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port2, 'root', {'report_host': hostname_ip});
testutil.deploySandbox(__mysql_sandbox_port3, 'root', {'report_host': hostname_ip});

shell.connect(__hostname_ip_uri1);
cluster = dba.createCluster('cluster', {'gtidSetIsComplete': true});
cluster.addInstance(__hostname_ip_uri2);
session.close();

////////////////////////////////////////////////////////////////////////////////
// Call the function shell.connectToPrimary([connectionData, password]) giving a valid connection data of an instance that is secondary member of a cluster. Validate that the the new active session is created on the primary.

var new_session;

//@<> connect using uri
function expect_connect_using_uri() {
  EXPECT_STDOUT_CONTAINS(`Reconnecting to the PRIMARY instance of an InnoDB cluster (${hostname_ip}:${__mysql_sandbox_port1})...`);
  EXPECT_STDOUT_CONTAINS(`Creating a Classic session to 'root@${hostname_ip}:${__mysql_sandbox_port1}'`);
  EXPECT_STDOUT_CONTAINS(`<ClassicSession:root@${hostname_ip}:${__mysql_sandbox_port1}`);
}

new_session = shell.connectToPrimary(__hostname_ip_uri2);
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

//@<> connect using X protocol
new_session = shell.connectToPrimary(__hostname_ip_x_uri2);
EXPECT_EQ(session, new_session);
new_session.close();

EXPECT_STDOUT_CONTAINS(`Reconnecting to the PRIMARY instance of an InnoDB cluster (${hostname_ip}:${__mysql_sandbox_x_port1})...`);
EXPECT_STDOUT_CONTAINS(`Creating an X protocol session to 'root@${hostname_ip}:${__mysql_sandbox_x_port1}'`);
EXPECT_STDOUT_CONTAINS(`<Session:root@${hostname_ip}:${__mysql_sandbox_x_port1}`);

//@<> connect using uri + password
new_session = shell.connectToPrimary(__hostname_ip_uri2.replace(':root', ''), 'root');
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

//@<> connect using uri + password prompt
testutil.expectPassword(`Please provide the password for '`, 'root');
shell.options.useWizards = true;
new_session = shell.connectToPrimary(__hostname_ip_uri2.replace(':root', ''));
shell.options.useWizards = wizards_backup;
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

//@<> connect using uri + overwrite password
new_session = shell.connectToPrimary(__hostname_ip_uri2.replace(':root', ':w00t'), 'root');
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

//@<> connect using dictionary
new_session = shell.connectToPrimary({'scheme': 'mysql', 'user': __user, 'password': 'root', 'host': hostname_ip, 'port': __mysql_sandbox_port2});
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

//@<> connect using dictionary + password
new_session = shell.connectToPrimary({'scheme': 'mysql', 'user': __user, 'host': hostname_ip, 'port': __mysql_sandbox_port2}, 'root');
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

//@<> connect using dictionary + password prompt
testutil.expectPassword(`Please provide the password for '`, 'root');
shell.options.useWizards = true;
new_session = shell.connectToPrimary({'scheme': 'mysql', 'user': __user, 'host': hostname_ip, 'port': __mysql_sandbox_port2});
shell.options.useWizards = wizards_backup;
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

//@<> connect using dictionary + overwrite password
new_session = shell.connectToPrimary({'scheme': 'mysql', 'user': __user, 'password': 'w00t', 'host': hostname_ip, 'port': __mysql_sandbox_port2}, 'root');
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

////////////////////////////////////////////////////////////////////////////////
//@<> connect using wrong password, exect error
EXPECT_THROWS(function() {
  shell.connectToPrimary(__sandbox_uri2, 'w00t');
}, `Shell.connectToPrimary: Access denied for user 'root'@'${__host}' (using password: YES)`);

////////////////////////////////////////////////////////////////////////////////
//@<> connect using no password - wizards disabled, exect error
shell.options.useWizards = false;
EXPECT_THROWS(function() {
  shell.connectToPrimary(__sandbox_uri2.replace(':root', ''));
}, `Shell.connectToPrimary: Access denied for user 'root'@'${__host}' (using password: NO)`);
shell.options.useWizards = wizards_backup;

////////////////////////////////////////////////////////////////////////////////
//@<> while being connected to a primary, try to redirect to primary using connection data pointing to primary, expect no new connection
function expect_no_new_connection() {
  EXPECT_STDOUT_CONTAINS_MULTILINE(`NOTE: Already connected to a PRIMARY.
<ClassicSession:root@${hostname_ip}:${__mysql_sandbox_port1}>`);
}

shell.connect(__hostname_ip_uri1);
new_session = shell.connectToPrimary(__hostname_ip_uri1);
EXPECT_EQ(session, new_session);
new_session.close();

expect_no_new_connection();

////////////////////////////////////////////////////////////////////////////////
//@<> while being connected to a primary, try to redirect to primary using connection data pointing to secondary, expect no new connection
shell.connect(__hostname_ip_uri1);
new_session = shell.connectToPrimary(__hostname_ip_uri2);
EXPECT_EQ(session, new_session);
new_session.close();

expect_no_new_connection();

////////////////////////////////////////////////////////////////////////////////
//@<> having a closed connection to a primary, redirect to primary using connection data pointing to primary, expect new connection
shell.connect(__hostname_ip_uri1);
session.close();
new_session = shell.connectToPrimary(__hostname_ip_uri1);
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

////////////////////////////////////////////////////////////////////////////////
//@<> having a closed connection to a primary, redirect to primary using connection data pointing to secondary, expect new connection
shell.connect(__hostname_ip_uri1);
session.close();
new_session = shell.connectToPrimary(__hostname_ip_uri2);
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

////////////////////////////////////////////////////////////////////////////////
//@<> while being connected to a secondary, redirect to primary using connection data pointing to primary, expect new connection
shell.connect(__hostname_ip_uri2);
new_session = shell.connectToPrimary(__hostname_ip_uri1);
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

////////////////////////////////////////////////////////////////////////////////
//@<> while being connected to a secondary, redirect to primary using connection data pointing to secondary, expect new connection
shell.connect(__hostname_ip_uri2);
new_session = shell.connectToPrimary(__hostname_ip_uri2);
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

////////////////////////////////////////////////////////////////////////////////
// Call the function shell.connectToPrimary([connectionData, password]) giving a valid connection data to an instance that is not member of a cluster. Validate that an exception is thrown because the target instance is not part of a cluster.

//@<> try to connect to an instance that is not a member of a cluster, expect error
function expect_standalone_error(func) {
  EXPECT_THROWS(func, 'Shell.connectToPrimary: Metadata schema of an InnoDB cluster or ReplicaSet not found');
}

expect_standalone_error(function() {
  shell.connectToPrimary(__hostname_ip_uri3);
});

////////////////////////////////////////////////////////////////////////////////
//@<> while being connected to a server, try to redirect using connection data pointing to standalone instance, expect error, make sure existing connection is not affected
shell.connect(__hostname_ip_uri2);
EXPECT_STDOUT_CONTAINS(`<ClassicSession:root@${hostname_ip}:${__mysql_sandbox_port2}>`);
expect_standalone_error(function() {
  shell.connectToPrimary(__hostname_ip_uri3);
});
EXPECT_EQ(`<ClassicSession:root@${hostname_ip}:${__mysql_sandbox_port2}>`, repr(session));
session.close();

////////////////////////////////////////////////////////////////////////////////
// Having a session created on a secondary instance of a cluster, call the function shell.connectToPrimary() giving no arguments. Validate that the the new active session is created on the primary instance of the cluster.

//@<> connect to primary while being connected to a secondary, expect no error
shell.connect(__hostname_ip_uri2);
new_session = shell.connectToPrimary();
EXPECT_EQ(session, new_session);
new_session.close();

expect_connect_using_uri();

////////////////////////////////////////////////////////////////////////////////
//@<> while being connected to a primary, try to redirect to primary, expect no new connection
shell.connect(__hostname_ip_uri1);
new_session = shell.connectToPrimary();
EXPECT_EQ(session, new_session);
new_session.close();

expect_no_new_connection();

////////////////////////////////////////////////////////////////////////////////
// Having a session created on a standalone instance, call the function shell.connectToPrimary() giving no arguments. Validate that an exception is thrown because the target instance is not part of a cluster.

//@<> while being connected to an instance that is not a member of a cluster, try to connect to primary, exect error
shell.connect(__hostname_ip_uri3);
expect_standalone_error(function() {
  shell.connectToPrimary();
});
session.close();

////////////////////////////////////////////////////////////////////////////////
// Call the function shell.connectToPrimary() giving no arguments with no active session. Validate that an exception is thrown because there is no active session.

//@<> call shell.connectToPrimary() with no arguments and no active session, expect error
EXPECT_THROWS(function () {
  shell.connectToPrimary();
}, 'Shell.connectToPrimary: Redirecting to a PRIMARY requires an active session.');

////////////////////////////////////////////////////////////////////////////////

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
