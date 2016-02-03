session.runSql("use sakila;")
session.runSql("drop table if exists sakila.friends;")
session.runSql("create table sakila.friends (name varchar(50), last_name varchar(50), age integer, gender varchar(20));")
session.runSql("INSERT INTO sakila.friends (name, last_name, age, gender) VALUES('jack','chan', 30,'male');")
session.runSql("INSERT INTO sakila.friends (name, last_name, age, gender) VALUES('ruben','morquecho', 40,'male');")
session.runSql("delete from sakila.friends where name = 'ruben';")

session.runSql("select * from sakila.friends where name = 'ruben';")
