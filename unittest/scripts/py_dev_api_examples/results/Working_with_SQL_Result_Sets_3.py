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
    print 'Rows affected: %s' % res.get_affected_row_count()

res = mySession.sql('CALL my_proc()').execute()

# Prints each returned result
more = True
while more:
  print_result(res)
  
  more = res.next_data_set()
