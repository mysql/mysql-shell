
function print_result(res) {
  if (res.hasData()) {
    // SELECT
    var columns = res.getColumns();
    var record = res.fetchOne();
    
    while (record){
      for (index in columns){
        print (columns[index].getColumnName() + ": " + record[index] + "\n");
      }
      
      // Gets the next record
      record = res.fetchOne();
    }
    
  } else {
    // INSERT, UPDATE, DELETE, ...
    print('Rows affected: ' + res.getAffectedRowCount());
  }
}

print_result(nodeSession.sql('DELETE FROM users WHERE age > 40').execute());
print_result(nodeSession.sql('SELECT * FROM users WHERE age = 40').execute());
