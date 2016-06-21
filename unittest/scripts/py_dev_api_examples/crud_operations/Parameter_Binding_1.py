# one bind() per parameter
myColl = db.get_collection('relatives')
juniors = myColl.find('alias = "jr"').execute().fetch_all()

for junior in juniors:
  myColl.modify('name = :param'). \
    set('parent_name',mysqlx.expr(':param')). \
    bind('param', junior.name).execute()
