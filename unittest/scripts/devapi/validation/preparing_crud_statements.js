print(myDocs.next().name)|Sally|
print(MyOtherDocs.next().name)|Molly|
print(myFind.bind('param1','mike').bind('param2', 39).execute().next())|undefined|
print(myFind.bind('param1','johannes').bind('param2', 28).execute().next())|undefined|
