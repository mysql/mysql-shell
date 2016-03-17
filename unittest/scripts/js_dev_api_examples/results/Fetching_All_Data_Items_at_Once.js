
var myResult = myTable.select(['name', 'age']).
  where('name like :name').bind('name','S%').
  execute();

var myRows = myResult.fetchAll();

for (index in myRows){
  print (myRows[index].name + " is " + myRows[index].age + " years old.");
}
