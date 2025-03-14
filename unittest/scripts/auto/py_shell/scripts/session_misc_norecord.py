#@<> setup
shell.connect(__mysqluripwd)

def com_select(sess):
    return int(sess.run_sql("show session status like 'Com_select'").fetch_one()[1])

#@<> get_sql_mode without tracking
session2 = shell.open_session(__mysqluripwd)

real_sql_mode = session2.run_sql("select @@sql_mode").fetch_one()[0]
EXPECT_EQ(real_sql_mode, session2.get_sql_mode())

#@<> sql_mode and tracking in classic session {VER(>=8.0)}

# session tracking is enabled by default on global session
session.run_sql("set sql_mode='ansi_quotes'")
EXPECT_EQ("ANSI_QUOTES", session.get_sql_mode())

#@<> sql_mode tracking in a new session {VER(>=8.0)}
EXPECT_EQ(real_sql_mode, session2.get_sql_mode())

# session tracking disabled by default in non-global sessions - in that case, it should always query sql_mode
num_selects = com_select(session2)
session2.run_sql("set sql_mode='ansi_quotes'")
EXPECT_EQ("ANSI_QUOTES", session2.get_sql_mode())
EXPECT_EQ("ANSI_QUOTES", session2.get_sql_mode())
EXPECT_EQ(2, com_select(session2) - num_selects)

tracked_vars = session2.track_system_variable("sql_mode")
EXPECT_TRUE("sql_mode" in tracked_vars)

# session tracking now enabled - no queries on sql_mode at all
num_selects = com_select(session2)
session2.run_sql("set sql_mode='ansi_quotes'")
EXPECT_EQ("ANSI_QUOTES", session2.get_sql_mode())
EXPECT_EQ("ANSI_QUOTES", session2.get_sql_mode())

session2.run_sql("set sql_mode=''")
EXPECT_EQ("", session2.get_sql_mode())
EXPECT_EQ("", session2.get_sql_mode())
EXPECT_EQ(0, com_select(session2) - num_selects)

#@<> client data
deleted = False


class Data:
    def __del__(self):
        global deleted
        deleted = True


session.set_client_data("mydata", {"HELLO": "WORLD"})
session.set_client_data("null", None)
session.set_client_data("binary", b"binarydata")
data = Data()
session.set_client_data("opaque", data)

EXPECT_EQ({"HELLO": "WORLD"}, session.get_client_data("mydata"))
EXPECT_EQ(None, session.get_client_data("null"))
EXPECT_EQ(data, session.get_client_data("opaque"))
EXPECT_EQ(b"binarydata", session.get_client_data("binary"))

EXPECT_EQ(None, session.get_client_data("invalid"))

del data

import gc

gc.collect()
EXPECT_FALSE(deleted)

session.set_client_data("opaque", None)
EXPECT_EQ(None, session.get_client_data("opaque"))

gc.collect()
EXPECT_TRUE(deleted)

#@<> cleanup
session2.close()
shell.disconnect()
