
// Collection.find() function with fixed values
var myColl = db.getCollection('my_collection');

var myRes1 = myColl.find('age = 15').execute();

// Using the .bind() function to bind parameters
var myRes2 = myColl.find('name = :param1 AND age = :param2').bind('param1','jack').bind('param2', 17).execute();

// Using named parameters
myColl.modify('name = :param').set('age', 37).
        bind('param', 'clare').execute();

// Binding works for all CRUD statements except add()
var myRes3 = myColl.find('name like :param').
        bind('param', 'J%').execute();
