# app code, requiring an explicit call to .execute()
db.getCollection('my_coll').add({'name': 'mike'}).execute();

# interactive shell command with direct access
# to collections from the db object
db.my_coll.add({'name': 'mike'});
