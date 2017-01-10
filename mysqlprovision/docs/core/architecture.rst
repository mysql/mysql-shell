***********************
High Level Architecture
***********************

The |orchestrator| is a plugin to the `MySQL Harness` and consist
of an execution engine, a lock manager, and functionality to handle
monitoring of the |orchestrator|. In addition to that, it is possible
to access the |orchestrator| through network interfaces available in
the |harness|.

Separate from the |orchestrator| a collection of |gadgets| are
available. The gadgets are executables designed to do a very specific
task, such as starting or stopping a server. The |gadgets| have
identical calling conventions on all platforms and are installed in a
generic location.

In Figure :ref:`orchestrator-architecture` you can see an illustration
of the high-level architecture of the orchestrator.

.. _orchestrator-architecture:

.. figure:: ../figs/MySQLOrchestrator.*
   :width: 15cm

   MySQL Orchestrator High Level Architecture
