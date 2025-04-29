//@ {DEF(MYSQLD_SECONDARY_SERVER_A)}

//@<> Testing Sandbox Deployment
let paths = [MYSQLD_SECONDARY_SERVER_A.path]
paths.push(os.path.dirname(paths[0]));

if (paths[1].endsWith('bin')) {
    paths.push(os.path.dirname(paths[1]));
}

for(index in paths) {
    let path = paths[index];

    print(`Testing: ${path}`)

    dba.deploySandboxInstance(__mysql_sandbox_port1,  { password: 'root', mysqldPath: path });
    EXPECT_NO_THROWS(function() {shell.connect(__sandbox_uri1)})

    //@<> Stop
    EXPECT_NO_THROWS(function() { dba.stopSandboxInstance(__mysql_sandbox_port1, {password: 'root'}) });

    //@<> Start
    EXPECT_NO_THROWS(function() { dba.startSandboxInstance(__mysql_sandbox_port1) });
    EXPECT_NO_THROWS(function() {shell.connect(__sandbox_uri1)})
    EXPECT_NO_THROWS(function() {shell.disconnect()})

    //@<> Kill
    EXPECT_NO_THROWS(function() { dba.killSandboxInstance(__mysql_sandbox_port1) });

    //@<> Delete
    EXPECT_NO_THROWS(function() { dba.deleteSandboxInstance(__mysql_sandbox_port1) });
}

