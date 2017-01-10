.. _gadget-protocol:

***************
Gadget Protocol
***************


Introduction
------------

The role of the orchestrator is to support execution of different
jobs and ensure that execution can recover after a failure and
progress and status information can be shown.

The jobs are executed by *MySQL Gadgets*. In order for the gadgets to
coordinate with the orchestrator, they need to follow a set of
conventions, which are described in this chapter.

In the grammar below, we use the following definitions (from `RFC 5234
<http://tools.ietf.org/html/rfc5234>`_):

ALPHA
  Denotes a alphabetic character (A-Z and a-z).

DIGIT
  Denotes a digit (0-9)

HEXDIGIT
  Denotes a hexadecimal digit (0-9, A-F, and a-f)

OCTET
  Denotes an 8-bit character (character value in the range 0-255).

VCHAR
  Visible (printable) character (character value in the range 0x21-0x7E).

WCHAR
  Word character. Letter, digit, ``-`` (dash), ``+`` (plus), ``_``
  (underscore), and ``.`` (period) since these are safe to use for
  executable on any platform.

SP
  Space

We are also using the following shorthands:

.. productionlist::
   string  : *( VCHAR / SP )
   word    : 1*WCHAR
   symbol  : ALPHA *( ALPHA / DIGIT )
   integer : DIGIT *DIGIT
   uuid    : 8HEXDIGIT 3( "-" 4HEXDIGIT ) 12HEXDIGIT


Protocol Architecture
---------------------

The interaction between the gadget and the orchestrator is done using a
protocol and with a certain set of conventions with respect.


Interacting with the gadget
~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the orchestrator start a gadget, it will be executed as a child
process of the orchestrator. This means that the child process belong
to the same process group and session as the orchestrator.

The standard output and standard input are used by the orchestrator
for interacting with the gadget. The format of the output is negotiated
as described in the section Negotiating the format, and the contents
on the standard input is dependent on the negotiated format.

Errors written to the standard error is captured into the logs and
optionally written on a user terminal. Note that the orchestrator do
not use the output on standard error for any other purpose.


Gadget invocation
~~~~~~~~~~~~~~~~~

The invocation of the gadget is done the same way as it was done if used
directly from the command line. Therefore, no distinction is expected 
regardinf the use of the gadgets to perform an action when
"invoked by the user" or "invoked by the orchestrator".


Signals
~~~~~~~

The gadget is expected to support the following signals:

``SIGINT``
    Abort execution of the procedure.

``SIGQUIT``
    Abort execution of the procedure and dump core. Used
    for debugging.


Note: This might not be supported for all platforms.


Options
~~~~~~~

The gadget is expected to recognize the following options:

.. option:: --undo

   Clean up from a previous execution of the gadgets. The same
   options together with all arguments are passed as part of the
   call.

.. option:: --abort

    Do the necessary cleanup work for aborting a previous execution.

.. option:: --help

    Provide help information about the gadget and exit. 

.. option:: --protocols <protocol>,...

    List of protocols supported by the gadget.

.. option:: --progress

    Passed if the invoker expect progress information to be shown.

.. option:: --getlocks

    Passed if the invoker want to see what resources are locked and
    with what sort of lock. The gadget shall print the locks on
    standard output and exit.


Protocol designator
~~~~~~~~~~~~~~~~~~~

Protocols designator consist of a protocol symbol and a protocol
version.  The protocol symbol is a sequence of one or more letters and
digits and starting with a letter. The protocol versions consist of a
major and minor version.

.. productionlist::
   protocol: `symbol` [ "-" `version` ]
   version : `integer` [ "." `integer` ]

.. table::

    +----------+---------------------------------------------+
    | Symbol   | Description                                 |
    +==========+=============================================+
    | text     | Text protocol (any version)                 |
    +----------+---------------------------------------------+
    | text-1   | Text protocol version 1 (any minor version) |
    +----------+---------------------------------------------+
    | json-1.2 | JSON protocol version 1.2 (only)            |
    +----------+---------------------------------------------+

Protocol negotiation
~~~~~~~~~~~~~~~~~~~~

.. productionlist::
   version-line: "Protocol" `protocol`
   error-line: "Error" `string`

To negotiate protocol the orchestrator invoke the gadget with a list
of the protocols that the orchestrator support using the
:option:`--protocols` option. This option is provided to **every**
invocation, even after recovery, since the supported protocols might
change between versions of the orchestrator.

If the version is omitted, the orchestrator accept any version of the
protocol, if the minor version is omitted, the orchestrator accept any
version of the protocol with the correct major version. If both the
major and the minor version is provided, the orchestrator accept this
version **or later**. This means that omitting the minor version is
equivalent to having a minor version of 0.

The gadget will reply by emitting a `version-line`, with the selected
protocol and protocol version on the standard output, or an
`error-line` with a message if a protocol could not be picked.

If the gadget support several of the protocols provided with the
:option:`--protocols` option, the first supported one should be
selected.

.. uml::

   participant "Orchestrator"
   participant "mysql-server-copy" as Gadget

   Orchestrator --> Gadget: mysql-server-copy start --protocols=json-2.1,text\n  source.example.com target.example.com

   activate Gadget
   alt support for JSON protocol version 2.3
     Orchestrator <-- Gadget: Protocol json-2.3
   else support text protocol
     Orchestrator <-- Gadget: Protocol text-1.0
   else none of the protocols supported
     Orchestrator <-- Gadget: Error no protocol supported
   end


Text Protocol Description
-------------------------

This section describes protocol ``text-0.9``.

Line format
~~~~~~~~~~~

Each line consist of a word denoting the action followed by space,
followed by the payload. The word can be any sequence of non-space
printable characters. The action word is case-independent.

Messages
~~~~~~~~

``STEP``
    Text to show in status.

``PROGINFO``
    Progress information with the total value to be reached and the unit
    of the values for the progress data.

``PROG``
    Progress for an activity (cumulative amount).

``LOCK``
    A lock description.


Resource locks
~~~~~~~~~~~~~~

.. productionlist::
   lock-line: "LOCK" `lock-declaration`
   lock-declaration: `resource` "=" ( "X" / "S" )
   resource: "mysql:" `uuid`

When asked for locks required, the gadget shall return a list of the
resources that need to be locked and the type of lock.  The resource
is always a physical entity and the format consists of an IANA
registered name and identifier. For MySQL servers, the identity is
the server UUID.

There are two lock types: exclusive locks and shared locks. Several
procedures can hold a shared lock at the same time, while an exclusive
lock can only be held by a single procedure.

Shared locks are used as read locks and they block exclusive locks
from being acquired on the locked resources.

Exclusive locks are used as write locks and they block all type
of locks (shared and exclusive) from being acquired on the locked resource. 

An example of lines emitted is::

    LOCK mysql:57280590-f64e-11e5-b143-23cd33c7b979=X
    LOCK mysql:640b94d4-f64e-11e5-b653-9735965f6a90=S
    LOCK mysql:6921742a-f64e-11e5-aab2-d7676f39569f=S


Step being executed
~~~~~~~~~~~~~~~~~~~

.. productionlist::
   declare-activity: * `text-line`
   text-line: "STEP" `string`

Along the gadget execution the step (action) being performed should be
declared. This information should be a small descriptive text that will
be shown in status information.

As an example, consider this activity diagram with the corresponding
notes denoting the lines for announcing this:

.. uml::

   (*) --> "Create Backup Image" as Create
   note right: STEP Creating Backup Image source.example.com
   Create --> "Copy Image" as Copy
   note right: STEP Copying image source.example.com target.example.com
   Copy --> "Restore Backup Image" as Restore
   note right: STEP Restoring Image target.example.com
   Restore --> (*)


Progress information
--------------------

.. productionlist::
   progress: [ `progress-info-line` 1*`progress-line` ]
   progress-info-line: "PROGINFO" `integer` `symbol`
   progress-line: "PROG" `integer`

Once execution of an activity have started, progress information can
be shown. The gadget proceeds by declaring the planned amount and the
unit of the amount using the ``PROGINFO`` message. As execution proceeds,
the progress is reported using the ``PROG`` message with the cumulative
amount of data processed.

If no ``PROGINFO`` message is given, it is an error to write ``PROG``
messages.

.. uml::

   participant "Orchestrator"
   participant "mysql-server-copy" as Gadget

   Orchestrator --> Gadget: mysql-server-copy ~--progress ~--protocol=text-1.0 ...
   note over Gadget: Start to execute create-image
   Orchestrator <-- Gadget: PROGINFO 123456 bytes
   Orchestrator <-- Gadget: PROG 1024
   Orchestrator <-- Gadget: PROG 2048
   ...
   Orchestrator <-- Gadget: PROG 123456
