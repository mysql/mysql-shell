def print_result(res):
  if res.hasData():
    # SELECT
    columns = res.getColumns()
    record = res.fetchOne()
    
    while record:
      index = 0
      
      for column in columns:
        print "%s: %s \n" % (column.getColumnName(), record[index])
        index = index + 1
        
      # Get the next record
      record = res.fetchOne()
      
  else:
    #INSERT, UPDATE, DELETE, ...
    print 'Rows affected: %s' % res.getAffectedRowCount()
    

print_result(nodeSession.sql('DELETE FROM users WHERE age > 40').execute())
print_result(nodeSession.sql('SELECT * FROM users WHERE age = 40').execute())
