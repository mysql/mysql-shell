# Connecting to MySQL and working with a Session
import mysqlx

# Connect to a dedicated MySQL server using a connection URL
mySession = mysqlx.get_node_session('mike:s3cr3t!@localhost')

# Get a list of all available schemas
schemaList = mySession.get_schemas()

print 'Available schemas in this session:\n'

# Loop over all available schemas and print their name
for schema in schemaList:
        print '%s\n' % schema.name

mySession.close()
