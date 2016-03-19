# one bind() per parameter
myColl = db.getCollection('relatives')
juniors = myColl.find('alias = "jr"').execute().fetchAll()

for junior in juniors:
  myColl.modify('name = :param'). \
    set('parent_name',mysqlx.expr(':param')). \
    bind('param', junior.name).execute()
