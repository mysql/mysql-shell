
// Gets the collection
var myColl = db.getCollection('my_collection');

// Insert a document
var res = myColl.add({_id: '00001', name: 'Jack', age: 15, height: 1.76 }).execute();

//@ Print the documentId that was assigned to the document {VER(>=8.0.5)}
print('Document Id:', res.getGeneratedIds()[0]);
