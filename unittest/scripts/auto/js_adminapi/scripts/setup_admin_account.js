function get_user_auth_string(session, user_name, host_name) {
    var result = session.runSql(
        "SELECT authentication_string FROM mysql.user where USER='" + user_name.toString() +
        "' and host='" + host_name.toString() + "'");
    var row = result.fetchOne();
    if (row == null) {
        testutil.fail("Query to get authentication_string returned no results");
    }
    return row[0];
}

function count_users_like(session, user_name, host_name) {
    var result = session.runSql(
        "SELECT COUNT(*) FROM mysql.user where USER='" + user_name.toString() +
        "' and host='" + host_name.toString() + "'");
    var row = result.fetchOne();
    if (row == null) {
        testutil.fail("Query to get get number of users returned no results");
    }
    return row[0];
}

function count_schema_privs(session, user_name, host_name) {
    var result = session.runSql(
        "SELECT COUNT(*) FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES WHERE GRANTEE = \"'" +
        user_name.toString() + "'@'" + host_name.toString() + "'\"");
    var row = result.fetchOne();
    if (row == null) {
        testutil.fail("Query to get get number of user schema privileges returned no results")
    }
    return row[0];
}

//@<> WL#13536 Deploy instances.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

session1 = mysql.getSession(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

//@<> WL#13536 create cluster
shell.connect(__sandbox_uri1);
var c = dba.createCluster("Cluster", {gtidSetIsComplete: true});
c.addInstance(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@ WL#13536 TSFR3_1 An error is thrown if a non supported format is passed to the user parameter
c.setupAdminAccount("'foo");

//@ WL#13536 BUG#30645140 An error is thrown if the username contains the @ symbol
c.setupAdminAccount("foo@bar@baz");

//@ WL#13536 BUG#30645140 but no error is thrown if the @symbol on the username is surrounded by quotes
c.setupAdminAccount("'foo@bar'@baz", {password: "foo"});

//@ WL#13536 BUG#30648813 Empty usernames are not supported
c.setupAdminAccount(" ");
c.setupAdminAccount("");
c.setupAdminAccount(" @");
c.setupAdminAccount("@");
c.setupAdminAccount(" @%");
c.setupAdminAccount("@%");

// BUG#31491092 reports that when the validate_password plugin is installed, setupAdminAccount() fails
// with an error indicating the password does not match the current policy requirements. This happened
// because the function was creating the account with 2 separate transactions: one to create the user
// without any password, and another to change the password of the created user

//@<> Install the validate_password plugin to verify the fix for BUG#31491092
ensure_plugin_enabled("validate_password", session1, "validate_password");
// configure the validate_password plugin for the lowest policy
session1.runSql('SET GLOBAL validate_password_policy=\'LOW\'');
session1.runSql('SET GLOBAL validate_password_length=1');

//@<> With validate_password plugin enabled, an error must be thrown when the password does not satisfy the requirements
EXPECT_THROWS_TYPE(function(){c.setupAdminAccount("%", {password: "foo"})}, __endpoint1 + ": Your password does not satisfy the current policy requirements", "MYSQLSH");

//@<> WL#13536 TSFR3_2 Host if not specified defaults to %
// account didnt't exist on the cluster
EXPECT_EQ(0, count_users_like(session1, "default_hostname", "%"));
EXPECT_EQ(0, count_users_like(session2, "default_hostname", "%"));
c.setupAdminAccount("default_hostname", {password: "fooo"});
EXPECT_EQ(1, count_users_like(session1, "default_hostname", "%"));
// account was replicated across the cluster
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
EXPECT_EQ(1, count_users_like(session2, "default_hostname", "%"));

// Uninstall the validate_password plugin: negative and positive tests done
ensure_plugin_disabled("validate_password", session1, "validate_password");

//@<OUT> WL#13536 TSFR3_2 check global privileges of created user
session1.runSql("SELECT PRIVILEGE_TYPE, IS_GRANTABLE FROM INFORMATION_SCHEMA.USER_PRIVILEGES WHERE GRANTEE = \"'default_hostname'@'%'\" ORDER BY PRIVILEGE_TYPE");

//@<OUT> WL#13536 TSFR3_2 check schema privileges of created user
session.runSql("SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES WHERE GRANTEE = \"'default_hostname'@'%'\" ORDER BY TABLE_SCHEMA, PRIVILEGE_TYPE");

//@<OUT> WL#13536 TSFR3_2 check table privileges of created user
session.runSql("SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES WHERE GRANTEE = \"'default_hostname'@'%'\" ORDER BY TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE");

//@<> WL#13536 TSFR3_3 Host specified works as expected
EXPECT_EQ(0, count_users_like(session1, "specific_host", "198.51.100.0/255.255.255.0"));
EXPECT_EQ(0, count_users_like(session2, "specific_host", "198.51.100.0/255.255.255.0"));
c.setupAdminAccount("specific_host@198.51.100.0/255.255.255.0", {password: "fooo"});
EXPECT_EQ(1, count_users_like(session1, "specific_host", "198.51.100.0/255.255.255.0"));
// account was replicated across the cluster
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
EXPECT_EQ(1, count_users_like(session2, "specific_host", "198.51.100.0/255.255.255.0"));

//@ WL#13536 TSFR3_4 An error is thrown if user exists but update option is false
c.setupAdminAccount("specific_host@198.51.100.0/255.255.255.0", {password: "fooo", update:false});

//@ WL#13536 TSFR3_4 An error is thrown if user exists but update option is not specified
c.setupAdminAccount("specific_host@198.51.100.0/255.255.255.0", {password: "fooo"});

//@<> WL#13536 TSFR3_5 Operation updates existing account privileges if update option is used
var admin_schema_privs_num = count_schema_privs(session1, "specific_host", "198.51.100.0/255.255.255.0");
var old_auth_string = get_user_auth_string(session1, "specific_host", "198.51.100.0/255.255.255.0");

// Drop one of the required privileges from the account we just created
session1.runSql("REVOKE CREATE on mysql_innodb_cluster_metadata.* FROM 'specific_host'@'198.51.100.0/255.255.255.0'");
EXPECT_EQ(admin_schema_privs_num - 1, count_schema_privs(session1, "specific_host", "198.51.100.0/255.255.255.0"));

// privilege was also dropped on secondary
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
EXPECT_EQ(admin_schema_privs_num - 1, count_schema_privs(session2, "specific_host", "198.51.100.0/255.255.255.0"));

// The operation should add missing privileges to existing accounts if update option is used
c.setupAdminAccount("specific_host@198.51.100.0/255.255.255.0", {update:true});
EXPECT_EQ(admin_schema_privs_num, count_schema_privs(session1, "specific_host", "198.51.100.0/255.255.255.0"));

// privilege was also restored on secondary
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
EXPECT_EQ(admin_schema_privs_num, count_schema_privs(session2, "specific_host", "198.51.100.0/255.255.255.0"));

// password was not changed
EXPECT_EQ(old_auth_string, get_user_auth_string(session1, "specific_host", "198.51.100.0/255.255.255.0"));
EXPECT_EQ(old_auth_string, get_user_auth_string(session2, "specific_host", "198.51.100.0/255.255.255.0"));

//@<> WL#13536 TSFR3_6 Operation updates existing account privileges and password if update option is used
var admin_schema_privs_num = count_schema_privs(session1, "specific_host","198.51.100.0/255.255.255.0");
var old_auth_string = get_user_auth_string(session1, "specific_host", "198.51.100.0/255.255.255.0");

// Drop one of the required privileges from the account we just created
session1.runSql("REVOKE CREATE on mysql_innodb_cluster_metadata.* FROM 'specific_host'@'198.51.100.0/255.255.255.0'");
EXPECT_EQ(admin_schema_privs_num - 1, count_schema_privs(session1, "specific_host", "198.51.100.0/255.255.255.0"));

// privilege was also dropped on secondary
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
EXPECT_EQ(admin_schema_privs_num - 1, count_schema_privs(session2, "specific_host", "198.51.100.0/255.255.255.0"));

// The operation should add missing privileges to existing accounts if update option is used
c.setupAdminAccount("specific_host@198.51.100.0/255.255.255.0", {password: "foo2", update:true});
EXPECT_EQ(admin_schema_privs_num, count_schema_privs(session1, "specific_host", "198.51.100.0/255.255.255.0"));

// privilege was also restored on secondary
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
EXPECT_EQ(admin_schema_privs_num, count_schema_privs(session2, "specific_host", "198.51.100.0/255.255.255.0"));

// password was updated on the whole cluster
EXPECT_NE(old_auth_string, get_user_auth_string(session1, "specific_host", "198.51.100.0/255.255.255.0"));
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
EXPECT_EQ(
    get_user_auth_string(session1, "specific_host", "198.51.100.0/255.255.255.0"),
    get_user_auth_string(session2, "specific_host", "198.51.100.0/255.255.255.0"));

//@<> WL#13536 TSFR4_1 Validate the password of the created user is the one specified
c.setupAdminAccount("password_test@%", {password: "1234"});
session.close();
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'password_test', password: '1234'});
session.close();
shell.connect(__sandbox_uri1);

//@<> WL#13536 TSFR5_1 Validate upon creating a new account with dryRun the list of privileges is shown but the account is not created
EXPECT_EQ(0, count_users_like(session1, "dryruntest", "%"));
c.setupAdminAccount("dryruntest", {password: "fooo", dryRun:true});
// account not created
EXPECT_EQ(0, count_users_like(session1, "dryruntest", "%"));

//@WL#13536 TSFR5_2 Validate that trying to upgrade a non existing account fails even with dryRun enabled
c.setupAdminAccount("dryruntest", {update: true, dryRun:true});

//@<> WL#13536 TSFR5_3 Validate upon updating an existing account with dryRun the list of privileges is shown but none is restored
// create account
c.setupAdminAccount("dryruntest", {password: "fooo"});
var all_schema_privs_num = count_schema_privs(session1, "dryruntest", "%");
session1.runSql("REVOKE EXECUTE on mysql_innodb_cluster_metadata.* FROM 'dryruntest'@'%'");
// 1 schema privilege was revoked
EXPECT_EQ(all_schema_privs_num - 1 , count_schema_privs(session1, "dryruntest", "%"));

c.setupAdminAccount("dryruntest", {update: true, dryRun:true});
// after operation the grant was not restored
EXPECT_EQ(all_schema_privs_num - 1 , count_schema_privs(session1, "dryruntest", "%"));

//@ WL#13536 TSFR5_4 Validate that upgrading an existing account fails if upgrade is false even with dryRun enabled
c.setupAdminAccount("dryruntest", {password: "fooo", dryRun:true});

//@<> WL#13536 TSFR6_3 Validate password is asked for creation of a new account if interactive mode enabled
testutil.expectPassword("Password for new account: ", "1111");
testutil.expectPassword("Confirm password: ", "1111");

shell.options.useWizards=1;
c.setupAdminAccount("interactive_test@%");
session.close();
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'interactive_test', password: '1111'});
session.close();
shell.connect(__sandbox_uri1);

//@<> WL#13536 TSFR6_4 Creating new account fails if password not provided and interactive mode is disabled
shell.options.useWizards=0;
c.setupAdminAccount("interactive_test_2@%");
EXPECT_EQ(0, count_users_like(session1, "interactive_test_2", "%"));

//@<> WL#13536 TSET_6 Validate operation fails if user doesn't have enough privileges to create/upgrade account
// Use one of the users we just created and drop one of its privileges
session.close();
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'interactive_test', password: '1111'});
session.runSql("REVOKE INSERT on mysql_innodb_cluster_metadata.* FROM 'interactive_test'@'%'");
c = dba.getCluster();
c.setupAdminAccount("missing_privs@%", {password: "plusultra"});
// User was not created
EXPECT_EQ(0, count_users_like(session1, "missing_privs", "%"));

//@<> WL#13536 simulate shell upgrade use case where we want update adminAccount privileges to upgrade metadata
// Added because of BUG#30612423
session.close();
shell.connect(__sandbox_uri3);
dba.configureInstance(__sandbox_uri3 ,{clusterAdmin: "old_shell", clusterAdminPassword: "123"});
// Create cluster using root account
var c = dba.createCluster("Cluster", {gtidSetIsComplete: true});
// DROP metadata privileges that were only added on this WL to simulate an old clusterAdminAccount
session.runSql("REVOKE ALL on mysql_innodb_cluster_metadata_bkp.* FROM 'old_shell'@'%'");
session.runSql("REVOKE ALL on mysql_innodb_cluster_metadata_previous.* FROM 'old_shell'@'%'");
c.disconnect();
// Get new cluster object using the old_shell account
shell.connect({host: localhost, port: __mysql_sandbox_port3, user: 'old_shell', password: '123'});
c = dba.getCluster();

// Validate that setupAdminAccount cannot be executed because of missing privileges
c.setupAdminAccount("missing_privs@%", {password: "plusultra"});

//@<> WL#15438 - requireCertIssuer and requireCertSubject
shell.connect(__sandbox_uri1);
c=dba.getCluster();

EXPECT_THROWS(function(){c.setupAdminAccount("cert@%", {requireCertIssuer:124})}, "Argument #2: Option 'requireCertIssuer' is expected to be of type String, but is Integer");
EXPECT_THROWS(function(){c.setupAdminAccount("cert@%", {requireCertSubject:124})}, "Argument #2: Option 'requireCertSubject' is expected to be of type String, but is Integer");
EXPECT_THROWS(function(){c.setupAdminAccount("cert@%", {requireCertIssuer:null})}, "Argument #2: Option 'requireCertIssuer' is expected to be of type String, but is Null");
EXPECT_THROWS(function(){c.setupAdminAccount("cert@%", {requireCertSubject:null})}, "Argument #2: Option 'requireCertSubject' is expected to be of type String, but is Null");

EXPECT_EQ(session.runSql("select * from mysql.user where user='cert'").fetchOne(), null);

// create with password
c.setupAdminAccount("cert1@%", {requireCertIssuer:"/CN=cert1issuer", requireCertSubject:"/CN=cert1subject", password:"pwd"});
user = session.runSql("select convert(x509_issuer using ascii), convert(x509_subject using ascii), authentication_string from mysql.user where user='cert1'").fetchOne();
EXPECT_EQ(user[0], "/CN=cert1issuer");
EXPECT_EQ(user[1], "/CN=cert1subject");
EXPECT_NE(user[2], "");

// create without password
c.setupAdminAccount("cert2@%", {requireCertIssuer:"/CN=cert2issuer", requireCertSubject:"/CN=cert2subject", password:""});

user = session.runSql("select convert(x509_issuer using ascii), convert(x509_subject using ascii), authentication_string from mysql.user where user='cert2'").fetchOne();

EXPECT_EQ(user[0], "/CN=cert2issuer");
EXPECT_EQ(user[1], "/CN=cert2subject");
EXPECT_EQ(user[2], "");

// just issuer
c.setupAdminAccount("cert3@%", {requireCertIssuer:"/CN=cert3issuer", password:""});

user = session.runSql("select convert(x509_issuer using ascii), convert(x509_subject using ascii), authentication_string from mysql.user where user='cert3'").fetchOne();

EXPECT_EQ(user[0], "/CN=cert3issuer");
EXPECT_EQ(user[1], "");
EXPECT_EQ(user[2], "");

// just subject
c.setupAdminAccount("cert4@%", {requireCertSubject:"/CN=cert4subject", password:""});

user = session.runSql("select convert(x509_issuer using ascii), convert(x509_subject using ascii), authentication_string from mysql.user where user='cert4'").fetchOne();

EXPECT_EQ(user[0], "");
EXPECT_EQ(user[1], "/CN=cert4subject");
EXPECT_EQ(user[2], "");

// "upgrade" account
c.setupAdminAccount("cert5@%", {password:"hello"});
user = session.runSql("select convert(x509_issuer using ascii), convert(x509_subject using ascii), authentication_string, password_lifetime from mysql.user where user='cert5'").fetchOne();
EXPECT_EQ(user[0], "");
EXPECT_EQ(user[1], "");
EXPECT_NE(user[2], "");
EXPECT_EQ(user[3], null);

c.setupAdminAccount("cert5@%", {requireCertSubject:"/CN=cert5subject", passwordExpiration:42, password:"", update:true});

user = session.runSql("select convert(x509_issuer using ascii), convert(x509_subject using ascii), authentication_string, password_lifetime from mysql.user where user='cert5'").fetchOne();
EXPECT_EQ(user[0], "");
EXPECT_EQ(user[1], "/CN=cert5subject");
EXPECT_EQ(user[2], "");
EXPECT_EQ(user[3], 42);

// reset pwd only
c.setupAdminAccount("cert5@%", {password:"abc", update:true});
user = session.runSql("select convert(x509_issuer using ascii), convert(x509_subject using ascii), authentication_string, password_lifetime from mysql.user where user='cert5'").fetchOne();
EXPECT_EQ(user[0], "");
EXPECT_EQ(user[1], "/CN=cert5subject");
EXPECT_NE(user[2], "");
EXPECT_EQ(user[3], 42);

// remove all cert options but leave pwd
c.setupAdminAccount("cert5@%", {requireCertSubject:"", update:true});
user = session.runSql("select convert(x509_issuer using ascii), convert(x509_subject using ascii), authentication_string, password_lifetime from mysql.user where user='cert5'").fetchOne();
EXPECT_EQ(user[0], "");
EXPECT_EQ(user[1], "");
EXPECT_NE(user[2], "");
EXPECT_EQ(user[3], 42);


//@<> WL#15438 - passwordExpiration
EXPECT_THROWS(function(){c.setupAdminAccount("test1@%", {passwordExpiration: "bla", password:""});}, "Argument #2: Option 'passwordExpiration' UInteger, 'NEVER' or 'DEFAULT' expected, but value is 'bla'");
EXPECT_THROWS(function(){c.setupAdminAccount("test1@%", {passwordExpiration: -1, password:""});}, "Argument #2: Option 'passwordExpiration' UInteger, 'NEVER' or 'DEFAULT' expected, but value is '-1'");
EXPECT_THROWS(function(){c.setupAdminAccount("test1@%", {passwordExpiration: 0, password:""});}, "Argument #2: Option 'passwordExpiration' UInteger, 'NEVER' or 'DEFAULT' expected, but value is '0'");
EXPECT_THROWS(function(){c.setupAdminAccount("test1@%", {passwordExpiration: 1.45, password:""});}, "Argument #2: Option 'passwordExpiration' UInteger, 'NEVER' or 'DEFAULT' expected, but value is Float");
EXPECT_THROWS(function(){c.setupAdminAccount("test1@%", {passwordExpiration: {}, password:""});}, "Argument #2: Option 'passwordExpiration' UInteger, 'NEVER' or 'DEFAULT' expected, but value is Map");
EXPECT_EQ(session.runSql("select * from mysql.user where user='test1'").fetchOne(), null);

function CHECK_LIFETIME(user, lifetime) {
    row = session.runSql("select password_lifetime from mysql.user where user=?", [user]).fetchOne();
    EXPECT_EQ(row[0], lifetime);
}

c.setupAdminAccount("expire1@%", {passwordExpiration:"never", password:""});
CHECK_LIFETIME("expire1", 0);
c.setupAdminAccount("expire2@%", {passwordExpiration:"default", password:""});
CHECK_LIFETIME("expire2", null);
c.setupAdminAccount("expire3@%", {passwordExpiration:null, password:""});
CHECK_LIFETIME("expire3", null);
c.setupAdminAccount("expire4@%", {passwordExpiration:42, password:""});
CHECK_LIFETIME("expire4", 42);
c.setupAdminAccount("expire5@%", {passwordExpiration:43, password:""});
CHECK_LIFETIME("expire5", 43);

//@<> WL#13536: Cleanup
c.disconnect();
session.close();
session1.close();
session2.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
