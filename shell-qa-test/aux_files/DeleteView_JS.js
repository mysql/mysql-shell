session.getSchema('sakila').getViews();
session.dropView('sakila','new_view');
session.getSchema('sakila').getViews();