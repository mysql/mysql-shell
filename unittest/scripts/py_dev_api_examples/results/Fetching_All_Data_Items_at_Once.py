
myResult = myTable.select(['name', 'age']).
  where('name like :name').bind('name','S%').
  execute()
  
myRows = myResult.fetchAll()
