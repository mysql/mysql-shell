session.sql("use sakila;").execute();
session.sql("drop table if exists sakila.friends;").execute();
session.sql("create table sakila.friends (name varchar(50), last_name varchar(50), age integer, gender varchar(20));").execute();
session.sql("INSERT INTO sakila.friends (name, last_name, age, gender) VALUES('jack','chan', 30,'male');").execute();
session.sql("INSERT INTO sakila.friends (name, last_name, age, gender) VALUES('ruben','morquecho', 40,'male');").execute();
var table = session.getSchema('sakila').getTable('friends');

table.delete().where("name=:nombre").orderBy(['last_name']).limit(1).bind('nombre','ruben').execute();

table.select().where('name=:nombre').orderBy(['last_name']).bind('nombre','ruben').execute();


