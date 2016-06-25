		    // Use the collection 'my_collection'
		    var myColl = db.getCollection('my_collection');

		    // Find a single document that has a field 'name' starts with an 'S'
		    var docs = myColl.find('name like :param').
		               limit(1).bind('param', 'S%').execute();

		    print(docs.fetchOne());

		    // Get all documents with a field 'name' that starts with an 'S'
		    docs = myColl.find('name like :param').
		            bind('param','S%').execute();

		    var myDoc;
		    while (myDoc = docs.fetchOne()) {
		    print(myDoc);
		    }
	    