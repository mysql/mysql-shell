import oci

config = oci.config.from_file(os.path.join(OCI_CONFIG_HOME, "config"))

def wipeout_bucket(os, namespace, bucket):
  print("Deleting objects in bucket {}".format(bucket))
  objects = os.list_objects(namespace, bucket)
  for o in objects.data.objects:
    os.delete_object(namespace, bucket, o.name)
  print("Aborting multi-part uploads for bucket {}".format(bucket))
  multiparts = os.list_multipart_uploads(namespace, bucket)
  for mp in multiparts.data:
    print(mp)
    os.abort_multipart_upload(namespace, bucket, mp.object, mp.upload_id)

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
