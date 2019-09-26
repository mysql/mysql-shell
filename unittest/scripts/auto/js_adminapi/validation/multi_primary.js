//@ Create single-primary cluster.
||

//@<OUT> GR enforce update everywhere checks is OFF for single-primary cluster.
OFF

//@ Switch cluster to multi-primary. {VER(>=8.0.13)}
||

//@ Add another instance to multi-primary cluster. {VER(>=8.0.13)}
||

//@<OUT> GR enforce update everywhere checks is ON on all instances. {VER(>=8.0.13)}
ON
ON
ON

//@ Remove one instance from the cluster. {VER(>=8.0.13)}
||

//@<OUT> GR enforce update everywhere checks must be OFF on removed instance (multi-primary). {VER(>=8.0.13)}
OFF

//@ Dissolve multi-primary cluster.
||

//@<OUT> GR enforce update everywhere checks is OFF on all instances after dissolve (multi-primary). {VER(>=8.0.13)}
OFF
OFF

//@ Create multi-primary cluster.
||

//@<OUT> GR enforce update everywhere checks is ON for multi-primary cluster.
ON

//@ Switch cluster to single-primary. {VER(>=8.0.13)}
||

//@ Add another instance to single-primary cluster. {VER(>=8.0.13)}
||

//@<OUT> GR enforce update everywhere checks is OFF on all instances. {VER(>=8.0.13)}
OFF
OFF
OFF

//@ Remove one instance from single-primary cluster. {VER(>=8.0.13)}
||

//@<OUT> GR enforce update everywhere checks must be OFF on removed instance (single-primary). {VER(>=8.0.13)}
OFF

//@ Dissolve single-primary cluster.
||

//@<OUT> GR enforce update everywhere checks is OFF on all instances after dissolve (single-primary). {VER(>=8.0.13)}
OFF
OFF
