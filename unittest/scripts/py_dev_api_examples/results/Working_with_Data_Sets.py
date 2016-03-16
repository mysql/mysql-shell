
myColl = db.getCollection('my_collection')

res = myColl.find('name like :name').bind('name','S%').execute()

doc = res.fetchOne()
while doc:
        print doc
        doc = res.fetchOne()
