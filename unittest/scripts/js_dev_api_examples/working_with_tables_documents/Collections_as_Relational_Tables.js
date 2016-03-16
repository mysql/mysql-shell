
// Get the customers collection as a table
var customers = db.getCollectionAsTable('customers');
customers.insert('doc').values('{"_id":"001", "name": "mike"}').execute();
