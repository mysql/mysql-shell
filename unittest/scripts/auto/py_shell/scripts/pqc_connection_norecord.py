import json
import re

# Hybrid PQC group used for deterministic negotiation checks.
PQC_KEX = "X25519MLKEM768"

# Classical group used for compatibility and PQC-rejection checks.
CLASSICAL_KEX = "X25519"

# Classical group spelling reported by the server status variable.
CLASSICAL_KEX_STATUS = "x25519"

# Ordered PQC and classical group list used to test multi-group input.
MIXED_KEX = f"{PQC_KEX}:{CLASSICAL_KEX}"

# Unknown group name used to test input validation.
INVALID_KEX = "not-a-valid-kex"

# Valid PQC group with incorrect casing used to test exact name matching.
WRONG_CASE_KEX = "x25519mlkem768"

# Shell is built with an OpenSSL version that supports PQC.
CLIENT_PQC = "client-pqc"

# Shell is built with an OpenSSL version that does not support PQC.
CLIENT_NO_PQC = "client-no-pqc"

# Server implements PQC and uses an OpenSSL version that supports it.
SERVER_PQC = "server-pqc"

# Server implements PQC but uses an OpenSSL version that does not support it.
SERVER_NO_PQC = "server-no-pqc"

# Server predates the server-side PQC implementation.
SERVER_LEGACY = "server-legacy"

# First server version that implements the server-side PQC feature.
PQC_SERVER_VERSION = 260700

# Classic Protocol URI supplied by the test runner.
CLASSIC_URI = f"mysql://{__mysqluripwd}"

# X Protocol URI supplied by the test runner.
X_URI = f"mysqlx://{__uripwd}"

# Connection protocols exercised by every generic test case.
URIS = (CLASSIC_URI, X_URI)

# Language modes exercised by every test case supported by this Shell build.
SHELL_MODES = ("py", "js") if __have_javascript else ("py",)

# Require an encrypted connection without constraining the TLS version.
TLS_REQUIRED_OPTIONS = {"ssl-mode": "REQUIRED"}

# Require an encrypted TLS 1.3 connection without constraining its algorithms.
TLS13_OPTIONS = {
    "ssl-mode": "REQUIRED",
    "tls-version": "TLSv1.3",
}

# Allow only classical KEX and explicitly disable both PQC-related switches.
CLASSICAL_TLS_OPTIONS = {
    "ssl-mode": "REQUIRED",
    "tls-kex": CLASSICAL_KEX,
    "force-pqc": False,
    "use-pqc-sign": False,
}

# Allow only the selected PQC KEX on a TLS 1.3 connection.
PQC_KEX_OPTIONS = {**TLS13_OPTIONS, "tls-kex": PQC_KEX}

# Require the explicitly selected PQC KEX.
FORCED_PQC_OPTIONS = {**PQC_KEX_OPTIONS, "force-pqc": True}

# Require PQC without constraining the KEX groups.
FORCE_PQC_ONLY_OPTIONS = {**TLS13_OPTIONS, "force-pqc": True}

# Exercise rejection of a classical KEX while PQC is required.
CLASSICAL_FORCED_PQC_OPTIONS = {
    **TLS13_OPTIONS,
    "tls-kex": CLASSICAL_KEX,
    "force-pqc": True,
}

# Exercise rejection of TLS 1.2 while PQC is required.
TLS12_FORCED_PQC_OPTIONS = {
    "ssl-mode": "REQUIRED",
    "tls-version": "TLSv1.2",
    "force-pqc": True,
}

# Allow an explicitly ordered PQC and classical KEX combination.
MIXED_KEX_OPTIONS = {
    **TLS13_OPTIONS,
    "tls-kex": MIXED_KEX,
    "force-pqc": False,
}

# Require PQC KEX and advertise support for PQC handshake signatures.
COMBINED_PQC_OPTIONS = {
    **FORCED_PQC_OPTIONS,
    "use-pqc-sign": True,
}

# Maps server status names to their labels in Shell status output.
TLS_STATUS_NAMES = {
    "Tls_key_exchange_algorithm": "TLS key exchange algorithm:",
    "Tls_sign_algorithm": "TLS signature algorithm:",
}

# Protocol-specific error fragments expected for invalid KEX configuration.
TLS_KEX_ERRORS = {
    "classic": "Failed to set TLS key exchange groups",
    "x": "Failed to set TLS key exchange groups",
}

# Protocol-specific errors expected when forced PQC is combined with TLS 1.2.
TLS12_FORCED_PQC_ERRORS = {
    "classic": "force_pqc requires TLSv1.3 with OpenSSL 3.5.0 or newer",
    "x": "error:00000000:lib(0)::reason(0)",
}

# Protocol-specific errors expected when the server cannot negotiate PQC.
PQC_UNAVAILABLE_SERVER_ERRORS = {
    "classic": "ssl/tls alert handshake failure",
    "x": "ssl/tls alert handshake failure",
}

# Protocol-specific errors expected when the client cannot enable forced PQC.
FORCE_PQC_UNSUPPORTED_ERRORS = {
    "classic": "force_pqc requires TLSv1.3 with OpenSSL 3.5.0 or newer",
    "x": "force_pqc requires TLSv1.3 with OpenSSL 3.5.0 or newer",
}

# Returns the active TLS protocol version for the current session.
TLS_VERSION_QUERY = (
    "SELECT VARIABLE_VALUE FROM performance_schema.session_status "
    "WHERE VARIABLE_NAME IN ('Ssl_version', 'Mysqlx_ssl_version') "
    "AND VARIABLE_VALUE <> ''"
)

# Returns the negotiated KEX algorithm for the current session.
KEX_STATUS_QUERY = (
    "SELECT VARIABLE_VALUE FROM performance_schema.session_status "
    "WHERE VARIABLE_NAME = 'Tls_key_exchange_algorithm'"
)

# Returns the TLS handshake signature algorithm for the current session.
SIGN_STATUS_QUERY = (
    "SELECT VARIABLE_VALUE FROM performance_schema.session_status "
    "WHERE VARIABLE_NAME = 'Tls_sign_algorithm'"
)


def parse_openssl_version(text):
    match = re.search(r"\bOpenSSL\s+(\d+)\.(\d+)\.(\d+)[a-z]*\b", text)
    if match is None:
        return None
    return tuple(int(part) for part in match.groups())


def open_session(uri, options=None):
    connection_options = {**shell.parse_uri(uri), **(options or {})}
    return shell.open_session(connection_options)


def status_value(session, scope, name):
    row = session.run_sql(
        f"SHOW {scope} STATUS LIKE '{name}'"
    ).fetch_one()
    if row is None:
        return None
    return row[1]


def run_mysqlsh(args, stdin=""):
    WIPE_OUTPUT()
    rc = testutil.call_mysqlsh(
        args, stdin, ["MYSQLSH_TERM_COLOR_MODE=nocolor"]
    )
    stdout = testutil.fetch_captured_stdout(False)
    stderr = testutil.fetch_captured_stderr(False)
    WIPE_OUTPUT()
    if rc == 0:
        return {"result": stdout.strip()}
    return {"error": f"{stdout}\n{stderr}".strip()}


def command_line_value(value):
    if isinstance(value, bool):
        return str(value).lower()
    return str(value)


def command_line_source(shell_mode, query):
    if shell_mode == "py":
        return f"print(session.run_sql({query!r}).fetch_one()[0])"
    if shell_mode == "js":
        return f"print(session.runSql({json.dumps(query)}).fetchOne()[0]);"
    raise ValueError(f"Unsupported shell mode: {shell_mode}")


def command_line_arguments(uri, shell_mode, options):
    args = ["--quiet-start=2", f"--{shell_mode}"]
    args.extend(
        f"--{name}={command_line_value(value)}"
        for name, value in options.items()
    )
    args.append(uri)
    return args


def open_session_source(shell_mode, connection_data, query):
    if shell_mode == "py":
        return (
            f"s = shell.open_session({connection_data!r})\n"
            "try:\n"
            f"    print(s.run_sql({query!r}).fetch_one()[0])\n"
            "finally:\n"
            "    s.close()"
        )
    if shell_mode == "js":
        return (
            f"const s = shell.openSession({json.dumps(connection_data)}); "
            "try { "
            f"print(s.runSql({json.dumps(query)}).fetchOne()[0]); "
            "} finally { s.close(); }"
        )
    raise ValueError(f"Unsupported shell mode: {shell_mode}")


def run_command_line(uri, shell_mode, options, query):
    args = command_line_arguments(uri, shell_mode, options)
    args.extend(["-e", command_line_source(shell_mode, query)])
    return run_mysqlsh(args)


def run_dictionary(uri, shell_mode, options, query):
    connection_options = {**shell.parse_uri(uri), **options}
    source = open_session_source(shell_mode, connection_options, query)
    return run_mysqlsh(["--quiet-start=2", f"--{shell_mode}", "-e", source])


def run_uri(uri, shell_mode, options, query):
    connection_options = {**shell.parse_uri(uri), **options}
    connection_uri = shell.unparse_uri(connection_options)
    source = open_session_source(shell_mode, connection_uri, query)
    return run_mysqlsh(["--quiet-start=2", f"--{shell_mode}", "-e", source])


def run_test_case(options, query):
    outcomes = {}
    for uri in URIS:
        protocol = "x" if uri.startswith("mysqlx:") else "classic"
        for shell_mode in SHELL_MODES:
            outcomes[(protocol, shell_mode, "command-line")] = run_command_line(
                uri, shell_mode, options, query
            )
            outcomes[(protocol, shell_mode, "uri")] = run_uri(
                uri, shell_mode, options, query
            )
            outcomes[(protocol, shell_mode, "dictionary")] = run_dictionary(
                uri, shell_mode, options, query
            )
    return outcomes


def expect_success(options, query, expected):
    for context, outcome in run_test_case(options, query).items():
        details = f"{context}: {outcome.get('error', '')}"
        if "result" in outcome:
            EXPECT_EQ(str(expected), outcome["result"], details)
        else:
            EXPECT_TRUE(False, details)


def expect_tls_algorithm(options, query, expected_values=None):
    for context, outcome in run_test_case(options, query).items():
        if "result" in outcome:
            actual = outcome["result"]
            allowed_values = expected_values
            if allowed_values is None:
                details = (
                    f"{context}: expected a non-empty value, got {actual!r}"
                )
                EXPECT_TRUE(bool(actual), details)
            else:
                details = (
                    f"{context}: actual: {actual!r}, "
                    f"expected one of: {allowed_values!r}"
                )
                EXPECT_TRUE(actual in allowed_values, details)
        else:
            EXPECT_TRUE(False, f"{context}: {outcome['error']}")


def expect_failure(options, query, expected_errors):
    for context, outcome in run_test_case(options, query).items():
        if "error" in outcome:
            error = outcome["error"]
            expected_error = expected_errors[context[0]]
            EXPECT_TRUE(expected_error in error, f"{context}: {error}")
        else:
            details = f"{context}: unexpected result: {outcome['result']}"
            EXPECT_TRUE(False, details)


def tls_status(uri, options):
    session = open_session(uri, options)
    try:
        return {
            name: status_value(session, "SESSION", name)
            for name in TLS_STATUS_NAMES
        }
    finally:
        session.close()


def status_output_values(output, label):
    prefix = f"{label}"
    return [
        line[len(prefix):].strip()
        for line in output.splitlines()
        if line.startswith(prefix)
    ]


def run_status_interfaces(uri, shell_mode, options):
    args = command_line_arguments(uri, shell_mode, options)
    args.insert(1, "--interactive=full")
    return run_mysqlsh(args, "shell.status()\n\\status\n\\quit\n")


def expect_status(options):
    for uri in URIS:
        protocol = "x" if uri.startswith("mysqlx:") else "classic"
        observed = tls_status(uri, options)
        for shell_mode in SHELL_MODES:
            context = (protocol, shell_mode)
            outcome = run_status_interfaces(uri, shell_mode, options)
            if "result" in outcome:
                for name, label in TLS_STATUS_NAMES.items():
                    value = observed[name]
                    expected = [] if value is None else [value, value]
                    actual = status_output_values(outcome["result"], label)
                    EXPECT_EQ(expected, actual, f"{context}: {name}")
            else:
                EXPECT_TRUE(False, f"{context}: {outcome['error']}")


def observe_server(uri):
    session = open_session(uri, {"ssl-mode": "REQUIRED"})
    try:
        return {
            "tls-kex": status_value(
                session, "SESSION", "Tls_key_exchange_algorithm"
            ),
            "tls-library": status_value(
                session, "GLOBAL", "Tls_library_version"
            ),
        }
    finally:
        session.close()


def classify_server(observation):
    if __version_num < PQC_SERVER_VERSION:
        return SERVER_LEGACY
    if observation["tls-kex"] is None:
        raise RuntimeError(
            "PQC-capable server does not expose "
            "Tls_key_exchange_algorithm"
        )
    tls_library = observation["tls-library"]
    if tls_library is None:
        raise RuntimeError(
            "Server exposes Tls_key_exchange_algorithm but not "
            "Tls_library_version"
        )
    openssl_version = parse_openssl_version(tls_library)
    if openssl_version is None:
        raise RuntimeError(
            f"Unable to parse server TLS library version: {tls_library}"
        )
    if openssl_version >= (3, 5, 0):
        return SERVER_PQC
    return SERVER_NO_PQC


#@<> Shell OpenSSL build information
EXPECT_TRUE(isinstance(__mysqlsh_openssl_version_number, int))
EXPECT_GT(__mysqlsh_openssl_version_number, 0)
EXPECT_TRUE(isinstance(__mysqlsh_openssl_version_text, str))
EXPECT_TRUE(len(__mysqlsh_openssl_version_text) > 0)

#@<> OpenSSL version parsing
EXPECT_EQ((3, 5, 7), parse_openssl_version("OpenSSL 3.5.7 9 Jun 2026"))
EXPECT_EQ((1, 1, 1), parse_openssl_version("OpenSSL 1.1.1k 25 Mar 2021"))
EXPECT_EQ(None, parse_openssl_version("unsupported TLS library"))

#@<> Discover client and server PQC capabilities
client_mode = (
    CLIENT_PQC
    if __mysqlsh_openssl_version_number >= 0x30500000
    else CLIENT_NO_PQC
)

server_observations = {
    "classic": observe_server(CLASSIC_URI),
    "x": observe_server(X_URI),
}
classic_observation = server_observations["classic"]
x_observation = server_observations["x"]

EXPECT_EQ(
    classic_observation["tls-kex"] is not None,
    x_observation["tls-kex"] is not None,
)
EXPECT_EQ(classic_observation["tls-library"], x_observation["tls-library"])

server_mode = classify_server(classic_observation)
EXPECT_EQ(server_mode, classify_server(x_observation))

mode = (client_mode, server_mode)
EXPECT_TRUE(
    mode
    in {
        (CLIENT_PQC, SERVER_PQC),
        (CLIENT_PQC, SERVER_NO_PQC),
        (CLIENT_PQC, SERVER_LEGACY),
        (CLIENT_NO_PQC, SERVER_PQC),
        (CLIENT_NO_PQC, SERVER_NO_PQC),
        (CLIENT_NO_PQC, SERVER_LEGACY),
    }
)

# WL17311-E2E-01
#@<> Existing connection defaults with PQC-capable client and server {mode == (CLIENT_PQC, SERVER_PQC)}
expect_success({}, TLS_VERSION_QUERY, "TLSv1.3")
expect_status({})

# WL17311-E2E-02
#@<> Connection interface acceptance with PQC-capable client and server {mode == (CLIENT_PQC, SERVER_PQC)}
expect_success(CLASSICAL_TLS_OPTIONS, "SELECT 1", 1)

# WL17311-E2E-03
#@<> Explicit PQC KEX with PQC-capable client and server {mode == (CLIENT_PQC, SERVER_PQC)}
expect_tls_algorithm(PQC_KEX_OPTIONS, KEX_STATUS_QUERY, (PQC_KEX,))
expect_status(PQC_KEX_OPTIONS)

# WL17311-E2E-04
#@<> Forced PQC with PQC-capable client and server {mode == (CLIENT_PQC, SERVER_PQC)}
expect_tls_algorithm(FORCED_PQC_OPTIONS, KEX_STATUS_QUERY, (PQC_KEX,))

# WL17311-E2E-05
#@<> Forced PQC with classical KEX {mode == (CLIENT_PQC, SERVER_PQC)}
expect_failure(
    CLASSICAL_FORCED_PQC_OPTIONS,
    "SELECT 1",
    TLS_KEX_ERRORS,
)

# WL17311-E2E-06
#@<> Forced PQC with TLS 1.2 {mode == (CLIENT_PQC, SERVER_PQC)}
expect_failure(
    TLS12_FORCED_PQC_OPTIONS,
    "SELECT 1",
    TLS12_FORCED_PQC_ERRORS,
)

# WL17311-E2E-07
#@<> Mixed KEX list with PQC-capable client and server {mode == (CLIENT_PQC, SERVER_PQC)}
expect_tls_algorithm(
    MIXED_KEX_OPTIONS,
    KEX_STATUS_QUERY,
    (PQC_KEX, CLASSICAL_KEX),
)
expect_status(MIXED_KEX_OPTIONS)

# WL17311-E2E-08
#@<> Invalid KEX with PQC-capable client and server {mode == (CLIENT_PQC, SERVER_PQC)}
expect_failure(
    {**TLS13_OPTIONS, "tls-kex": INVALID_KEX},
    "SELECT 1",
    TLS_KEX_ERRORS,
)

# WL17311-E2E-08
#@<> Wrong-case KEX with PQC-capable client and server {mode == (CLIENT_PQC, SERVER_PQC)}
expect_failure(
    {**TLS13_OPTIONS, "tls-kex": WRONG_CASE_KEX},
    "SELECT 1",
    TLS_KEX_ERRORS,
)

# WL17311-E2E-09
#@<> Disabled PQC signatures with PQC-capable client and server {mode == (CLIENT_PQC, SERVER_PQC)}
expect_tls_algorithm(
    {**TLS13_OPTIONS, "use-pqc-sign": False},
    SIGN_STATUS_QUERY,
)
expect_status({**TLS13_OPTIONS, "use-pqc-sign": False})

# WL17311-E2E-09
#@<> Enabled PQC signatures with PQC-capable client and server {mode == (CLIENT_PQC, SERVER_PQC)}
expect_tls_algorithm(
    {**TLS13_OPTIONS, "use-pqc-sign": True},
    SIGN_STATUS_QUERY,
)
expect_status({**TLS13_OPTIONS, "use-pqc-sign": True})

# WL17311-E2E-10
#@<> Combined PQC options with PQC-capable client and server {mode == (CLIENT_PQC, SERVER_PQC)}
expect_tls_algorithm(COMBINED_PQC_OPTIONS, KEX_STATUS_QUERY, (PQC_KEX,))
expect_tls_algorithm(COMBINED_PQC_OPTIONS, SIGN_STATUS_QUERY)
expect_status(COMBINED_PQC_OPTIONS)

# WL17311-E2E-11
#@<> Empty TLS status values with PQC-capable client and server {mode == (CLIENT_PQC, SERVER_PQC)}
expect_success({"ssl-mode": "DISABLED"}, KEX_STATUS_QUERY, "")
expect_success({"ssl-mode": "DISABLED"}, SIGN_STATUS_QUERY, "")
expect_status({"ssl-mode": "DISABLED"})

# WL17311-E2E-01
#@<> Existing connection defaults with PQC-capable client and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_success({}, TLS_VERSION_QUERY, "TLSv1.3")
expect_status({})

# WL17311-E2E-02
#@<> Connection interface acceptance with PQC-capable client and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_success(CLASSICAL_TLS_OPTIONS, "SELECT 1", 1)

# WL17311-E2E-03
#@<> Explicit PQC KEX with PQC-capable client and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_failure(PQC_KEX_OPTIONS, "SELECT 1", PQC_UNAVAILABLE_SERVER_ERRORS)

# WL17311-E2E-04
#@<> Forced PQC with PQC-capable client and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_failure(FORCED_PQC_OPTIONS, "SELECT 1", PQC_UNAVAILABLE_SERVER_ERRORS)

# WL17311-E2E-05
#@<> Forced PQC with classical KEX and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_failure(
    CLASSICAL_FORCED_PQC_OPTIONS,
    "SELECT 1",
    TLS_KEX_ERRORS,
)

# WL17311-E2E-06
#@<> Forced PQC with TLS 1.2 and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_failure(
    TLS12_FORCED_PQC_OPTIONS,
    "SELECT 1",
    TLS12_FORCED_PQC_ERRORS,
)

# WL17311-E2E-07
#@<> Mixed KEX list with PQC-capable client and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_success(MIXED_KEX_OPTIONS, "SELECT 1", 1)
expect_status(MIXED_KEX_OPTIONS)

# WL17311-E2E-08
#@<> Invalid KEX with PQC-capable client and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_failure(
    {**TLS13_OPTIONS, "tls-kex": INVALID_KEX},
    "SELECT 1",
    TLS_KEX_ERRORS,
)

# WL17311-E2E-08
#@<> Wrong-case KEX with PQC-capable client and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_failure(
    {**TLS13_OPTIONS, "tls-kex": WRONG_CASE_KEX},
    "SELECT 1",
    TLS_KEX_ERRORS,
)

# WL17311-E2E-09
#@<> Disabled PQC signatures with PQC-capable client and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_success(
    {**TLS13_OPTIONS, "use-pqc-sign": False},
    "SELECT 1",
    1,
)
expect_status({**TLS13_OPTIONS, "use-pqc-sign": False})

# WL17311-E2E-09
#@<> Enabled PQC signatures with PQC-capable client and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_success(
    {**TLS13_OPTIONS, "use-pqc-sign": True},
    "SELECT 1",
    1,
)
expect_status({**TLS13_OPTIONS, "use-pqc-sign": True})

# WL17311-E2E-10
#@<> Combined PQC options with PQC-capable client and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_failure(
    COMBINED_PQC_OPTIONS,
    "SELECT 1",
    PQC_UNAVAILABLE_SERVER_ERRORS,
)

# WL17311-E2E-11
#@<> Empty TLS status values with PQC-capable client and non-PQC server {mode == (CLIENT_PQC, SERVER_NO_PQC)}
expect_success({"ssl-mode": "DISABLED"}, KEX_STATUS_QUERY, "")
expect_success({"ssl-mode": "DISABLED"}, SIGN_STATUS_QUERY, "")
expect_status({"ssl-mode": "DISABLED"})

# WL17311-COMPAT-01
# WL17311-COMPAT-04
#@<> Ordinary connection interfaces with legacy server {server_mode == SERVER_LEGACY}
expect_success({}, "SELECT 1", 1)

# WL17311-COMPAT-02
# WL17311-COMPAT-03
#@<> Status interfaces with legacy server {server_mode == SERVER_LEGACY}
expect_status({})

# WL17311-COMPAT-05
#@<> Classical options with legacy server {server_mode == SERVER_LEGACY}
expect_success(CLASSICAL_TLS_OPTIONS, "SELECT 1", 1)
expect_status(CLASSICAL_TLS_OPTIONS)

# WL17311-OLDSSL-01
#@<> Existing defaults with non-PQC client and PQC-capable server {mode == (CLIENT_NO_PQC, SERVER_PQC)}
expect_success(TLS_REQUIRED_OPTIONS, "SELECT 1", 1)
expect_tls_algorithm(
    TLS_REQUIRED_OPTIONS,
    KEX_STATUS_QUERY,
    (CLASSICAL_KEX_STATUS,),
)
expect_tls_algorithm(TLS_REQUIRED_OPTIONS, SIGN_STATUS_QUERY)
expect_status(TLS_REQUIRED_OPTIONS)

# WL17311-OLDSSL-02
#@<> Classical options with non-PQC client and PQC-capable server {mode == (CLIENT_NO_PQC, SERVER_PQC)}
expect_success(CLASSICAL_TLS_OPTIONS, "SELECT 1", 1)
expect_tls_algorithm(
    CLASSICAL_TLS_OPTIONS,
    KEX_STATUS_QUERY,
    (CLASSICAL_KEX_STATUS,),
)

# WL17311-OLDSSL-03
#@<> PQC KEX with non-PQC client and PQC-capable server {mode == (CLIENT_NO_PQC, SERVER_PQC)}
expect_failure(PQC_KEX_OPTIONS, "SELECT 1", TLS_KEX_ERRORS)

# WL17311-OLDSSL-03
#@<> Mixed KEX list with non-PQC client and PQC-capable server {mode == (CLIENT_NO_PQC, SERVER_PQC)}
expect_failure(MIXED_KEX_OPTIONS, "SELECT 1", TLS_KEX_ERRORS)

# WL17311-OLDSSL-04
#@<> Forced PQC with non-PQC client and PQC-capable server {mode == (CLIENT_NO_PQC, SERVER_PQC)}
expect_failure(
    FORCE_PQC_ONLY_OPTIONS,
    "SELECT 1",
    FORCE_PQC_UNSUPPORTED_ERRORS,
)

# WL17311-OLDSSL-05
#@<> Disabled PQC signatures with non-PQC client and PQC-capable server {mode == (CLIENT_NO_PQC, SERVER_PQC)}
expect_success(
    {**TLS13_OPTIONS, "use-pqc-sign": False},
    "SELECT 1",
    1,
)
expect_tls_algorithm(
    {**TLS13_OPTIONS, "use-pqc-sign": False},
    SIGN_STATUS_QUERY,
)

# WL17311-OLDSSL-05
#@<> Enabled PQC signatures with non-PQC client and PQC-capable server {mode == (CLIENT_NO_PQC, SERVER_PQC)}
expect_success(
    {**TLS13_OPTIONS, "use-pqc-sign": True},
    "SELECT 1",
    1,
)
expect_tls_algorithm(
    {**TLS13_OPTIONS, "use-pqc-sign": True},
    SIGN_STATUS_QUERY,
)

# WL17311-OLDSSL-06
#@<> Classical TLS status with non-PQC client and PQC-capable server {mode == (CLIENT_NO_PQC, SERVER_PQC)}
expect_tls_algorithm(
    CLASSICAL_TLS_OPTIONS,
    KEX_STATUS_QUERY,
    (CLASSICAL_KEX_STATUS,),
)
expect_tls_algorithm(CLASSICAL_TLS_OPTIONS, SIGN_STATUS_QUERY)
expect_status(CLASSICAL_TLS_OPTIONS)

# WL17311-OLDSSL-01
#@<> Existing defaults with non-PQC client and server {mode == (CLIENT_NO_PQC, SERVER_NO_PQC)}
expect_success(TLS_REQUIRED_OPTIONS, "SELECT 1", 1)
expect_status(TLS_REQUIRED_OPTIONS)

# WL17311-OLDSSL-02
#@<> Classical options with non-PQC client and server {mode == (CLIENT_NO_PQC, SERVER_NO_PQC)}
expect_success(CLASSICAL_TLS_OPTIONS, "SELECT 1", 1)

# WL17311-OLDSSL-03
#@<> PQC KEX with non-PQC client and server {mode == (CLIENT_NO_PQC, SERVER_NO_PQC)}
expect_failure(PQC_KEX_OPTIONS, "SELECT 1", TLS_KEX_ERRORS)

# WL17311-OLDSSL-03
#@<> Mixed KEX list with non-PQC client and server {mode == (CLIENT_NO_PQC, SERVER_NO_PQC)}
expect_failure(MIXED_KEX_OPTIONS, "SELECT 1", TLS_KEX_ERRORS)

# WL17311-OLDSSL-04
#@<> Forced PQC with non-PQC client and server {mode == (CLIENT_NO_PQC, SERVER_NO_PQC)}
expect_failure(
    FORCE_PQC_ONLY_OPTIONS,
    "SELECT 1",
    FORCE_PQC_UNSUPPORTED_ERRORS,
)

# WL17311-OLDSSL-05
#@<> Disabled PQC signatures with non-PQC client and server {mode == (CLIENT_NO_PQC, SERVER_NO_PQC)}
expect_success(
    {**TLS13_OPTIONS, "use-pqc-sign": False},
    "SELECT 1",
    1,
)
expect_status({**TLS13_OPTIONS, "use-pqc-sign": False})

# WL17311-OLDSSL-05
#@<> Enabled PQC signatures with non-PQC client and server {mode == (CLIENT_NO_PQC, SERVER_NO_PQC)}
expect_success(
    {**TLS13_OPTIONS, "use-pqc-sign": True},
    "SELECT 1",
    1,
)
expect_status({**TLS13_OPTIONS, "use-pqc-sign": True})

# WL17311-OLDSSL-06
#@<> Classical TLS status with non-PQC client and server {mode == (CLIENT_NO_PQC, SERVER_NO_PQC)}
expect_status(CLASSICAL_TLS_OPTIONS)
