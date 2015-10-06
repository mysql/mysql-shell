// Assumptions: test schema assigned to db, my_collection collection exists

var myColl = db.getCollection('my_collection');

// Only prepare a Collection.remove() operation, but don't run it yet
var myRemove = myColl.remove('name = :param1 AND age = :param2');

// Binding parameters to the prepared function and .execute()
myRemove.bind('param1', 'mike').bind('param2', 39).execute();
myRemove.bind('param1', 'johannes').bind('param2', 28).execute();

// Binding works for all CRUD statements but add()
var myFind = myColl.find('name like :param1 AND age > :param2');

var myDocs = myFind.bind('param1', 'S%').bind('param2', 18).execute();
var MyOtherDocs = myFind.bind('param1', 'M%').bind('param2', 24).execute();