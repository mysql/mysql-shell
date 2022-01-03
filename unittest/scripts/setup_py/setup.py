import queue
import time
import threading
from __future__ import print_function
import difflib
import hashlib
import mysqlsh
import os.path
import random
import re
import string
import sys
import inspect


def get_members(object):
    all_exports = dir(object)

    exports = []
    for member in all_exports:
        if not member.startswith('__'):
            exports.append(member)

    return exports

##
# Verifies if a variable is defined, returning true or false accordingly
# @param cb An anonymous function that simply executes the variable to be
# verified, example:
#
# defined(lambda:myVar)
#
# Will return True if myVar is defined or False if not.
##


def defined(cb):
    try:
        cb()
        return True
    except:
        return False


def create_root_from_anywhere(session):
    session.run_sql("SET SQL_LOG_BIN=0")
    session.run_sql("CREATE USER root@'%' IDENTIFIED BY 'root'")
    session.run_sql("GRANT ALL ON *.* to root@'%' WITH GRANT OPTION")
    session.run_sql("SET SQL_LOG_BIN=1")


def has_ssh_environment():
    import os
    if "SSH_URI" in os.environ and "MYSQL_OVER_SSH_URI" in os.environ and "SSH_USER_URI" in os.environ:
        return True
    return False


def has_oci_environment(context):
    if context not in ['OS', 'MDS']:
        return False
    variables = ['OCI_CONFIG_HOME',
                 'OCI_COMPARTMENT_ID',
                 'OS_NAMESPACE',
                 'OS_BUCKET_NAME']
    if context == 'MDS':
        variables = variables + ['MDS_URI']
    missing = []
    for variable in variables:
        if variable not in globals():
            missing.append(variable)
    if (len(missing)):
        sys.stderr.write("Missing Variables: {}".format(", ".join(missing)))
        return False
    return True


def has_aws_environment():
    variables = ['MYSQLSH_S3_BUCKET_NAME']
    missing = []
    g = globals()
    for variable in variables:
        if variable not in g:
            missing.append(variable)
    if len(missing):
        sys.stderr.write("Missing AWS Variables: {}".format(", ".join(missing)))
        return False
    return True

def is_re_instance(o):
    return isinstance(o, is_re_instance.__re_type)


is_re_instance.__re_type = type(re.compile(''))


class __TextMatcher:
    def __init__(self, o):
        self.__is_re = is_re_instance(o)
        if not self.__is_re and not isinstance(o, str):
            raise Exception(
                "Expected str or re.Pattern, but got: " + str(type(o)))
        self.__o = o

    def __str__(self):
        if self.__is_re:
            return "re.Pattern('" + self.__o.pattern + "')"
        else:
            return self.__o

    def matches(self, s):
        if self.__is_re:
            return self.__o.search(s) is not None
        else:
            return s.find(self.__o) != -1


def __caller_context():
    # 0 is here, 1 is the EXPECT_ call, 2 is the test code calling EXPECT_
    frame = inspect.stack()[2]
    return f"{frame}"


def EXPECT_EQ(expected, actual, note=""):
    if expected != actual:
        if not note:
            note = __caller_context()
        context = "Tested values don't match as expected: "+note + \
            "\n\tActual:   " + str(actual) + "\n\tExpected: " + str(expected)
        testutil.fail(context)


def EXPECT_NE(expected, actual, note=""):
    if expected == actual:
        if not note:
            note = __caller_context()
        context = "Tested values should not match: "+note + \
            "\n\tActual: " + str(actual) + "\n\tExpected: " + str(expected)
        testutil.fail(context)


def EXPECT_EQ_TEXT(expected, actual, note=""):
    if expected != actual:
        if not note:
            note = __caller_context()
        import difflib
        context = "Tested values don't match as expected: "+note + \
            "\n".join(difflib.context_diff(expected, actual,
                                           fromfile="Expected", tofile="Actual"))
        testutil.fail(context)


def EXPECT_IN(actual, expected_values, note=""):
    if actual not in expected_values:
        if not note:
            note = __caller_context()
        context = "Tested value not one of expected: "+note+"\n\tActual:   " + \
            str(actual) + "\n\tExpected: " + str(expected_values)
        testutil.fail(context)


def EXPECT_LE(expected, actual, note=""):
    if expected > actual:
        if not note:
            note = __caller_context()
        context = "Tested values not as expected: "+note+"\n\t" + \
            str(expected)+" (expected) <= "+str(actual)+" (actual)"
        testutil.fail(context)


def EXPECT_LT(expected, actual, note=""):
    if expected >= actual:
        if not note:
            note = __caller_context()
        context = "Tested values not as expected: "+note+"\n\t" + \
            str(expected)+" (expected) < "+str(actual)+" (actual)"
        testutil.fail(context)


def EXPECT_GE(expected, actual, note=""):
    if expected < actual:
        if not note:
            note = __caller_context()
        context = "Tested values not as expected: "+note+"\n\t" + \
            str(expected)+" (expected) >= "+str(actual)+" (actual)"
        testutil.fail(context)


def EXPECT_GT(expected, actual, note=""):
    if expected <= actual:
        if not note:
            note = __caller_context()
        context = "Tested values not as expected: "+note+"\n\t" + \
            str(expected)+" (expected) > "+str(actual)+" (actual)"
        testutil.fail(context)


def EXPECT_BETWEEN(expected_from, expected_to, actual, note=""):
    if expected_from <= actual <= expected_to:
        pass
    else:
        if not note:
            note = __caller_context()
        context = "Tested value not as expected: "+note + \
            f"\n\t{expected_from} <= {actual} <= {expected_to}"
        testutil.fail(context)


def EXPECT_NOT_BETWEEN(expected_from, expected_to, actual, note=""):
    if expected_from <= actual <= expected_to:
        if not note:
            note = __caller_context()
        context = "Tested value not as expected: "+note + \
            f"\n\tNOT ({expected_from} <= {actual} <= {expected_to})"
        testutil.fail(context)


def EXPECT_DELTA(expected, allowed_delta, actual, note=""):
    if not note:
        note = __caller_context()
    EXPECT_BETWEEN(expected - allowed_delta, expected +
                   allowed_delta, actual, note)


def EXPECT_TRUE(value, note=""):
    if not value:
        if not note:
            note = __caller_context()
        context = f"Tested value '{value}' expected to be true but is false"
        if note:
            context += ": "+note
        testutil.fail(context)


def EXPECT_FALSE(value, note=""):
    if value:
        if not note:
            note = __caller_context()
        context = f"Tested value '{value}' expected to be false but is true"
        if note:
            context += ": "+note
        testutil.fail(context)


def EXPECT_THROWS(func, etext):
    assert callable(func)
    m = __TextMatcher(etext)
    try:
        func()
        testutil.fail(
            "<red>Missing expected exception throw like " + str(m) + "</red>")
        return False
    except Exception as e:
        exception_message = type(e).__name__ + ": " + str(e)
        if not m.matches(exception_message):
            testutil.fail("<red>Exception expected:</red> " + str(m) +
                          "\n\t<yellow>Actual:</yellow> " + exception_message)
            return False
        return True


def EXPECT_MAY_THROW(func, etext):
    assert callable(func)
    m = __TextMatcher(etext)
    ret = None
    try:
        ret = func()
    except Exception as e:
        exception_message = type(e).__name__ + ": " + str(e)
        if not m.matches(exception_message):
            testutil.fail("<red>Exception expected:</red> " + str(m) +
                          "\n\t<yellow>Actual:</yellow> " + exception_message)
    return ret


def EXPECT_NO_THROWS(func, context=""):
    assert callable(func)
    try:
        func()
    except Exception as e:
        testutil.fail("<b>Context:</b> " + __test_context +
                      "\n<red>Unexpected exception thrown (" + context + "): " + str(e) + "</red>")


def EXPECT_STDOUT_CONTAINS(text, note=None):
    out = testutil.fetch_captured_stdout(False)
    err = testutil.fetch_captured_stderr(False)
    if out.find(text) == -1:
        if not note:
            note = __caller_context()
        context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text + \
            "\n<yellow>Actual stdout:</yellow> " + out + \
            "\n<yellow>Actual stderr:</yellow> " + err
        testutil.fail(context)


def EXPECT_STDERR_CONTAINS(text, note=None):
    out = testutil.fetch_captured_stdout(False)
    err = testutil.fetch_captured_stderr(False)
    if err.find(text) == -1:
        if not note:
            note = __caller_context()
        context = "<b>Context:</b> " + __test_context + "\n<red>Missing output:</red> " + text + \
            "\n<yellow>Actual stdout:</yellow> " + out + \
            "\n<yellow>Actual stderr:</yellow> " + err
        testutil.fail(context)


def WIPE_STDOUT():
    line = testutil.fetch_captured_stdout(True)
    while line != "":
        line = testutil.fetch_captured_stdout(True)


def WIPE_STDERR():
    line = testutil.fetch_captured_stderr(True)
    while line != "":
        line = testutil.fetch_captured_stderr(True)


def WIPE_OUTPUT():
    WIPE_STDOUT()
    WIPE_STDERR()


def WIPE_SHELL_LOG():
    testutil.wipe_file_contents(testutil.get_shell_log_path())


def EXPECT_SHELL_LOG_CONTAINS(text, note=None):
    log_file = testutil.get_shell_log_path()
    match_list = testutil.grep_file(log_file, text)
    if len(match_list) == 0:
        log_out = testutil.cat_file(log_file)
        testutil.fail(f"<b>Context:</b> {__test_context}\n<red>Missing log output:</red> {text}\n<yellow>Actual log output:</yellow> {log_out}")

def EXPECT_SHELL_LOG_CONTAINS_COUNT(text, count):
    if not isinstance(count, int):
        raise TypeError('"count" argument must be a number.')
    log_file = testutil.get_shell_log_path()
    match_list = testutil.grep_file(log_file, text)

    if len(match_list) != count:
        log_out = testutil.cat_file(log_file)
        testutil.fail(
            f"<b>Context:</b> {__test_context}\n<red>Missing log output:</red> {text}\n<yellow>Actual log output:</yellow> {log_out}")


def EXPECT_SHELL_LOG_NOT_CONTAINS(text, note=None):
    log_file = testutil.get_shell_log_path()
    match_list = testutil.grep_file(log_file, text)
    if len(match_list) != 0:
        if not note:
            note = __caller_context()
        log_out = testutil.cat_file(log_file)
        testutil.fail(
            f"<b>Context:</b> {__test_context}\n<red>Unexpected log output:</red> {text}\n<yellow>Actual log output:</yellow> {log_out}")


def EXPECT_SHELL_LOG_MATCHES(re, note=None):
    log_file = testutil.get_shell_log_path()
    with open(log_file, "r", encoding="utf-8") as f:
        log_out = f.read()
    if re.search(log_out) is None:
        if not note:
            note = __caller_context()
        testutil.fail(
            f"<b>Context:</b> {__test_context}\n<red>Missing match for:</red> {re.pattern}\n<yellow>Actual log output:</yellow> {log_out}")


def EXPECT_STDOUT_MATCHES(re, note=None):
    out = testutil.fetch_captured_stdout(False)
    err = testutil.fetch_captured_stderr(False)
    if re.search(out) is None:
        if not note:
            note = __caller_context()
        context = "<b>Context:</b> " + __test_context + "\n<red>Missing match for:</red> " + re.pattern + \
            "\n<yellow>Actual stdout:</yellow> " + out + \
            "\n<yellow>Actual stderr:</yellow> " + err
        testutil.fail(context)


def EXPECT_STDOUT_NOT_CONTAINS(text, note=None):
    out = testutil.fetch_captured_stdout(False)
    err = testutil.fetch_captured_stderr(False)
    if out.find(text) != -1:
        if not note:
            note = __caller_context()
        context = "<b>Context:</b> " + __test_context + "\n<red>Unexpected output:</red> " + text + \
            "\n<yellow>Actual stdout:</yellow> " + out + \
            "\n<yellow>Actual stderr:</yellow> " + err
        testutil.fail(context)


def EXPECT_FILE_CONTAINS(expected, path, note=None):
    with open(path, encoding='utf-8') as f:
        contents = f.read()
    if contents.find(expected) == -1:
        if not note:
            note = __caller_context()
        context = "<b>Context:</b> " + __test_context + "\n<red>Missing contents:</red> " + expected + \
            "\n<yellow>Actual contents:</yellow> " + \
            contents + "\n<yellow>File:</yellow> " + path
        testutil.fail(context)


def EXPECT_FILE_MATCHES(re, path, note=None):
    with open(path, encoding='utf-8') as f:
        contents = f.read()
    if re.search(contents) is None:
        if not note:
            note = __caller_context()
        testutil.fail(
            f"<b>Context:</b> {__test_context}\n<red>Missing match for:</red> {re.pattern}\n<yellow>Actual contents:</yellow> {contents}\n<yellow>File:</yellow> {path}")


def EXPECT_FILE_NOT_CONTAINS(expected, path, note=None):
    with open(path, encoding='utf-8') as f:
        contents = f.read()
    if contents.find(expected) != -1:
        if not note:
            note = __caller_context()
        context = "<b>Context:</b> " + __test_context + "\n<red>Unexpected contents:</red> " + \
            expected + "\n<yellow>Actual contents:</yellow> " + \
            contents + "\n<yellow>File:</yellow> " + path
        testutil.fail(context)


def EXPECT_FILE_NOT_MATCHES(re, path, note=None):
    with open(path, encoding='utf-8') as f:
        contents = f.read()
    if re.search(contents) is not None:
        if not note:
            note = __caller_context()
        testutil.fail(
            f"<b>Context:</b> {__test_context}\n<red>Unexpected match for:</red> {re.pattern}\n<yellow>Actual contents:</yellow> {contents}\n<yellow>File:</yellow> {path}")


def validate_crud_functions(crud, expected):
    actual = dir(crud)

    # Ensures expected functions are on the actual list
    missing = []
    for exp_funct in expected:
        try:
            pos = actual.index(exp_funct)
            actual.remove(exp_funct)
        except:
            missing.append(exp_funct)

    if len(missing) == 0:
        print("All expected functions are available\n")
    else:
        print("Missing Functions:", missing)

    if len(actual) == 0 or (len(actual) == 1 and actual[0] == 'help'):
        print("No additional functions are available\n")
    else:
        print("Extra Functions:", actual)


def validate_members(object, expected_members):
    all_members = dir(object)

    # Remove the python built in members
    members = []
    for member in all_members:
        if not member.startswith('__'):
            members.append(member)

    missing = []
    for expected in expected_members:
        try:
            index = members.index(expected)
            members.remove(expected)
        except:
            missing.append(expected)

    errors = []
    error = ""
    if len(members):
        error = "Unexpected Members: %s" % ', '.join(members)
        errors.append(error)

    error = ""
    if len(missing):
        error = "Missing Members: %s" % ', '.join(missing)
        errors.append(error)

    if len(errors):
        testutil.fail(', '.join(errors))


def print_differences(source, target):
    src_lines = []
    tgt_lines = []

    with open(source) as f:
        src_lines = f.readlines()

    with open(target) as f:
        tgt_lines = f.readlines()

    for line in difflib.context_diff(src_lines, tgt_lines, fromfile=source, tofile=target):
        testutil.dprint(line)


def ensure_plugin_enabled(plugin_name, session, plugin_soname=None):
    if plugin_soname is None:
        plugin_soname = plugin_name

    os = session.run_sql('select @@version_compile_os').fetch_one()[0]

    if os == "Win32" or os == "Win64":
        ext = "dll"
    else:
        ext = "so"

    try:
        session.run_sql("INSTALL PLUGIN {0} SONAME '{1}.{2}';".format(
            plugin_name, plugin_soname, ext))
    except mysqlsh.DBError as e:
        if 1125 != e.code:
            raise e


def ensure_plugin_disabled(plugin_name, session):
    is_installed = session.run_sql(
        "SELECT COUNT(1) FROM INFORMATION_SCHEMA.PLUGINS WHERE PLUGIN_NAME LIKE '" + plugin_name + "';").fetch_one()[0]

    if is_installed:
        session.run_sql("UNINSTALL PLUGIN " + plugin_name + ";")

# Starting 8.0.24 the client lib started reporting connection error using
# host:port format, previous versions used just the host.
#
# This function is used to format the host description accordingly.


def libmysql_host_description(hostname, port):
    if testutil.version_check(__libmysql_version_id, ">", "8.0.23"):
        return hostname + ":" + str(port)

    return hostname


def get_socket_path(session, uri=None):
    if uri is None:
        uri = session.uri
    row = session.run_sql(
        f"SELECT @@{'socket' if 'mysql' == shell.parse_uri(uri).scheme else 'mysqlx_socket'}, @@datadir").fetch_one()
    if row[0][0] == '/':
        p = row[0]
    else:
        p = os.path.join(row[1], row[0])
    if len(p) > 100:
        testutil.fail("socket path is too long (>100): " + p)
    return p


def get_socket_uri(session, uri=None):
    if uri is None:
        uri = session.uri
    parsed = shell.parse_uri(uri)
    if "password" not in parsed:
        parsed["password"] = ""
    new_uri = {}
    for key in ["scheme", "user", "password"]:
        new_uri[key] = parsed[key]
    new_uri["socket"] = get_socket_path(session, uri)
    return shell.unparse_uri(new_uri)


def random_string(lower, upper=None):
    if upper is None:
        upper = lower
    return ''.join(random.choices(string.ascii_letters + string.digits, k=random.randint(lower, upper)))


def random_email():
    return random_string(10, 40) + "@" + random_string(10, 40) + "." + random_string(3)


def md5sum(s):
    return hashlib.md5(s.encode("utf-8")).hexdigest()


class Docker_manipulator:
    def __init__(self, docker_name, data_path):
        self.data_path = data_path
        self.dockerfile = os.path.join(
            self.data_path, docker_name, "Dockerfile")
        self.docker_name = docker_name
        self.docker_image_name = "mysql-sshd-shell-{}".format(docker_name)

        if not os.path.exists(self.dockerfile):
            testutil.fail(
                "The Dockerfile is missing for {}".format(docker_name))

        self.hash = self.__get_hash()
        import docker
        self.client = docker.from_env()
        self.container = None

    def __get_hash(self):
        import hashlib
        with open(self.dockerfile, "rb") as f:
            return hashlib.sha256(f.read()).hexdigest()

    def __pre_run_check(self):
        # before runnig it check if there's already such container if so, kill it and start again
        try:
            print("About to find old container")
            c = self.client.containers.get(self.docker_name)
            if c.status == "running":
                c.kill()
            self.client.containers.prune()
            print("Old container found and removed")
        except:
            # this should be fine as containers.get will throw when container is not found
            pass

        try:
            print("Checking image tag")
            self.img = self.client.images.get(self.docker_image_name)
            if self.img.attrs.get("ContainerConfig").get("Labels").get("hash") != self.hash:
                self.client.images.remove(image=self.img.id, force=True)
                print("Old image found which is outdated, removed")
        except Exception as e:
            pass

    def __build_and_run(self):
        try:
            self.img
        except:
            self.img = self.client.images.build(path=os.path.join(
                self.data_path, self.docker_name), tag=self.docker_image_name,
                labels={"hash": self.hash})[0]
            print("Build new docker image")

        self.container = self.client.containers.run(image=self.img.id, detach=True, name=self.docker_name,
                                                    environment=["MYSQL_ROOT_PASSWORD=sandbox"], ports={"2222/tcp": 2222})
        print("Started new container")

    def log_watcher(self, log_fetcher):
        while True:
            try:
                self._queue.put_nowait(log_fetcher.next())
            except:
                break

    def run(self):
        self._queue = queue.Queue(10)
        self.__pre_run_check()
        self.__build_and_run()

        log_iter = self.container.logs(stream=True)
        watcher = threading.Thread(target=self.log_watcher, args=(log_iter,))
        watcher.start()

        while True:
            if self.client.containers.get(self.container.id).status == "running":
                break
            else:
                time.sleep(1)

        while True:
            try:
                line = self._queue.get(block=True, timeout=10)
                match = re.search(
                    br"port: ([0-9]{4})\s MySQL Community Server - GPL", line)
                if match:
                    print("Found port: {:g}".format(match.groups()[0]))
                    log_iter.close()
                    break
            except Exception as e:
                break
        print("Waiting for container to fully start")
        log_iter.close()
        watcher.join()
        print("Container is ready")

    def cleanup(self, remove_image=False):
        if self.container is not None:
            self.container.kill()  # kill the container
            # the python client.images.build leave a mess, clean it
            self.client.containers.prune()
            if remove_image:
                self.client.images.remove(image=self.img.id, force=True)
