# Get the customers collection as a table
customers = db.get_collection_as_table('customers')
customers.insert('doc').values('{"_id":"001", "name": "mike", "last_name": "connor"}').execute()

# Now do a find operation to retrieve the inserted document
result = customers.select(["doc->'$.name'", "doc->'$.last_name'"]).where("doc->'$._id' = '001'").execute()

record = result.fetch_one()

print "Name : %s\n"  % record[0]
print "Last Name : %s\n"  % record[1]
