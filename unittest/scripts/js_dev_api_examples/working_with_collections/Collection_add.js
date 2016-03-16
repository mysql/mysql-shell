
		    // Create a new collection
		    var myColl = db.createCollection('my_collection');

		    // Insert a document
		    myColl.add( { name: 'Sakila', age: 15 } ).execute();

		    // Insert several documents at once
		    myColl.add( [
		    { name: 'Susanne', age: 24 },
		    { name: 'Mike', age: 39 } ] ).execute();
	    