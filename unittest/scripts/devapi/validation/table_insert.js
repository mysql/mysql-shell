var tstRes = myTable.select().orderBy(['name']).execute();||
print(tstRes.next().name);|Jack|
print(tstRes.next().name);|Mike|