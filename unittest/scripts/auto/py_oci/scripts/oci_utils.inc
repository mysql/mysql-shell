import oci
import datetime
import time
from oci.object_storage.models.create_preauthenticated_request_details import CreatePreauthenticatedRequestDetails

config = oci.config.from_file(os.path.join(OCI_CONFIG_HOME, "config"))

def wipeout_bucket(os, namespace, bucket):
  print("Deleting objects in bucket {}".format(bucket))
  objects = os.list_objects(namespace, bucket)
  for o in objects.data.objects:
    os.delete_object(namespace, bucket, o.name)
  print("Aborting multi-part uploads for bucket {}".format(bucket))
  multiparts = os.list_multipart_uploads(namespace, bucket)
  for mp in multiparts.data:
    os.abort_multipart_upload(namespace, bucket, mp.object, mp.upload_id)
  print("Deleting PARs for bucket {}".format(bucket))
  pars = os.list_preauthenticated_requests(namespace, bucket)
  for par in pars.data:
    os.delete_preauthenticated_request(namespace, bucket, par.id)


def delete_bucket(bucket, namespace=""):
  os = oci.object_storage.ObjectStorageClient(config)
  if namespace == "":
    namespace = os.get_namespace().data
  wipeout_bucket(os, namespace, bucket)
  os.delete_bucket(namespace, bucket)

def delete_object(bucket, name, namespace=""):
  os = oci.object_storage.ObjectStorageClient(config)
  if namespace == "":
    namespace = os.get_namespace().data
  os.delete_object(namespace, bucket, name)


def prepare_empty_bucket(bucket, namespace=""):
  os = oci.object_storage.ObjectStorageClient(config)
  if namespace == "":
    namespace = os.get_namespace().data
  try:
    os.get_bucket(namespace, bucket)
    exists = True
  except Exception as e:
    exists = False
    print(e)
  if exists:
    wipeout_bucket(os, namespace, bucket)
  else:
    os.create_bucket(namespace, oci.object_storage.models.CreateBucketDetails(
          name=bucket,
          compartment_id=OCI_COMPARTMENT_ID,
          storage_tier='Standard'
      ))
      
def execute_oci_shell(command):
  import paramiko
  client = paramiko.SSHClient()
  client.set_missing_host_key_policy(paramiko.client.AutoAddPolicy)
  client.connect(OCI_INSTANCE_HOST, username=OCI_INSTANCE_USER, passphrase=OCI_SSH_PKEY_PASSPHRASE, key_filename=OCI_SSH_PKEY_PATH)
  mds_uri="{}:{}@{}".format(MDS_USER, MDS_PASSWORD, MDS_HOST)
  stdin, stdout, stderr = client.exec_command("{} {} {}".format(OCI_INSTANCE_SHELL_PATH, mds_uri, command))
  for line in stdout.readlines():
    sys.stdout.write(line)
  for line in stderr.readlines():
    sys.stderr.write(line)

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

def create_par(namespace, bucket, access_type, name, time_expires, object_name = None):
    details = CreatePreauthenticatedRequestDetails(name=name, object_name=object_name, access_type=access_type, time_expires=time_expires)
    config = oci.config.from_file(os.path.join(OCI_CONFIG_HOME, "config"))
    os_client = oci.object_storage.ObjectStorageClient(config)
    par = os_client.create_preauthenticated_request(namespace, bucket, details)
    return "https://objectstorage.{}.oraclecloud.com{}".format(config["region"], par.data.access_uri)
