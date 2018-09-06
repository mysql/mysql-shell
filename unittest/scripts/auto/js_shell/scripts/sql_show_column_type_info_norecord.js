shell.connect(__uripwd);
\sql
\option outputFormat = tabbed

//@ Setup
drop database if exists scti_test;
create database scti_test;
CREATE TABLE scti_test.t (
		`tint` TINYINT,
		`tintu` TINYINT UNSIGNED,
		`smallint` SMALLINT,
		`smallintu` SMALLINT UNSIGNED,
		`mediumint` MEDIUMINT,
		`mediumintu` MEDIUMINT UNSIGNED,
		`int` INT,
		`intu` INT UNSIGNED,
		`bigint` BIGINT AUTO_INCREMENT PRIMARY KEY,
		`bigintu` BIGINT UNSIGNED,
		`float` FLOAT( 10, 2 ),
		`double` DOUBLE,
		`decimal` DECIMAL( 10, 2 ),
		`date` DATE,
		`datetime` DATETIME,
		`timestamp` TIMESTAMP,
		`time` TIME,
		`year` YEAR NOT NULL,
		`tinyblob` TINYBLOB,
		`tinytext` TINYTEXT,
		`blob` BLOB,
		`text` TEXT,
		`mediumblob` MEDIUMBLOB,
		`mediumtext` MEDIUMTEXT,
		`longblob` LONGBLOB,
		`longtext` LONGTEXT,
		`enum` ENUM( '1', '2', '3' ),
		`set` SET( '1', '2', '3' ),
		`json` JSON,
		`geometry` GEOMETRY,
		`bool` BOOL,
		`char` CHAR( 10 ),
		`varchar` VARCHAR(20),
		`binary` BINARY( 20 ),
		`varbinary` VARBINARY( 20 )
		);
\js

//@ Column type info disabled X
testutil.callMysqlsh([__uripwd, '--sql', '--vertical', '-e', 'select * from scti_test.t;'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ Column type info enabled X {VER(>=8.0.11)}
testutil.callMysqlsh([__uripwd, '--column-type-info', '--sql', '--vertical', '-e', 'select * from scti_test.t;'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ Column type info enabled X JSON {VER(>=8.0.11)}
testutil.callMysqlsh([__uripwd, '--column-type-info', '--sql', '--json', '-e', 'select * from scti_test.t;'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ Column type info disabled classic
testutil.callMysqlsh([__mysqluripwd, '--sql', '--vertical', '-e', 'select * from scti_test.t;'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ Column type info enabled classic {VER(>=8.0.11)}
testutil.callMysqlsh([__mysqluripwd, '--column-type-info', '--sql', '--vertical', '-e', 'select * from scti_test.t;'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ Column type info enabled X 5.7 {VER(<8.0.0)}
testutil.callMysqlsh([__uripwd, '--column-type-info', '--sql', '--vertical', '-e', 'select * from scti_test.t;'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

//@ Column type info enabled classic 5.7 {VER(<8.0.0)}
testutil.callMysqlsh([__mysqluripwd, '--column-type-info', '--sql', '--vertical', '-e', 'select * from scti_test.t;'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
