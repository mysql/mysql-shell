function print_result(res) {
  if (res.hasData()) {
    // SELECT
    var columns = res.getColumns();
    var record = res.fetchOne();

    while (record){
      for (index in columns){
        print (columns[index].getColumnName() + ": " + record[index] + "\n");
      }

      // Get the next record
      record = res.fetchOne();
    }

  } else {
    // INSERT, UPDATE, DELETE, ...
    print('Rows affected: ' + res.getAffectedItemsCount());
  }
}


var res = mySession.sql('CALL my_proc()').execute();

// Prints each returned result
var more = true;
while (more){
  print_result(res);

  more = res.nextResult();
}
