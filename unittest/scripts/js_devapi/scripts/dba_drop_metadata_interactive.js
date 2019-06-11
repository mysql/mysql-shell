// Assumptions: smart deployment functions available

// Smart deployment
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

shell.connect({scheme:'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});

// Assumptions: reset_or_deploy_sandboxes available
dba.createCluster("tempCluster", {memberSslMode: "REQUIRED", clearReadOnly: true});

//@# Invalid dropMetadataSchema call
dba.dropMetadataSchema(1,2,3,4,5);
dba.dropMetadataSchema({not_valid:true});

//@# drop metadata: no user response
dba.dropMetadataSchema()

//@# drop metadata: user response no
dba.dropMetadataSchema()

//@# drop metadata: expect prompt, user response no
dba.dropMetadataSchema({clearReadOnly:true})

//@# drop metadata: force option
dba.dropMetadataSchema({force:false});
dba.dropMetadataSchema({force:true});

session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});

//@# drop metadata: user response yes
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect({scheme:'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});

dba.createCluster("tempCluster", {memberSslMode: "REQUIRED", clearReadOnly: true});

dba.dropMetadataSchema()

session.close();

// Smart deployment cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
