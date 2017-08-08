# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

dba.drop_metadata_schema({'force':True})

#@ Cluster: validating members
if __have_ssl:
  cluster = dba.create_cluster('devCluster', {'memberSslMode': 'REQUIRED'})
else:
  cluster = dba.create_cluster('devCluster', {'memberSslMode': 'DISABLED'})

rset = cluster.get_replica_set()

all_members = dir(rset)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

print "Replica Set Members: %d" % len(members)
validateMember(members, 'name')
validateMember(members, 'get_name')
validateMember(members, 'add_instance')
validateMember(members, 'remove_instance')
validateMember(members, 'help')
validateMember(members, 'rejoin_instance')

#@# Cluster: add_instance errors
rset.add_instance()
rset.add_instance(5,6,7,1)
rset.add_instance(5,5)
rset.add_instance('',5)
rset.add_instance( 5)
rset.add_instance({'host': '127.0.0.1', 'schema': 'abs', 'user': "sample", 'auth-method': 56});
rset.add_instance({'port': __mysql_sandbox_port1});
rset.add_instance({'host': '127.0.0.1', 'port': __mysql_sandbox_port1}, 'root');

# Cleanup
dba.drop_cluster('devCluster', {'dropDefaultReplicaSet': True})
