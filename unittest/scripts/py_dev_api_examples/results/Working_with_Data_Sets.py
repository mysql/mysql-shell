myColl = db.get_collection('my_collection')

res = myColl.find('name like :name').bind('name','S%').execute()

doc = res.fetch_one()
while doc:
        print(doc)
        doc = res.fetch_one()
