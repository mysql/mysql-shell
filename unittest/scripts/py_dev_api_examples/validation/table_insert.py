tstRes = myTable.select().orderBy(['name']).execute()||
print tstRes.fetchOne().name|Jack|
print tstRes.fetchOne().name|Mike|