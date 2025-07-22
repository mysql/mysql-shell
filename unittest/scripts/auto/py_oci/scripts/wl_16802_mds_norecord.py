#@ {has_oci_environment('MDS')}

#@<> INCLUDE dump_utils.inc

#@<> INCLUDE oci_utils.inc

#@<> entry point
# imports
from collections.abc import Callable
import copy
import os
import os.path
import random
import re

# types
class Bucket_info:
    def __init__(self, url: str, options: dict):
        self.url = url
        self.options = options
        self.osBucketName = options["osBucketName"]
        self.osNamespace = options["osNamespace"]
        self.ociConfigFile = options["ociConfigFile"]

class Test_case:
    def __init__(self, url: str, options: dict, *, error: str = "",
                 output: str = "", not_output: str = "",
                 setup: Callable[[], None] = None,
                 cleanup: Callable[[], None] = None):
        self.url = url
        self.options = options
        self.error = error
        self.output = output
        self.not_output = not_output
        self.setup = setup
        self.cleanup = cleanup

class Dump_test_case(Test_case):
    pass

class Load_test_case(Test_case):
    def __init__(self, url: str, options: dict, *,
                 expect_heatwave_loaded: bool = False, error: str = "",
                 output: str = "", not_output: str = "",
                 setup: Callable[[], None] = None,
                 cleanup: Callable[[], None] = None):
        super().__init__(url, options, error=error, output=output, not_output=not_output, setup=setup, cleanup=cleanup)
        self.expect_heatwave_loaded = expect_heatwave_loaded

# constants
class Load_errors:
    unknown_location = "Cannot load the InnoDB based vector store tables as regular tables, location of their data is not known."
    not_mhs = "Cannot convert the InnoDB based vector store tables to Lakehouse, target instance is not a HeatWave instance."
    unsupported_mhs = "Cannot convert the InnoDB based vector store tables to Lakehouse, target HeatWave instance needs to be 9.4.1 or newer."
    non_oci_location = "Cannot convert the InnoDB based vector store tables to Lakehouse, their data is not stored in OCI's Object Storage."

run_mhs_tests = True
run_lakehouse_tests = has_oci_environment('MDS+LH')

test_schema = "vs-wl16802"

local_dump = os.path.join(__tmp_dir, "wl16802-dump")
local_vector_store_dir = "wl16802-vector-store"
local_vector_store_dump = os.path.join(__tmp_dir, local_vector_store_dir)

dump_bucket = Bucket_info("mysqlsh-ut-wl16802-dump", {
    "osBucketName": OS_BUCKET_NAME,
    "osNamespace": OS_NAMESPACE,
    "ociConfigFile": oci_config_file,
})

vector_store_bucket_prefix = "mysqlsh-ut-wl16802-vector-store"
vector_store_bucket = Bucket_info(vector_store_bucket_prefix, {
    "osBucketName": MDS_LH_BUCKET if run_lakehouse_tests else OS_BUCKET_NAME,
    "osNamespace": OS_NAMESPACE,
    "ociConfigFile": oci_config_file,
})

dump_par, dump_par_id = create_par_with_id(
    dump_bucket.osNamespace,
    dump_bucket.osBucketName,
    "AnyObjectReadWrite",
    "wl16802-dump-par",
    today_plus_days(1, True),
    f"{dump_bucket.url}/",
    "ListObjects"
)

vector_store_par, vector_store_par_id = create_par_with_id(
    vector_store_bucket.osNamespace,
    vector_store_bucket.osBucketName,
    "AnyObjectReadWrite",
    "wl16802-vector-store-par",
    today_plus_days(1, True),
    f"{vector_store_bucket.url}/",
    "ListObjects"
)

# helpers
def clean_instance(s):
    s.run_sql("DROP SCHEMA IF EXISTS !", [test_schema])

def random_float():
    i = int.from_bytes(random.randbytes(4))
    if random_float.mask == i & random_float.mask or 0 == i & random_float.mask:
        # all ones - it's a NaN or Inf, all zeros - it's a subnormal value, fix it
        exp = random.randint(1, 254) << 23
        i = i & ~random_float.mask | exp
    # swap bytes to use order expected by MySQL
    return hex(int.from_bytes(i.to_bytes(length=4, byteorder='little')))[2:].zfill(8)

random_float.mask = 0x7F800000

def random_vector(entries):
    return '0x' + ''.join([random_float() for i in range(random.randint(1, entries))])

def supports_vector_store(s):
    return testutil.version_check(s.run_sql('SELECT @@version').fetch_one()[0], ">=", "9.4.1")

def wipe_local_dump():
    wipe_dir(local_dump)

def wipe_local_vector_store_dir():
    wipe_dir(local_vector_store_dir)

def wipe_local_vector_store_dump():
    wipe_dir(local_vector_store_dump)

def wipe_bucket(b: Bucket_info):
    wipeout_bucket_mt(10, b.osBucketName, b.osNamespace, b.ociConfigFile, False, False, b.url)

def wipe_dump_bucket():
    wipe_bucket(dump_bucket)

def wipe_vector_store_bucket():
    wipe_bucket(vector_store_bucket)

def wipe_all():
    wipe_local_dump()
    wipe_local_vector_store_dump()
    wipe_dump_bucket()
    wipe_vector_store_bucket()
    if mhs_session:
        clean_instance(mhs_session)
    if lh_session:
        clean_instance(lh_session)

def dumper_test(tc: Dump_test_case):
    if "ocimds" not in tc.options:
        tc.options["ocimds"] = True
    if "compatibility" not in tc.options:
        tc.options["compatibility"] = ["ignore_missing_pks"]
    if "showProgress" not in tc.options:
        tc.options["showProgress"] = False
    if tc.setup:
        tc.setup()
    PREPARE_PAR_IS_SECRET_TEST()
    if tc.error:
        EXPECT_THROWS(lambda: util.dump_schemas([test_schema], tc.url, tc.options), tc.error)
    else:
        EXPECT_NO_THROWS(lambda: util.dump_schemas([test_schema], tc.url, tc.options), "dump_schemas() should not throw")
    EXPECT_PAR_IS_SECRET()
    if tc.output:
        EXPECT_STDOUT_CONTAINS(tc.output)
    if tc.not_output:
        EXPECT_STDOUT_NOT_CONTAINS(tc.not_output)
    if tc.cleanup:
        tc.cleanup()

def loader_test(tc: Load_test_case):
    if "resetProgress" not in tc.options:
        tc.options["resetProgress"] = True
    if "showProgress" not in tc.options:
        tc.options["showProgress"] = False
    if tc.setup:
        tc.setup()
    PREPARE_PAR_IS_SECRET_TEST()
    if tc.error:
        EXPECT_THROWS(lambda: util.load_dump(tc.url, tc.options), tc.error)
    else:
        EXPECT_NO_THROWS(lambda: util.load_dump(tc.url, tc.options), "load_dump() should not throw")
        if not tc.expect_heatwave_loaded is None:
            if tc.expect_heatwave_loaded:
                EXPECT_STDOUT_CONTAINS(loader_test.tables_were_loaded_msg)
            else:
                EXPECT_STDOUT_NOT_CONTAINS(loader_test.tables_were_loaded_msg)
    EXPECT_PAR_IS_SECRET()
    if tc.output:
        EXPECT_STDOUT_CONTAINS(tc.output)
    if tc.not_output:
        EXPECT_STDOUT_NOT_CONTAINS(tc.not_output)
    if tc.cleanup:
        tc.cleanup()

loader_test.tables_were_loaded_msg = "were loaded into HeatWave cluster"

def setup_local_test(tc: Load_test_case):
    tc.expect_heatwave_loaded = False
    tc.setup=lambda: [shell.connect(__sandbox_uri2)]
    tc.cleanup=lambda: [clean_instance(local_session)]
    return tc

def setup_mhs_test(tc: Load_test_case):
    tc.expect_heatwave_loaded = False
    tc.setup=lambda: [shell.connect(MDS_URI)]
    tc.cleanup=lambda: [clean_instance(mhs_session)]
    return tc

def setup_lakehouse_test(tc: Load_test_case, auto_heatwave_loaded: bool = True):
    heatwave_loaded = None
    if auto_heatwave_loaded:
        heatwave_loaded = True
        if not tc.options.get("convertInnoDbVectorStore", "auto") in ["auto", "convert"]:
            heatwave_loaded = False
        if tc.options.get("heatwaveLoad", "vector_store") != "vector_store":
            heatwave_loaded = False
        if not tc.options.get("loadData", True):
            heatwave_loaded = False
    tc.expect_heatwave_loaded = heatwave_loaded
    tc.setup=lambda: [shell.connect(MDS_LH_URI)]
    tc.cleanup=lambda: [clean_instance(lh_session)]
    return tc

all_sessions = []

def open_sesion(uri):
    s = mysql.get_session(uri)
    global all_sessions
    all_sessions.append(s)
    return s

class Cross_check_tests:
    def __init__(self):
        self.__tests = []
    def add_dumper_test(self, tc: Dump_test_case, desc: str):
        if "checksum" not in tc.options:
            tc.options["checksum"] = True
        # dumper tests + all loader tests for this dump
        self.__tests.append(((tc, desc), []))
    def add_loader_test(self, tc: Load_test_case, desc: str):
        # appends a loader test to the last dump
        self.__tests[-1][1].append((tc, desc))
    def add_local_test(self, tc: Load_test_case, desc: str):
        self.add_loader_test(setup_local_test(tc), desc)
    def add_mhs_test(self, tc: Load_test_case, desc: str):
        if run_mhs_tests:
            self.add_loader_test(setup_mhs_test(tc), desc)
    def add_lh_test(self, tc: Load_test_case, desc: str):
        if run_lakehouse_tests:
            setup_lakehouse_test(tc)
            self.add_loader_test(setup_lakehouse_test(tc), desc)
    def add_remote_tests(self, tc: Load_test_case, desc: str):
        self.add_mhs_test(copy.deepcopy(tc), desc + " (MHS)")
        self.add_lh_test(tc, desc + " (LH)")
    def add_convert_vector_store_tests(self, tc: Load_test_case, name: str, relative_location: bool, remote_location: bool):
        def describe(test):
            return f"load of {name} - 'convertInnoDbVectorStore': '{t.options['convertInnoDbVectorStore']}' - {'error' if t.error else 'success'}"
        t = copy.deepcopy(tc)
        t.options["convertInnoDbVectorStore"] = "keep"
        t.error = "" if relative_location else Load_errors.unknown_location
        if "checksum" not in t.options:
            t.options["checksum"] = True
        self.add_local_test(t, "local " + describe(t))
        t = copy.deepcopy(tc)
        t.options["convertInnoDbVectorStore"] = "skip"
        t.error = ""
        self.add_local_test(t, "local " + describe(t))
        t = tc
        t.options["convertInnoDbVectorStore"] = "convert"
        t.error = "" if remote_location else Load_errors.non_oci_location
        self.add_remote_tests(tc, "remote " + describe(t))
    def run(self):
        for d in self.__tests:
            tc = d[0][0]
            desc = d[0][1]
            # call the cleanup once all the loader tests are finished
            cleanup = tc.cleanup
            tc.cleanup = None
            print(f"-> Running dumper test '{desc}':", self.__describe(tc))
            dumper_test(tc)
            for tc, desc in d[1]:
                print(f"-> Running loader test '{desc}':", self.__describe(tc))
                loader_test(tc)
            if cleanup:
                cleanup()
    def __describe(self, tc: Test_case):
        s = f"url: '{tc.url}'"
        if "osBucketName" in tc.options:
            s += f" (bucket: '{tc.options['osBucketName']}')"
        if "lakehouseTarget" in tc.options:
            target = tc.options["lakehouseTarget"]
            s += f", lakehouseTarget.url: '{target['outputUrl']}'"
            if "osBucketName" in target:
                s += f" (bucket: '{target['osBucketName']}')"
        if "lakehouseSource" in tc.options:
            source = tc.options["lakehouseSource"]
            if "par" in source:
                s += f", lakehouseSource.par: '{source['par']}'"
            if "bucket" in source:
                s += f", lakehouseSource.bucket: '{source['bucket']}', lakehouseSource.prefix: '{source['prefix']}'"
        return s

#@<> Setup
testutil.deploy_sandbox(__mysql_sandbox_port1, "root", { "local_infile": 1 })
testutil.wait_sandbox_alive(__mysql_sandbox_port1)

testutil.deploy_sandbox(__mysql_sandbox_port2, "root", { "local_infile": 1 })
testutil.wait_sandbox_alive(__mysql_sandbox_port2)

dump_session = open_sesion(__sandbox_uri1)
local_session = open_sesion(__sandbox_uri2)
mhs_session = open_sesion(MDS_URI)

if not supports_vector_store(mhs_session):
    run_mhs_tests = False
    mhs_session.close()
    mhs_session = None

if run_lakehouse_tests:
    lh_session = open_sesion(MDS_LH_URI)
    if not supports_vector_store(lh_session):
        run_lakehouse_tests = False
        lh_session.close()
        lh_session = None
else:
    lh_session = None

# create tables
dump_session.run_sql("CREATE SCHEMA !", [test_schema])

dump_session.run_sql("""CREATE TABLE !.genai_1 (
`v` vector(8) DEFAULT NULL COMMENT 'GENAI_OPTIONS=EMBED_MODEL_ID=model'
) ENGINE=InnoDB SECONDARY_ENGINE=rapid
""", [test_schema])

dump_session.run_sql("""CREATE TABLE !.genai_2 (
`v` vector(8) DEFAULT NULL COMMENT 'GENAI_OPTIONS=EMBED_MODEL_ID=model',
`key` int NOT NULL,
UNIQUE KEY `key` (`key`)
) ENGINE=InnoDB SECONDARY_ENGINE=rapid
""", [test_schema])

dump_session.run_sql("""CREATE TABLE !.genai_3 (
`id` int NOT NULL,
`v` vector(8) DEFAULT NULL COMMENT 'GENAI_OPTIONS=EMBED_MODEL_ID=model',
`key` int NOT NULL,
PRIMARY KEY (`id`),
UNIQUE KEY `key` (`key`)
) ENGINE=InnoDB SECONDARY_ENGINE=rapid
""", [test_schema])

dump_session.run_sql("""CREATE TABLE !.genai_empty (
`v` vector(8) DEFAULT NULL COMMENT 'GENAI_OPTIONS=EMBED_MODEL_ID=model'
) ENGINE=InnoDB SECONDARY_ENGINE=rapid
""", [test_schema])

dump_session.run_sql("""CREATE TABLE !.noai (
`id` int NOT NULL AUTO_INCREMENT,
`v` vector(8) DEFAULT NULL,
`key` int NOT NULL,
PRIMARY KEY (`id`),
UNIQUE KEY `key` (`key`)
) ENGINE=InnoDB SECONDARY_ENGINE=rapid
""", [test_schema])

row_count = 1000

dump_session.run_sql(f"INSERT INTO !.noai (`v`, `key`) VALUES {','.join(['({0}, {1})'.format(random_vector(8), i) for i in range(row_count)])}", [test_schema])
dump_session.run_sql(f"INSERT INTO !.genai_3 (`id`, `v`, `key`) SELECT `id`, `v`, `key` from !.noai", [test_schema, test_schema])
dump_session.run_sql(f"INSERT INTO !.genai_2 (`v`, `key`) SELECT `v`, `key` from !.noai", [test_schema, test_schema])
dump_session.run_sql(f"INSERT INTO !.genai_1 (`v`) SELECT `v` from !.noai", [test_schema, test_schema])

dump_session.run_sql(f"ANALYZE TABLE !.genai_1", [test_schema])
dump_session.run_sql(f"ANALYZE TABLE !.genai_2", [test_schema])
dump_session.run_sql(f"ANALYZE TABLE !.genai_3", [test_schema])
dump_session.run_sql(f"ANALYZE TABLE !.noai", [test_schema])

# wipe everything
wipe_all()

#### cross-check tests
run_cct_tests = True

# WL16802-TSFR_2_1_4_3_1, WL16802-TSFR_2_1_7_3 - load into MHS and LH instances
# is done with 'convertInnoDbVectorStore' set to 'convert', other tests verify
# that 'auto' is the same as 'convert' in this case

#@<> 1. dump to a local directory {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    local_dump,
    {},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_local_dump()],
), "1. dump to a local directory")

## load of 1.
cct.add_convert_vector_store_tests(Load_test_case(
    local_dump,
    {},
), "1.", True, False)

cct.run()

#@<> 2. dump to a local directory, lakehouseTarget.outputUrl is set {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": local_vector_store_dir}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_local_dump(), wipe_local_vector_store_dir()],
), "2. dump to a local directory, lakehouseTarget.outputUrl is set")

## load of 2.
cct.add_convert_vector_store_tests(Load_test_case(
    local_dump,
    {},
), "2.", True, False)

cct.run()

#@<> 3. dump to a local directory, 'lakehouseTarget' is set to a bucket {run_cct_tests}
cct = Cross_check_tests()

# WL16802-TSFR_1_2_1_1_1 - bucket
cct.add_dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": vector_store_bucket.url, **vector_store_bucket.options}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_local_dump(), wipe_vector_store_bucket()],
), "3. dump to a local directory, 'lakehouseTarget' is set to a bucket")

## load of 3.
cct.add_convert_vector_store_tests(Load_test_case(
    local_dump,
    {},
), "3.", False, True)

cct.run()

#@<> 4. dump to a local directory, 'lakehouseTarget' is set to a PAR {run_cct_tests}
cct = Cross_check_tests()

# WL16802-TSFR_1_2_1_1_1 - PAR
cct.add_dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": vector_store_par}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_local_dump(), wipe_vector_store_bucket()],
), "4. dump to a local directory, 'lakehouseTarget' is set to a PAR")

## load of 4.
cct.add_convert_vector_store_tests(Load_test_case(
    local_dump,
    {},
), "4.", False, True)

cct.run()

#@<> 5. dump to a bucket {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    vector_store_bucket.url,
    {**vector_store_bucket.options},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_vector_store_bucket()],
), "5. dump to a bucket")

## load of 5.
cct.add_convert_vector_store_tests(Load_test_case(
    vector_store_bucket.url,
    {**vector_store_bucket.options},
), "5.", True, True)

cct.run()

#@<> 6. dump to a bucket, lakehouseTarget.outputUrl is set {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    vector_store_bucket.url,
    {**vector_store_bucket.options, "lakehouseTarget": {"outputUrl": vector_store_bucket_prefix + "/vector-store-data"}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_vector_store_bucket()],
), "6. dump to a bucket, lakehouseTarget.outputUrl is set")

## load of 6.
cct.add_convert_vector_store_tests(Load_test_case(
    vector_store_bucket.url,
    {**vector_store_bucket.options},
), "6.", True, True)

cct.run()

#@<> 7. dump to a bucket, 'lakehouseTarget' is set to another bucket {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    dump_bucket.url,
    {**dump_bucket.options, "lakehouseTarget": {"outputUrl": vector_store_bucket.url, **vector_store_bucket.options}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_dump_bucket(), wipe_vector_store_bucket()],
), "7. dump to a bucket, 'lakehouseTarget' is set to another bucket")

## load of 7.
cct.add_convert_vector_store_tests(Load_test_case(
    dump_bucket.url,
    {**dump_bucket.options},
), "7.", False, True)

cct.run()

#@<> 8. dump to a bucket, 'lakehouseTarget' is set to a PAR {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    dump_bucket.url,
    {**dump_bucket.options, "lakehouseTarget": {"outputUrl": vector_store_par}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_dump_bucket(), wipe_vector_store_bucket()],
), "8. dump to a bucket, 'lakehouseTarget' is set to a PAR")

## load of 8.
cct.add_convert_vector_store_tests(Load_test_case(
    dump_bucket.url,
    {**dump_bucket.options},
), "8.", False, True)

cct.run()

#@<> 9. dump to a PAR {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    vector_store_par,
    {},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_vector_store_bucket()],
), "9. dump to a PAR")

## load of 9.
cct.add_convert_vector_store_tests(Load_test_case(
    vector_store_par,
    {"progressFile": ""},
), "9.", True, True)

cct.run()

#@<> 10. dump to a PAR, lakehouseTarget.outputUrl is set {run_cct_tests}
cct = Cross_check_tests()

# WL16802-TSFR_1_2_1_3_1
cct.add_dumper_test(Dump_test_case(
    vector_store_par,
    {"lakehouseTarget": {"outputUrl": vector_store_bucket_prefix + "/vector-store-data"}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_vector_store_bucket()],
), "10. dump to a PAR, lakehouseTarget.outputUrl is set")

## load of 10.
cct.add_convert_vector_store_tests(Load_test_case(
    vector_store_par,
    {"progressFile": ""},
), "10.", True, True)

cct.run()

#@<> 11. dump to a PAR, 'lakehouseTarget' is set to a bucket {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    dump_par,
    {"lakehouseTarget": {"outputUrl": vector_store_bucket.url, **vector_store_bucket.options}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_dump_bucket(), wipe_vector_store_bucket()],
), "11. dump to a PAR, 'lakehouseTarget' is set to a bucket")

## load of 11.
cct.add_convert_vector_store_tests(Load_test_case(
    dump_par,
    {"progressFile": ""},
), "11.", False, True)

cct.run()

#@<> 12. dump to a PAR, 'lakehouseTarget' is set to another PAR {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    dump_par,
    {"lakehouseTarget": {"outputUrl": vector_store_par}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_dump_bucket(), wipe_vector_store_bucket()],
), "12. dump to a PAR, 'lakehouseTarget' is set to another PAR")

## load of 12.
cct.add_convert_vector_store_tests(Load_test_case(
    dump_par,
    {"progressFile": ""},
), "12.", False, True)

cct.run()

#@<> bucket PAR, we assume it's safe to delete everything from the OS_BUCKET_NAME bucket {run_cct_tests}
bucket_par, bucket_par_id = create_par_with_id(
    OS_NAMESPACE,
    OS_BUCKET_NAME,
    "AnyObjectReadWrite",
    "wl16802-bucket-par",
    today_plus_days(1, True),
    None,
    "ListObjects"
)

def wipe_bucket_par():
    wipeout_bucket_mt(10, OS_BUCKET_NAME, OS_NAMESPACE, oci_config_file, False, False, None)

#@<> wipe the bucket PAR {run_cct_tests}
wipe_bucket_par()

#@<> 13. dump to a bucket PAR {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    bucket_par,
    {},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_bucket_par()],
), "13. dump to a bucket PAR")

## load of 13.
cct.add_convert_vector_store_tests(Load_test_case(
    bucket_par,
    {"lakehouseSource": {"par": bucket_par + "vector-store/"}, "progressFile": ""},
), "13.", True, True)

cct.run()

#@<> 14. dump to a bucket PAR, lakehouseTarget.outputUrl is set {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    bucket_par,
    {"lakehouseTarget": {"outputUrl": vector_store_bucket_prefix}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_bucket_par()],
), "14. dump to a bucket PAR, lakehouseTarget.outputUrl is set")

## load of 14.
cct.add_convert_vector_store_tests(Load_test_case(
    bucket_par,
    {"lakehouseSource": {"par": bucket_par + vector_store_bucket_prefix}, "progressFile": ""},
), "14.", True, True)

cct.run()

#@<> 15. dump to a bucket PAR, 'lakehouseTarget' is set to a bucket {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    bucket_par,
    {"lakehouseTarget": {"outputUrl": vector_store_bucket.url, **vector_store_bucket.options}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_bucket_par(), wipe_vector_store_bucket()],
), "15. dump to a bucket PAR, 'lakehouseTarget' is set to a bucket")

## load of 15.
cct.add_convert_vector_store_tests(Load_test_case(
    bucket_par,
    {"progressFile": ""},
), "15.", False, True)

cct.run()

#@<> 16. dump to a bucket PAR, 'lakehouseTarget' is set to another PAR {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    bucket_par,
    {"lakehouseTarget": {"outputUrl": vector_store_par}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_bucket_par(), wipe_vector_store_bucket()],
), "16. dump to a bucket PAR, 'lakehouseTarget' is set to another PAR")

## load of 16.
cct.add_convert_vector_store_tests(Load_test_case(
    bucket_par,
    {"progressFile": ""},
), "16.", False, True)

cct.run()

#@<> 17. dump to a bucket, 'lakehouseTarget' is set to a bucket PAR {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    vector_store_bucket.url,
    {**vector_store_bucket.options, "lakehouseTarget": {"outputUrl": bucket_par}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_bucket_par(), wipe_vector_store_bucket()],
), "17. dump to a bucket, 'lakehouseTarget' is set to a bucket PAR")

## load of 17.
cct.add_convert_vector_store_tests(Load_test_case(
    vector_store_bucket.url,
    {**vector_store_bucket.options, "lakehouseSource": {"par": bucket_par}, "progressFile": ""},
), "17.", False, True)

cct.run()

#@<> 18. dump to a PAR, 'lakehouseTarget' is set to a bucket PAR {run_cct_tests}
cct = Cross_check_tests()

cct.add_dumper_test(Dump_test_case(
    vector_store_par,
    {"lakehouseTarget": {"outputUrl": bucket_par}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_bucket_par(), wipe_vector_store_bucket()],
), "18. dump to a PAR, 'lakehouseTarget' is set a bucket PAR")

## load of 18.
cct.add_convert_vector_store_tests(Load_test_case(
    vector_store_par,
    {"lakehouseSource": {"par": bucket_par}, "progressFile": ""},
), "18.", False, True)

cct.run()

#### end of bucket PAR tests

#@<> cleanup cross-check tests {run_cct_tests}
delete_par(
    OS_NAMESPACE,
    OS_BUCKET_NAME,
    bucket_par_id
)
del bucket_par
del wipe_bucket_par

#@<> WL16802-TSFR_1 - dump vector store tables without 'lakehouseTarget', verify contents
dumper_test(Dump_test_case(
    local_dump,
    {},
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

main_dump_dir = local_dump
dump_location = os.path.join(main_dump_dir, "dump")
vector_store_location = os.path.join(main_dump_dir, "vector-store")

# main directory has a @.json file with redirection capability
EXPECT_CAPABILITIES(os.path.join(main_dump_dir, "@.json"), [ dump_dir_redirection_capability ])
## WL16802-FR1.1.1
# WL16802-FR1.1.6 - dump directory has a @.json file with vector store capability
EXPECT_CAPABILITIES(os.path.join(dump_location, "@.json"), [ innodb_vector_store_capability ])
# dump directory has metadata files of vector store tables
EXPECT_TRUE(os.path.isfile(os.path.join(dump_location, encode_table_basename(test_schema, "genai_1") + ".json")))
# dump directory has SQL files of vector store tables
EXPECT_TRUE(os.path.isfile(os.path.join(dump_location, encode_table_basename(test_schema, "genai_1") + ".sql")))
# WL16802-FR1.1.5 - vector store directory has data files of vector store tables, which are using the same format as regular data files
EXPECT_TRUE(os.path.isfile(os.path.join(vector_store_location, encode_table_basename(test_schema, "genai_1") + "@0.csv.gz")))

# WL16802-FR1.1.2 - all locations are printed on screen
EXPECT_STDOUT_CONTAINS(f"""
Main dump directory: '{main_dump_dir}'
Dump location: '{dump_location}'
Vector store location: '{vector_store_location}'
""")

# validate dialect (csv-unix - WL16802-FR1.1.3) and compression (gzip - WL16802-FR1.1.4)
metadata = read_json(os.path.join(dump_location, encode_table_basename(test_schema, "genai_1") + ".json"))
EXPECT_EQ("gzip", metadata["compression"])
EXPECT_EQ(",", metadata["options"]["fieldsTerminatedBy"])
EXPECT_EQ("\"", metadata["options"]["fieldsEnclosedBy"])
EXPECT_EQ(False, metadata["options"]["fieldsOptionallyEnclosed"])
EXPECT_EQ("\\", metadata["options"]["fieldsEscapedBy"])
EXPECT_EQ("\n", metadata["options"]["linesTerminatedBy"])

#@<> WL16802-TSFR_1 - cleanup
wipe_local_dump()

#@<> WL16802-TSFR_1_1_4 - dump vector store tables without 'lakehouseTarget', with 'compression':'none', verify contents
dumper_test(Dump_test_case(
    local_dump,
    {"compression": "none"},
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

main_dump_dir = local_dump
dump_location = os.path.join(main_dump_dir, "dump")
vector_store_location = os.path.join(main_dump_dir, "vector-store")

# WL16802-FR1.1.5 - vector store directory has data files of vector store tables, which are using the same format as regular data files
EXPECT_TRUE(os.path.isfile(os.path.join(vector_store_location, encode_table_basename(test_schema, "genai_1") + "@0.csv")))

# validate compression (none - WL16802-FR1.1.4)
metadata = read_json(os.path.join(dump_location, encode_table_basename(test_schema, "genai_1") + ".json"))
EXPECT_EQ("none", metadata["compression"])

#@<> WL16802-TSFR_1_1_4 - cleanup
wipe_local_dump()

#@<> WL16802-TSFR_1_1_6 - dump without vector store tables
dumper_test(Dump_test_case(
    local_dump,
    {"includeTables": [quote_identifier(test_schema, "noai")]},
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

metadata_file = os.path.join(local_dump, "@.json")

# no redirection
EXPECT_NO_CAPABILITIES(metadata_file, [ dump_dir_redirection_capability ])
# no vector store capability
EXPECT_NO_CAPABILITIES(metadata_file, [ innodb_vector_store_capability ])

#@<> WL16802-TSFR_1_1_6 - cleanup
wipe_local_dump()

#@<> WL16802-TSFR_1_2_1_1 - unsupported key in 'lakehouseTarget'
shell.connect(__sandbox_uri1)

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"unknown": "unknown"}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' Invalid and missing options (invalid: unknown), (missing: outputUrl)",
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": "url", "unknown": "unknown"}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' Invalid options: unknown",
))

#@<> WL16802-TSFR_1_2_1_2 - outputUrl key missing in 'lakehouseTarget'
dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' Missing required options: outputUrl",
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

#@<> WL16802-TSFR_1_2_1_2_1 - invalid values in 'lakehouseTarget'
shell.connect(__sandbox_uri1)

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": []}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' Option 'outputUrl' is expected to be of type String, but is Array",
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": "url", "osBucketName": []}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' Option 'osBucketName' is expected to be of type String, but is Array",
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": "url", "osNamespace": "value"}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' The option 'osNamespace' cannot be used when the value of 'osBucketName' option is not set.",
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": "url", "ociConfigFile": "value"}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' The option 'ociConfigFile' cannot be used when the value of 'osBucketName' option is not set.",
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": "url", "ociProfile": "value"}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' The option 'ociProfile' cannot be used when the value of 'osBucketName' option is not set.",
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": "url", "ociAuth": "api_key"}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' The option 'ociAuth' cannot be used when the value of 'osBucketName' option is not set.",
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": "url", "osBucketName": "value", "osNamespace": []}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' Option 'osNamespace' is expected to be of type String, but is Array",
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": "url", "osBucketName": "value", "ociConfigFile": []}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' Option 'ociConfigFile' is expected to be of type String, but is Array",
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": "url", "osBucketName": "value", "ociProfile": []}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' Option 'ociProfile' is expected to be of type String, but is Array",
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": "url", "osBucketName": "value", "ociAuth": []}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' Option 'ociAuth' is expected to be of type String, but is Array",
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": "url", "osBucketName": "value", "ociAuth": "unknown"}},
    error="TypeError: Argument #3: Option 'lakehouseTarget' Invalid value of 'ociAuth' option, expected one of: api_key, instance_obo_user, instance_principal, resource_principal, security_token, but got: unknown.",
))

#@<> WL16802-TSFR_1_2_2_1 - dump vector store tables with 'lakehouseTarget' set to null, verify vector store location
dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": None},
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

vector_store_location = os.path.join(local_dump, "vector-store")
EXPECT_TRUE(os.path.isfile(os.path.join(vector_store_location, encode_table_basename(test_schema, "genai_1") + "@0.csv.gz")))

#@<> WL16802-TSFR_1_2_2_1 - cleanup
wipe_local_dump()

#@<> WL16802-TSFR_1_2_3 - valid 'lakehouseTarget' with ocimds: false
dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": local_vector_store_dump}, "ocimds": False},
    error="ValueError: Argument #3: The 'ocimds' option must be enabled when using the 'lakehouseTarget' option.",
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

#@<> WL16802-TSFR_1_2_4 - valid 'lakehouseTarget' with ocimds: true but no vector data
dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": local_vector_store_dump}, "ocimds": True, "includeTables": [quote_identifier(test_schema, "noai")]},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    output="NOTE: No InnoDB-based vector store tables included in the dump, 'lakehouseTarget' option ignored.",
))

#@<> WL16802-TSFR_2_1_3 - 'convertInnoDbVectorStore' set when loading a dump without vector data
loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "keep"},
    output="NOTE: The 'convertInnoDbVectorStore' option is set, but dump does not contain InnoDB based vector store tables."
)))

#@<> WL16802-TSFR_2_2_2 - 'lakehouseSource' set when loading a dump without vector data
loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"bucket": "bucket", "namespace": "namespace", "region": "region"}},
    output="NOTE: The 'lakehouseSource' option is set, but dump does not contain InnoDB based vector store tables."
)))

#@<> 'heatwaveLoad' set when loading a dump without vector data
loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"heatwaveLoad": "none"},
    output="NOTE: The 'heatwaveLoad' option is set, but dump does not contain InnoDB based vector store tables."
)))

# nothing printed when 'convertInnoDbVectorStore' is not given
loader_test(setup_local_test(Load_test_case(
    local_dump,
    {},
    not_output="convertInnoDbVectorStore"
)))

#@<> WL16802-TSFR_1_2_4 - cleanup
wipe_local_dump()

#@<> WL16802-TSFR_1_2_5 - vector data stored in another directory
dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": local_vector_store_dir}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

EXPECT_TRUE(os.path.isfile(os.path.join(local_vector_store_dir, encode_table_basename(test_schema, "genai_1") + "@0.csv.gz")))

#@<> WL16802-TSFR_1_2_5 - cleanup
wipe_local_dump()
wipe_local_vector_store_dir()

#@<> WL16802-TSFR_2_1_1 - invalid value of 'convertInnoDbVectorStore'
loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "unknown"},
    error="ValueError: Argument #2: The value of the 'convertInnoDbVectorStore' option must be set to one of: 'auto', 'convert', 'keep', 'skip'.",
)))

#@<> remote dump into a single directory
# dump to a bucket, it's going to be loadable both into local and MHS instance
dumper_test(Dump_test_case(
    dump_bucket.url,
    {**dump_bucket.options},
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

#@<> WL16802-TSFR_2_1_2 - load into MHS instance, 'convertInnoDbVectorStore' is by default set to 'auto', tables are kept as is
# WL16802-TSFR_2_3_3_2 - 'heatwaveLoad' is ignored when loading into non-MHS instance
loader_test(setup_local_test(Load_test_case(
    dump_bucket.url,
    {**dump_bucket.options, "heatwaveLoad": "vector_store"},
    output="4 InnoDB-based vector store tables were loaded as regular tables.",
)))

EXPECT_SHELL_LOG_CONTAINS("The 'convertInnoDbVectorStore' option has been automatically set to: 'keep'")

#@<> WL16802-TSFR_2_1_2 - load into MHS instance, 'convertInnoDbVectorStore' is by default set to 'auto', tables are converted {run_mhs_tests}
loader_test(setup_mhs_test(Load_test_case(
    dump_bucket.url,
    {**dump_bucket.options},
    output="4 InnoDB-based vector store tables were converted to Lakehouse.",
)))

EXPECT_STDOUT_CONTAINS("The dump contains InnoDB based vector store tables, but target instance doesn't have the Lakehouse storage configured. Data will not be loaded into these tables.")
EXPECT_SHELL_LOG_CONTAINS("The 'convertInnoDbVectorStore' option has been automatically set to: 'convert'")

#@<> WL16802-TSFR_2_1_5 - load into MHS instance, 'convertInnoDbVectorStore' is set to 'skip' {run_mhs_tests}
loader_test(setup_mhs_test(Load_test_case(
    dump_bucket.url,
    {**dump_bucket.options, "convertInnoDbVectorStore": "skip"},
    output="4 InnoDB-based vector store tables were skipped.",
)))

#@<> WL16802-TSFR_2_1_6 - load into MHS instance, 'convertInnoDbVectorStore' is set to 'keep' {run_mhs_tests}
loader_test(setup_mhs_test(Load_test_case(
    dump_bucket.url,
    {**dump_bucket.options, "convertInnoDbVectorStore": "keep"},
    output="4 InnoDB-based vector store tables were loaded as regular tables.",
)))

#@<> WL16802-TSFR_2_2_1_2_1 - load into LH instance, location is not accessible by LH, data is not loaded {run_lakehouse_tests}
loader_test(setup_lakehouse_test(Load_test_case(
    dump_bucket.url,
    {**dump_bucket.options, "convertInnoDbVectorStore": "convert"},
    output="4 InnoDB-based vector store tables were converted to Lakehouse, 4 failed to load into HeatWave cluster.",
), False))

# FR2.3.7 errors reported during secondary load are non-fatal
EXPECT_STDOUT_MATCHES(re.compile(r"WARNING: \[Worker00\d\]: Failed to execute secondary load for table `{0}`\.`genai_2`: MySQL Error 6068 \(HY000\): No files found corresponding to the following data locations".format(test_schema)))

#@<> WL16802-TSFR_2_2_1_2_1 - load into LH instance, with PAR data is loaded {run_lakehouse_tests}
# WL16802-TSFFR_2_3_5 - "convertInnoDbVectorStore": "convert" + "heatwaveLoad": "vector_store"
loader_test(setup_lakehouse_test(Load_test_case(
    dump_bucket.url,
    {**dump_bucket.options, "convertInnoDbVectorStore": "convert", "lakehouseSource": {"par": dump_par + "vector-store/"}, "heatwaveLoad": "vector_store"},
    output="4 InnoDB-based vector store tables were converted to Lakehouse, 4 were loaded into HeatWave cluster.",
)))

#@<> WL16802-TSFR_2_3_3_1 - load into LH instance, 'convertInnoDbVectorStore' is set to 'keep' {run_lakehouse_tests}
loader_test(setup_lakehouse_test(Load_test_case(
    dump_bucket.url,
    {**dump_bucket.options, "convertInnoDbVectorStore": "keep", "heatwaveLoad": "vector_store"},
    output="4 InnoDB-based vector store tables were loaded as regular tables.",
)))

# 'heatwaveLoad' is explicilty set, even though tables are not converted, a note is printed
EXPECT_STDOUT_CONTAINS("The dump contains InnoDB based vector store tables which are not going to be converted to Lakehouse. The 'heatwaveLoad' option is ignored.")

#@<> FR2.3.6 - load into LH instance, 'convertInnoDbVectorStore' is set to 'convert' and 'loadData' to False - data is not loaded into cluster {run_lakehouse_tests}
loader_test(setup_lakehouse_test(Load_test_case(
    dump_bucket.url,
    {**dump_bucket.options, "convertInnoDbVectorStore": "convert", "loadData": False},
    output="4 InnoDB-based vector store tables were converted to Lakehouse.",
)))

#@<> WL16802-TSFR_2_3_4_1 - load into LH instance, 'convertInnoDbVectorStore' is set to 'convert' and 'heatwaveLoad' to 'none' - data is not loaded into cluster {run_lakehouse_tests}
loader_test(setup_lakehouse_test(Load_test_case(
    dump_bucket.url,
    {**dump_bucket.options, "convertInnoDbVectorStore": "convert", "heatwaveLoad": "none"},
    output="4 InnoDB-based vector store tables were converted to Lakehouse.",
)))

#@<> remote dump into a single directory - cleanup
wipe_dump_bucket()

#@<> local dump into a single directory - setup
dumper_test(Dump_test_case(
    local_dump,
    {},
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

#@<> WL16802-TSFR_2_1_4_1 - load into local instance with 'convertInnoDbVectorStore' to 'auto'
loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "auto"},
    output="4 InnoDB-based vector store tables were loaded as regular tables.",
)))

#@<> WL16802-TSFR_2_1_7_1 - load into local instance with 'convertInnoDbVectorStore' to 'convert'
loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "convert"},
    error=Load_errors.not_mhs,
)))

#@<> WL16802-TSFR_2_1_4_2_1 - load into remote instance with 'convertInnoDbVectorStore' to 'auto' {run_mhs_tests}
loader_test(setup_mhs_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "auto"},
    output="4 InnoDB-based vector store tables were loaded as regular tables.",
)))

#@<> WL16802-TSFR_2_1_7_2 - load local dump into remote instance with 'convertInnoDbVectorStore' to 'convert' {run_mhs_tests}
loader_test(setup_mhs_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "convert"},
    output="ERROR: The dump contains InnoDB based vector store tables, their data is not available in OCI's Object Storage, cannot convert them to Lakehouse tables. Set the 'lakehouseSource' to a valid value to continue with the load operation.",
    error=Load_errors.non_oci_location,
)))

#@<> local dump into a single directory - cleanup
wipe_local_dump()

#@<> local dump, vector store data stored remotely - setup
dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": vector_store_bucket.url, **vector_store_bucket.options}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

#@<> load into local instance with 'convertInnoDbVectorStore' set to 'keep'
loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "keep"},
    output="The dump contains InnoDB based vector store tables, but the location of their data is not known, cannot load them as regular tables. Set the 'convertInnoDbVectorStore' to 'skip' to ignore them and continue with the load operation.",
    error=Load_errors.unknown_location,
)))

#@<> load into local instance with 'convertInnoDbVectorStore' set to 'skip'
loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "skip"},
)))

#@<> load into remote instance with 'convertInnoDbVectorStore' to 'convert' {run_mhs_tests}
loader_test(setup_mhs_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "convert"},
)))

#@<> local dump, vector store data stored remotely - cleanup
wipe_local_dump()
wipe_vector_store_bucket()

#@<> WL16802-TSFR_2_2_1_2_2 - dump to local directory, upload vector store to OS, provide lakehouseSource - setup {run_lakehouse_tests}
# dump to local and object storage first, remove local dump, keep vector store
dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": vector_store_bucket.url, **vector_store_bucket.options}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_local_dump()],
))

# create local-only dump
dumper_test(Dump_test_case(
    local_dump,
    {},
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

#@<> WL16802-TSFR_2_2_1_2_2 - load into LH instance, load fails {run_lakehouse_tests}
loader_test(setup_lakehouse_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "convert"},
    error=Load_errors.non_oci_location,
)))

#@<> WL16802-TSFR_2_2_1_2_2 - load into LH instance, with PAR data is loaded {run_lakehouse_tests}
loader_test(setup_lakehouse_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "convert", "lakehouseSource": {"par": vector_store_par}},
    output="4 InnoDB-based vector store tables were converted to Lakehouse, 4 were loaded into HeatWave cluster.",
)))

#@<> WL16802-TSFR_2_2_1_2_3 - load into LH instance, with bucket info data is loaded {run_lakehouse_tests}
# WL16802-TSFR_2_2_1_3 - 'prefix' is used here
# WL16802-TSFR_2_3_2 - 'heatwaveLoad' is not set, but data is loaded
loader_test(setup_lakehouse_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "convert", "lakehouseSource": {"bucket": vector_store_bucket.osBucketName, "namespace": vector_store_bucket.osNamespace, "region": oci_region, "prefix": vector_store_bucket_prefix}},
    output="4 InnoDB-based vector store tables were converted to Lakehouse, 4 were loaded into HeatWave cluster.",
)))

#@<> WL16802-TSFR_2_3_5_1 - SECONDARY_LOAD failure during the load, then resuming the load {run_lakehouse_tests and not __dbug_off}
testutil.set_trap("mysql", [f"sql regex ALTER TABLE `{test_schema}`\\.`genai_2` SECONDARY_LOAD.*"], { "code": 6068, "msg": "Injected error.", "state": "HY000" })

tc = setup_lakehouse_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "convert", "lakehouseSource": {"par": vector_store_par}},
    output="4 InnoDB-based vector store tables were converted to Lakehouse, 3 were loaded into HeatWave cluster, 1 failed to load.",
))
tc.cleanup = None

loader_test(tc)
EXPECT_STDOUT_CONTAINS(f"Failed to execute secondary load for table `{test_schema}`.`genai_2`: MySQL Error 6068 (HY000): Injected error.")

testutil.clear_traps("mysql")

loader_test(setup_lakehouse_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "convert", "lakehouseSource": {"par": vector_store_par}, "resetProgress": False},
    output="4 InnoDB-based vector store tables were converted to Lakehouse, 1 were loaded into HeatWave cluster.",
)))

#@<> WL16802-TSFR_2_2_1_2_2 - cleanup {run_lakehouse_tests}
wipe_local_dump()
wipe_vector_store_bucket()

#@<> WL16802-TSFR_2_2_1_3_4 - validation of 'lakehouseSource'
loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": []},
    error="TypeError: Argument #2: Option 'lakehouseSource' is expected to be of type Map, but is Array",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: either 'par' sub-option or 'bucket', 'namespace', 'region' sub-options have to be set",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"prefix": "prefix"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: either 'par' sub-option or 'bucket', 'namespace', 'region' sub-options have to be set",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"par": "par", "bucket": "bucket"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: both 'par' sub-option and 'bucket', 'namespace', 'region' sub-options cannot be set at the same time",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"par": "par", "namespace": "namespace"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: both 'par' sub-option and 'bucket', 'namespace', 'region' sub-options cannot be set at the same time",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"par": "par", "region": "region"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: both 'par' sub-option and 'bucket', 'namespace', 'region' sub-options cannot be set at the same time",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"par": "par", "prefix": "prefix"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: both 'par' and 'prefix sub-options cannot be set at the same time",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"par": "par"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: the 'par' sub-option should be set to a PAR URL",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"bucket": "bucket"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: all of 'bucket', 'namespace', 'region' sub-options have to be set",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"namespace": "namespace"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: all of 'bucket', 'namespace', 'region' sub-options have to be set",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"region": "region"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: all of 'bucket', 'namespace', 'region' sub-options have to be set",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"bucket": "bucket", "namespace": "namespace"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: all of 'bucket', 'namespace', 'region' sub-options have to be set",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"bucket": "bucket", "region": "region"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: all of 'bucket', 'namespace', 'region' sub-options have to be set",
)))

loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"lakehouseSource": {"namespace": "namespace", "region": "region"}},
    error="TypeError: Argument #2: Option 'lakehouseSource' Invalid value of lakehouseSource: all of 'bucket', 'namespace', 'region' sub-options have to be set",
)))

#@<> WL16802-TSFR_2_3_1 - invalid value of 'heatwaveLoad'
loader_test(setup_local_test(Load_test_case(
    local_dump,
    {"heatwaveLoad": "unknown"},
    error="ValueError: Argument #2: The value of the 'heatwaveLoad' option must be set to one of: 'none', 'vector_store'.",
)))

#@<> FR3 - a warning is printed during a copy of vector store tables to MHS {run_mhs_tests}
shell.connect(__sandbox_uri1)

EXPECT_NO_THROWS(lambda: util.copy_schemas([test_schema], MDS_URI, {"compatibility": ["ignore_missing_pks"], "showProgress": False}), "copy_schemas() should not throw")

EXPECT_STDOUT_CONTAINS("WARNING: SRC: Copy utilities currently do not support automatic conversion of InnoDB-based vector store tables to Lakehouse.")

#@<> FR3 - a warning is printed during a copy of vector store tables to MHS - cleanup {run_mhs_tests}
clean_instance(mhs_session)

#@<> BUG#38224892 - tables were reported as converted even though an error was reported while converting them {run_mhs_tests}
test_table = "bug_38224892"
dump_session.run_sql("""
CREATE TABLE !.! (
  `id` int NOT NULL AUTO_INCREMENT,
  `vec` vector(2048) DEFAULT NULL COMMENT 'GENAI_OPTIONS=EMBED_MODEL_ID=123',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=262130 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci SECONDARY_ENGINE=rapid
""", [test_schema, test_table])

dumper_test(Dump_test_case(
    local_dump,
    {"includeTables": [quote_identifier(test_schema, test_table)], "lakehouseTarget": {"outputUrl": vector_store_bucket.url, **vector_store_bucket.options}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
))

loader_test(setup_mhs_test(Load_test_case(
    local_dump,
    {"convertInnoDbVectorStore": "convert"},
    output="The used table type doesn't support AUTO_INCREMENT columns",
    error="Error loading dump",
)))

EXPECT_STDOUT_NOT_CONTAINS("converted to Lakehouse")

#@<> BUG#38224892 - tables were reported as converted even though an error was reported while converting them - cleanup {run_mhs_tests}
wipe_local_dump()
wipe_vector_store_bucket()

#@<> BUG#38224892 - fail if directory where vector store data is going to be stored is not empty
dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": local_vector_store_dir}},
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_local_dump()],
))

dumper_test(Dump_test_case(
    local_dump,
    {"lakehouseTarget": {"outputUrl": local_vector_store_dir}},
    error=f"Invalid value of 'lakehouseTarget' option, the specified directory '{local_vector_store_dir}' already exists at the target location {os.path.abspath(local_vector_store_dir)} and is not empty.",
    setup=lambda: [shell.connect(__sandbox_uri1)],
    cleanup=lambda: [wipe_local_dump(), wipe_local_vector_store_dir()],
))

#@<> Cleanup
wipe_all()

delete_par(
    dump_bucket.osNamespace,
    dump_bucket.osBucketName,
    dump_par_id
)

delete_par(
    vector_store_bucket.osNamespace,
    vector_store_bucket.osBucketName,
    vector_store_par_id
)

for s in all_sessions:
    if s.is_open():
        s.close()

testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
