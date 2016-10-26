import mysqlx

# Connect to server
session = mysqlx.get_node_session( {
        'host': 'localhost', 'port': 33060,
        'dbUser': 'mike', 'dbPassword': 's3cr3t!' } )

# Get the Schema test
db = session.get_schema('test')

# Create a new collection
myColl = db.create_collection('my_collection')

# Start a transaction
session.start_transaction()
try:
        myColl.add({'name': 'Jack', 'age': 15, 'height': 1.76, 'weight': 69.4}).execute()
        myColl.add({'name': 'Susanne', 'age': 24, 'height': 1.65}).execute()
        myColl.add({'name': 'Mike', 'age': 39, 'height': 1.9, 'weight': 74.3}).execute()
        
        # Commit the transaction if everything went well
        session.commit()
        
        print 'Data inserted successfully.'
except Exception, err:
        # Rollback the transaction in case of an error
        session.rollback()
        
        # Printing the error message
        print 'Data could not be inserted: %s' % str(err)
