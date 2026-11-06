# Build Instructions for MySQL Shell

## 1. Prerequisites

### Required tools and libraries
- CMake 3.17.5+ (3.19+ on macOS, 3.21+ when using Visual Studio 2022)
- GCC 14, Visual Studio 2022 17.13+, or Clang 17+
- zip
- python 3.10+
- libssh 0.9.2
- openssl 3.x
- antlr4-runtime (C++ runtime ≥ 4.10)
- git (if you checkout from github instead of using source packages)

#### Suggested packages for EL9-like distributions
```bash
sudo yum install cmake gcc-toolset-14 openssl-devel ncurses-devel rpcgen python3-devel libssh-devel libcurl-devel patchelf
```

Please adjust the specific package name for python3 to ensure version 3.10 or newer is installed.

You might also need to enable the developer EPEL repo for patchelf:

```bash
sudo dnf config-manager --enable ol9_developer_EPEL
```

#### Suggested packages for Ubuntu 24.04 LTS
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build \
    python3-dev python3-venv python3-pip \
    libssl-dev libssh-dev libcurl4-openssl-dev \
    libprotobuf-dev protobuf-compiler \
    liblz4-dev zlib1g-dev pkg-config patchelf \
    libantlr4-runtime-dev
```

#### Build and install the ANTLR4 runtime
Many Linux distributions do not currently ship a new enough ANTLR4 C++ runtime, so
if yours doesn't, build it from source before configuring CMake.

```bash
mkdir -p ~/mysql-src
cd ~/mysql-src
git clone --depth 1 --branch 4.13.2 --sparse \
  https://github.com/antlr/antlr4.git antlr4-runtime
cd antlr4-runtime
git sparse-checkout set runtime/Cpp
cmake -S runtime/Cpp -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DANTLR_BUILD_CPP_TESTS=OFF \
  -DCMAKE_INSTALL_PREFIX="$HOME/mysql-src/antlr4-runtime-install"
cmake --build build --parallel
cmake --build build --target install
```

On EL9-like systems, activate `gcc-toolset-14` before running the CMake
commands if it is not already active.

### Optional
- GraalVM 23.0.1 (for JavaScript support)

## 2. Check Out the Sources

Place the MySQL Server and MySQL Shell repositories under the same parent
directory so the relative paths used below remain valid.

```bash
mkdir -p ~/mysql-src
cd ~/mysql-src
git clone --depth 1 --branch mysql-26.10.0 https://github.com/mysql/mysql-server.git mysql-server
git clone --depth 1 --branch 26.10.0 https://github.com/mysql/mysql-shell.git mysql-shell
```

You may also use source archives for a specific version from
https://dev.mysql.com/downloads/mysql/ and https://dev.mysql.com/downloads/shell/.

## 3. Prepare the MySQL Server Build

The MySQL Shell relies on the MySQL client libraries. Build them once before
configuring the shell.

1. Create a dedicated build directory:
```bash
mkdir ~/mysql-src/mysql-server/bld
cd ~/mysql-src/mysql-server/bld
```
2. Configure the server:
```bash
cmake .. -DWITH_AUTHENTICATION_CLIENT_PLUGINS=YES -DWITH_TIRPC=bundled
```
On Windows, configure the server with Visual Studio 2022 instead:
```bash
cmake .. -G "Visual Studio 17 2022" -A x64 \
    -DWITH_AUTHENTICATION_CLIENT_PLUGINS=YES
```
3. Compile:
```bash
cmake --build . --parallel $(nproc --all)
```
On Windows, build the `RelWithDebInfo` configuration:
```bash
cmake --build . --config RelWithDebInfo --parallel
```
or if you'd like to build only what's needed by MySQL Shell:
```bash
cmake --build . --parallel $(nproc --all) --target mysqlclient
cmake --build . --parallel $(nproc --all) --target mysqlxclient
cmake --build . --parallel $(nproc --all) --target mysqlxclient_lite
cmake --build . --parallel $(nproc --all) --target libprotobuf-lite
cmake --build . --parallel $(nproc --all) --target mysql_config_editor
cmake --build . --parallel $(nproc --all) --target mysql_binlog_event_standalone
cmake --build . --parallel $(nproc --all) --target mysqlbinlog
cmake --build . --parallel $(nproc --all) --target routing_guidelines_object
cmake --build . --parallel $(nproc --all) --target mysql_native_password # optional
```
On Windows, add `--config RelWithDebInfo` and omit `$(nproc --all)` in each
of the target-specific commands above.

## 4. Build MySQL Shell

### Linux and macOS

1. Create the build directory:
```bash
mkdir ~/mysql-src/mysql-shell/bld
cd ~/mysql-src/mysql-shell/bld
```
2. Configure the project:
```bash
cmake .. \
    -DMYSQL_SOURCE_DIR=`pwd`/../../mysql-server \
    -DMYSQL_BUILD_DIR=`pwd`/../../mysql-server/bld \
    -DHAVE_PYTHON=1
```
If you built the ANTLR4 runtime from source instead of installing a system
package, also pass:
```bash
-DBUNDLED_ANTLR_DIR="$HOME/mysql-src/antlr4-runtime-install"
```
3. Build and install the shell:
```bash
cmake --build . --parallel $(nproc --all)
sudo make install
```

### Windows

1. Create the build directory:
```bash
mkdir bld
cd bld
```
2. Configure with Visual Studio:
```bash
cmake .. -G "Visual Studio 17 2022" -A x64 \
    -DMYSQL_SOURCE_DIR=../../mysql-server \
    -DMYSQL_BUILD_DIR=../../mysql-server/bld \
    -DBUNDLED_ANTLR_DIR=C:/path/to/antlr4-runtime-install \
    -DHAVE_PYTHON=1 \
    -DPYTHON_LIBRARY=<path_to_python_library> \
    -DPYTHON_INCLUDE_DIR=<python_src>/include
```
3. Build the `RelWithDebInfo` configuration:
```bash
cmake --build . --config RelWithDebInfo --parallel
```

Alternatively, open `mysqlsh.sln` in Visual Studio 2022 and build the
`RelWithDebInfo` configuration.

All Windows dependencies must be compiled with matching settings (Release vs.
Debug and /MD vs. /MT).

## 5. Optional Features

### JavaScript support

1. Build the native Polyglot API library by following `ext/polyglot/README.txt`.
2. Provide the compiled library and include paths to CMake:
```bash
-DJIT_EXECUTOR_LIB=<path_to_polyglot_api>
```

Note: GraalVM 23.0.1 is required; newer releases may not be compatible.

### Unit tests

Enable unit test targets by adding the following option during configuration:
```bash
-DWITH_TESTS=1
```

### Additional Python modules

Note that if you'd like to install additional Python modules, you must install
them in the Python runtime directories that MySQL Shell was compiled with.
To make sure that's the case, execute `pip` from mysqlsh itself. 

Example:
```bash
mysqlsh --pym pip install certifi PyYAML
```

## 6. Building RPM and Debian Packages

The `packaging/rpm` and `packaging/debian` directories contain small CMake
projects that generate the RPM spec file and the `debian/` directory. They
only generate files; the packages are then built with `rpmbuild` or
`dpkg-buildpackage`.

Like a regular build, the packages are built against a MySQL Server source
**and** build tree of the same series (sections 2 and 3). Installed
`mysql-devel` or `libmysqlclient-dev` packages are not used. The build tree
must contain `CMakeCache.txt`, otherwise configuration stops.

### MySQL Server targets

Besides the targets listed in section 3, the server build tree must contain the
client authentication plugins that the packages install (`lib/mysql/plugins`
and `libfido2`). Build them in the server build directory:

```bash
cd ~/mysql-src/mysql-server/bld
for t in mysql_native_password authentication_oci_client \
         authentication_openid_connect_client authentication_webauthn_client \
         authentication_ldap_sasl_client authentication_kerberos_client; do
    cmake --build . --parallel $(nproc --all) --target $t
done
```

`mysqlbinlog` is mandatory: configuring MySQL Shell fails if
`runtime_output_directory/mysqlbinlog` is missing. `mysql_config_editor` is
not packaged unless its path is passed explicitly (`bundled_mysql_config_editor`
for RPM, `BUNDLED_MYSQL_CONFIG_EDITOR` for Debian, see below).

If you build with `WITH_PROTOBUF=system` (RPM `with_protobuf`, Debian
`WITH_PROTOBUF`), configure the MySQL Server tree with `-DWITH_PROTOBUF=system`
as well. Otherwise the generated `mysqlx.pb.h` does not match the system
`protoc`.

### Build an RPM

Install `rpm-build` and `rpmdevtools`, then generate the spec file. It is
written to the root of the MySQL Shell source tree:

```bash
cd ~/mysql-src/mysql-shell
cmake -S packaging/rpm -B /tmp/mysql-shell-rpm-init
```

Add `-DCOMMUNITY_BUILD=OFF` to build the commercial package. `rpmbuild`
expects a source archive that unpacks to the directory named in the spec:

```bash
rpmdev-setuptree
name=$(awk '/^Name:/{print $2; exit}' mysql-shell.spec)
srcdir=$(sed -n 's/^%setup .*-n[[:space:]]\+//p' mysql-shell.spec | head -n1 \
         | sed "s/%{name}/${name}/")
tar czf ~/rpmbuild/SOURCES/$srcdir.tar.gz --exclude=.git \
    --transform "s,^\.,$srcdir," .
cp mysql-shell.spec ~/rpmbuild/SPECS/
```

Then build, passing the build configuration as `--define` switches:

```bash
rpmbuild -ba ~/rpmbuild/SPECS/mysql-shell.spec \
    --define "with_mysql_source $HOME/mysql-src/mysql-server" \
    --define "bundled_antlr $HOME/mysql-src/antlr4-runtime-install"
```

The packages are written to `~/rpmbuild/RPMS`. The spec reads these switches:

- `with_mysql_source`: MySQL Server source directory; enables packaging of the
  client plugins and `libfido2`
- `with_mysql_build`: MySQL Server build directory (default:
  `<with_mysql_source>/bld`)
- `bundled_antlr`: install prefix of the ANTLR4 C++ runtime; it is bundled into
  the package
- `bundled_openssl`: OpenSSL installation to bundle
- `bundled_ssh`: libssh installation to bundle
- `bundled_curl`: libcurl installation to use (passed as `WITH_CURL`)
- `bundled_python`: Python installation to bundle
- `bundled_shared_python`: also package `libpython` (use with `bundled_python`)
- `python_deps`: directory with the pure-Python modules to package (`certifi`,
  `PyYAML`)
- `bundled_mysql_config_editor`: path of the built `mysql_config_editor`
- `bundled_mysqlbinlog`: path of the built `mysqlbinlog` (default: from the
  MySQL Server build tree)
- `jit_executor_lib`: Polyglot API library, enables JavaScript support
- `with_protobuf`: value of `WITH_PROTOBUF` (for example `system`)
- `bundle_fido`: set to `0` to leave `libfido2` out of the package
- `with_tests`: build the unit tests

Without `python_deps` and `bundled_python`, the built-in plugins import
`certifi` and `yaml` from the system Python at run time.

### Build a Debian or Ubuntu package

Install `debhelper`, `dpkg-dev`, `fakeroot` and `lsb-release`, plus the build
dependencies from section 1. Generate the `debian/` directory, which is
written to the root of the MySQL Shell source tree, then build:

```bash
cd ~/mysql-src/mysql-shell
cmake -S packaging/debian -B /tmp/mysql-shell-deb-init \
    -DMYSQL_SOURCE_DIR=$HOME/mysql-src/mysql-server \
    -DHAVE_PYTHON=1
dpkg-buildpackage -us -uc -b
```

The `.deb` files are written to the parent directory of the source tree.
`HAVE_PYTHON` must be passed explicitly. Without it the package is built
without Python support. The generator accepts these options:

- `MYSQL_SOURCE_DIR`: MySQL Server source directory
- `MYSQL_BUILD_DIR`: MySQL Server build directory (default:
  `${MYSQL_SOURCE_DIR}/bld`)
- `HAVE_PYTHON`: build with Python support
- `BUNDLED_PYTHON_DIR`: Python installation to bundle
- `PYTHON_DEPS`: directory with the pure-Python modules to package (`certifi`,
  `PyYAML`)
- `BUNDLED_ANTLR_DIR`: install prefix of the ANTLR4 C++ runtime
- `BUNDLED_SSH_DIR`: libssh installation to bundle
- `WITH_CURL`: libcurl installation to use
- `BUNDLED_MYSQL_CONFIG_EDITOR`: path of the built `mysql_config_editor`
- `BUNDLED_MYSQLBINLOG`: path of the built `mysqlbinlog`
- `JIT_EXECUTOR_LIB`: Polyglot API library, enables JavaScript support
- `WITH_PROTOBUF`: value of `WITH_PROTOBUF` (for example `system`)
- `BUNDLE_FIDO`: include `libfido2` in the package (default `ON`)
- `COMMUNITY_BUILD`: `OFF` builds the `mysql-shell-commercial` package
- `DEBIAN_REVISION`: Debian revision of the package version (default `1`)
- `WITH_TESTS`: build the unit tests

Without `PYTHON_DEPS` and `BUNDLED_PYTHON_DIR`, the built-in plugins need the
`python3-certifi` and `python3-yaml` packages at run time.

On Debian 13 the packaged ANTLR4 runtime is 4.9, which is older than the
required 4.10; build it as described in section 1 and pass
`-DBUNDLED_ANTLR_DIR`. Ubuntu 24.04 ships 4.10.1, which can be used as is.

Debian packages are built against the system OpenSSL.

Known issues on Ubuntu 24.04:

- Building with the default `dpkg-buildflags` (LTO) fails with
  `-Werror=stringop-overflow` in the bundled linenoise-ng.
- `dh_shlibdeps` does not find `libfido2` in the private library directory.


Copyright (c) 2016, 2026, Oracle and/or its affiliates.
