
var myResult = myTable.select(['name', 'age']).
  where('name like :name').bind('name','S%').
  execute();

var myRows = myResult.fetchAll();