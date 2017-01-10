************
Introduction
************

The role of the |orchestrator| is to provide a platform for resilient
execution of administrative and management procedures. The
|orchestrator| works as the "extended arm" of the database
administrator and allow standard tasks to be automated.


Concepts and terminology
========================

**mysqld instance**
  A running instance of MySQL server.

**farm**
  The collection of nodes that make up the database layer.

**node**
  A mysqld instance, a router, an orchestrator, or any other running
  instance that deliver a service for the database layer.

**high availability group**
  A high availability group is a set of servers acting together to
  deliver a high availability database service. In this specification,
  MySQL Group Replication is used to deliver the high availability of
  the service, but the concept is generic and is not restricted to any
  specific implementation.

  An important aspect of the high availability group is that all
  servers contain the same data. This make reasoning about the groups
  easy and administration clear.

  We refer to this just as "groups" when it is clear from the context.
