
// Gets the collection
var myColl = db.getCollection('my_collection');

// Insert a document
var res = myColl.add({_id: '002', name: 'Jack', age: 15, height: 1.76 }).add({_id: '003', name: 'Jim', age: 68, height: 1.72 }).execute();

//@ Print the last document id {VER(>=8.0.5)}
print("ID for Jim's Document:", res.getGeneratedIds()[1]);
