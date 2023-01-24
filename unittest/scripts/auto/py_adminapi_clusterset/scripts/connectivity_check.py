#@ {VER(>=8.0.27)}
#@<> Setup

import os
import shutil
ssldir = os.path.dirname(os.path.dirname(
    testutil.get_sandbox_conf_path(__mysql_sandbox_port1)))+"/ssl"
if os.path.exists(ssldir):
    shutil.rmtree(ssldir)

# SB1, SB2 and SB3 are OK
testutil.ssl_create_ca('ca', '/CN=TestCertIssuer')
testutil.ssl_create_cert(f'sb1', 'ca', f'/CN={hostname}')
options = {
    "ssl_ca": f"{ssldir}/ca.pem",
    "ssl_cert": f"{ssldir}/sb1-cert.pem",
    "ssl_key": f"{ssldir}/sb1-key.pem",
    "report_host": f"{hostname}"
}
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", options)

testutil.ssl_create_cert(f'sb2', 'ca', f'/CN={hostname}')
options = {
    "ssl_ca": f"{ssldir}/ca.pem",
    "ssl_cert": f"{ssldir}/sb2-cert.pem",
    "ssl_key": f"{ssldir}/sb2-key.pem",
    "report_host": f"{hostname}"
}
testutil.deploy_sandbox(__mysql_sandbox_port2, "root", options)

testutil.ssl_create_cert(f'sb3', 'ca', f'/CN={hostname}')
options = {
    "ssl_ca": f"{ssldir}/ca.pem",
    "ssl_cert": f"{ssldir}/sb3-cert.pem",
    "ssl_key": f"{ssldir}/sb3-key.pem",
    "report_host": f"{hostname}"
}
testutil.deploy_sandbox(__mysql_sandbox_port3, "root", options)

# SB4 has cert from different CA
testutil.ssl_create_ca('cab', '/CN=TestCertIssuer2')
testutil.ssl_create_cert(f'sb4', 'cab', f'/CN={hostname}')
options = {
    "ssl_ca": f"{ssldir}/cab.pem",
    "ssl_cert": f"{ssldir}/sb4-cert.pem",
    "ssl_key": f"{ssldir}/sb4-key.pem",
    "report_host": f"{hostname}"
}
testutil.deploy_sandbox(__mysql_sandbox_port4, "root", options)

# SB5 has cert from correct CA but wrong host/subject
testutil.ssl_create_cert(f'sb5', 'ca', f'/CN=example.com')
options = {
    "ssl_ca": f"{ssldir}/ca.pem",
    "ssl_cert": f"{ssldir}/sb5-cert.pem",
    "ssl_key": f"{ssldir}/sb5-key.pem",
    "report_host": f"{hostname}"
}
testutil.deploy_sandbox(__mysql_sandbox_port5, "root", options)


def reset_all():
    reset_multi([__mysql_sandbox_port1,__mysql_sandbox_port2,__mysql_sandbox_port3,__mysql_sandbox_port4,__mysql_sandbox_port5])

def merge(*dicts):
    o = {}
    for d in dicts:
        o.update(d)
    return o

def TEST_COMBINATION(memberSslMode, clusterSetSslMode, memberAuthType):
    cert_issuer = {}
    cert_subject = {}
    if memberAuthType != "PASSWORD":
        cert_issuer = {"certIssuer": "/CN=TestCertIssuer"}
    if memberAuthType == "CERT_SUBJECT":
        cert_subject = {"certSubject": f"/CN={hostname}"}
    create_options = merge({"gtidSetIsComplete": 1, "memberAuthType": memberAuthType, "memberSslMode": memberSslMode}, cert_issuer, cert_subject)
    print("\n###### createCluster@sb4", memberSslMode, clusterSetSslMode, memberAuthType)
    shell.connect(__sandbox_uri4)
    if memberSslMode in ["VERIFY_CA", "VERIFY_IDENTITY"]:
        EXPECT_THROWS(lambda: dba.create_cluster("cluster", create_options), "Authentication error during connection check", "create_cluster@sb4")
        EXPECT_STDOUT_CONTAINS(f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port4}' was issued by '/CN=TestCertIssuer' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port4}' and that the value specified for certIssuer itself is correct.")
    elif memberAuthType in ["CERT_ISSUER", "CERT_SUBJECT"]:
        # can't verify client certificate if the cert is not from a valid CA
        EXPECT_THROWS(lambda: dba.create_cluster("cluster", create_options), "Authentication error during connection check", "create_cluster@sb4")
        EXPECT_STDOUT_CONTAINS(f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port4}' was issued by '/CN=TestCertIssuer' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port4}' and that the value specified for certIssuer itself is correct.")
    else:
        EXPECT_NO_THROWS(lambda: dba.create_cluster("cluster", create_options), "create_cluster@sb4")
        reset_instance(session)
    WIPE_OUTPUT()
    print("\n###### createCluster@sb5", memberSslMode, clusterSetSslMode, memberAuthType)
    shell.connect(__sandbox_uri5)
    if memberSslMode in ["VERIFY_IDENTITY"]:
        EXPECT_THROWS(lambda: dba.create_cluster("cluster", create_options), "SSL connection check error", "create_cluster@sb5")
    elif memberAuthType in ["CERT_SUBJECT"]:
        # can't verify client certificate if the cert is not from a valid CA
        EXPECT_THROWS(lambda: dba.create_cluster("cluster", create_options), "Authentication error during connection check", "create_cluster@sb4")
        EXPECT_STDOUT_CONTAINS(f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port5}' was issued by '/CN=TestCertIssuer' (certIssuer), specifically for '/CN={hostname}' (certSubject) and is recognized at '{hostname}:{__mysql_sandbox_port5}'.")
    else:
        EXPECT_NO_THROWS(lambda: dba.create_cluster("cluster", create_options), "create_cluster@sb5")
        reset_instance(session)
    WIPE_OUTPUT()
    print("\n###### createCluster@sb1", memberSslMode, clusterSetSslMode, memberAuthType)
    shell.connect(__sandbox_uri1)
    c = dba.create_cluster("cluster", create_options)
    WIPE_OUTPUT()
    print("\n###### addInstance@sb2", memberSslMode, clusterSetSslMode, memberAuthType)
    c.add_instance(__sandbox_uri2, cert_subject)
    cs = c.create_cluster_set("cs", {"clusterSetReplicationSslMode": clusterSetSslMode})
    WIPE_OUTPUT()
    print("\n###### addInstance@sb4", memberSslMode, clusterSetSslMode, memberAuthType)
    if memberSslMode in ["VERIFY_CA", "VERIFY_IDENTITY"]:
        EXPECT_THROWS(lambda: c.add_instance(__sandbox_uri4, cert_subject), "Authentication error during connection check", "add_instance@sb4")
    elif memberAuthType in ["CERT_ISSUER", "CERT_SUBJECT"]:
        # can't verify client certificate if the cert is not from a valid CA
        EXPECT_THROWS(lambda: c.add_instance(__sandbox_uri4, cert_subject), "Authentication error during connection check", "add_instance@sb4")
        EXPECT_STDOUT_CONTAINS(f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port4}' was issued by '/CN=TestCertIssuer' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port4}' and that the value specified for certIssuer itself is correct.")
    else:
        EXPECT_NO_THROWS(lambda: c.add_instance(__sandbox_uri4, cert_subject), "add_instance@sb4")
        c.remove_instance(__sandbox_uri4)
    WIPE_OUTPUT()
    print("\n###### addInstance@sb5", memberSslMode, clusterSetSslMode, memberAuthType)
    if memberSslMode in ["VERIFY_IDENTITY"]:
        EXPECT_THROWS(lambda: c.add_instance(__sandbox_uri5, cert_subject), "SSL connection check error", "add_instance@sb5")
    elif memberAuthType in ["CERT_SUBJECT"]:
        # can't verify client certificate if the cert is not from a valid CA
        EXPECT_THROWS(lambda: c.add_instance(__sandbox_uri5, cert_subject), "Authentication error during connection check", "add_instance@sb4")
        EXPECT_STDOUT_CONTAINS(f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port5}' was issued by '/CN=TestCertIssuer' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port5}' and that the value specified for certIssuer itself is correct.")
    else:
        EXPECT_NO_THROWS(lambda: c.add_instance(__sandbox_uri5, cert_subject), "add_instance@sb5")
        c.remove_instance(__sandbox_uri5)
        shell.connect(__sandbox_uri5)
        reset_instance(session)
    WIPE_OUTPUT()
    print("\n###### createReplicaCluster@sb4", memberSslMode, clusterSetSslMode, memberAuthType)
    if clusterSetSslMode in ["VERIFY_CA", "VERIFY_IDENTITY"]:
        EXPECT_THROWS(lambda: cs.create_replica_cluster(__sandbox_uri4, "replica", cert_subject), "Authentication error during connection check", "create_replica_cluster@sb4")
    elif memberAuthType in ["CERT_ISSUER", "CERT_SUBJECT"]:
        EXPECT_THROWS(lambda: cs.create_replica_cluster(__sandbox_uri4, "replica", cert_subject), "Authentication error during connection check", "create_replica_cluster@sb4")
        EXPECT_STDOUT_CONTAINS(f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port4}' was issued by '/CN=TestCertIssuer' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port4}' and that the value specified for certIssuer itself is correct.")
    else:
        EXPECT_NO_THROWS(lambda: cs.create_replica_cluster(__sandbox_uri4, "replica", cert_subject), "create_replica_cluster@sb4")
        cs.remove_cluster("replica")
        shell.connect(__sandbox_uri4)
        reset_instance(session)
    WIPE_OUTPUT()
    print("\n###### createReplicaCluster@sb5", memberSslMode, clusterSetSslMode, memberAuthType)
    if clusterSetSslMode in ["VERIFY_IDENTITY"]:
        if memberAuthType == "CERT_SUBJECT":
            EXPECT_THROWS(lambda: cs.create_replica_cluster(__sandbox_uri5, "replica", cert_subject), "Authentication error during connection check", "create_replica_cluster@sb5")
            EXPECT_STDOUT_CONTAINS(f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port5}' was issued by '/CN=TestCertIssuer' (certIssuer), specifically for '/CN={hostname}' (certSubject) and is recognized at '{hostname}:{__mysql_sandbox_port5}'.")
        else:
            EXPECT_THROWS(lambda: cs.create_replica_cluster(__sandbox_uri5, "replica", cert_subject), "SSL connection check error", "create_replica_cluster@sb5")
            EXPECT_STDOUT_CONTAINS(f"ERROR: SSL certificate subject verification (VERIFY_IDENTITY) of instance '{hostname}:{__mysql_sandbox_port1}' failed at '{hostname}:{__mysql_sandbox_port5}'")
    elif memberAuthType in ["CERT_SUBJECT"]:
        EXPECT_THROWS(lambda: cs.create_replica_cluster(__sandbox_uri5, "replica", cert_subject), "Authentication error during connection check", "create_replica_cluster@sb5")
    else:
        EXPECT_NO_THROWS(lambda: cs.create_replica_cluster(__sandbox_uri5, "replica", cert_subject), "create_replica_cluster@sb5")
        cs.remove_cluster("replica")
        shell.connect(__sandbox_uri5)
        reset_instance(session)
    WIPE_OUTPUT()
    print("\n###### createReplicaCluster@sb3", memberSslMode, clusterSetSslMode, memberAuthType)
    cs.create_replica_cluster(__sandbox_uri3, "replica", cert_subject)
    WIPE_OUTPUT()
    shell.dump_rows(session.run_sql(
        "select user, x509_issuer, x509_subject from mysql.user where user like 'mysql_innodb%'"))
    # check that ISSUER is required by all accounts (GR and AR)
    if memberAuthType in ["CERT_ISSUER", "CERT_SUBJECT"]:
       EXPECT_EQ(0, session.run_sql(
           "select count(*) from mysql.user where user like 'mysql_innodb%' and x509_issuer = ''").fetch_one()[0], "accounts without issuer")
    if memberAuthType == "CERT_SUBJECT":
       EXPECT_EQ(0, session.run_sql(
           "select count(*) from mysql.user where user like 'mysql_innodb%' and x509_subject = ''").fetch_one()[0], "accounts without subject")



#@<> report_host is wrong
shell.connect(__sandbox_uri2)
# 198.51.100.0/24 is reserved test address
session.run_sql("set persist_only report_host='198.51.100.100'")
testutil.restart_sandbox(__mysql_sandbox_port2)
shell.connect(__sandbox_uri1)

c = dba.create_cluster("cluster")
cs = c.create_cluster_set("cs")

EXPECT_THROWS(lambda: cs.create_replica_cluster(__sandbox_uri2, "replica"),
              "Server address configuration error")

EXPECT_STDOUT_CONTAINS(
    f"This instance reports its own address as 198.51.100.100:{__mysql_sandbox_port2}")

shell.options['dba.connectivityChecks']=False
EXPECT_THROWS(lambda: cs.create_replica_cluster(__sandbox_uri2, "replica", {"recoveryMethod":"clone"}), "Group Replication failed to start: ")
shell.options['dba.connectivityChecks']=True

shell.connect(__sandbox_uri2)
session.run_sql("set persist_only report_host=?", [hostname])
testutil.restart_sandbox(__mysql_sandbox_port2)

cs.disconnect()
c.disconnect()

reset_all()

#@<> report_port is wrong {False}
session.run_sql("set persist_only report_host=?", [hostname])
session.run_sql("set persist_only report_port=?", [__mysql_sandbox_port4])
testutil.restart_sandbox(__mysql_sandbox_port1)
shell.connect(__sandbox_uri1)
EXPECT_THROWS(lambda: dba.create_cluster("cluster"),
              "Server address configuration error")

EXPECT_STDOUT_CONTAINS(
    f"This instance reports its own address as {hostname}:{__mysql_sandbox_port4}")

reset_instance(session)
session.run_sql("set persist_only report_port=?", [__mysql_sandbox_port1])
testutil.restart_sandbox(__mysql_sandbox_port1)
shell.connect(__sandbox_uri1)


# memberSslMode - DISABLED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY
# clusterSetSslMode - DISABLED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY
# memberAuthType - PASSWORD, CERT_ISSUER, CERT_SUBJECT

#@<> DISABLED, DISABLED, PASSWORD

TEST_COMBINATION("DISABLED", "DISABLED", "PASSWORD")

reset_all()

#@<> Tests: REQUIRED, REQUIRED, PASSWORD

TEST_COMBINATION("REQUIRED", "REQUIRED", "PASSWORD")

reset_all()

#@<> Tests: REQUIRED, REQUIRED, CERT_ISSUER

TEST_COMBINATION("REQUIRED", "REQUIRED", "CERT_ISSUER")

reset_all()

#@<> Tests: REQUIRED, REQUIRED, CERT_SUBJECT

TEST_COMBINATION("REQUIRED", "REQUIRED", "CERT_SUBJECT")

reset_all()

#@<> Tests: VERIFY_CA, VERIFY_CA, CERT_ISSUER

TEST_COMBINATION("VERIFY_CA", "VERIFY_CA", "CERT_ISSUER")

reset_all()

#@<> Tests: VERIFY_CA, VERIFY_CA, CERT_SUBJECT

TEST_COMBINATION("VERIFY_CA", "VERIFY_CA", "CERT_SUBJECT")

reset_all()

#@<> Tests: VERIFY_IDENTITY, VERIFY_IDENTITY, CERT_ISSUER

TEST_COMBINATION("VERIFY_IDENTITY", "VERIFY_IDENTITY", "CERT_ISSUER")

reset_all()

#@<> Tests: VERIFY_IDENTITY, VERIFY_IDENTITY, CERT_SUBJECT

TEST_COMBINATION("VERIFY_IDENTITY", "VERIFY_IDENTITY", "CERT_SUBJECT")

reset_all()

#@<> Cleanup
if os.path.exists(ssldir):
    shutil.rmtree(ssldir)
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
testutil.destroy_sandbox(__mysql_sandbox_port4)
testutil.destroy_sandbox(__mysql_sandbox_port5)
