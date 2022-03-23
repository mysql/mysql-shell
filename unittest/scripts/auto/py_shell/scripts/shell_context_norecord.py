# this unittest is used to test scopes creations
#@<> Check delegates
import threading
import queue
callback_print_queue = queue.Queue()
callback_error_queue = queue.Queue()
callback_diag_queue = queue.Queue()

def thread_sample_print(thread_name):
    prompt_return = True
    def print_delegate(text):
        callback_print_queue.put(text)
    def diag_delegate(text):
        callback_diag_queue.put(text)
    def prompt_delegate(prompt, options):
        ret_caption = prompt
        if "defaultValue" in options:
            ret_caption = options["defaultValue"]
        ret_value=f"'{ret_caption}', options: {str(options)}"
        return prompt_return, ret_value
    ctx = shell.create_context({"printDelegate": print_delegate, "promptDelegate": prompt_delegate,
                                "diagDelegate": diag_delegate})
    sh = ctx.get_shell()
    print("this will be available on the callback queue")
    EXPECT_EQ("'caption', options: {\"type\": \"text\"}", sh.prompt("caption"))
    EXPECT_EQ("'caption', options: {\"description\": [\"Paragraph 1\", \"Paragraph 2\"], \"type\": \"text\"}", sh.prompt("caption", {"description":["Paragraph 1", "Paragraph 2"]}))
    EXPECT_EQ("'caption', options: {\"type\": \"password\"}", sh.prompt("caption", {'type':'password'}))
    EXPECT_EQ("'Other Value', options: {\"defaultValue\": \"Other Value\", \"title\": \"Titled Prompt\", \"type\": \"text\"}", sh.prompt("caption", {"title":"Titled Prompt", "defaultValue":"Other Value"}))
    EXPECT_EQ("'caption', options: {\"title\": \"File Open\", \"type\": \"fileOpen\"}", sh.prompt("caption", {"title":"File Open", "type":"fileOpen"}))
    EXPECT_EQ("'caption', options: {\"title\": \"File Save\", \"type\": \"fileSave\"}", sh.prompt("caption", {"title":"File Save", "type":"fileSave"}))
    EXPECT_EQ("'caption', options: {\"title\": \"Open Folder\", \"type\": \"directory\"}", sh.prompt("caption", {"title":"Open Folder", "type":"directory"}))
    EXPECT_EQ("'caption', options: {\"no\": \"&No\", \"type\": \"confirm\", \"yes\": \"&Yes\"}", sh.prompt("caption", {"type":"confirm"}))
    EXPECT_EQ("'caption', options: {\"alt\": \"Cancel\", \"no\": \"&Reject\", \"type\": \"confirm\", \"yes\": \"&Accept\"}", sh.prompt("caption", {"type":"confirm", "yes":"&Accept", "no": "&Reject", "alt": "Cancel"}))
    EXPECT_EQ("'caption', options: {\"options\": [\"one\", \"two\", \"three\"], \"type\": \"select\"}", sh.prompt("caption", {"type":"select", "options":["one", "two", "three"]}))
    prompt_return = False
    EXPECT_THROWS(lambda: sh.prompt("caption"), "Cancelled")
    EXPECT_THROWS(lambda: sh.prompt("caption", {'type':'password'}), "Cancelled")
    EXPECT_THROWS(lambda: sh.prompt("caption", {'type':'fileOpen'}), "Cancelled")
    EXPECT_THROWS(lambda: sh.prompt("caption", {'type':'fileSave'}), "Cancelled")
    EXPECT_THROWS(lambda: sh.prompt("caption", {'type':'directory'}), "Cancelled")
    EXPECT_THROWS(lambda: sh.prompt("caption", {'type':'confirm'}), "Cancelled")
    EXPECT_THROWS(lambda: sh.prompt("caption", {'type':'select', 'options': ['one', 'two']}), "Cancelled")
    ctx.finalize()
    sh = ctx.get_shell() # this will cause error to be shown on the diag queue

th = threading.Thread(target = thread_sample_print, args=("my thread", ))
th.start()
print("normal print")
EXPECT_STDOUT_CONTAINS("normal print")
th.join()
diag_message = []
while True:
    try:
        diag_message.append(callback_diag_queue.get_nowait())
    except:
        break
print("===>", "".join(diag_message))
EXPECT_TRUE("".join(diag_message).find("The Shell context has been finalized already") != -1)
print_message = []
while True:
    try:
        print_message.append(callback_print_queue.get_nowait())
    except:
        break
EXPECT_EQ("this will be available on the callback queue", "".join(print_message).rstrip())

#@<> Check if works without delegates
def thread_sample_print(thread_name):
    ctx = shell.create_context({})
    print("this will print nowhere")
    del ctx
th = threading.Thread(target = thread_sample_print, args=("my thread", ))
th.start()
th.join()
EXPECT_TRUE(callback_print_queue.empty())

#@<> Check if works with bad input options, nothing should happen
def thread_sample_test_opts(thread_name):
        EXPECT_THROWS(lambda: shell.create_context({"printDelegate": "bad print delegate", "promptDelegate":"bad prompt delegae"}), "TypeError")
th = threading.Thread(target = thread_sample_test_opts, args=("my thread", ))
th.start()
th.join()

#@<> Check if we can register report on the context and get help
def thread_sample_report(thread_name):
    def print_delegate(text):
        callback_print_queue.put(text)
    ctx = shell.create_context({"printDelegate":print_delegate})
    sh = ctx.get_shell()
    sess = sh.connect(__mysqluripwd)
    callback_print_queue.queue.clear() # we must clean the print queue here
    def report(s):
        print('Output from a report')
        return {'report' : []}
    sh.register_report('report', 'print', report)
    EXPECT_THROWS(lambda: shell.help('report'), "RuntimeError") # report shouldn't be available in the global ctx
    print(sh.reports.help('report'))
    print(sh.reports.report(sess)) # check if report has been properly registered
    del sh
    del ctx
th = threading.Thread(target = thread_sample_report, args=("my thread", ))
th.start()
th.join()
print_message = []
while True:
    try:
        print_message.append(callback_print_queue.get_nowait())
    except:
        break
expected_message="""NAME
      report

SYNTAX
      shell.reports.report(session)

WHERE
      session: Object - A Session object to be used to execute the report.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.
Output from a report
{"report": []}"""
EXPECT_EQ(expected_message, ''.join(print_message).rstrip())

#@<> Check log location
tmp_file = os.path.join(__tmp_dir,"context_test.log")
if os.path.exists(tmp_file):
    os.remove(tmp_file)
def thread_sample_log(thread_name):
    ctx = shell.create_context({"logFile": tmp_file})
    sh = ctx.get_shell()
    sh.log("INFO","Sample error log line")
    del sh
    del ctx
th = threading.Thread(target = thread_sample_log, args=("my thread", ))
th.start()
th.join()
EXPECT_TRUE(os.path.exists(tmp_file), "File: {} doesn't exist".format(tmp_file))
with open(tmp_file, 'r') as f:
    line = f.read().rstrip()
    EXPECT_TRUE(line.endswith("Info: Sample error log line"), "Log file is missing correct data")
os.remove(tmp_file)

#@<> Try to log to directory without proper permissions
import os
log_location = os.path.join("/usr", "context_test.log")
if __os_type == "windows":
    log_location = os.environ["TMPDIR"]
def thread_sample_log_permission(thread_name):
    EXPECT_THROWS(lambda: shell.create_context({"logFile": log_location}), "RuntimeError")
th = threading.Thread(target = thread_sample_log_permission, args=("my thread", ))
th.start()
th.join()

#@<> Try to log to directory instead of a file
import os
log_directory = "/usr"
if __os_type == "windows":
    log_directory = os.environ["WINDIR"]
def thread_sample_log_dir(thread_name):
    EXPECT_THROWS(lambda: shell.create_context({"logFile": log_directory}), "RuntimeError")
th = threading.Thread(target = thread_sample_log_dir, args=("my thread", ))
th.start()
th.join()

#@<> Create extension object on the context
def thread_sample_extension_object(thread_name):
    def print_delegate(text):
        callback_print_queue.put(text)
    def diag_delegate(text):
        callback_print_queue.put(text)
    ctx = shell.create_context({"printDelegate":print_delegate, "diagDelegate": diag_delegate})
    sh = ctx.get_shell()
    obj = sh.create_extension_object()
    sh.register_global("myCtxGlobalObject", obj, {"brief": "sample brief", "details":["details first line", "details second line"]})
    EXPECT_TRUE("myCtxGlobalObject" in ctx.globals, "myCtxGlobalObject not found on the context") # check if object is visible as a global
    sh.add_extension_object_member(obj, "myCtxInteger", 5, {"brief": "Simple description for integer member.",
                                              "details": ["The member myInteger should only be used for obscure purposes."]
                                              })
    EXPECT_TRUE("my_ctx_integer" in dir(ctx.globals.myCtxGlobalObject), "myCtxInteger not found in the myCtxGlobalObject")
    del sh
    del ctx
th = threading.Thread(target = thread_sample_extension_object, args=("my thread", ))
th.start()
th.join()

#@<> Test multiple threads accessing a session
def thread_sample_select(thread_no):
    ctx = shell.create_context({})
    sh = ctx.get_shell()
    sess = sh.connect(__mysqluripwd)
    res = sess.run_sql("SELECT {} FROM DUAL".format(thread_no))
    EXPECT_EQ(thread_no, res.fetch_one()[0])
    del sh
    del ctx

threads = []
for i in range(0, 5):
    th = threading.Thread(target = thread_sample_select, args=(i,))
    th.start()
    threads.append(th)

for th in threads:
    th.join()

#@<> Test create_context from main thread (fail)
EXPECT_THROWS(lambda: shell.create_context({}), "Shell.create_context: This function cannot be called from the main thread")
