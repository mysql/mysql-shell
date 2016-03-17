
// one bind() per parameter
var myColl = db.getCollection('relatives');
var juniors = myColl.find('alias = "jr"').execute().fetchAll();

for (var index in juniors){
  myColl.modify('name = :param').
    set('parent_name',mysqlx.expr(':param')).
    bind('param', juniors[index].name).execute();
}