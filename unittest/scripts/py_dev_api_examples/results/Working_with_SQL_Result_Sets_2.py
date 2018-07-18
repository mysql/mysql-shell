def print_result(res):
  if res.has_data():
    # SELECT
    columns = res.get_columns()
    record = res.fetch_one()
    
    while record:
      index = 0
      
      for column in columns:
        print "%s: %s \n" % (column.get_column_name(), record[index])
        index = index + 1
      
      # Get the next record
      record = res.fetch_one()
  else:
    #INSERT, UPDATE, DELETE, ...
    print 'Rows affected: %s' % res.get_affected_items_count()

print_result(mySession.sql('DELETE FROM users WHERE age > 40').execute())
print_result(mySession.sql('SELECT * FROM users WHERE age = 40').execute())
