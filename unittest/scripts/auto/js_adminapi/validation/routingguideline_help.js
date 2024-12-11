//@<OUT> Object Help
NAME
      RoutingGuideline - Routing Guidelines operations

DESCRIPTION
      A RoutingGuideline can be used to control routing behavior of MySQL
      Router through rules that define possible destination MySQL servers for
      incoming client sessions.

      Routing Guidelines consist of two main components: destination classes,
      and routes.

      Destination classes

      Destination classes group MySQL server instances in the topology using
      expressions. These expressions define the conditions under which servers
      are added to a destination class. Each class can then be used to form a
      pool of candidate instances for routing. A server can belong to multiple
      destination classes simultaneously. However, only ONLINE instances in the
      topology are considered when forming the candidate pool.

      Routes

      Routes, on the other hand, specify an expression that can match incoming
      client sessions, and a list of destination classes to form a pool of
      candidate MySQL servers to route them to.

      Routes specify how incoming client sessions are classified and directed.
      When a client connects to a MySQL Router port:

      1. Routes are evaluated in order of appearance in the routing guideline (
      top-bottom)

      2. The first matching route determines the routing behavior for that
      session. A list of candidate destinations is built.

      3. Candidate destinations are organized in a tiered list:

      - Each tier contains one or more destination classes.
      - MySQL Router evaluates the availability of instances tier by tier.
      - If no instances in the current tier are available, the next tier is
        evaluated.
      - If no instances in any tier are available, the client connection is
        rejected.

      Topology monitoring

      MySQL Router continuously monitors the topology of its topology. If the
      topology changes:

      - Servers are reclassified dynamically.
      - Existing client sessions are re-evaluated.
      - Sessions connected to servers no longer in their candidate pool are
        disconnected.

      Example Use Cases for Client Classification

      Routing Guidelines can classify clients based on a variety of attributes,
      such as:

      - Source IP Address: Direct clients from a specific subnet to a
        particular group of servers.
      - MySQL Username: Route connections authenticated with a specific MySQL
        user to designated servers.
      - Default Schema: Direct traffic based on the schema selected during
        connection initialization.
      - Session Connection Attributes: Use attributes like client name,
        version, or operating system (from
        `performance_schema.session_connect_attrs`).
      - Router Port: Route based on the MySQL Router port used by the client.

      For the full list of session-related variables to define the matching
      rules for routes , see \? RoutingGuideline.addRoute()

      Example Use Cases for Destinations

      Destinations can be defined with broad or specific criteria, such as:

      - A specific IP address or hostname.
      - Server Version (e.g., MySQL 8.0.29 or higher).
      - Role (e.g., Primary, Secondary, or Read-Replica).
      - Custom tags (e.g., 'performance = "high"' or 'type = "compliance"').

      For the full list of server-related variables to define the destination
      classes matching rules, see \? RoutingGuideline.addDestination()

      Practical Example

      1. Client classification:

      - '$.router.port.ro': Clients connecting to Router's RO port are directed
        to read-only servers.
      - '$.session.user': Clients authenticated as "admin_user" are routed to
        servers optimized for administrative operations.

      2. Destination Classes:

      - Write traffic: A class for (Primary) servers that handle write traffic.
      - Read-only traffic: A class for (Secondary) servers that handle
        read-only operations.
      - Compliance: A class for servers in "compliance mode", ensuring
        adherence to regulatory requirements.
      - Target port: A route that matches a specific "$.session.targetPort" and
        directs traffic to target destinations, prioritized in tiers.

      Adding Destinations and Routes

      Destination classes can be added with the addDestination() method.

      For example: rg.addDestination("ReadOnlyServers", "$.server.memberRole =
      SECONDARY")

      Routes can be added with the addRoute() method.

      For example: rg.addRoute('priority_reads', '$.session.user = "myapp"',
      ['round-robin(Primary)'])

PROPERTIES
      name
            Returns the name of the Routing Guideline.

FUNCTIONS
      addDestination(name, match[, options])
            Adds a destination class that groups MySQL instances in the Routing
            Guideline.

      addRoute(name, match, destinations, options)
            Adds a new route that defines how client sessions matching specific
            criteria are routed to defined destinations.

      asJson()
            Returns the Routing Guideline as a JSON document.

      copy(name)
            Creates a full copy of the target Routing Guideline with a new
            name.

      destinations()
            List destination classes defined for the Routing Guideline.

      export(file)
            Exports the Routing Guideline to a file.

      getName()
            Returns the name of the Routing Guideline.

      help([member])
            Provides help about this class and it's members

      removeDestination(name)
            Remove a destination class from the Routing Guideline.

      removeRoute(name)
            Remove a route from the Routing Guideline.

      rename()
            Rename the target Routing Guideline.

      routes()
            List routes defined for the Routing Guideline.

      setDestinationOption(destinationName, option, value)
            Updates a specific option for a named destination in the Routing
            Guideline.

      setRouteOption(routeName, option, value)
            Updates a specific option for a named route in the Routing
            Guideline.

      show([options])
            Displays a comprehensive summary of the Routing Guideline.

//@<OUT> Name
NAME
      name - Returns the name of the Routing Guideline.

SYNTAX
      <RoutingGuideline>.name

//@<OUT> asJson
NAME
      asJson - Returns the Routing Guideline as a JSON document.

SYNTAX
      <RoutingGuideline>.asJson()

RETURNS
      A dictionary representing the Routing Guideline.

DESCRIPTION
      The returned Routing Guideline can be modified and fed to parse().

//@<OUT> destinations
NAME
      destinations - List destination classes defined for the Routing
                     Guideline.

SYNTAX
      <RoutingGuideline>.destinations()

RETURNS
      ShellResult object with destination classes defined for the Guideline.

//@<OUT> routes
NAME
      routes - List routes defined for the Routing Guideline.

SYNTAX
      <RoutingGuideline>.routes()

RETURNS
      ShellResult object with routes classes defined for the Guideline.


//@<OUT> addDestination
NAME
      addDestination - Adds a destination class that groups MySQL instances in
                       the Routing Guideline.

SYNTAX
      <RoutingGuideline>.addDestination(name, match[, options])

WHERE
      name: Name for the destination class to be added.
      match: An expression string for matching MySQL server instances.
      options: Dictionary with additional options.

RETURNS
      Nothing.

DESCRIPTION
      A destination class specifies an expression that groups together MySQL
      server instances from the topology. They can be used to define candidate
      destinations for a route to forward client sessions to. Destination
      classes group instances based on matching attributes, and each instance
      can belong to one or more destination classes.

      For an overview on Routing Guidelines, see \? RoutingGuideline.

      Expression Syntax

      The matching expression can contain variables related to the MySQL server
      instances ('$.server.*') and the Router itself ('$.router.*') and must
      evaluate to either true or false. Each variable represents a specific
      attribute of the server or the router, allowing fine-grained control over
      grouping.

      The following server-related variables are available:

      - $.server.label: "<hostname>:<port>"
      - $.server.address: Public address of the instance (@report_host or
        '@hostname')
      - $.server.port: MySQL classic protocol port number
      - $.server.uuid: The `@server_uuid` of the instance
      - $.server.version: Version number of the instance (MMmmpp) (e.g. 80400)
      - $.server.memberRole: PRIMARY, SECONDARY, or READ_REPLICA (if the
        instance is part of an InnoDB Cluster)
      - $.server.tags: A key-value object containing user-defined tags stored
        in the topology's metadata for that instance.
      - $.server.clusterName: Name of the Cluster the instance is part of.
      - $.server.clusterSetName: Name of the ClusterSet the instance belongs
        to.
      - $.server.clusterRole: Role of the Cluster the instance is part of, if
        it's in a ClusterSet. One of PRIMARY, REPLICA, or UNDEFINED.
      - $.server.isClusterInvalidated: Indicates if the Cluster the instance
        belongs to is invalidated.

      The following router-related variables are available:

      - $.router.port.rw: Listening port for classic protocol read-write
        connections.
      - $.router.port.ro: Listening port for classic protocol read-only
        connections.
      - $.router.port.rw_split: Listening port for classic protocol read-write
        splitting connections.
      - $.router.localCluster: Name of the cluster configured as local to the
        Router.
      - $.router.hostname: Hostname where the Router is running.
      - $.router.address: IP address on which the Router is listening.
      - $.router.tags: A key-value object containing user-defined tags stored
        in the topology's metadata for that Router instance.
      - $.router.routeName: Name of the Routing plugin used by the Router.
      - $.router.name: Name of the Router.

      Examples

      rg.add_destination("SecondaryInstances", "$.server.memberRole =
      'SECONDARY'")

      rg.add_destination("US_Instances", "$.server.address in
      ['us-east-1.example.com', 'us-west-2.example.com']")

      rg.add_destination("MarkedRouters", "$.router.tags.mark = 'special'")

//@<OUT> addRoute
NAME
      addRoute - Adds a new route that defines how client sessions matching
                 specific criteria are routed to defined destinations.

SYNTAX
      <RoutingGuideline>.addRoute(name, match, destinations, options)

WHERE
      name: The name of the route to be added.
      match: An expression string for matching incoming client sessions.
      destinations: An array of destination selectors in the form of `strategy(
                    destination, ...)`.
      options: An optional dictionary with additional options.

RETURNS
      Nothing.

DESCRIPTION
      This command adds a route that evaluates a given expression against
      attributes of incoming client sessions. If the expression matches, the
      client session is routed to one of the specified destination instances,
      based on the defined routing strategies.

      Individual routes can be selectively enabled or disabled using the
      'enabled' flag. If disabled, the route will not be considered during
      routing decisions.

      For an overview on Routing Guidelines, see \? RoutingGuideline.

      Source Expression

      The 'match' parameter specifies the client session matching rule. It can
      contain variables related to the incoming client session ('$.session.*')
      and the router itself ('$.router.*'). The expression must evaluate to
      either true or false.

      The following session-related variables are available:

      - $.session.targetIP: IP address of the Router the client is connected
        to.
      - $.session.targetPort: Router port the client is connected to.
      - $.session.sourceIP: IP address the client is connecting from.
      - $.session.user: Username the client is authenticated as.
      - $.session.connectAttrs: Client connection attributes.
      - $.session.schema: Default schema the client has selected.
      - $.session.randomValue: Random value in a range 0.0 <= x < 1.0.

      The following router-related variables are available:

      - $.router.port.rw: Listening port for classic protocol read-write
        connections.
      - $.router.port.ro: Listening port for classic protocol read-only
        connections.
      - $.router.port.rw_split: Listening port for classic protocol Read-Write
        splitting connections.
      - $.router.localCluster: Name of the cluster configured as local to the
        Router.
      - $.router.hostname: Hostname where Router is running.
      - $.router.address: IP address on which the Router is listening.
      - $.router.tags: A key-value object containing user-defined tags stored
        in the topology's metadata for that Router instance.
      - $.router.routeName: Name of the Routing plugin used by the Router.
      - $.router.name: Name of the Router.

      The '$.session.connectAttrs' variable refers to attributes sent by the
      client during the connection handshake. These attributes are not simple
      scalars but key-value pairs that describe details about the client
      environment. You can use these attributes to match specific criteria for
      routing.

      For more information on the available Client connection attributes see
      https://dev.mysql.com/doc/refman/en/performance-schema-connection-attribute-tables.html#performance-schema-connection-attributes-available

      Destinations

      The 'destinations' parameter specifies the list of candidate destination
      classes for routing, using one or more routing strategies. A destination
      list must be defined in the format '[strategy(destination, ...), ...]',
      where:

      'strategy' is one of the supported routing strategies:

      - first-available: Selects the first available instance in the order
        specified.
      - round-robin: Distributes connections evenly among available instances.

      'destination' is the name of the destination class defined in the Routing
      Guideline. Each class is a group of instances matching a specific set of
      rules.

      Destinations are organized in tiers, and MySQL Router will evaluate each
      tier in order of priority. If no servers are available in the first tier,
      it will proceed to the next until it finds a suitable destination or
      exhausts the list.

      To add a new destination class, use the addDestination method.

      Options

      The following options can be specified when adding a route:

      - enabled (boolean): Indicates whether the route is active. Set to 'true'
        by default.
      - connectionSharingAllowed (boolean): Specifies if the route allows
        connection sharing. Set to 'true' by default.
      - dryRun (boolean): If set to 'true', validates the route without
        applying it. Disabled by default.
      - 'order' (uinteger): Specifies the position of a route within the
        Routing Guideline (lower values indicate higher priority).

      Examples

      guideline.addRoute('priority_reads', '$.session.user = "myapp"',
      ['round-robin(Primary)'])

      guideline.addRoute('local_reads', '$.session.sourceIP = "192.168.1.10"',
      ['round-robin(Secondary)'], {enabled: true})

      guideline.addRoute('backup_client', '$.session.connectAttrs.program_name
      = "mysqldump"', ['first-available(backup_server)'],{
      connectionSharingAllowed: false, enabled: true }

      guideline.addRoute('tagged', '$.router.tags.marker = "special"',
      ['round-robin(Special)'], { connectionSharingAllowed: true, enabled: true
      } );

//@<OUT> removeDestination
NAME
      removeDestination - Remove a destination class from the Routing
                          Guideline.

SYNTAX
      <RoutingGuideline>.removeDestination(name)

WHERE
      name: Name of the destination class

RETURNS
      Nothing

//@<OUT> removeRoute
NAME
      removeRoute - Remove a route from the Routing Guideline.

SYNTAX
      <RoutingGuideline>.removeRoute(name)

WHERE
      name: Name of the route

RETURNS
      Nothing

//@<OUT> show
NAME
      show - Displays a comprehensive summary of the Routing Guideline.

SYNTAX
      <RoutingGuideline>.show([options])

WHERE
      options: Dictionary with additional options.

RETURNS
      Nothing

DESCRIPTION
      This function describes the Routing Guideline by evaluating expressions
      and listing all destinations and routes, including the information about
      the servers of the target topology that match the destinations and
      routes. It also identifies all destinations not referenced by any route.

      Destinations that rely on expressions involving the target router port or
      address (e.g., '$.router.port' or '$.router.address') may produce results
      that differ from actual behavior in MySQL Router. Since this function
      evaluates the guideline outside the context of the router, these
      expressions are not matched against live router runtime data and may not
      reflect the router's operational reality. On that case, the option
      'router' must be set to specify the Router instance that should be used
      for evaluation of the guideline.

      The options dictionary may contain the following attributes:

      - router: Identifier of the Router instance to be used for evaluation.

//@<OUT> rename
NAME
      rename - Rename the target Routing Guideline.

SYNTAX
      <RoutingGuideline>.rename()

DESCRIPTION
      This functions allows changing the name of an existing Routing Guideline.

      @returns Nothing

//@<OUT> setDestinationOption
NAME
      setDestinationOption - Updates a specific option for a named destination
                             in the Routing Guideline.

SYNTAX
      <RoutingGuideline>.setDestinationOption(destinationName, option, value)

WHERE
      destinationName: The name of the destination whose option needs to be
                       updated.
      option: The option to update for the specified destination.
      value: The new value for the specified option.

RETURNS
      Nothing.

DESCRIPTION
      This command allows modifying the properties of a destination within a
      Routing Guideline. It is used to change the matching rule for identifying
      MySQL server instances that belong to this destination class.

      The following destination options are supported:

      - match (string): The matching rule used to identify servers for this
        destination class.

      Examples

      rg.setDestinationOption("EU_Regions", "match", "$.server.address in
      ['eu-central-1.example.com', 'eu-west-1.example.com']");

      rg.setDestinationOption("Primary_Instances", "match",
      "$.server.memberRole = 'PRIMARY'");

      For more information on destinations, see \?
      RoutingGuideline.addDestination.

      For more information about Routing Guidelines, see \? RoutingGuideline.

//@<OUT> setRouteOption
NAME
      setRouteOption - Updates a specific option for a named route in the
                       Routing Guideline.

SYNTAX
      <RoutingGuideline>.setRouteOption(routeName, option, value)

WHERE
      routeName: The name of the route whose option needs to be updated.
      option: The option to update for the specified route.
      value: The new value for the specified option.

RETURNS
      Nothing.

DESCRIPTION
      This command allows modifying the properties of a route within a Routing
      Guideline. It is used to change the matching rule for client sessions,
      update the destinations, enable or disable the route, or change its
      priority.

      The following route options are supported:

      - match (string): The client connection matching rule.
      - destinations (string): A list of destinations using routing strategies
        in the format `strategy(destination1, destination2, ...)`, ordered by
        priority, highest to lowest
      - enabled (boolean): Set to true to enable the route, or false to disable
        it.
      - connectionSharingAllowed (boolean): Set to true to enable connection
        sharing, or false to disable it.
      - order (uinteger): Specifies the position of a route within the Routing
        Guideline (lower values indicate higher priority).

      Examples

      rg.setRouteOption("read_traffic", "match", "$.session.user =
      'readonly_user'");

      rg.setRouteOption("write_traffic", "enabled", true);

      For more information on routes definition, see \?
      RoutingGuideline.addRoute.

      For more information about Routing Guidelines, see \? RoutingGuideline.

//@<OUT> copy
NAME
      copy - Creates a full copy of the target Routing Guideline with a new
             name.

SYNTAX
      <RoutingGuideline>.copy(name)

WHERE
      name: The name for the new Routing Guideline.

RETURNS
      A Routing Guideline object.

DESCRIPTION
      This command creates a duplicate of the target Routing Guideline,
      including all its routes and destination classes. The new guideline must
      have a unique name within the topology, and the name must follow the same
      naming rules defined for Routing Guidelines. This command is useful for
      quickly creating new guidelines based on existing ones, without manually
      duplicating all the definitions.

      Example

      rg_copy = rg.copy("newGuideline");

      The example above creates a new Routing Guideline named 'newGuideline'
      with the same configuration as the original guideline.

      For more information about Routing Guidelines, see \? RoutingGuideline.

//@<OUT> export
NAME
      export - Exports the Routing Guideline to a file.

SYNTAX
      <RoutingGuideline>.export(file)

WHERE
      file: The file path where the Routing Guideline will be saved.

RETURNS
      Nothing.

DESCRIPTION
      This command exports a complete copy of the Routing Guideline to a file
      in JSON format. The output file can then be used for backup, sharing, or
      applying the Routing Guideline to another topology.

      For more information on Routing Guidelines, see \? RoutingGuideline.
