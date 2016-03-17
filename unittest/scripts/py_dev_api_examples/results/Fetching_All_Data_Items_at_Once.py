
myResult = myTable.select(['name', 'age']) \
  .where('name like :name').bind('name','S%') \
  .execute()
  
myRows = myResult.fetchAll()

for row in myRows:
  print "%s is %s years old." % (row.name, row.age)
