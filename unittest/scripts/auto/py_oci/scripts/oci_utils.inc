import oci
import oci.signer
import configparser
import datetime
import time
import queue
import re
import threading
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat

from oci.object_storage.models.create_preauthenticated_request_details import CreatePreauthenticatedRequestDetails

config = oci.config.from_file(os.path.join(OCI_CONFIG_HOME, "config"))


def wipeout_bucket_mt(workers, bucket, ns="", config_file=os.path.join(OCI_CONFIG_HOME, "config"), wipe_pars=True):
    start = time.time()
    config = oci.config.from_file(config_file)
    os = oci.object_storage.ObjectStorageClient(config)
    if ns == "":
        ns = os.get_namespace().data
    q = queue.Queue()
    def worker():
        try:
            os = oci.object_storage.ObjectStorageClient(oci.config.from_file(config_file))
            while True:
                w = q.get()
                if w is not None:
                    try:
                        print(f"--- wipeout_bucket_mt('{bucket}') - worker task: {w[0]} ---")
                        w[1](os)
                    except Exception as e:
                        print(f"--- wipeout_bucket_mt('{bucket}') - exception in worker task \"{w[0]}\": {e} ---")
                q.task_done()
                if w is None:
                    break
        except Exception as e:
            print(f"--- wipeout_bucket_mt('{bucket}') - exception in worker: {e} ---")
    threads = []
    for i in range(workers):
        t = threading.Thread(target=worker)
        t.start()
        threads.append(t)
    try:
        def abort_multipart(mp):
            return lambda os: os.abort_multipart_upload(ns, bucket, mp.object, mp.upload_id)
        for mp in oci.pagination.list_call_get_all_results(os.list_multipart_uploads, ns, bucket).data:
            q.put((f"Aborting multipart upload of '{mp.object}', upload ID: {mp.upload_id}", abort_multipart(mp)))
        def delete_object(name):
            return lambda os: os.delete_object(ns, bucket, name)
        for o in oci.pagination.list_call_get_all_results(os.list_objects, ns, bucket).data.objects:
            q.put((f"Deleting object: '{o.name}'", delete_object(o.name)))
        def delete_par(id):
            return lambda os: os.delete_preauthenticated_request(ns, bucket, id)
        if wipe_pars:
            for par in oci.pagination.list_call_get_all_results(os.list_preauthenticated_requests, ns, bucket).data:
                q.put((f"Deleting PAR: '{par.id}'", delete_par(par.id)))
    except Exception as e:
        print(f"--- wipeout_bucket_mt('{bucket}') - exception: {e} ---")
    for i in range(workers):
        q.put(None)
    q.join()
    for t in threads:
        t.join()
    print(f"--- wipeout_bucket_mt('{bucket}') took {time.time() - start} seconds ---")

def delete_bucket(bucket, namespace=""):
  os_client = oci.object_storage.ObjectStorageClient(config)
  if namespace == "":
    namespace = os_client.get_namespace().data
  try:
    os_client.get_bucket(namespace, bucket)
  except:
    print(f"--- delete_bucket('{bucket}') - bucket does not exist ---")
    return
  wipeout_bucket_mt(12, bucket, namespace)
  os_client.delete_bucket(namespace, bucket, retry_strategy=oci.retry.DEFAULT_RETRY_STRATEGY)

def delete_object(bucket, name, namespace=""):
  os_client = oci.object_storage.ObjectStorageClient(config)
  if namespace == "":
    namespace = os_client.get_namespace().data
  os_client.delete_object(namespace, bucket, name, retry_strategy=oci.retry.DEFAULT_RETRY_STRATEGY)


def prepare_empty_bucket(bucket, namespace="", wipe_pars=True):
  os_client = oci.object_storage.ObjectStorageClient(config)
  if namespace == "":
    namespace = os_client.get_namespace().data
  try:
    os_client.get_bucket(namespace, bucket)
    exists = True
  except Exception as e:
    exists = False
    print(f"--- prepare_empty_bucket('{bucket}') - bucket does not exist, creating ---")
  if exists:
    wipeout_bucket_mt(12, bucket, namespace, wipe_pars=wipe_pars)
  else:
    os_client.create_bucket(namespace, oci.object_storage.models.CreateBucketDetails(
          name=bucket,
          compartment_id=OCI_COMPARTMENT_ID,
          storage_tier='Standard'
      ))

def list_oci_objects(namespace, bucket, prefix):
    config = oci.config.from_file(os.path.join(OCI_CONFIG_HOME, "config"))
    os_client = oci.object_storage.ObjectStorageClient(config)
    objects = os_client.list_objects(namespace, bucket, prefix=prefix, fields="name,size")
    return objects.data.objects

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
        return f"https://{namespace}.objectstorage.{config['region']}.oci.customer-oci.com{par.data.access_uri}{par.data.object_name}"
    else:
        return f"https://{namespace}.objectstorage.{config['region']}.oci.customer-oci.com{par.data.access_uri}"

def put_object(namespace, bucket, name, content):
    config = oci.config.from_file(os.path.join(OCI_CONFIG_HOME, "config"))
    os_client = oci.object_storage.ObjectStorageClient(config)
    os_client.put_object(namespace, bucket, name, content)

local_progress_file = "my_load_progress.txt"

def remove_local_progress_file():
    remove_file("my_load_progress.txt")

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
    expr = re.compile(r"/p/(.+?)/n/")
    def check_text(text, context):
        for line in text.splitlines():
            for match in expr.findall(line):
                if "<secret>" != match:
                    testutil.fail(f"{context} contains unmasked PAR ({match}): {line}")
    check_text(testutil.fetch_captured_stdout(False), "stdout")
    check_text(testutil.fetch_captured_stderr(False), "stderr")
    check_text(testutil.cat_file(testutil.get_shell_log_path()), "Shell log")

def convert_par(par):
    m = re.match(r"^https:\/\/(?:([^\.]+)\.)?objectstorage\.([^\.]+)\.[^\/]+(\/p\/.+\/n\/(.+)\/b\/.*\/o\/(?:(?:.*)\/)?.*)$", par)
    if m is None:
        raise Exception(f"This is not a PAR: {par}")
    if m.group(1) is None:
        # old format, return an URL with a dedicated endpoint
        return f"https://{m.group(4)}.objectstorage.{m.group(2)}.oci.customer-oci.com{m.group(3)}"
    else:
        # PAR with a dedicated endpoint, return old format
        return f"https://objectstorage.{m.group(2)}.oraclecloud.com{m.group(3)}"

def get_session_token(config_file=os.path.join(OCI_CONFIG_HOME, "config")):
    config = oci.config.from_file(config_file)
    public_key = oci.signer.load_private_key_from_file(config["key_file"], config.get("pass_phrase", None)).public_key().public_bytes(Encoding.PEM, PublicFormat.SubjectPublicKeyInfo).decode('ascii')
    identity_data_plane_client = oci.identity_data_plane.DataplaneClient(config)
    response = identity_data_plane_client.generate_user_security_token(generate_user_security_token_details=oci.identity_data_plane.models.GenerateUserSecurityTokenDetails(public_key=public_key, session_expiration_in_minutes=30))
    return response.data.token

def read_config_file(path, profile="DEFAULT"):
    config = configparser.ConfigParser()
    config.read(path)
    settings = {}
    for key, value in config.items(profile):
        settings[key] = value
    return settings

def write_config_file(path, settings, profile="DEFAULT"):
    config = configparser.ConfigParser()
    config.read_dict({ profile: settings })
    with open(path, "w") as f:
        config.write(f)
