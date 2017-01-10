************************************
*mysql-server-clone* - Clone Servers
************************************
.. index:: mysql-server-clone

This section describe the architecture of the *mysql-server-clone* gadget that
implements the needed procedures to clone a MySQL server.


..
	Motivation
  	==========

    Before adding a new server to an active group, it is necessary to
    ensure that the server is up to date with respect to the application
    data in the group.

    If this is not done, it will be necessary for the new server to apply
    all binary logs already applied on the other servers. The obvious
    problem with this is that it can take considerable time to apply a
    long replication log, but if binary logs are rotated it might not even
    be possible.

    For that reason, a server cloning gadget is critical for being able to
    build a group of highly available servers using group replication.