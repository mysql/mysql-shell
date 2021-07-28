import oci
import datetime
import time
import queue
import threading

from oci.object_storage.models.create_preauthenticated_request_details import CreatePreauthenticatedRequestDetails

config = oci.config.from_file(os.path.join(OCI_CONFIG_HOME, "config"))


def wipeout_bucket_mt(workers, bucket, ns="", config_file=os.path.join(OCI_CONFIG_HOME, "config")):
    start = time.time()
    q = queue.Queue()
    def worker():
        os = oci.object_storage.ObjectStorageClient(oci.config.from_file(config_file))
        while True:
            w = q.get()
            if w is None:
                q.task_done()
                break
            w(os)
            q.task_done()
    threads = []
    for i in range(workers):
        t = threading.Thread(target=worker)
        t.start()
        threads.append(t)
    try:
        config = oci.config.from_file(config_file)
        os = oci.object_storage.ObjectStorageClient(config)
        if ns == "":
            ns = os.get_namespace().data
        def abort_multipart(mp):
            return lambda os: os.abort_multipart_upload(ns, bucket, mp.object, mp.upload_id)
        for mp in oci.pagination.list_call_get_all_results(os.list_multipart_uploads, ns, bucket).data:
            q.put(abort_multipart(mp))
        def delete_object(name):
            return lambda os: os.delete_object(ns, bucket, name)
        for o in oci.pagination.list_call_get_all_results(os.list_objects, ns, bucket).data.objects:
            q.put(delete_object(o.name))
        def delete_par(id):
            return lambda os: os.delete_preauthenticated_request(ns, bucket, id)
        for par in oci.pagination.list_call_get_all_results(os.list_preauthenticated_requests, ns, bucket).data:
            q.put(delete_par(par.id))
    except:
        print("--- wipeout_bucket_mt() - exception: {0} ---".format(sys.exc_info()[0]))
    for i in range(workers):
        q.put(None)
    q.join()
    for t in threads:
        t.join()
    print("--- wipeout_bucket_mt() took {0} seconds ---".format(time.time() - start))

def delete_bucket(bucket, namespace=""):
  os_client = oci.object_storage.ObjectStorageClient(config)
  if namespace == "":
    namespace = os_client.get_namespace().data
  wipeout_bucket_mt(4, bucket, namespace)
  os_client.delete_bucket(namespace, bucket, retry_strategy=oci.retry.DEFAULT_RETRY_STRATEGY)

def delete_object(bucket, name, namespace=""):
  os_client = oci.object_storage.ObjectStorageClient(config)
  if namespace == "":
    namespace = os_client.get_namespace().data
  os_client.delete_object(namespace, bucket, name, retry_strategy=oci.retry.DEFAULT_RETRY_STRATEGY)


def prepare_empty_bucket(bucket, namespace=""):
  os_client = oci.object_storage.ObjectStorageClient(config)
  if namespace == "":
    namespace = os_client.get_namespace().data
  try:
    os_client.get_bucket(namespace, bucket)
    exists = True
  except Exception as e:
    exists = False
    print(e)
  if exists:
    wipeout_bucket_mt(4, bucket, namespace)
  else:
    os_client.create_bucket(namespace, oci.object_storage.models.CreateBucketDetails(
          name=bucket,
          compartment_id=OCI_COMPARTMENT_ID,
          storage_tier='Standard'
      ))

def get_manifest(namespace, bucket, prefix):
    manifest_file = '@.manifest.json'
    testutil.download_oci_object(namespace, bucket, prefix + '/@.manifest.json', manifest_file)
    fh = open(manifest_file, "r")
    manifest = json.load(fh)
    fh.close()
    os.remove(manifest_file)
    return manifest

def list_oci_objects(namespace, bucket, prefix):
    config = oci.config.from_file(os.path.join(OCI_CONFIG_HOME, "config"))
    os_client = oci.object_storage.ObjectStorageClient(config)
    objects = os_client.list_objects(namespace, bucket, prefix=prefix, fields="name,size")
    return objects.data.objects

def validate_full_dump(namespace, bucket, prefix, expire_time):
    manifest = get_manifest(namespace, bucket, prefix)
    EXPECT_TRUE("contents" in manifest, "Missing 'contents' in manifest")
    EXPECT_TRUE("lastUpdate" in manifest, "Missing 'lastUpdate' in manifest")
    EXPECT_TRUE("startTime" in manifest, "Missing 'startTime' in manifest")
    EXPECT_TRUE("endTime" in manifest, "Missing 'endTime' in manifest")
    EXPECT_TRUE("expireTime" in manifest, "Missing 'expireTime' in manifest")
    EXPECT_TRUE(manifest["expireTime"].startswith(expire_time), "Unexpected 'expireTime' in manifest, expected {}, got {}".format(expire_time, manifest["expireTime"]))
    # Get the bucket objects
    objects = list_oci_objects(namespace, bucket, prefix)
    # Validates the manifest contains an enty for each object on the bucket (except the manifest file)
    EXPECT_EQ(len(objects), len(manifest["contents"]) + 1)
    # Creates a dictionary with the manifest items
    manifest_dict = {}
    for object in manifest["contents"]:
        EXPECT_TRUE("parId" in object, "Missing 'parId' in manifest object")
        EXPECT_TRUE("parUrl" in object, "Missing 'parUrl' in manifest object")
        EXPECT_TRUE("objectName" in object, "Missing 'objectName' in manifest object")
        EXPECT_TRUE("objectSize" in object, "Missing 'objectSize' in manifest object")
        manifest_dict[object["objectName"]] = object
    # Creates a list with the created PARs
    config = oci.config.from_file(os.path.join(OCI_CONFIG_HOME, "config"))
    os_client = oci.object_storage.ObjectStorageClient(config)
    pars = os_client.list_preauthenticated_requests(namespace, bucket)
    par_list = []
    for par in pars.data:
        par_list.append(par.name)
    # Now validates every object is in the manifest and a PAR exists
    for object in objects:
        if object.name != prefix + '/@.manifest.json':
            EXPECT_TRUE(object.name in manifest_dict, "Missing object '{}' in manifest".format(object.name))
            mobject = manifest_dict[object.name]
            EXPECT_EQ(object.size, mobject["objectSize"])
            par_name = "shell-dump-" + object.name
            EXPECT_TRUE(par_name in par_list, "Missing PAR '{}'".format(par_name))

def today_plus_days(count, rfc_3339_format=False):
    t = time.gmtime()
    now=datetime.date(t.tm_year, t.tm_mon, t.tm_mday)
    day_delta = datetime.timedelta(days=count)
    new_date = now + day_delta
    # The RFC3339 Format reques GMT time format
    if rfc_3339_format:
        return new_date.isoformat()+"T00:00:00Z"
    else:
        return new_date.isoformat()

def create_par(namespace, bucket, access_type, name, time_expires, object_name = None, bucket_listing_action=None):
    details = CreatePreauthenticatedRequestDetails(name=name, object_name=object_name, access_type=access_type, time_expires=time_expires, bucket_listing_action=bucket_listing_action)
    config = oci.config.from_file(os.path.join(OCI_CONFIG_HOME, "config"))
    os_client = oci.object_storage.ObjectStorageClient(config)
    par = os_client.create_preauthenticated_request(namespace, bucket, details)
    if par.data.access_type.startswith("Any") and object_name is not None:
        return "https://objectstorage.{}.oraclecloud.com{}{}".format(config["region"], par.data.access_uri,par.data.object_name)
    else:
        return "https://objectstorage.{}.oraclecloud.com{}".format(config["region"], par.data.access_uri)

prev_log_level = shell.options["logLevel"]

def PREPARE_PAR_IS_SECRET_TEST():
    global prev_log_level
    prev_log_level = shell.options["logLevel"]
    shell.options["logLevel"] = 8
    WIPE_OUTPUT()
    WIPE_SHELL_LOG()

def EXPECT_PAR_IS_SECRET():
    global prev_log_level
    shell.options["logLevel"] = prev_log_level
    def check_text(text, context):
        expr = re.compile(r"/p/(.+)/n/")
        for line in text.splitlines():
            match = expr.search(line)
            if match is not None and "<secret>" != match.group(1):
                testutil.fail(context + " contains unmasked PAR: " + line)
    check_text(testutil.fetch_captured_stdout(False), "stdout")
    check_text(testutil.fetch_captured_stderr(False), "stderr")
    check_text(testutil.cat_file(testutil.get_shell_log_path()), "Shell log")
