Native sample Instructions

On Windows:

0. If you already have the dev libs & headers installed, you can skip to step 6 (except for step 3).
   Otherwise, go to step 1.

1. First it is necessary to run build the dependencies, these instructions assume using DLL & MD runtime (because
   the managed sample needs them), but this project can be used with static linking and MT runtime too.

2. In order to build in MD runtime it is necessary to build all dependencies too
The dependencies are
a) ng-deps (v8)
b) Protobuf 2.6.1 (optional)
c) MySql Server libclient
d) boost 1.57 (no need to recompile, as only headers are used).
e) Python 2.7

3. To build the server in MD mode, apply patch file cmake-server-patch.txt to the cmake server file
   <server>/cmake/os/windows.cmake
Then you can build the server with
cd <server>
mkdir MY_BUILD
cd MY_BUILD
cmake -DWINDOWS_RUNTIME_MD=1 ..

4. To build ng-deps, open the file build_win32.bat (or build_win64.bat) and change the line
python v8\build\gyp_v8 -Dtarget_arch=ia32 -Dcomponent=static_library
to
python v8\build\gyp_v8 -Dtarget_arch=ia32 -Dcomponent=shared_library
and run the script.

5. For Protobuf 2.6.1, it is necessary to open the solution & manually change the runtime from MT -> MD.

6. Now you can run the cmake for xshell, like the following sample
IMPORTANT: Use the parameter "-DWINDOWS_RUNTIME_MD=1" to generate DLL binaries with MD runtime (required by CLR
interop in managed sample)
cmake -DV8_INCLUDE_DIR=C:\src\mysql-server\ng-deps-qa-MD\v8\include -DV8_LIB_DIR=C:\src\mysql-server\ng-deps-qa-MD\v8\build\Debug\lib -DBOOST_ROOT=C:\src\mysql-server\boost\boost_1_57_0 -DMYSQL_DIR="C:\MySQL\mysql-5.6.22-win32-MD-debug" -DPROTOBUF_LIBRARY=C:\src\mysql-server\protobuf\protobuf-2.6.1\vsprojects\Debug\libprotobuf.lib -DPROTOBUF_INCLUDE_DIR=C:\src\mysql-server\protobuf\protobuf-2.6.1\src -DWITH_PROTOBUF=C:\src\mysql-server\protobuf\protobuf-2.6.1 -DWINDOWS_RUNTIME_MD=1 -DCMAKE_INSTALL_PREFIX=C:\src\mysql-server\ngshell-install-script-MD-2\install_image2 ..

8. The previous step will automatically copy the v8.dll & v8.lib to the CMAKE_INSTALL_PREFIX/lib folder.

9. Build the shell:
  msbuild mysh.sln /p:Configuration=Debug

10. Now you can install the dependencies: ...

msbuild install.vcxproj

11. Now you can build the native sample, with
cd <native-dir>
mkdir MY_BUILD
cd MY_BUILD
cmake -DDEPLOY_DIR=C:\src\mysql-server\ngshell-install-script-MD-2\install_image2 -DMYSQL_CLIENT_LIB="C:\MySQL\mysql-5.6.22-win32-MD-debug\lib\mysqlclient.lib" -DBOOST_INCLUDE_DIR=C:\src\mysql-server\boost\boost_1_57_0 ..
msbuild sample_client.sln /p:Configuration=Debug

12. Now for the native sample run the install script to copy DEPLOY_DIR (same as CMAKE_INSTALL_PREFIX in main shell build).

msbuild INSTALL.vcxproj

13. Now you can zip the whole contents of DEPLOY_DIR (C:\src\mysql-server\ngshell-install-script-MD-2\install_image2), that's the bundle for clients to use.


On Linux:
0. On Linux you can compile the shell with something like this
cd <shell-root>
mkdir MY_BUILD
cd MY_BUILD
# Next line specifies -DCMAKE_INSTALL_PREFIX=/home/fernando/src/ngshell-install-script-final/install_image/
# where the files will be installed
cmake -DV8_INCLUDE_DIR=/home/fernando/src/ng-qa/ngshell-deps/v8/include -DV8_LIB_DIR=/home/fernando/src/ng-qa/ngshell-deps/v8/out/x64.debug/lib.target -DWITH_TESTS=1 -DWITH_GTEST=/home/fernando/src/ng-qa/ngshell-deps/v8/testing/gtest/bld/ -DMYSQL_DIR=/home/fernando/src/mysql-server/mysql-advanced-5.6.24-linux-glibc2.5-x86_64/ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/home/fernando/src/ngshell-install-script-final/install_image/ -DPROTOBUF_INCLUDE_DIR=/usr/local/include/ -DPROTOBUF_LIBRARY=/usr/local/lib64/ ..
make
make install
1. The previous step will automatically copy the libv8.so  to the CMAKE_INSTALL_PREFIX/lib folder.
2. Next you can build the native sample, for example:
cd <shell-root>/samples/native
mkdir MY_BUILD
cd MY_BUILD
cmake -DDEPLOY_DIR=/home/fernando/src/ngshell-readonly-frozen-7/deployed/ -DMYSQL_CLIENT_LIB="/home/fernando/src/mysql-server/mysql-advanced-5.6.24-linux-glibc2.5-x86_64/lib/libmysqlclient.so" -DPROTOBUF_LIBRARY=/usr/local/lib64/libprotobuf.so -DPYTHON_LIBS=/usr/lib64/libpython2.7.so ..
make

3. Now you can run 
  make install
To copy the shell abstraction the CMAKE_INSTALL_PREFIX/DEPLOY_DIR, zip everything and make the bundle available to clients.

4. You can also run the sample client with ./shell_client_app



*** About the sample code ***

The class Simple_shell_client encapsultes the shell,
It is possible to inherit from this class to override callbacks like print (for print instrinsic) and print_err (for shell
errors like syntax issues). Both the native sample & managed sample use this approach.
The sample just does an REPL in SQL, then switch to JS and another REPL
each REPL is broken with the "\quit" command.

IMPORTANT: The sample session loads the mysqlx module, and thus assumes, there is a module folder in the same 
path than shell library.

Sample Session:

 Sample session, first in SQL, until command \quit is entered, then in
JS mode, until \quit is entered again.
As sample session would be
sql> use sakila;
sql> select * from actor limit 5;
\quit
js> var mysqlx = require('mysqlx').mysqlx;
js> var mysession = mysqlx.getNodeSession("root:123@localhost:33060");
js> mysession.sql("show databases").execute();
js> dir(mysession);
js> dir(mysession.mysql);
js> mysession.sql('drop schema if exists js_shell_test;').execute();
js> mysession.sql('create schema js_shell_test;').execute();
js> mysession.sql('use js_shell_test;').execute();
js> mysession.sql('create table table1 (name varchar(50), age integer, gender varchar(20));').execute();

js> var schema = mysession.getSchema('js_shell_test');
js> var table = schema.getTable('table1');

js> var result = table.insert({name: 'jack', age: 17, gender: 'male'}).execute();
js> var result = table.insert({name: 'adam', age: 15, gender: 'male'}).execute();
js> var result = table.insert({name: 'brian', age: 14, gender: 'male'}).execute();
js> var result = table.insert({name: 'alma', age: 13, gender: 'female'}).execute();
js> var result = table.insert({name: 'carol', age: 14, gender: 'female'}).execute();
js> var result = table.insert({name: 'donna', age: 16, gender: 'female'}).execute();
js> var result = table.insert({name: 'angel', age: 14, gender: 'male'}).execute();

js> table.select().execute();

js> var crud = table.delete();
js> crud.where('age = 15');
js> crud.execute();
js> table.select().execute();

js> var result = table.insert({name: 'adam', age: 15, gender: 'male'}).execute();

js> var crud = table.update();
js> crud.set({ name: 'Adam Sanders'}).where("name = 'adam'");
js> crud.execute();
js> table.select().execute();


// etc...
js> 
