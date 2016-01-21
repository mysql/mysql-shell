var mysqlx=require('mysqlx').mysqlx;
var session=mysqlx.getNodeSession('root:guidev!@localhost:33060');
session.sql('USE sakila;').execute();
session.sql('drop table if exists testDB2;').execute();
session.sql('create table testDB2 (name varchar(50), age integer, last_name varchar(100));').execute();