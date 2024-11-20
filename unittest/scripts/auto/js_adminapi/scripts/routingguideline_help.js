//@<> Setup
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var cluster = scene.cluster
var rg;
EXPECT_NO_THROWS(function() { rg = cluster.createRoutingGuideline("myguideline");} );

//@ Object Help
\? routingguideline

//@ Name
rg.help("name")

//@ Name, \? [USE:Name]
\? routingguideline.name

//@ asJson
rg.help("asJson")

//@ asJson, \? [USE:asJson]
\? routingguideline.asJson

//@ destinations
rg.help("destinations")

//@ destinations, \? [USE:destinations]
\? routingguideline.destinations

//@ routes
rg.help("routes")

//@ routes, \? [USE:routes]
\? routingguideline.routes

//@ addDestination
rg.help("addDestination")

//@ addDestination, \? [USE:addDestination]
\? routingguideline.addDestination

//@ addRoute
rg.help("addRoute")

//@ addRoute, \? [USE:addRoute]
\? routingguideline.addRoute

//@ removeDestination
rg.help("removeDestination")

//@ removeDestination, \? [USE:removeDestination]
\? routingguideline.removeDestination

//@ removeRoute
rg.help("removeRoute")

//@ removeRoute, \? [USE:removeRoute]
\? routingguideline.removeRoute

//@ show
rg.help("show")

//@ show, \? [USE:show]
\? routingguideline.show

//@ rename
rg.help("rename")

//@ rename, \? [USE:rename]
\? routingguideline.rename

//@ setDestinationOption
rg.help("setDestinationOption")

//@ setDestinationOption, \? [USE:setDestinationOption]
\? routingguideline.setDestinationOption

//@ setRouteOption
rg.help("setRouteOption")

//@ setRouteOption, \? [USE:setRouteOption]
\? routingguideline.setRouteOption

//@ copy
rg.help("copy")

//@ copy, \? [USE:copy]
\? routingguideline.copy

//@ export
rg.help("export")

//@ export, \? [USE:export]
\? routingguideline.export

//@<> Cleanup
scene.destroy();
