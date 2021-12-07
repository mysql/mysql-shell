#@<> Setup
import http.server
import threading
import os
import zipfile
import json
import urllib
import posixpath

try:
    from http.server import BaseHTTPRequestHandler, HTTPServer
    from socketserver import ThreadingMixIn
except:
    from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
    from SocketServer import ThreadingMixIn

# Sets the shell user path for testing purposes
if "MYSQLSH_USER_CONFIG_HOME" not in os.environ:
    os.environ["MYSQLSH_USER_CONFIG_HOME"] = __user_config_path

# Sets the https proxy if defined
restore_https_proxy = False
https_proxy = ""
if "MYSQLSH_HTTPS_PROXY" in os.environ:
    restore_https_proxy = True
    if "https_proxy" in os.environ:
        https_proxy = os.environ["https_proxy"]
    os.environ["https_proxy"] = os.environ["MYSQLSH_HTTPS_PROXY"]


import socket
from contextlib import closing

def find_free_port():
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
        s.bind(('', 0))
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        return s.getsockname()[1]


DEFAULT_HTTP_ROOT=os.path.join(__user_config_path, "default_plugin_repo")

HTTP_ROOT = os.path.join(__user_config_path, "plugin_repo")
DEFAULT_REPO_PATH = os.path.join(HTTP_ROOT, "windows", "")
PLUGIN_PATH = os.path.join(HTTP_ROOT, "repo")
PLUGIN_CODE = '''
from mysqlsh.plugin_manager import plugin, plugin_function

@plugin
class repo:
    """
    Plugin stored in a repository.

    This plugin was installed using the built in plugins plugin.
    """
    pass

@plugin_function("repo.version")
def version():
    """
    Prints a specific version of the plugin.
    """
    return "--plugin-version-placeholder--"
'''
default_manifest = {
    "fileType": "MySQL Shell Plugins Manifest",
    "version": "0.0.1",
    "releaseVersion": "2021.02.26.17.00",
    "repository": {
        "name": "Official MySQL Shell Plugin Repository",
        "description": "The official MySQL Shell Plugin Repository maintained by the MySQL Team at Oracle Corp."
    },
    "plugins": [
        {
            "caption": "Oracle Cloud Plugin",
            "name": "cloud",
            "moduleName": "cloud_plugin",
            "description": "Plugin to manage the MySQL Database Service on OCI.",
            "latestVersion": "0.1.9",
            "editions": [
                {
                    "name": "community",
                    "packagePrefix": "mysql-shell-cloud-plugin",
                    "licenseHeader": "./resources/license_header_community.txt",
                    "licenseFile": "./resources/LICENSE_gpl.txt"
                },
                {
                    "name": "commercial",
                    "packagePrefix": "mysql-shell-commercial-cloud-plugin",
                    "licenseHeader": "./resources/license_header_commercial.txt",
                    "licenseFile": "./resources/LICENSE_commercial.txt"
                }
            ],
            "versions": [
                {
                    "version": "0.1.9",
                    "developmentStage": "preview",
                    "changes": [
                        "Added support for host_image_id and tags in create.mysqlDbSystem and tags for compute instances."
                    ],
                    "urls": {
                        "community": "https://insidemysql.com/downloads/mysql-shell-cloud-plugin-0.1.9.zip",
                        "commercial": "https://insidemysql.com/downloads/mysql-shell-commercial-cloud-plugin-0.1.9.zip"
                    }
                }
            ]
        }
    ]
}

base_manifest = {
    "fileType": "MySQL Shell Plugins Manifest",
    "version": "0.0.1",
    "releaseVersion": "2020.09.03.04.00",
    "repository": {
        "name": "Testing MySQL Shell Plugin Repository",
        "description": "A testing repository to be used while testing the plugins builtin plugin."
    },
    "plugins": [
        {
            "caption": "Repo Testing Plugin",
            "name": "repo",
            "moduleName": "repo",
            "description": "Plugin to test the plugins builtin plugin.",
            "latestVersion": "0.0.1",
            "editions": [
                {
                    "name": "community",
                    "packagePrefix": "mysql-shell-test-plugin",
                    "licenseHeader": "./resources/license_header_community.txt",
                    "licenseFile": "./resources/LICENSE_gpl.txt"
                },
                {
                    "name": "commercial",
                    "packagePrefix": "mysql-shell-commercial-test-plugin",
                    "licenseHeader": "./resources/license_header_commercial.txt",
                    "licenseFile": "./resources/LICENSE_commercial.txt"
                }
            ],
            "versions": []
        }
    ]
}
PORT = find_free_port()

def add_version_to_manifest(version, changes, shellMin=None, shellMax=None):
    base_manifest["plugins"][0]["latestVersion"]=version
    version_data = {
        "version": version,
        "developmentStage": "preview",
        "changes": changes,
        "urls": {
            "community": "http://127.0.0.1:" + str(PORT) + "/mysql-shell-test-plugin-" + version + ".zip",
            "commercial": "http://127.0.0.1:" + str(PORT) + "/mysql-shell-commercial-test-plugin-" + version + ".zip"
        }
    }
    if shellMin is not None:
        version_data["shellVersionMin"] = shellMin
    if shellMax is not None:
        version_data["shellVersionMax"] = shellMax
    base_manifest["plugins"][0]["versions"].insert(0, version_data)

def create_manifest(folder, data):
    manifest_fh = open(os.path.join(folder, 'mysql-shell-plugins-manifest.json'), 'w')
    manifest_fh.write(json.dumps(data, indent=4))
    manifest_fh.close()

def zip_folder(source_folder, target_path):
    zip_fh = zipfile.ZipFile(target_path, 'w', zipfile.ZIP_DEFLATED)
    for root, dirs, files in os.walk(source_folder):
        for file in files:
            full_path = os.path.join(root, file)
            arc_path=full_path[len(source_folder)+1:]
            zip_fh.write(full_path, arc_path)
    zip_fh.close()

def create_default_manifest():
    # Updates the manifest
    testutil.mkdir("manifest/manifest", True)
    create_manifest("manifest/manifest", default_manifest)
    manifest_folder=os.path.join(HTTP_ROOT, "windows", "installer")
    testutil.mkdir(manifest_folder, True)
    zip_folder("manifest", os.path.join(manifest_folder, "manifest.zip"))
    testutil.rmdir("manifest", True)


def update_plugin(version, changes, shellMin=None, shellMax=None):
    # Updates the manifest
    add_version_to_manifest(version, changes, shellMin=shellMin, shellMax=shellMax)
    create_manifest(HTTP_ROOT, base_manifest)
    testutil.mkdir(PLUGIN_PATH)
    new_code = PLUGIN_CODE.replace("--plugin-version-placeholder--", version)
    testutil.create_file(os.path.join(PLUGIN_PATH, "init.py"), new_code)
    zip_folder(PLUGIN_PATH, os.path.join(HTTP_ROOT, "mysql-shell-test-plugin-" + version + ".zip"))
    testutil.rmdir(PLUGIN_PATH, True)

testutil.rmfile(os.path.join(__user_config_path, 'plugin-repositories.json'))
testutil.mkdir(HTTP_ROOT)

create_default_manifest()

version = shell.version.split()[1].split('-')[0].split('.')
base_version = ".".join(version)
v_plus_1 = f"{version[0]}.{version[1]}.{int(version[2])+1}"
v_minus_1 = f"{version[0]}.{version[1]}.{int(version[2])-1}"
v_minus_2 = f"{version[0]}.{version[1]}.{int(version[2])-2}"

update_plugin("0.0.1", ["Initial Version"], shellMin=v_minus_2, shellMax=v_minus_1)

Handler = http.server.SimpleHTTPRequestHandler
class CustomHTTPRequestHandler(Handler):
    def log_error(self, format, *args):
        super().log_message(format, *args)
    def log_message(self, format, *args):
        pass
    def translate_path(self, path):
        """
        This function has been taken from Python 3.8 to adapt enable Python 3.6
        to serve files from a custom path (HTTP_ROOT), this is because in 3.6
        the target folder can not be customized.
        """
        # abandon query parameters
        path = path.split('?',1)[0]
        path = path.split('#',1)[0]
        # Don't forget explicit trailing slash when normalizing. Issue17324
        trailing_slash = path.rstrip().endswith('/')
        try:
            path = urllib.parse.unquote(path, errors='surrogatepass')
        except UnicodeDecodeError:
            path = urllib.parse.unquote(path)
        path = posixpath.normpath(path)
        words = path.split('/')
        words = filter(None, words)
        path = HTTP_ROOT
        for word in words:
            if os.path.dirname(word) or word in (os.curdir, os.pardir):
                # Ignore components that are not a simple file/directory name
                continue
            path = os.path.join(path, word)
        if trailing_slash:
            path += '/'
        return path


class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    daemon_threads = True

def test(server):
    print("serving at port", PORT)
    server.serve_forever()
    print("server stopped")


httpd = ThreadedHTTPServer(("127.0.0.1", PORT), CustomHTTPRequestHandler)
x = threading.Thread(target=test, args=(httpd, ))
x.start()

def execute(command, prompt_answers=""):
    args = ["--py", "--quiet-start=2", "-ifull", "-e"]
    env = ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + __user_config_path]
    # Hacks the official plugin for all calls except the help ones
    if not command.startswith("\\?"):
        command = "from mysqlsh.plugin_manager import repositories;repositories.DEFAULT_PLUGIN_REPOSITORIES[0]['url']='http://127.0.0.1:" + str(PORT) + "/windows/installer/manifest.zip';" + command
    rc = testutil.call_mysqlsh(args + [command], prompt_answers, env)


#@<> Install the test repository
TEST_REPOSITORY="http://127.0.0.1:" + str(PORT) + '/mysql-shell-plugins-manifest.json'
# Sends the answer to the following prompts as 'repo' and 'yes':
# - Are you sure you want to add the repository 'Testing MySQL Shell Plugin Repository' [yes/NO]:
execute("plugins.repositories.add('" + TEST_REPOSITORY + "')", "yes")

#@<> Help on built-in plugins plugin
execute("\\? plugins")

#@<> Tests the about function
execute("plugins.about()")

#@<> Help on built-in plugins.repositories object
#execute("\\? plugins.repositories")

#@<> Tests the info function
execute("plugins.info()")

#@<> Tests the details function without name specified
# Sends the answer to the following prompts as 'repo' and 'yes':
# - Please enter the index or name of a plugin:
execute("plugins.details()", "repo")

#@<> Tests the details function with a specific plugin
execute("plugins.details('repo')")

#@<> Lists the plugin repositories
execute("plugins.repositories.list()")

#@<> Lists the plugins
execute("plugins.list()")

#@<> Attempt installing the test plugin, shell above valid version range
execute("plugins.install('repo')")

#@<> Attempt installing the test plugin, shell above max version
update_plugin("0.0.2", ["Initial Version"], shellMax=v_minus_1)
execute("plugins.install('repo')")

#@<> Attempt installing the test plugin, shell below min version
update_plugin("0.0.3", ["Initial Version"], shellMin=v_plus_1)
execute("plugins.install('repo')")

#@<> Install the test plugin
update_plugin("0.0.4", ["Initial Version"], shellMin=base_version, shellMax=base_version)
execute("plugins.install('repo')")
execute("print('Test Plugin Version: ' + repo.version())")

#@<> Lists installed plugins in a new shell session
execute("plugins.list()")

#@<> Attempts reinstalling a plugin
execute("plugins.install('repo')")

#@<> Attempts forces reinstalling a plugin
execute("plugins.install('repo', force_install=True)")

#@<> Upgrades the test plugin version in the repository
update_plugin("0.0.5", ["Upgraded Version"])
execute("plugins.list()")

#@<> Upgrades the test plugin locally
# Sends the answer to the following prompt as 'yes':
# - Please enter the index or name of a plugin:
# - Are you sure you want to update the 'Repo Testing Plugin' [YES/no]:
execute("plugins.update()", "repo\nyes")
execute("print('Test Plugin Version: ' + repo.version())")

#@<> Downgrades the test plugin to the initial version
execute("plugins.install('repo', version='0.0.4', force_install=True)")
execute("print('Test Plugin Version: ' + repo.version())")

#@<> Upgrades the test plugin again, using name
execute("plugins.list()")
# Sends the answer to the following prompt as 'yes':
# - Are you sure you want to update the 'Repo Testing Plugin' [YES/no]:
execute("plugins.update('repo')", "yes")
execute("print('Test Plugin Version: ' + repo.version())")

#@<> Uninstalls the test plugin
# Sends the answer to the following prompts as 'repo' and 'yes':
# - Please enter the index or name of a plugin:
# - Are you sure you want to uninstall the plugin 'repo' [YES/no]:
execute("plugins.uninstall()", "repo\nyes")
execute("print('Test Plugin Version: ' + repo.version())")

#@<> Removes the test plugin repository
execute("plugins.repositories.remove(url='" + TEST_REPOSITORY + "')", "repo\nyes")

#@<> Lists the plugin repositories again
execute("plugins.repositories.list()")

#@<> Finalize
print("Shutting down...")
httpd.shutdown()
testutil.rmdir(HTTP_ROOT, True)
testutil.rmdir(os.path.join(__user_config_path, 'plugins'))
testutil.rmfile(os.path.join(__user_config_path, 'plugin-repositories.json'))

# Restores the https proxy
if restore_https_proxy:
    https_proxy = os.environ["https_proxy"] = https_proxy

