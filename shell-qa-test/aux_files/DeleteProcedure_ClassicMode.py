session.runSql('use sakila;');
session.runSql('DROP PROCEDURE IF EXISTS my_procedure;');
session.runSql("delimiter // ");
session.runSql("create procedure my_procedure (INOUT incr_param INT) BEGIN SET incr_param = incr_param + 1 ; END//  ");
session.runSql("delimiter ; ");
session.runSql("select name from mysql.proc ;");
session.runSql("select name from mysql.proc where name like 'my_procedure';");
session.runSql('DROP PROCEDURE IF EXISTS my_procedure;');
session.runSql("select name from mysql.proc where name like 'my_procedure';");