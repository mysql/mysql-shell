
// Get the customers collection as a table
var customers = db.getCollectionAsTable('customers');
customers.insert('doc').values('{"_id":"001", "name": "mike", "last_name": "connor"}').execute();

// Now do a find operation to retrieve the inserted document
var result = customers.select(["doc->'$.name'", "doc->'$.last_name'"]).where("doc->'$._id' = '001'").execute();

var record = result.fetchOne();

print ("Name : "  + record[0]);
print ("Last Name : "  + record[1]);
