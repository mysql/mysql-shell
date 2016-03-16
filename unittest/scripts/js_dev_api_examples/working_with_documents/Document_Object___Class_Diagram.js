
// Create a new collection 'my_collection'
var myColl = db.createCollection('my_collection');

// Insert JSON data directly
myColl.add({name: 'Sakila', age: 15});

// Inserting several docs at once
myColl.add([ {name: 'Susanne', age: 24},
  {name: 'Mike', age: 39} ]);
