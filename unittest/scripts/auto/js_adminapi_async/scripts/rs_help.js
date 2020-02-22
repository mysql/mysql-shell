//@ {VER(>=8.0.11)}

// Tests help of ReplicaSet functions.


//@<> Initialization.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

shell.connect(__sandbox_uri1);

//@<> Create Async replicaset.
var rs = dba.createReplicaSet('dev');

session.close();

//@ Object Help
rs.help();

//@ Object Global Help [USE:Object Help]
\? ReplicaSet

//@ Name
rs.help("name");

//@ Name, \? [USE:Name]
\? ReplicaSet.name

//@ Add Instance
rs.help("addInstance");

//@ Add Instance \? [USE:Add Instance]
\? ReplicaSet.addInstance

//@ Disconnect
rs.help("disconnect");

//@ Disconnect \? [USE:Disconnect]
\? ReplicaSet.disconnect

//@ Force Primary Instance
rs.help("forcePrimaryInstance");

//@ Force Primary Instance \? [USE:Force Primary Instance]
\? ReplicaSet.forcePrimaryInstance

//@ Get Name
rs.help("getName");

//@ Get Name \? [USE:Get Name]
\? ReplicaSet.getName

//@ Help
rs.help("help");

//@ Help \? [USE:Help]
\? ReplicaSet.help

//@ Rejoin Instance
rs.help("rejoinInstance");

//@ Rejoin Instance \? [USE:Rejoin Instance]
\? ReplicaSet.rejoinInstance

//@ Remove Instance
rs.help("removeInstance");

//@ Remove Instance \? [USE:Remove Instance]
\? ReplicaSet.removeInstance

//@ Set Primary Instance
rs.help("setPrimaryInstance");

//@ Set Primary Instance \? [USE:Set Primary Instance]
\? ReplicaSet.setPrimaryInstance

//@ setupAdminAccount
rs.help("setupAdminAccount")

//@ setupAdminAccount. \? [USE:setupAdminAccount]
\? ReplicaSet.setupAdminAccount

//@ setupAdminAccount. \help [USE:setupAdminAccount]
\help ReplicaSet.setupAdminAccount

//@ setupRouterAccount
rs.help("setupRouterAccount")

//@ setupRouterAccount. \? [USE:setupRouterAccount]
\? ReplicaSet.setupRouterAccount

//@ Status
rs.help("status");

//@ Status \? [USE:Status]
\? ReplicaSet.status

//@ options
rs.help("options")

//@ options. \? [USE:options]
\? ReplicaSet.options

//@ options. \help [USE:options]
\help ReplicaSet.options

//@ setOption
rs.help("setOption")

//@ setOption. \? [USE:setOption]
\? ReplicaSet.setOption

//@ setOption. \help [USE:setOption]
\help ReplicaSet.setOption

//@ setInstanceOption
rs.help("setInstanceOption")

//@ setInstanceOption. \? [USE:setInstanceOption]
\? ReplicaSet.setInstanceOption

//@ setInstanceOption. \help [USE:setInstanceOption]
\help ReplicaSet.setInstanceOption

//@<> Clean-up.
rs.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
