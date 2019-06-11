// NOTE: Cannot be recorder (use testutil.deploySandbox()) because the
// innodb_page_size must be set during initialization and cannot be changed
// after that.
if (real_host_is_loopback) {
  testutil.skip("This test requires that the hostname does not resolve to loopback.");
}

var __sandbox_dir = testutil.getSandboxPath();

//Auxiliary function to wait (from setup.js)
function wait(timeout, wait_interval, condition){
  waiting = 0;
  res = condition();
  while(!res && waiting < timeout) {
    os.sleep(wait_interval);
    waiting = waiting + 1;
    res = condition();
  }
  return res;
}

//Function to clean sandbox (from setup.js).
function cleanup_sandbox(port) {
    println ('Stopping the sandbox at ' + port + ' to delete it...');
    try {
      stop_options = {}
      stop_options['password'] = 'root';
      if (__sandbox_dir != '')
        stop_options['sandboxDir'] = __sandbox_dir;

      dba.stopSandboxInstance(port, stop_options);
    } catch (err) {
      println(err.message);
    }

    options = {}
    if (__sandbox_dir != '')
      options['sandboxDir'] = __sandbox_dir;

    var deleted = false;

    print('Try deleting sandbox at: ' + port);
    deleted = wait(10, 1, function() {
      try {
        dba.deleteSandboxInstance(port, options);

        println(' succeeded');
        return true;
      } catch (err) {
        println(' failed: ' + err.message);
        return false;
      }
    });
    if (deleted) {
      println('Delete succeeded at: ' + port);
    } else {
      println('Delete failed at: ' + port);
    }
}

//@ Deploy instances (with specific innodb_page_size).
dba.deploySandboxInstance(__mysql_sandbox_port1, {allowRootFrom:"%", mysqldOptions: ["innodb_page_size=4k", "report_host="+hostname], password: 'root', sandboxDir:__sandbox_dir});
dba.deploySandboxInstance(__mysql_sandbox_port2, {allowRootFrom:"%", mysqldOptions: ["innodb_page_size=8k", "report_host="+hostname], password: 'root', sandboxDir:__sandbox_dir});

var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);

//@<OUT> checkInstanceConfiguration error with innodb_page_size=4k.
dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});

//@<OUT> configureLocalInstance error with innodb_page_size=4k.
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});

//@ create cluster fails with nice error if innodb_page_size=4k
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("test_cluster", {gtidSetIsComplete: true});

//@ create cluster works with innodb_page_size=8k (> 4k)
shell.connect(__sandbox_uri2);
var cluster = dba.createCluster("test_cluster", {gtidSetIsComplete: true});

//@ Clean-up deployed instances.
cluster.dissolve({force: true});
session.close();
cluster.disconnect();
cleanup_sandbox(__mysql_sandbox_port1);
cleanup_sandbox(__mysql_sandbox_port2);
