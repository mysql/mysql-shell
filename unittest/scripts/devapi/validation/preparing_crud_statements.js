print(myDocs.fetchOne().name)|Sally|
print(MyOtherDocs.fetchOne().name)|Molly|
print(myFind.bind('param1','mike').bind('param2', 39).execute().fetchOne())|undefined|
print(myFind.bind('param1','johannes').bind('param2', 28).execute().fetchOne())|undefined|
