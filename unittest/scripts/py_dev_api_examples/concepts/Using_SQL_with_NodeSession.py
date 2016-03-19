import mysqlx

# Connect to server using a NodeSession
mySession = mysqlx.getNodeSession('mike:s3cr3t!@localhost')

# Switch to use schema 'test'
mySession.sql("USE test").execute()

# In a NodeSession context the full SQL language can be used
sql = """CREATE PROCEDURE my_add_one_procedure
                                 (INOUT incr_param INT)
                                 BEGIN 
                                         SET incr_param = incr_param + 1;
                                 END
                        """

mySession.sql(sql).execute()
mySession.sql("SET @my_var = ?").bind(10).execute()
mySession.sql("CALL my_add_one_procedure(@my_var)").execute()
mySession.sql("DROP PROCEDURE my_add_one_procedure").execute()

# Use a SQL query to get the result
myResult = mySession.sql("SELECT @my_var").execute()

# Gets the row and prints the first column
row = myResult.fetchOne()
print row[0]

mySession.close()
