# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

dba.drop_metadata_schema({'enforce':True})
clusterPassword = 'testing'
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
validateMember(members, 'add_seed_instance')
validateMember(members, 'add_instance')
validateMember(members, 'remove_instance')
validateMember(members, 'get_replica_set')

#@ Cluster: add_seed_instance
# Added this to enable add_instance, full testing of add_seed_instance is needed
cluster.add_seed_instance(clusterPassword, {'host': __host, 'port':__mysql_port}, __pwd)

#@# Cluster: add_instance errors
cluster.add_instance()
cluster.add_instance(5,6,7,1)
cluster.add_instance(5,5)
cluster.add_instance('',5)
cluster.add_instance(clusterPassword, 5)
cluster.add_instance(clusterPassword, {'host': __host, 'schema': 'abs', 'user':"sample", 'authMethod':56})
cluster.add_instance(clusterPassword, {"port": __port})
cluster.add_instance(clusterPassword, {'host': __host, 'port':__mysql_port}, __pwd)

# Cleanup
dba.drop_cluster('devCluster', {"dropDefaultReplicaSet": True})
