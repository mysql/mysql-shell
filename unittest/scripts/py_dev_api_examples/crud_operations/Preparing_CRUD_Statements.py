myColl = db.get_collection('my_collection')

# Only prepare a Collection.remove() operation, but do not run it yet
myRemove = myColl.remove('name = :param1 AND age = :param2')

# Binding parameters to the prepared function and .execute()
myRemove.bind('param1', 'mike').bind('param2', 39).execute()
myRemove.bind('param1', 'johannes').bind('param2', 28).execute()

# Binding works for all CRUD statements but add()
myFind = myColl.find('name like :param1 AND age > :param2')

myDocs = myFind.bind('param1', 'S%').bind('param2', 18).execute()
MyOtherDocs = myFind.bind('param1', 'M%').bind('param2', 24).execute()
