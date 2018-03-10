
// Create a new collection 'my_collection'
var myColl = db.createCollection('my_collection');

// Insert JSON data directly
myColl.add({_id: '8901', name: 'Sakila', age: 15});

// Inserting several docs at once
myColl.add([ {_id: '8902', name: 'Susanne', age: 24},
  {_id: '8903', name: 'Mike', age: 39} ]);
