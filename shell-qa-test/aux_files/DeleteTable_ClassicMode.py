session.run_sql("use sakila;")
session.run_sql("drop table if exists sakila.friends;")
session.run_sql("create table sakila.friends (name varchar(50), last_name varchar(50), age integer, gender varchar(20));")
session.run_sql("INSERT INTO sakila.friends (name, last_name, age, gender) VALUES('jack','chan', 30,'male');")
session.run_sql("INSERT INTO sakila.friends (name, last_name, age, gender) VALUES('ruben','morquecho', 40,'male');")
session.run_sql("delete from sakila.friends where name = 'ruben';")

session.run_sql("select * from sakila.friends where name = 'ruben';")
