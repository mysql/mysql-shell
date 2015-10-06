// Assumptions: test schema assigned to db

var myColl = db.getCollection('my_collection');

var res = myColl.find('name like :name').bind('name','S%')
        .execute();

var doc;
while (doc = res.fetchOne()) {
        print(doc);
}
