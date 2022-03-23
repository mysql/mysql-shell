#@<> Setup
import datetime
testutil.deploy_sandbox(__mysql_sandbox_port1, "root")

session=mysql.get_session(__sandbox_uri1)

session.run_sql("create schema dates")
session.run_sql("set session sql_mode = ''")
session.run_sql("create table dates.data(id int, one datetime, two date)")
session.run_sql("insert into dates.data values (1, '2020-01-01 00:00:00','2020-01-01')")
session.run_sql("insert into dates.data values (2, '2020-00-00 00:00:00','2020-00-00')")

#@<> Get valid data
res = session.run_sql("select * from dates.data where id = 1")
row = res.fetch_one()

EXPECT_TRUE(isinstance(row[1], datetime.datetime))
EXPECT_TRUE(isinstance(row[2], datetime.date))

EXPECT_EQ('2020-01-01 00:00:00', str(row[1]))
EXPECT_EQ('2020-01-01', str(row[2]))


#@<> Get invalid data
res = session.run_sql("select * from dates.data where id = 2")
row = res.fetch_one()

EXPECT_TRUE(isinstance(row[1], str))
EXPECT_TRUE(isinstance(row[2], str))

EXPECT_EQ('2020-00-00 00:00:00', row[1])
EXPECT_EQ('2020-00-00', row[2])

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
