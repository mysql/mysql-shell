
MySQL Session Recorder and Replayer
===================================

1. What it is

Below is an abstract view of a shell test:

  +------+                         +-------------+                     +-------+
  | Test |   --function call-->    | Tested Code |     --DB APIs-->    | MySQL |
  +------+                         +-------------+                     +-------+
                                          |
                                          v
                                 +------------------+
                                 | System Resources |
                                 +------------------+

Our goal here is to allow unit-tests to execute deterministically. For that, the
test must be able to rely on a deterministic environment. Things like external files,
database state, time, random numbers, network must all behave in the exact way that
was assumed when the test was designed/written. If any of these change between runs,
they would be unintentionally testing different things.

The problem is that some of these things cannot be easily isolated. Mocks based on
abstract interfaces are a common way to replace external components with simulated
resources that behave the way we want, but that requires the code to be designed
and implemented in a way that allows them to be used. They also take a lot of work
to implement and update when things change. And they're also not suitable to test
changes in external components: they're good to see that the queries sent to MySQL
didn't change by accident, but they won't tell whether the code still works against
a newer MySQL version.

The session recorder and replayer are special objects that intercept MySQL DB API calls,
allowing database interactions to be recorded during test development and then
replayed during test runs.

So, in general, any shell operation that interacts with a DB should be executable without
actually connecting to any DB server, which makes them much faster.


The basic workflow is:

  1- Write test normally against a real database, in the exact state that we want
  2- Run the test in recording mode to ensure that the test passes and also
     to record all database interactions to trace files
  3- On day-to-day test runs, the tests will run in replay mode, which will
     use the trace files to simulate database sessions

Benefits:

  - Much faster test runs
  - Deterministic test runs
  - Easily run tests against all supported versions of MySQL
  - Easier to create databases mocks

2. How it works

The shell concentrates all its database operations into subclasses of a common
db::ISession interface.

In addition to the mysql::Session and mysqlx::Session classes,
a replay::Recorder and a replay::Replayer class exist.

replay::Recorder overrides the concrete Session classes and works exactly like
the real Session class, except that it intercepts all DB method calls and
records them into a JSON trace file, besids sending and receiving the queries
to the real DB at the same time. It's like a proxy that sits between the
client code and the real database.

replay::Replayer also overrides the concrete Session classes, but instead of
connecting to MySQL, it will load recorded request/response pairs from a trace file.
It will expect ISession calls to happen in the same order as they were recorded and
return the same results that were returned before to the caller.

2.1. mysqlprovision

Because mysqlprovision is an external program, it must use mysqlsh as its
Python interpreter so that the same overridable Session classes are used by it.
So, by calling mysqlprovision with mysqlsh, all DB operations it does will also
go through the same recording/replaying infrastructure as the rest of the shell.

2.2. sandboxes

Actual sandboxes should only be used during test runs that need real database
sessions. Test replays should not deal with sandboxes.

The testutils object can be used from both C++ and scripts to deploy, start,
stop and kill sandboxes. Whether the object will work with real sandboxes or
not can be controlled the same way as the session recorder/replayer.

testutils can also be used to create snapshots of my.cnf from sandboxes, so that
tests for code that edit config files (configureLocalInstance) will find
my.cnf in the same place as from sandboxes are actually running.

It is important that every test is self-contained and puts the target server in the
exact state it assumes as a pre-condition for the test case.

2.3. Random data

Functionality that rely on random data (like replication user creation)
must have the random generation part isolated into a class that can be replaced
during tests with a version that is not random.

utils/trandom.h serves that purpose.

A per-test case info file is also saved and contains things like:
* ports used for the sandboxes during recording, so that replay can use these
too regardless of how sandbox port environment vars are set
* hostname (as in @@hostname) from the recording environment

2.4. Other external resources

Network ports, files and other external resources that can vary must be either
mocked or recorded/replayed like DB sessions, to assure that tests will run the
same way no matter where they're executed.


3. How to run tests

By default, run_unit_tests will run in replay mode, using traces located in
unittest/traces/8.0.4/

If --direct is given, the replayer will be disabled and the tests
will run against the real database.

If --record=<version> is given, the tests will run against the real database
in recording mode and save traces under the <version> subdir.

--replay[=<version>] runs the tests using the given <version>, or the default
one.

3.1. Trace Files

One trace file is created per session. Each new session created has a new
auto-incremented number appended to its trace file name. Trace files created
through mysqlprovision have a mp_<counter> component to the filename, where
counter is the number of times mysqlprovision was called for non-sandbox
operations.

3.2. Updating trace files

Whenever some functionality changes, the entire test suite must be executed in
replay mode. If some test fails during replay, then it must be re-executed in
direct (or recording) mode, against a real database.

If a replay fails but direct execution works:
    * The session traces are outdated and must be re-recorded.
    * If the re-recording still fails, the test is not deterministic and
      the test case needs to be fixed.

If a replay fails and the direct execution also fails, then it can be:
    * test code must be updated
    * an actual bug that must be fixed

If a replay passes but the direct execution fails:
    * ???

To check and update trace files, you can run the unittest/traces/rebuild_traces
script. It will run all AdminAPI tests in replay mode, re-record the ones
that fail and then verify the newly recorded session.

3.3. Multiple MySQL versions

Because AdminAPI must support both MySQL 5.7 and 8.0, the test suite must be executed
against both versions and there must also be a trace file set for each.

When a new MySQL version is released and supported, all traces must be re-generated
for that new server.

Other variations may also be needed, like different trace file sets for servers running
in Windows and Linux.

