from mysqlsh import mysqlx

# Connect to server
mySession = mysqlx.get_session( {
        'host': 'localhost', 'port': 33060,
        'dbUser': 'mike', 'dbPassword': 's3cr3t!' } )

# Get the Schema test
myDb = mySession.get_schema('test')

# Create a new collection
myColl = myDb.create_collection('my_collection')

# Start a transaction
mySession.start_transaction()
try:
    myColl.add({'name': 'Jack', 'age': 15, 'height': 1.76, 'weight': 69.4}).execute()
    myColl.add({'name': 'Susanne', 'age': 24, 'height': 1.65}).execute()
    myColl.add({'name': 'Mike', 'age': 39, 'height': 1.9, 'weight': 74.3}).execute()
    
    # Commit the transaction if everything went well
    reply = mySession.commit()
    
    # handle warnings
    if reply.warning_count:
      for warning in result.get_warnings():
        print 'Type [%s] (Code %s): %s\n' % (warning.level, warning.code, warning.message)
    
    print 'Data inserted successfully.'
except Exception, err:
    # Rollback the transaction in case of an error
    reply = mySession.rollback()
    
    # handle warnings
    if reply.warning_count:
      for warning in result.get_warnings():
        print 'Type [%s] (Code %s): %s\n' % (warning.level, warning.code, warning.message)
    
    # Printing the error message
    print 'Data could not be inserted: %s' % str(err)
