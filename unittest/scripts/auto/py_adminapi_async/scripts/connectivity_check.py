#@ {VER(>=8.0.11)}
#@<> Setup

import os
import shutil
ssldir = os.path.dirname(os.path.dirname(
    testutil.get_sandbox_conf_path(__mysql_sandbox_port1)))+"/ssl"
if os.path.exists(ssldir):
    shutil.rmtree(ssldir)

testutil.ssl_create_ca('ca', '/CN=TestCertIssuer')
testutil.ssl_create_ca('cax', '/CN=AnotherTestCertIssuer')
testutil.ssl_create_cert(f'sb1', 'ca', f'/CN={hostname}')
options = {
    "ssl_ca": f"{ssldir}/ca.pem",
    "ssl_cert": f"{ssldir}/sb1-cert.pem",
    "ssl_key": f"{ssldir}/sb1-key.pem",
    "report_host": f"{hostname}"
}
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", options)

testutil.ssl_create_cert(f'sb2', 'ca', f'/CN={hostname}')
testutil.ssl_create_cert(f'sb2-badcert', 'ca', f'/CN=example.com')
testutil.ssl_create_cert(f'sb2-badca', 'cax', f'/CN={hostname}')
options = {
    "ssl_ca": f"{ssldir}/ca.pem",
    "ssl_cert": f"{ssldir}/sb2-cert.pem",
    "ssl_key": f"{ssldir}/sb2-key.pem",
    "report_host": f"{hostname}"
}
testutil.deploy_sandbox(__mysql_sandbox_port2, "root", options)


def dissolve():
    session1 = mysql.get_session(__sandbox_uri1)
    session1.run_sql("set global super_read_only=0")
    session1.run_sql("drop schema if exists mysql_innodb_cluster_metadata")
    session1.run_sql("reset master")
    session1.close()
    session2 = mysql.get_session(__sandbox_uri2)
    session2.run_sql("set global super_read_only=0")
    session2.run_sql("drop schema if exists mysql_innodb_cluster_metadata")
    session2.run_sql("stop slave")
    session2.run_sql("reset slave all")
    session2.run_sql("reset master")
    session2.close()


#@<> report_host is wrong
shell.connect(__sandbox_uri1)
# 198.51.100.0/24 is reserved test address
session.run_sql("set persist_only report_host='198.51.100.100'")
testutil.restart_sandbox(__mysql_sandbox_port1)
shell.connect(__sandbox_uri1)
EXPECT_THROWS(lambda: dba.create_replica_set("rs"),
              "Server address configuration error")

EXPECT_STDOUT_CONTAINS(
    f"This instance reports its own address as 198.51.100.100:{__mysql_sandbox_port1}")

shell.options["dba.connectivityChecks"]=False
EXPECT_NO_THROWS(lambda: dba.create_replica_set("rs"))
shell.options["dba.connectivityChecks"]=True

dissolve()

session.run_sql("set persist_only report_host=?", [hostname])
testutil.restart_sandbox(__mysql_sandbox_port1)
shell.connect(__sandbox_uri1)

#@<> report_port is wrong {False}
session.run_sql("set persist_only report_host=?", [hostname])
session.run_sql("set persist_only report_port=?", [__mysql_sandbox_port4])
testutil.restart_sandbox(__mysql_sandbox_port1)
shell.connect(__sandbox_uri1)
EXPECT_THROWS(lambda: dba.create_replica_set("rs"),
              "Server address configuration error")

EXPECT_STDOUT_CONTAINS(
    f"This instance reports its own address as {hostname}:{__mysql_sandbox_port4}")

dissolve()
session.run_sql("set persist_only report_port=?", [__mysql_sandbox_port1])
testutil.restart_sandbox(__mysql_sandbox_port1)
shell.connect(__sandbox_uri1)

#@<> certIssuer is wrong
EXPECT_THROWS(lambda: dba.create_replica_set("rs", {
              "memberAuthType": "CERT_ISSUER", "certIssuer": "/CN=whatever"}),
              "Authentication error during connection check")
EXPECT_STDOUT_CONTAINS(
    f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port1}' was issued by '/CN=whatever' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port1}' and that the value specified for certIssuer itself is correct.")

dissolve()

#@<> certIssuer is OK
shell.connect(__sandbox_uri1)
EXPECT_NO_THROWS(lambda: dba.create_replica_set(
    "rs", {"memberAuthType": "CERT_ISSUER", "certIssuer": "/CN=TestCertIssuer"}))

dissolve()

#@<> certSubject is wrong

EXPECT_THROWS(lambda: dba.create_replica_set(
    "rs", {"memberAuthType": "CERT_SUBJECT", "certIssuer": "/CN=TestCertIssuer", "certSubject": "/CN=whatever.com"}),
    "Authentication error during connection check")
EXPECT_STDOUT_CONTAINS(
    f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port1}' was issued by '/CN=TestCertIssuer' (certIssuer), specifically for '/CN=whatever.com' (certSubject) and is recognized at '{hostname}:{__mysql_sandbox_port1}'.")
dissolve()


#@<> all OK - VERIFY_CA
shell.connect(__sandbox_uri1)
rs = dba.create_replica_set("rs", {"replicationSslMode": "VERIFY_CA"})
rs.add_instance(__sandbox_uri2, {"recoveryMethod": "clone"})
dissolve()

#@<> all OK - VERIFY_IDENTITY
shell.connect(__sandbox_uri1)
rs = dba.create_replica_set("rs", {"replicationSslMode": "VERIFY_IDENTITY"})

rs.add_instance(__sandbox_uri2, {"recoveryMethod": "clone"})
dissolve()

#@<> cert CA doesn't match - check VERIFY_CA

# self-check will work, but peer check should fail
shell.connect(__sandbox_uri2)
for sql in [f"set persist_only ssl_ca=/*(*/'{ssldir}/cax.pem'/*)*/",
            f"set persist_only ssl_cert=/*(*/'{ssldir}/sb2-badca-cert.pem'/*)*/",
            f"set persist_only ssl_key=/*(*/'{ssldir}/sb2-badca-key.pem'/*)*/"]:
    session.run_sql(sql)
testutil.restart_sandbox(__mysql_sandbox_port2)

shell.connect(__sandbox_uri1)
rs = dba.create_replica_set("rs", {"replicationSslMode": "VERIFY_CA"})

EXPECT_THROWS(lambda: rs.add_instance(
    __sandbox_uri2, {"recoveryMethod": "clone"}), "SSL connection check error")

EXPECT_STDOUT_CONTAINS(
    f"ERROR: SSL certificate issuer verification (VERIFY_CA) of instance '{hostname}:{__mysql_sandbox_port2}' failed at '{hostname}:{__mysql_sandbox_port1}'")

shell.options['dba.connectivityChecks']=False
EXPECT_THROWS(lambda: rs.add_instance(
    __sandbox_uri2, {"recoveryMethod": "clone"}), "Timeout waiting for replication to start")
shell.options['dba.connectivityChecks']=True

dissolve()

#@<> cert CA doesn't match - check CERT_ISSUER
shell.connect(__sandbox_uri1)
rs = dba.create_replica_set("rs", {"replicationSslMode": "REQUIRED", "memberAuthType": "CERT_ISSUER", "certIssuer": "/CN=TestCertIssuer"})

EXPECT_THROWS(lambda:rs.add_instance(__sandbox_uri2), "Authentication error during connection check")
EXPECT_STDOUT_CONTAINS(f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port2}' was issued by '/CN=TestCertIssuer' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port2}' and that the value specified for certIssuer itself is correct.")

dissolve()

#@<> with certSubject 

rs = dba.create_replica_set("rs", {"replicationSslMode": "REQUIRED", "memberAuthType": "CERT_SUBJECT", "certIssuer": "/CN=TestCertIssuer", "certSubject": f"/CN={hostname}"})

EXPECT_THROWS(lambda:rs.add_instance(__sandbox_uri2, {"certSubject": f"/CN={hostname}"}), "Authentication error during connection check")

EXPECT_STDOUT_CONTAINS(f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port2}' was issued by '/CN=TestCertIssuer' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port2}' and that the value specified for certIssuer itself is correct.")

dissolve()

#@<> cert subject matches but CA is wrong
shell.connect(__sandbox_uri2)
for sql in [f"set persist_only ssl_ca=/*(*/'{ssldir}/ca.pem'/*)*/",
            f"set persist_only ssl_cert=/*(*/'{ssldir}/sb2-badca-cert.pem'/*)*/",
            f"set persist_only ssl_key=/*(*/'{ssldir}/sb2-badca-key.pem'/*)*/"]:
    session.run_sql(sql)
testutil.restart_sandbox(__mysql_sandbox_port2)

shell.connect(__sandbox_uri1)
rs = dba.create_replica_set("rs", {"replicationSslMode": "VERIFY_IDENTITY"})

EXPECT_THROWS(lambda: rs.add_instance(
    __sandbox_uri2, {"recoveryMethod": "clone"}), "SSL connection check error")

EXPECT_STDOUT_CONTAINS(
    f"ERROR: SSL certificate subject verification (VERIFY_IDENTITY) of instance '{hostname}:{__mysql_sandbox_port2}' failed at '{hostname}:{__mysql_sandbox_port2}'")

dissolve()

#@<> cert subject doesn't match
shell.connect(__sandbox_uri2)
for sql in [f"set persist_only ssl_ca=/*(*/'{ssldir}/cax.pem'/*)*/",
            f"set persist_only ssl_cert=/*(*/'{ssldir}/sb2-badcert-cert.pem'/*)*/",
            f"set persist_only ssl_key=/*(*/'{ssldir}/sb2-badcert-key.pem'/*)*/"]:
    session.run_sql(sql)
testutil.restart_sandbox(__mysql_sandbox_port2)

shell.connect(__sandbox_uri1)
rs = dba.create_replica_set("rs", {"replicationSslMode": "VERIFY_IDENTITY"})

EXPECT_THROWS(lambda: rs.add_instance(
    __sandbox_uri2, {"recoveryMethod": "clone"}), "SSL connection check error")

EXPECT_STDOUT_CONTAINS(
    f"ERROR: SSL certificate subject verification (VERIFY_IDENTITY) of instance '{hostname}:{__mysql_sandbox_port2}' failed at '{hostname}:{__mysql_sandbox_port2}'")

dissolve()

#@<> cert subject doesn't match (but CA does)
shell.connect(__sandbox_uri1)
rs = dba.create_replica_set("rs", {"replicationSslMode": "VERIFY_CA"})

EXPECT_THROWS(lambda: rs.add_instance(
    __sandbox_uri2, {"recoveryMethod": "clone"}), "SSL connection check error")

EXPECT_STDOUT_CONTAINS(
    f"ERROR: SSL certificate issuer verification (VERIFY_CA) of instance '{hostname}:{__mysql_sandbox_port2}' failed at '{hostname}:{__mysql_sandbox_port2}'")

rs.disconnect()

dissolve()

#@<> Cleanup
if os.path.exists(ssldir):
    shutil.rmtree(ssldir)
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
