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

def TEST_COMBINATION(memberSslMode, memberAuthType):
    cert_issuer = {}
    cert_subject = {}
    if memberAuthType != "PASSWORD":
        cert_issuer = {"certIssuer": "/CN=TestCertIssuer"}
    if memberAuthType == "CERT_SUBJECT":
        cert_subject = {"certSubject": f"/CN={hostname}"}
    create_options = merge({"gtidSetIsComplete": 1, "memberAuthType": memberAuthType, "memberSslMode": memberSslMode}, cert_issuer, cert_subject)
    print("\n###### createCluster@sb4", memberSslMode, memberAuthType)
    shell.connect(__sandbox_uri4)
    if memberSslMode in ["VERIFY_CA", "VERIFY_IDENTITY"]:
        EXPECT_THROWS(lambda: dba.create_cluster("cluster", create_options), "Authentication error during connection check", "create_cluster@sb4")
        EXPECT_STDOUT_CONTAINS(
            f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port4}' was issued by '/CN=TestCertIssuer' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port4}' and that the value specified for certIssuer itself is correct.")
    elif memberAuthType in ["CERT_ISSUER", "CERT_SUBJECT"]:
        # can't verify client certificate if the cert is not from a valid CA
        EXPECT_THROWS(lambda: dba.create_cluster("cluster", create_options), "Authentication error during connection check", "create_cluster@sb4")
        EXPECT_STDOUT_CONTAINS(
            f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port4}' was issued by '/CN=TestCertIssuer' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port4}' and that the value specified for certIssuer itself is correct.")
    else:
        EXPECT_NO_THROWS(lambda: dba.create_cluster("cluster", create_options), "create_cluster@sb4")
        reset_instance(session)
    WIPE_OUTPUT()
    print("\n###### createCluster@sb5", memberSslMode, memberAuthType)
    shell.connect(__sandbox_uri5)
    if memberSslMode in ["VERIFY_IDENTITY"]:
        EXPECT_THROWS(lambda: dba.create_cluster("cluster", create_options), "SSL connection check error", "create_cluster@sb5")
    elif memberAuthType in ["CERT_SUBJECT"]:
        # can't verify client certificate if the cert is not from a valid CA
        EXPECT_THROWS(lambda: dba.create_cluster("cluster", create_options), "Authentication error during connection check", "create_cluster@sb4")
        EXPECT_STDOUT_CONTAINS(
            f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port5}' was issued by '/CN=TestCertIssuer' (certIssuer), specifically for '/CN={hostname}' (certSubject) and is recognized at '{hostname}:{__mysql_sandbox_port5}'.")
    else:
        EXPECT_NO_THROWS(lambda: dba.create_cluster("cluster", create_options), "create_cluster@sb5")
        reset_instance(session)
    WIPE_OUTPUT()
    print("\n###### createCluster@sb1", memberSslMode, memberAuthType)
    shell.connect(__sandbox_uri1)
    c = dba.create_cluster("cluster", create_options)
    WIPE_OUTPUT()
    print("\n###### addInstance@sb2", memberSslMode, memberAuthType)
    c.add_instance(__sandbox_uri2, cert_subject)
    WIPE_OUTPUT()
    print("\n###### addInstance@sb4", memberSslMode, memberAuthType)
    if memberSslMode in ["VERIFY_CA", "VERIFY_IDENTITY"]:
        EXPECT_THROWS(lambda: c.add_instance(__sandbox_uri4, cert_subject),
                      "Authentication error during connection check", "add_instance@sb4")
    elif memberAuthType in ["CERT_ISSUER", "CERT_SUBJECT"]:
        # can't verify client certificate if the cert is not from a valid CA
        EXPECT_THROWS(lambda: c.add_instance(__sandbox_uri4, cert_subject),
                      "Authentication error during connection check", "add_instance@sb4")
        EXPECT_STDOUT_CONTAINS(
            f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port4}' was issued by '/CN=TestCertIssuer' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port4}' and that the value specified for certIssuer itself is correct.")
    else:
        EXPECT_NO_THROWS(lambda: c.add_instance(
            __sandbox_uri4, cert_subject), "add_instance@sb4")
        c.remove_instance(__sandbox_uri4)
    WIPE_OUTPUT()
    print("\n###### addInstance@sb5", memberSslMode, memberAuthType)
    if memberSslMode in ["VERIFY_IDENTITY"]:
        EXPECT_THROWS(lambda: c.add_instance(__sandbox_uri5, cert_subject),
                      "SSL connection check error", "add_instance@sb5")
    elif memberAuthType in ["CERT_SUBJECT"]:
        # can't verify client certificate if the cert is not from a valid CA
        EXPECT_THROWS(lambda: c.add_instance(__sandbox_uri5, cert_subject),
                      "Authentication error during connection check", "add_instance@sb4")
        EXPECT_STDOUT_CONTAINS(
            f"ERROR: Authentication error during connection check. Please ensure that the SSL certificate (@@ssl_cert) configured for '{hostname}:{__mysql_sandbox_port5}' was issued by '/CN=TestCertIssuer' (certIssuer), is recognized at '{hostname}:{__mysql_sandbox_port5}' and that the value specified for certIssuer itself is correct.")
    else:
        EXPECT_NO_THROWS(lambda: c.add_instance(
            __sandbox_uri5, cert_subject), "add_instance@sb5")
        c.remove_instance(__sandbox_uri5)
        shell.connect(__sandbox_uri5)
        reset_instance(session)
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


if __version_num >= 80027:
    gr_port1 = __mysql_sandbox_port1
    gr_port2 = __mysql_sandbox_port2
else:
    gr_port1 = __mysql_sandbox_port1*10+1
    gr_port2 = __mysql_sandbox_port2*10+1

def TEST_COMBINATION_LOCAL_ADDRESS(memberSslMode, memberAuthType):
    cert_issuer = {}
    cert_subject = {}
    local_address1 = {"localAddress":f"{hostname}:{gr_port1}"}
    local_address2 = {"localAddress":f"{hostname}:{gr_port2}"}
    if memberAuthType != "PASSWORD":
        cert_issuer = {"certIssuer": "/CN=TestCertIssuer"}
    if memberAuthType == "CERT_SUBJECT":
        cert_subject = {"certSubject": f"/CN={hostname}"}
    print("\n###### createCluster@sb1 localAddress", memberSslMode, memberAuthType)
    shell.connect(__sandbox_uri1)
    EXPECT_THROWS(lambda:dba.create_cluster("cluster", merge({"gtidSetIsComplete": 1, "memberAuthType": memberAuthType,
                           "memberSslMode": memberSslMode}, cert_issuer, cert_subject, local_address1)), "Server address configuration error", "create")
    EXPECT_STDOUT_CONTAINS(f"ERROR: MySQL server at '198.51.100.100:{__mysql_sandbox_port1}' can't connect to '198.51.100.100'")
    WIPE_OUTPUT()
    print("\n###### addInstance@sb2 localAddress", memberSslMode, memberAuthType)
    shell.connect(__sandbox_uri3)
    c=dba.create_cluster("cluster", merge({"gtidSetIsComplete": 1, "memberAuthType": memberAuthType,
                           "memberSslMode": memberSslMode}, cert_issuer, cert_subject))
    WIPE_OUTPUT()
    EXPECT_THROWS(lambda:c.add_instance(__sandbox_uri2, merge(cert_subject, local_address2)), "Server address configuration error", "add")
    EXPECT_STDOUT_CONTAINS(f"ERROR: MySQL server at '{hostname}:{__mysql_sandbox_port3}' can't connect to '198.51.100.100'")
    WIPE_OUTPUT()

#@<> report_host is wrong
# 198.51.100.0/24 is reserved test address
testutil.change_sandbox_conf(__mysql_sandbox_port1, "report_host", "198.51.100.100")
testutil.restart_sandbox(__mysql_sandbox_port1)

testutil.change_sandbox_conf(__mysql_sandbox_port2, "report_host", "198.51.100.100")
testutil.restart_sandbox(__mysql_sandbox_port2)

shell.connect(__sandbox_uri2)
shell.options['dba.connectivityChecks'] = False
EXPECT_THROWS(lambda: dba.create_cluster("replica"),
              "Group Replication failed to start: ")
shell.options['dba.connectivityChecks'] = True

EXPECT_STDOUT_CONTAINS(
    f"This instance reports its own address as 198.51.100.100:{__mysql_sandbox_port2}")

reset_all()

#@<> gr_local_address DISABLED, PASSWORD
TEST_COMBINATION_LOCAL_ADDRESS("DISABLED", "PASSWORD")

reset_all()

#@<> gr_local_address REQUIRED, PASSWORD
TEST_COMBINATION_LOCAL_ADDRESS("REQUIRED", "PASSWORD")

reset_all()

#@<> gr_local_address VERIFY_CA, CERT_ISSUER
TEST_COMBINATION_LOCAL_ADDRESS("VERIFY_CA", "CERT_ISSUER")

reset_all()

#@<> gr_local_address VERIFY_IDENTITY, CERT_SUBJECT
TEST_COMBINATION_LOCAL_ADDRESS("VERIFY_IDENTITY", "CERT_SUBJECT")

reset_all()

#@<> restore report_host
testutil.change_sandbox_conf(__mysql_sandbox_port1, "report_host", hostname)
testutil.restart_sandbox(__mysql_sandbox_port1)

testutil.change_sandbox_conf(__mysql_sandbox_port2, "report_host", hostname)
testutil.restart_sandbox(__mysql_sandbox_port2)

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
# memberAuthType - PASSWORD, CERT_ISSUER, CERT_SUBJECT

#@<> DISABLED, PASSWORD

TEST_COMBINATION("DISABLED", "PASSWORD")

reset_all()

#@<> Tests: REQUIRED, PASSWORD

TEST_COMBINATION("REQUIRED", "PASSWORD")

reset_all()

#@<> Tests: REQUIRED, CERT_ISSUER

TEST_COMBINATION("REQUIRED", "CERT_ISSUER")

reset_all()

#@<> Tests: REQUIRED, CERT_SUBJECT

TEST_COMBINATION("REQUIRED", "CERT_SUBJECT")

reset_all()

#@<> Tests: VERIFY_CA, CERT_ISSUER

TEST_COMBINATION("VERIFY_CA", "CERT_ISSUER")

reset_all()

#@<> Tests: VERIFY_CA, CERT_SUBJECT

TEST_COMBINATION("VERIFY_CA", "CERT_SUBJECT")

reset_all()

#@<> Tests: VERIFY_IDENTITY, CERT_ISSUER

TEST_COMBINATION("VERIFY_IDENTITY", "CERT_ISSUER")

reset_all()

#@<> Tests: VERIFY_IDENTITY, CERT_SUBJECT

TEST_COMBINATION("VERIFY_IDENTITY", "CERT_SUBJECT")

# Must ensure certificate is valid even if connecting through local_address

#@<> Cleanup
if os.path.exists(ssldir):
    shutil.rmtree(ssldir)
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.destroy_sandbox(__mysql_sandbox_port3)
testutil.destroy_sandbox(__mysql_sandbox_port4)
testutil.destroy_sandbox(__mysql_sandbox_port5)
