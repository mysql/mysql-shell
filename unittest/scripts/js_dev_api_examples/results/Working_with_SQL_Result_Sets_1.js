
var res = mySession.sql('CALL my_proc()').execute();

if (res.hasData()){

  var row = res.fetchOne();
  if (row){
    print('List of row available for fetching.');
    do {
      print(row);
    } while (row = res.fetchOne());
  }
  else{
    print('Empty list of rows.');
  }
}
else {
  print('No row result.');
}
