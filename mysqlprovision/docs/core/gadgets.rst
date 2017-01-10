.. _mysql-gadgets:

*******
Gadgets
*******

*MySQL Gadgets* are isolated software components used to execute
different administrative actions and are the nuts and bolts of
administrating the database tier.

MySQL Gadgets can be executed from the command line as well as from
the |orchestrator| and the MySQL Shell. Normal execution of MySQL
Gadgets print informative error messages as usual, but when executed
from the orchestrator a dedicated protocol is used, described
in :ref:`gadget-protocol`.


Naming Convention
=================

For all the gadgets, the format used is ``mysql-<entity>-<action>``
where *entity* is an subject that is being operated on and *action* is
a verb denoting what is being done with the entity.  We have opted to
use the ``mysql`` prefix since we want to support adding third-party
gadgets, so ``memcached-server-start`` could mean to start a
`Memcached <https://memcached.org/>`_ daemon.


Examples
========

In this section you will see some examples of gadgets and how they can
be used.

``mysql-server-clone``
  Gadget used to clone servers.

``mysql-gr-admin``
  Gadget used to administrate MySQL Group Replication, allowing the execution
  of actions such as: bootstrapping a group, adding a server to an existing
  group, or removing a server from a group.

Examples for how the gadgets above can be used are:

- To clone all the data from a server::

    mysql-server-clone --source=server1.example.com --target=server2.example.com

- To add a server to the single existing group, the following command could be issued::

    mysql-gr-admin --add --peer-server=server1.example.com --server=server2.example.com


These examples are not intended to be a specification but rather
reflect an idea of how the gadgets could be used.  The commands are
independent on the platform and are executed the same way on all
existing platforms.
