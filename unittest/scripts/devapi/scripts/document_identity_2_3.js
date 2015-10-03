// Assumptions: test schema assigned to db, my_collection collection exists

// using a books unique ISBN as the object ID
var myColl = db.getCollection('my_collection');

myColl.add( {
        _id: "978-1449374020",
        title: "MySQL Cookbook: Solutions for Database Developers and Administrators"
}).execute();

var book = myColl.find('_id = "978-1449374020"').execute();