//@ Initialization
||

//@ Create single-primary cluster
||

//@ Success adding instance 2
||

//@ Success adding instance 3
||

//@ Cluster.dissolve no force error
||Cluster.dissolve: Cannot drop cluster: The cluster is not empty.

//@ Success dissolving single-primary cluster
||

//@ Cluster.dissolve already dissolved
||Cluster.dissolve: Can't call function 'dissolve' on a dissolved cluster

//@ Create multi-primary cluster
||

//@ Success adding instance 2 mp
||

//@ Success adding instance 3 mp
||

//@ Success dissolving multi-primary cluster
||

//@ Create single-primary cluster 2
||

//@ Success adding instance 2 2
||

//@ Success adding instance 3 2
||

//@ Success dissolving cluster 2
||

//@ Create multi-primary cluster 2
||

//@ Success adding instance 2 mp 2
||

//@ Success adding instance 3 mp 2
||

//@ Success dissolving multi-primary cluster 2
||

//@ Finalization
||
