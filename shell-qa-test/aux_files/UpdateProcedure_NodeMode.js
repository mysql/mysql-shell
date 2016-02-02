session.sql("use sakila;").execute();
session.sql("DROP procedure IF EXISTS Test2;").execute(); 

session.sql("Create procedure Test2() BEGIN SELECT count(*) as NumberOfCountries_Upd3 FROM country; END").execute();

session.sql("call Test2;").execute();
