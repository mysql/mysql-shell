schema = session.getSchema('sakila');
table = schema.getTable('actor');
table.update().set('last_name','Update by Py Node STDIN').where('actor_id < :ID').orderBy(['actor_id desc']).limit(3).bind('ID',40).execute();
table.select().where("actor_id < 40").orderBy(['actor_id desc']).limit(3).execute();