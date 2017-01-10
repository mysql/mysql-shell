.. _`execute-engine`:

****************
Execution Engine
****************

The key role of the |orchestrator| is to handle execution of
jobs on explicit request or when certain events occur. Each job
correspond to an explicit gadget invokation described in :ref:`execute-job`.
Each task might be composed by a set of jobs (one or more).

.. _`gadgets-jobs`:

Gadgets and Jobs
================

.. uml::
   :align: right
   :caption: **Figure 1.** Task for adding server to group replication

   (*) --> if "Replication user exists" then
     -right-> [false] if "Binary log is enabled" then
       -right->[true] "Stop server"
       --> "Turn off binary log"
       --> "Start server"
       -left-> "Create replication user" as CreateUser
     else
       --> [false] CreateUser
     endif
     --> "Set replication privileges"
     -left-> "Stop server" as StopServer2
   else
     --> [true] StopServer2
   endif
   --> "Edit configuration file"
   note right
    log-bin
    binlog-format=row
    gtid-mode=on
    enforce-gtid-consistency
    log-slave-updates
    master-info-repository=TABLE
    relay-log-info-repository=TABLE
    transaction-write-set-extraction=MURMUR32
   end note
   --> "Start server with group replication plugin" as StartServer2
   --> "Set server variables"
   note right
     group_replication_group_name = <UUID>
     group_replication_recovery_user = <user>
     group_replication_recovery_password = <password>
     group_replication_local_address = <address>
     group_replication_peer_addresses = <peer-addresses>
   end note
   if "Bootstrapping New Group" then
      -right-> [true] "Enable bootstrap group replication variable"
      --> "Start group replication" as StartGR2
      --> "Wait until server is on-line" as Wait2
      --> "Disable bootstrap group variable"
      --> (*)
   else
      --> [false] "Start group replication" as StartGR1
      --> "Wait until server is on-line" as Wait1
      --> (*)
   endif

*Tasks* that the database administrator (or anybody
else) want to execute. They consist of a sequence of *jobs* ( *steps*
or *activities*) that are executed to perform the specific task. An
example of a task to add a server to a group can be seen in
Figure 1.

In the orchestrator, the jobs are provided by *MySQL Gadgets* which
are executable files that can be invoked to execute the job.
In order to collaborate with the orchestrator, the gadgets
need to follow specific conventions on what options to accept, what
information to provide to the orchestrator, and what input to accept.
The protocol covering the interaction between the orchestrator and the
gadgets are outlined in the section :ref:`gadget-protocol`.  You can
find more information in the :ref:`mysql-gadgets` chapter.

When a *job* is started through a gadget, it is running
inside the orchestrator and is assigned an identifier in the form
of a *job UUID*. If a gadget is started multiple times, each job
will receive a distinct job UUID. While running, the job maintain the
state of the execution inside the orchestrator by recording:

- Job UUID.

- Time the job started

- Gadget used to execute the job.

- Parameters passed to the gadget.

- Locks on objects required for the execution of the job.

- Current activity executing.

- State of execution, e.g., is it waiting for some action.

- Progress of current activity executing (optional).

An example of three jobs with their associated information is (a few
fields are skipped):
  
+----------+------------------------+------------------------+------------------------+
|          | Job 1                  | Job 2                  | Job 3                  |
+==========+========================+========================+========================+
| UUID     | d8c1bd32-...d04b2c47a3 | db2e6d54-...c27b5ec137 | dff9d81e-...ca5b3fc28c |
+----------+------------------------+------------------------+------------------------+
| Start    | 03/27/2016 09:02:58 PM | 03/27/2016 09:02:58 PM | 03/27/2016 09:02:58 PM |
+----------+------------------------+------------------------+------------------------+
| Gadget   | mysql-group-add        | mysql-group-add        | mysql-shard-split      |
+----------+------------------------+------------------------+------------------------+
| Param    | slave3.example.com     | slave2.example.com     | shard1.example.com     |
|          |                        |                        | shard2.example.com     |
|          |                        |                        | shard3.example.com     |
+----------+------------------------+------------------------+------------------------+
| Locks    | slave3.example.com [X] | slave2.example.com [X] | shard1.example.com [S] |
|          | slave1.example.com [S] | slave1.example.com [S] | shard2.example.com [X] |
|          |                        |                        | shard3.example.com [X] |
+----------+------------------------+------------------------+------------------------+
| Activity | Cloning server         | Starting server        | Filling shards         |
+----------+------------------------+------------------------+------------------------+
| State    | Copying                | Waiting                | Copying                |
+----------+------------------------+------------------------+------------------------+
| Progress | 25%                    | 78%                    | 2.36 / 5 GiB           |
+----------+------------------------+------------------------+------------------------+


.. _`fabric-recovery`:

Job Execution and Recovery
==========================

The main goal of the |orchestrator| is to execute various
administrative tasks on the behalf of the database administrator
so that he do not have to monitor the progress of different
activities all the time and can automate even complex administrative
tasks taking several days to accomplish.

To be useful to the administrator, it is necessary that the
orchestrator have features that allow the it to avoid scheduling
conflicting jobs, ensure that job execution state is recoverable, and
that a job can be aborted.

Get Job Locks
-------------

Since different jobs access different resources it is critical
to avoid two jobs working with the same resource. For example,
taking a server out of production and making it group leader at the
same time is clearly not very sensible.

The |orchestrator| handle this by implementing a *lock manager* that
also handle scheduling of tasks that access conflicting resources.
Locks are acquired before the job starts executing and are
released when the job stop executing.

The jobs manipulate external resources and not only data in the
metadata storage, so in order to avoid the need for deadlock detection
and "rolling back" changes to external resources, a conservative
locking approach is used.  This means that the gadgets need to declare
the resources as well as the type of lock that is needed they intend
to use when given the :option:`--getlocks` option, which is called
prior to starting a job.

Resources are always physical entities, for example, MySQL servers or
hardware, so the gadget need to announce what resources it need to
lock and what kind of lock is required.  The orchestrator will then
put the job in waiting state until the requested locks can be
acquired. Once the locks are acquired, the job will start executing
the gadget as described in :ref:`execute-job`.

.. note::

   It is still an open question if we should allow locks on logical
   entities, e.g., groups and shards. Concrete examples of situations
   where these resources need to be locked are welcome.

.. uml::
   :caption: Figure 2. Getting job locks from gadget

   participant "Orchestrator" as Orch
   participant "mysql-server-copy" as Gadget

   activate Orch
   -> Orch: mysql-orchestrator -c mysql-server-copy\nsource.example.com target.example.com
   Orch -> Gadget: mysql-server-copy ~--protocol=text-1.0 ~--getlocks\nsource.example.com target.example.com
   activate Gadget
   Orch <- Gadget: *lock declaration*
   Orch <- Gadget: ""EXIT_SUCCESS""
   deactivate Gadget
   ... wait for locks to be acquired ...


.. _execute-job:

Executing a Job
---------------

To start executing a job once the locks are acquired,
|orchestrator| simply executes the gadgets with the parameters
(i.e., options and arguments) given by the user.

.. uml::
   :caption: Figure 3. Starting a job

   participant "Orchestrator" as Orch
   participant "mysql-server-copy" as Gadget

   Orch -> Gadget: mysql-server-copy ~--protocol=text-1.0\nsource.example.com target.example.com
   activate Gadget
   Orch <-- Gadget: ""STEP Creating backup image""
   Orch <-- Gadget: ""STEP Copy backup image source.example.com target.example.com""
   ...
   Orch <-- Gadget: ""EXIT_SUCCESS""
   deactivate Gadget


.. _execute-resilient:

Resilient Execution
-------------------

|orchestrator| support *resilient execution* in the sense that if the
execution of a job fails because of a crash, |orchestrator| is
able to recover the execution at the point it was aborted.

In order to support resilient execution, it is necessary for the
|orchestrator| to ensure that the next job to execute is in durable
storage before starting to execute a job. The reason for this is
that the orchestrator shall be able to re-start a job in the event
of a catastrophic failure.

.. uml::
   :caption: **Figure 4.** Example of persisting and running a job

   participant "Orchestrator" as Orch
   participant Storage
   participant "mysql-server-copy" as Gadget

   activate Storage
   Orch --> Storage: write("""JOB START mysql-server-copy ...""")
   Orch --> Storage: sync
   Orch <-- Storage: OK
   deactivate Storage
   activate Orch
   Orch --> Gadget: mysql-server-copy ~--protocols=text-1.0 \nsource.example.com target.example.com
   activate Gadget
   Orch <-- Gadget: ""STEP Creating backup image""
   Orch <-- Gadget: ""STEP Copy backup image source.example.com target.example.com""
   ...
   Orch <-- Gadget: ""EXIT_SUCCESS""
   deactivate Gadget
   activate Storage
   Orch --> Storage: write("""JOB DONE mysql-server-copy ...""")
   Orch --> Storage: sync
   Orch <-- Storage: OK
   deactivate Storage
   <-- Orch: DONE

If the exit code is 0 (``EXIT_SUCCESS``),
it is assumed that the job completed successfully.   

.. _execute-recover:

Execution Recovery
------------------

If an activity fails because of an orchestrator failure, it is
necessary to recover the execution from where it was aborted. Since a
partially executed activity potentially can have left garbage that was
not cleaned up, it is necessary to execute undo actions 
prior to re-starting the job that failed.

Note that for the recovery to work property, it is necessary that:

- The undo action is idempotent.

  If there is a crash in the middle of an undo action, it need to be
  possible to re-execute the undo action and get the same result
  regardless of whether the previous execution completed successfully
  before the crash or not.

- The gadget need to detect if the activity has been done.

  An activity can actually have executed to completion before
  exiting with success, but it was not recorded before the crash
  and therefore an attempt will be made to recovery the completed
  activity.

An example reverting the changes of the job in the previous subsection would
be:

.. uml::
   :caption: **Figure 5.** Example of execution of a compensating action

   participant "Orchestrator" as Orch
   participant Storage
   participant "mysql-server-copy" as Gadget

   --> Orch: start
   activate Orch
   ...
   Orch  -[#white]> Storage
   activate Storage
   Orch <-- Storage: read("JOB START mysql-server-copy ...")
   deactivate Storage
   Orch --> Gadget: mysql-server-copy ~--undo ~--protocols=text-1.0\nsource.example.com target.example.com
   activate Gadget
   Orch <-- Gadget: STEP ...
   ...
   Orch <-- Gadget: ""EXIT_SUCCESS""
   deactivate Gadget
   activate Storage
   Orch --> Storage: write("JOB UNDO mysql-server-copy ...")
   Orch --> Storage: sync
   Orch <-- Storage: OK
   deactivate Storage
   <-- Orch: DONE


Aborting a Job
--------------

A running job can be aborted by calling the gadget with the
:option:`--abort` option. For this case, a complete cleanup of the
job should be done.