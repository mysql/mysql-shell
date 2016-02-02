session.runSql("use sakila;");
session.runSql("DROP procedure IF EXISTS Test;");

session.runSql("Create procedure Test() BEGIN SELECT count(*) as NumberOfCountries_Upd FROM country; END");

session.runSql("call Test;");