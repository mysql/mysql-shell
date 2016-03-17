
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
        
      # Gets the next record
      record = res.fetchOne()
      
  else:
    #INSERT, UPDATE, DELETE, ...
    print 'Rows affected: %s' % res.getAffectedRowCount()

res = nodeSession.sql('CALL my_proc()').execute()

# Prints each returned result
more = True
while more:
  print_result(res)
  
  more = res.nextDataSet()
