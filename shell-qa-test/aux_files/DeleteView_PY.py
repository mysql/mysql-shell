session.get_schema('sakila').get_views()
session.drop_view('sakila','new_view')
session.get_schema('sakila').get_views()