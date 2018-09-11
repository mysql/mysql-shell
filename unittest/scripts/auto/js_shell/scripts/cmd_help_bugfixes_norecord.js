// BUG#28508724 MYSQLSH GIVES INCONSISTENT RESPONSES FROM \? HELP LOOK UPS

//@ help mysql
\help mysql

//@ help with trailing space [USE:help mysql]
\help mysql 

//@ help with leading space [USE:help mysql]
\help  mysql

//@ help with space before command [USE:help mysql]
 \help mysql
