tstRes = myColl.find().sort(['name']).execute()||
print tstRes.fetchOne().name|Mike|
print tstRes.fetchOne().name|Sakila|
print tstRes.fetchOne().name|Susanne|
session.dropCollection('test','my_collection')||