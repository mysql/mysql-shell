Managed sample Instructions

1. Please first follow ..\native\README.txt to build native sample (as the managed sample depends on it).
2. Now, from the managed sample root folder, go to the lib folder, open simple_shell_client_managed.vcxproj with notepad
  locate this section 
  <PropertyGroup>
    <BoostHeaderPath>C:\src\mysql-server\boost\boost_1_57_0</BoostHeaderPath>
    <NativeSampleHeaderPath>C:\src\mysql-server\ngshell-install-script-MD-2\samples\native\lib</NativeSampleHeaderPath>
    <MySqlXShellHeadersPath>C:\src\mysql-server\ngshell-install-script-MD-2\install_image\include</MySqlXShellHeadersPath>
    <MySqlXShellLibsPath>C:\src\mysql-server\ngshell-install-script-MD-2\install_image\lib</MySqlXShellLibsPath>
    <MySqlServerLibsPath>C:\MySQL\mysql-5.6.22-win32-MD-debug\lib\</MySqlServerLibsPath>
    <NativeSampleLibPath>C:\src\mysql-server\ngshell-install-script-MD-2\samples\native\MY_BUILD\lib\Debug</NativeSampleLibPath>
  </PropertyGroup>

And reedit the property values with the correct paths, where
- BoostHeaderPath is the path where boost folder with header are located
- NativeSampleHeaderPath is the path of samples/native/lib (header simple_shell_client.h)
- MySqlXShellHeadersPath is the path well all MySql Shell headers are located like (C:\Program Files (x86)\mysh\include) or
  xshell root folder.
- MySqlXShellLibsPath is the path of all MySql Shell .lib files like (C:\Program Files (x86)\mysh\lib), or a path that contains
  mysqlshcore.lib, mysqlshtypes.lib, v8.lib, mysqlclient.lib
- MySqlServerLibsPath, lib path for mysqlclient.lib
- NativeSampleLibPath, the path where the native/lib output is generated (file shell_client_lib.lib).

3. Now you can build the solution, from Visual Studio or command line:
IMPORTANT: The lib project is strong named with the Connector/NET key, this key needs to be installed in the dev machine with
  sn -i cnet.snk
  (the key may change in the future)
  If you dont want to strong-name the assembly, just open the file managed/lib/simple_shell_client_wrapper.cpp and comment
  out the lines
    [assembly:AssemblyDelaySign(false)];
    [assembly:AssemblyKeyName("ConnectorNet")];
 (Strong naming is important for Visual Studio client, not for testing purposes).
END OF IMPORTANT

 msbuild simple_shell_client_managed.sln /p:Configuration=Debug


4. Now you can build the sample C# client, open the project at managed/app/SimpleShellClientSharp.csproj

5. Add/correct the assembly reference to simple_shell_client_managed.dll created in step 3.
Also copy the whole set of dependencies into the managed\app\bin\Debug folder of the managed sample:
- mysqlshcore.dll
- mysqlshtypes.dll
- shell_client_lib.dll
- simple_shell_client_managed.dll
- v8.dll

6. You can run the sample.
  (For sample input, refer to README.txt in native sample).

