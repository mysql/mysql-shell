#@{VER(>= 8.0)}
import itertools
import os

connuri = [f"{__mysql_uri}", f"{__uri}", f"mysql://{__mysql_uri}", f"mysqlx://{__uri}"]
tlsver = ["TLSv1", "TLSv1.1", "TLSv1.2"]
tls_deprecation_message = "WARNING: TLS versions TLSv1 and TLSv1.1 are now deprecated and will be removed in a future release of MySQL Shell. Use TLSv1.2 or TLSv1.3."

# Depend on tls-version variable server configuration and OpenSSL version used
# connection may or may not throw an error, therefore we call EXPECT_MAY_THROW
# and act accordingly.
#
# For example, if server does not support TLSv1 we get:
#
#     $ mysqlsh mysql://root@127.0.0.1:4000?tls-version=TLSv1
#     (...)
#     WARNING: TLS versions TLSv1 and TLSv1.1 are now deprecated and will be removed in a future release of MySQL Shell. Use TLSv1.2 or TLSv1.3.
#     Creating a Classic session to 'root@127.0.0.1:4000?tls-version=TLSv1'
#     MySQL Error 2026 (HY000): SSL connection error: error:1409442E:SSL routines:ssl3_read_bytes:tlsv1 alert protocol version
#
# or
#
#     WARNING: TLS versions TLSv1 and TLSv1.1 are now deprecated and will be removed in a future release of MySQL Shell. Use TLSv1.2 or TLSv1.3.
#     Creating a session to 'root@localhost:2515?tls-version=TLSv1'
#     MySQL Error (2026): Shell.connect: SSL connection error: error:141E70BF:SSL routines:tls_construct_client_hello:no protocols available
#

#@<> shell.connect tls-version
for s in connuri:
    for tlsval in tlsver:
        WIPE_OUTPUT()
        connstring = f"{s}?tls-version={tlsval}"
        EXPECT_MAY_THROW(lambda: shell.connect(connstring), ":SSL routines:")
        if tlsval == "TLSv1.2":
            EXPECT_STDOUT_NOT_CONTAINS(tls_deprecation_message)
        else:
            EXPECT_STDOUT_CONTAINS(tls_deprecation_message)
        WIPE_OUTPUT()

#@<> shell.connect tls-versions
for l in range(len(tlsver)):
    for s in connuri:
        for tls in itertools.combinations(tlsver, r=l):
            WIPE_OUTPUT()
            tlsval = ",".join(tls)
            connstring = f"{s}?tls-versions=[{tlsval}]"
            print("### ", connstring)
            if len(tlsval) == 0:
                if connstring.startswith("mysql://") or connstring.startswith(__mysql_uri + "?"):
                    EXPECT_THROWS(lambda: shell.connect(connstring), "SSL connection error: TLS version is invalid")
                elif connstring.startswith("mysqlx://") or connstring.startswith(__uri + "?"):
                    EXPECT_MAY_THROW(lambda: shell.connect(connstring), ":SSL routines:")
                    EXPECT_STDOUT_NOT_CONTAINS(tls_deprecation_message)
            elif tlsval == "TLSv1.2":
                EXPECT_MAY_THROW(lambda: shell.connect(connstring), ":SSL routines:")
                EXPECT_STDOUT_NOT_CONTAINS(tls_deprecation_message)
            else:
                EXPECT_MAY_THROW(lambda: shell.connect(connstring), ":SSL routines:")
                EXPECT_STDOUT_CONTAINS(tls_deprecation_message)
            WIPE_OUTPUT()

#@<> shell.open_session
for s in connuri:
    for tlsval in tlsver:
        WIPE_OUTPUT()
        connstring = f"{s}?tls-version={tlsval}"
        sess = EXPECT_MAY_THROW(lambda: shell.open_session(connstring), ":SSL routines:")
        EXPECT_EQ(True, sess.is_open() if sess else True)
        if tlsval == "TLSv1.2":
            EXPECT_STDOUT_NOT_CONTAINS(tls_deprecation_message)
        else:
            EXPECT_STDOUT_CONTAINS(tls_deprecation_message)
        if sess is not None:
            sess.close()
        WIPE_OUTPUT()

#@<> mysql.get_session and mysql.get_classic_session
for func in (mysql.get_session, mysql.get_classic_session):
    for tlsval in tlsver:
        WIPE_OUTPUT()
        connstring = f"{__mysql_uri}?tls-version={tlsval}"
        sess = EXPECT_MAY_THROW(lambda: func(connstring), ":SSL routines:")
        EXPECT_EQ(True, sess.is_open() if sess else True)
        if tlsval == "TLSv1.2":
            EXPECT_STDOUT_NOT_CONTAINS(tls_deprecation_message)
        else:
            EXPECT_STDOUT_CONTAINS(tls_deprecation_message)
        if sess is not None:
            sess.close()
        WIPE_OUTPUT()

#@<> mysqlx.get_session
for tlsval in tlsver:
    WIPE_OUTPUT()
    connstring = f"{__uri}?tls-version={tlsval}"
    sess = EXPECT_MAY_THROW(lambda: mysqlx.get_session(connstring), ":SSL routines:")
    EXPECT_EQ(True, sess.is_open() if sess else True)
    if tlsval == "TLSv1.2":
        EXPECT_STDOUT_NOT_CONTAINS(tls_deprecation_message)
    else:
        EXPECT_STDOUT_CONTAINS(tls_deprecation_message)
    if sess is not None:
        sess.close()
    WIPE_OUTPUT()

#@<> connection url from command line - classic protocol
testutil.call_mysqlsh([f"mysql://{__mysqluripwd}?tls-version=TLSv1.1", "--json", "--", "shell", "--help"])
EXPECT_STDOUT_CONTAINS('''"warning": "TLS versions TLSv1 and TLSv1.1 are now deprecated and will be removed in a future release of MySQL Shell. Use TLSv1.2 or TLSv1.3."''')

#@<> connection url from command line - X protocol
testutil.call_mysqlsh([f"mysqlx://{__uripwd}?tls-version=TLSv1.1", "--json", "--", "shell", "--help"])
EXPECT_STDOUT_CONTAINS('''"warning": "TLS versions TLSv1 and TLSv1.1 are now deprecated and will be removed in a future release of MySQL Shell. Use TLSv1.2 or TLSv1.3."''')

#@<> Run script file with deprecated tls mysql connection uri
script_file = os.path.join(f"{__tmp_dir}, connect_with_old_tls.py")
with open(script_file, "w") as fh:
    fh.write(f'''shell.connect("mysql://{__mysqluripwd}?tls-version=TLSv1.1")''')
testutil.call_mysqlsh(["--file", script_file])
EXPECT_STDOUT_CONTAINS("TLS versions TLSv1 and TLSv1.1 are now deprecated and will be removed in a future release of MySQL Shell. Use TLSv1.2 or TLSv1.3.")
testutil.rmfile(script_file)

#@<> Run script file with deprecated tls mysqlx connection uri
script_file = os.path.join(f"{__tmp_dir}, connect_with_old_tls.py")
with open(script_file, "w") as fh:
    fh.write(f'''shell.connect("mysqlx://{__uripwd}?tls-version=TLSv1.1")''')
testutil.call_mysqlsh(["--file", script_file])
EXPECT_STDOUT_CONTAINS("TLS versions TLSv1 and TLSv1.1 are now deprecated and will be removed in a future release of MySQL Shell. Use TLSv1.2 or TLSv1.3.")
testutil.rmfile(script_file)

#@<> Run script file in interactive mode with deprecated tls mysql connection uri
script_file = os.path.join(f"{__tmp_dir}, connect_with_old_tls.py")
with open(script_file, "w") as fh:
    fh.write(f"\connect mysql://{__mysqluripwd}?tls-version=TLSv1.1")
testutil.call_mysqlsh(["-i", "--file", script_file])
EXPECT_STDOUT_CONTAINS("TLS versions TLSv1 and TLSv1.1 are now deprecated and will be removed in a future release of MySQL Shell. Use TLSv1.2 or TLSv1.3.")
testutil.rmfile(script_file)

#@<> Run script file in interactive mode with deprecated tls mysqlx connection uri
script_file = os.path.join(f"{__tmp_dir}, connect_with_old_tls.py")
with open(script_file, "w") as fh:
    fh.write(f"\connect mysqlx://{__uripwd}?tls-version=TLSv1.1")
testutil.call_mysqlsh(["-i", "--file", script_file])
EXPECT_STDOUT_CONTAINS("TLS versions TLSv1 and TLSv1.1 are now deprecated and will be removed in a future release of MySQL Shell. Use TLSv1.2 or TLSv1.3.")
testutil.rmfile(script_file)

#@<> util.check_for_server_upgrade
target = f"{__mysql_uri}?tls-version=TLSv1.1"
EXPECT_MAY_THROW(lambda: util.check_for_server_upgrade(target), "")
EXPECT_STDOUT_CONTAINS(tls_deprecation_message)

admin_api_full_test = True

#@<> cluster setup
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {"report_host": hostname, "tls-version": "TLSv1.1,TLSv1.2,TLSv1.3"})
testutil.deploy_sandbox(__mysql_sandbox_port2, "root", {"report_host": hostname, "tls-version": "TLSv1.1,TLSv1.2,TLSv1.3"})
try:
    shell.connect(__sandbox_uri1 + "?tls-version=TLSv1.1")
except Exception as e:
    # Modern Linux distributions like EL8, Debian 10 and Ubuntu 20.04 restrict
    # minimum TLS protocol version to TLSv1.2.
    # Even if MySQL Server allow tls-version=TLSv1,TLSv1.1,TLSv1.2,
    # OpenSSL configuration forbids connection negotiation with lower than
    # TLSv1.2 version.
    # If this is a case, we detect this and skip TLSv1/TLSv1.1 tests.
    admin_api_full_test = False

#@<> cluster connection setup {admin_api_full_test == True}
shell.connect(__sandbox_uri1 + "?tls-version=TLSv1.1")
session.run_sql("show variables like '%tls_%'")

#@<> create cluster {admin_api_full_test == True}
clus = dba.create_cluster("q")

#@<> check instance configuration {admin_api_full_test == True}
dba.check_instance_configuration(__sandbox_uri2 + "?tls-version=TLSv1.1")
EXPECT_STDOUT_CONTAINS(tls_deprecation_message)

#@<> check cluster state {admin_api_full_test == True}
clus.check_instance_state(__sandbox_uri2)
EXPECT_STDOUT_CONTAINS(tls_deprecation_message)

#@<> add new cluster instance {admin_api_full_test == True}
clus.add_instance(__sandbox_uri2, {"recoveryMethod": "incremental"})
testutil.wait_member_state(__mysql_sandbox_port2, "ONLINE")
EXPECT_STDOUT_CONTAINS(tls_deprecation_message)

#@<> cluster rejoin {admin_api_full_test == True}
clus.rejoin_instance(__sandbox_uri2)
EXPECT_STDOUT_CONTAINS(tls_deprecation_message)

#@<> remove cluster instance {admin_api_full_test == True}
clus.remove_instance(__sandbox_uri2 + "?tls-version=TLSv1.1")
EXPECT_STDOUT_CONTAINS(tls_deprecation_message)

#@<> cleanup cluster
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)


#@<> replicaset setup {admin_api_full_test == True}
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", {"report_host": hostname, "tls-version": "TLSv1.1,TLSv1.2,TLSv1.3"})
testutil.deploy_sandbox(__mysql_sandbox_port2, "root", {"report_host": hostname, "tls-version": "TLSv1.1,TLSv1.2,TLSv1.3"})

#@<> replicaset connection setup {admin_api_full_test == True}
shell.connect(__sandbox_uri1 + "?tls-version=TLSv1.1")

#@<> create replicaset {admin_api_full_test == True}
rset = dba.create_replica_set("q")

#@<> add new replicaset instance {admin_api_full_test == True}
rset.add_instance(__sandbox_uri2, {"recoveryMethod": "incremental"})
EXPECT_STDOUT_CONTAINS(tls_deprecation_message)

#@<> sync + replicaset stop {admin_api_full_test == True}
testutil.wait_member_transactions(__mysql_sandbox_port2, __mysql_sandbox_port1)

#@<> replicaset to 2 {admin_api_full_test == True}
rset.set_primary_instance(__sandbox_uri2)
EXPECT_STDOUT_CONTAINS(tls_deprecation_message)

#@<> replicaset to 1 {admin_api_full_test == True}
rset.set_primary_instance(__sandbox_uri1)
EXPECT_STDOUT_CONTAINS(tls_deprecation_message)

#@<> force replicaset {admin_api_full_test == True}
testutil.stop_sandbox(__mysql_sandbox_port1, {"wait": 1})
shell.connect(__sandbox_uri2 + "?tls-version=TLSv1.1")
rset = dba.get_replica_set()
rset.force_primary_instance(__sandbox_uri2)

#@<> replicaset rejoin {admin_api_full_test == True}
testutil.start_sandbox(__mysql_sandbox_port1)
s = rset.status()
sb1 = f"{hostname}:{__mysql_sandbox_port1}"
EXPECT_EQ(s.replicaSet.topology[sb1].status, "INVALIDATED")

rset.rejoin_instance(__sandbox_uri1)
EXPECT_STDOUT_CONTAINS(tls_deprecation_message)

s = rset.status()
EXPECT_EQ(s.replicaSet.topology[sb1].status, "ONLINE")

#@<> remove replicaset instance {admin_api_full_test == True}
rset.remove_instance(__sandbox_uri1 + "?tls-version=TLSv1.1")
EXPECT_STDOUT_CONTAINS(tls_deprecation_message)

#@<> cleanup replicaset {admin_api_full_test == True}
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
