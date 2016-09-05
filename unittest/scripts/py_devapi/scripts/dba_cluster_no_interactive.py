# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

dba.drop_metadata_schema({'enforce':True})
clusterPassword = 'testing';

#@ Cluster: validating members
cluster = dba.create_cluster('devCluster', clusterPassword)

all_members = dir(cluster)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Cluster Members: %d" % len(members)
validateMember(members, 'name')
validateMember(members, 'get_name')
validateMember(members, 'admin_type')
validateMember(members, 'get_admin_type')
validateMember(members, 'add_instance')
validateMember(members, 'remove_instance')
validateMember(members, 'get_replica_set')
validateMember(members, 'rejoin_instance');
validateMember(members, 'describe');
validateMember(members, 'status');

#@ Cluster: remove_instance
cluster.remove_instance({'host': '127.0.0.1', 'port': __mysql_sandbox_port1});

#@ Cluster: add_instance
cluster.add_instance({'dbUser': 'root', 'host': '127.0.0.1', 'port': __mysql_sandbox_port1}, 'root')

#@# Cluster: add_instance errors
cluster.add_instance()
cluster.add_instance(5,6,7,1)
cluster.add_instance(5,5)
cluster.add_instance('',5)
cluster.add_instance( 5)
cluster.add_instance({'host': '127.0.0.1', 'schema': 'abs', 'user':"sample", 'authMethod':56})
cluster.add_instance({'port': __mysql_sandbox_port1})
cluster.add_instance({'host': '127.0.0.1', 'port':__mysql_sandbox_port1}, 'root')

# Cleanup
dba.drop_cluster('devCluster', {"dropDefaultReplicaSet": True})
