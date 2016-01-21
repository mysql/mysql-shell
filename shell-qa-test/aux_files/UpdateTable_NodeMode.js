var schema = session.getSchema('sakila');
var table = schema.getTable('actor');
table.update().set('last_name','Update by JS Node STDIN').set('first_name','Updated by JS Node STDIN').where('actor_id < :ID').orderBy(['actor_id desc']).limit(3).bind('ID',50).execute();
table.select().where("actor_id < 50").orderBy(['actor_id desc']).limit(3).execute();