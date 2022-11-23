//@{__azure_configured}
shell.connect(__mysqluripwd + "/mysql");

var utils = require("_utils")
utils.setupFailureTests();


//@<> Account SAS Token Validation Tests
account_sas_tests = [
    // Missing SAS Required Tokens
    ["ss=bfqt&srt=sco&sp=rwdlacupiytfx&se=2022-11-23T05:31:36Z&st=2022-11-22T21:31:36Z&spr=https&sig=0bEAWf%2FtR%2BHhHDkHdP5khrLh3YrcN0X%2BIvirOVgXqbA%3D", "the following attributes are missing: Signed Version"],
    ["sv=2021-06-08&ss=bfqt&srt=sco&se=2022-11-23T05:31:36Z&st=2022-11-22T21:31:36Z&spr=https&sig=0bEAWf%2FtR%2BHhHDkHdP5khrLh3YrcN0X%2BIvirOVgXqbA%3D", "the following attributes are missing: Signed Permissions"],
    ["sv=2021-06-08&ss=bfqt&sp=rwdlacupiytfx&se=2022-11-23T05:31:36Z&st=2022-11-22T21:31:36Z&spr=https&sig=0bEAWf%2FtR%2BHhHDkHdP5khrLh3YrcN0X%2BIvirOVgXqbA%3D", "the following attributes are missing: Signed Resource Types"],
    ["sv=2021-06-08&ss=bfqt&srt=sco&sp=rwdlacupiytfx&st=2022-11-22T21:31:36Z&spr=https&sig=0bEAWf%2FtR%2BHhHDkHdP5khrLh3YrcN0X%2BIvirOVgXqbA%3D", "the following attributes are missing: Expiration Time"],
    ["sv=2021-06-08&ss=bfqt&srt=sco&sp=rwdlacupiytfx&se=2022-11-23T05:31:36Z&st=2022-11-22T21:31:36Z&spr=https", "the following attributes are missing: Signature"],

    // Missing Blob Service Access
    ["sv=2021-06-08&ss=fqt&srt=sco&sp=rwdlacupiytfx&se=2022-11-23T05:31:36Z&st=2022-11-22T21:31:36Z&spr=https&sig=0bEAWf%2FtR%2BHhHDkHdP5khrLh3YrcN0X%2BIvirOVgXqbA%3D", "it is missing access to the Blob Storage Service"],

    // Missing Container And Object Access
    ["sv=2021-06-08&ss=bfqt&srt=so&sp=rwdlacupiytfx&se=2022-11-23T05:31:36Z&st=2022-11-22T21:31:36Z&spr=https&sig=0bEAWf%2FtR%2BHhHDkHdP5khrLh3YrcN0X%2BIvirOVgXqbA%3D", "does not give access to the container"],
    ["sv=2021-06-08&ss=bfqt&srt=sc&sp=rwdlacupiytfx&se=2022-11-23T05:31:36Z&st=2022-11-22T21:31:36Z&spr=https&sig=0bEAWf%2FtR%2BHhHDkHdP5khrLh3YrcN0X%2BIvirOVgXqbA%3D", "does not give access to the container objects"],

    // Missing Permissions (Read/List)
    ["sv=2021-06-08&ss=bfqt&srt=sco&sp=wdlacupiytfx&se=2022-11-23T05:31:36Z&st=2022-11-22T21:31:36Z&spr=https&sig=0bEAWf%2FtR%2BHhHDkHdP5khrLh3YrcN0X%2BIvirOVgXqbA%3D", "it is missing the following permissions: Read"],
    ["sv=2021-06-08&ss=bfqt&srt=sco&sp=rwdacupiytfx&se=2022-11-23T05:31:36Z&st=2022-11-22T21:31:36Z&spr=https&sig=0bEAWf%2FtR%2BHhHDkHdP5khrLh3YrcN0X%2BIvirOVgXqbA%3D", "it is missing the following permissions: List"],
]

for (index in account_sas_tests) {
    utils.testFailure(["--azureContainerName=something", "--azureStorageSasToken", `${account_sas_tests[index][0]}`], [], `The Shared Access Signature Token defined at the 'azureStorageSasToken' option is invalid, ${account_sas_tests[index][1]}`);
}

//@<> Account SAS Token Validation Tests - Dump Specific
account_sas_tests = [
    // Missing Permissions (Create/Write)
    ["sv=2021-06-08&ss=bfqt&srt=sco&sp=rdlaupiytfx&se=2022-11-23T05:31:36Z&st=2022-11-22T21:31:36Z&spr=https&sig=0bEAWf%2FtR%2BHhHDkHdP5khrLh3YrcN0X%2BIvirOVgXqbA%3D", "it is missing the following permissions: Create or Write"],
]

for (index in account_sas_tests) {
    utils.testDumpFailure(["--azureContainerName=something", "--azureStorageSasToken", `${account_sas_tests[index][0]}`], [], `The Shared Access Signature Token defined at the 'azureStorageSasToken' option is invalid, ${account_sas_tests[index][1]}`);
}


//@<> Container SAS Token Validation Tests
container_sas_tests = [
    // Missing SAS Required Tokens
    ["sp=racwdli&st=2022-11-22T23:24:14Z&se=2022-11-23T07:24:14Z&spr=https&sr=c&sig=%2Bi5WV7%2FvWGdbpkt3xxYQnAmNNpGJpLN9T93YPWPuZFs%3D", "the following attributes are missing: Signed Version"],
    ["st=2022-11-22T23:24:14Z&se=2022-11-23T07:24:14Z&spr=https&sv=2021-06-08&sr=c&sig=%2Bi5WV7%2FvWGdbpkt3xxYQnAmNNpGJpLN9T93YPWPuZFs%3D", "the following attributes are missing: Signed Permissions"],
    ["sp=racwdli&st=2022-11-22T23:24:14Z&se=2022-11-23T07:24:14Z&spr=https&sv=2021-06-08&sig=%2Bi5WV7%2FvWGdbpkt3xxYQnAmNNpGJpLN9T93YPWPuZFs%3D", "the following attributes are missing: Signed Resource"],
    ["sp=racwdli&st=2022-11-22T23:24:14Z&spr=https&sv=2021-06-08&sr=c&sig=%2Bi5WV7%2FvWGdbpkt3xxYQnAmNNpGJpLN9T93YPWPuZFs%3D", "the following attributes are missing: Expiration Time"],
    ["sp=racwdli&st=2022-11-22T23:24:14Z&se=2022-11-23T07:24:14Z&spr=https&sv=2021-06-08&sr=c", "the following attributes are missing: Signature"],

    // Missing Container Access
    ["sp=racwdli&st=2022-11-22T23:10:19Z&se=2022-11-23T07:10:19Z&spr=https&sv=2021-06-08&sr=o&sig=WKEJhgxGftVFD2GChBEa1MVDN8AvK01%2B1vMdEKmaN0Y%3D", "does not give access to the container"],

    // Missing Permissions (Read/List)
    ["sp=acwdli&st=2022-11-22T23:10:19Z&se=2022-11-23T07:10:19Z&spr=https&sv=2021-06-08&sr=c&sig=WKEJhgxGftVFD2GChBEa1MVDN8AvK01%2B1vMdEKmaN0Y%3D", "it is missing the following permissions: Read"],
    ["sp=racwdi&st=2022-11-22T23:10:19Z&se=2022-11-23T07:10:19Z&spr=https&sv=2021-06-08&sr=c&sig=WKEJhgxGftVFD2GChBEa1MVDN8AvK01%2B1vMdEKmaN0Y%3D", "it is missing the following permissions: List"],
]

for (index in container_sas_tests) {
    utils.testFailure(["--azureContainerName=something", "--azureStorageSasToken", `${container_sas_tests[index][0]}`], [], `The Shared Access Signature Token defined at the 'azureStorageSasToken' option is invalid, ${container_sas_tests[index][1]}`);
}

//@<> Container SAS Token Validation Tests - Dump Specific
container_sas_tests = [
    // Missing Permissions (Create/Write)
    ["sp=radli&st=2022-11-22T23:10:19Z&se=2022-11-23T07:10:19Z&spr=https&sv=2021-06-08&sr=c&sig=WKEJhgxGftVFD2GChBEa1MVDN8AvK01%2B1vMdEKmaN0Y%3D", "it is missing the following permissions: Create or Write"],
]

for (index in container_sas_tests) {
    utils.testDumpFailure(["--azureContainerName=something", "--azureStorageSasToken", `${container_sas_tests[index][0]}`], [], `The Shared Access Signature Token defined at the 'azureStorageSasToken' option is invalid, ${container_sas_tests[index][1]}`);
}

//@<> TS_R2_1 - Conflicting options with OCI
utils.testFailure(["--azureContainerName=something", "--osBucketName=something"], [], "The option 'azureContainerName' cannot be used when the value of 'osBucketName' option is set.");

//@<> TS_R2_2 - Conflicting options with AWS
utils.testFailure(["--azureContainerName=something", "--s3BucketName=something"], [], "The option 's3BucketName' cannot be used when the value of 'azureContainerName' option is set.");

//@<> Using azure options without azureContainerName
utils.testFailure(["--azureConfigFile=something"], [], "The option 'azureConfigFile' cannot be used when the value of 'azureContainerName' option is not set.");
utils.testFailure(["--azureStorageAccount=something"], [], "The option 'azureStorageAccount' cannot be used when the value of 'azureContainerName' option is not set.");
utils.testFailure(["--azureStorageSasToken=something"], [], "The option 'azureStorageSasToken' cannot be used when the value of 'azureContainerName' option is not set.");

//@<> TS_R2_3 - Unexisting Container
utils.testFailure(["--azureContainerName=unexisting"], [], "The specified container does not exist.",{
    "importTable": "Failed opening object 'path' in READ mode: Failed to get summary for object 'path': Not Found (404)",
    "loadDump": "Failed opening object 'sample/@.json' in READ mode: Failed to get summary for object 'sample/@.json': Not Found (404)"
});

//@<> TS_R2_4 - Invalid Container Name
utils.testFailure(["--azureContainerName=ab"], [], "The specified resource name length is not within the permissible limits.",{
    "importTable": "Failed to get summary for object 'path': Bad Request (400)",
    "loadDump": "Failed to get summary for object 'sample/@.json': Bad Request (400)"
});

//@<> TS_R5_1-A - Unexisting Custom Azure Configuration File
utils.testFailure(["--azureContainerName=something", "--azureConfigFile=azure_config"], ['AZURE_STORAGE_CONNECTION_STRING='], "The azure configuration file 'azureConfigFile' does not exist.");

//@<> TS_R5_1-B - Invalid Custom Azure Configuration File
testutil.mkdir("azure_config");
utils.testFailure(["--azureContainerName=something", "--azureConfigFile=azure_config"], ['AZURE_STORAGE_CONNECTION_STRING='], "The azure configuration file 'azureConfigFile' is invalid.");

//@<> TS_R4_4 - Missing Default Configuration File
utils.testFailure(["--azureContainerName=something"], ['HOME=azure_config', 'AZURE_STORAGE_CONNECTION_STRING='], "The Azure Storage Account('azureStorageAccount') is not defined.");

//@<> TS_R4_2, TS_R4_3 - Reading Configuration File from default location (The error confirms the configuration was read successfully)
testutil.mkdir("azure_config/.azure");
testutil.createFile("azure_config/.azure/config", `[storage]\nconnection_string=${utils.getConnectionString(__azure_endpoint, true, __azure_account, __azure_key)}`);
utils.testFailure(["--azureContainerName=something"], ['HOME=azure_config', 'AZURE_STORAGE_CONNECTION_STRING='], "The specified container does not exist.",{
    "importTable": "Failed opening object 'path' in READ mode: Failed to get summary for object 'path': Not Found (404)",
    "loadDump": "Failed opening object 'sample/@.json' in READ mode: Failed to get summary for object 'sample/@.json': Not Found (404)"
});


//@<> TS_R5_3 - Empty configuration file option is ignored
testutil.mkdir("azure_config/.azure");
utils.testFailure(["--azureContainerName=something", "--azureConfigFile", ""], ['HOME=azure_config', 'AZURE_STORAGE_CONNECTION_STRING='], "The specified container does not exist.",{
    "importTable": "Failed opening object 'path' in READ mode: Failed to get summary for object 'path': Not Found (404)",
    "loadDump": "Failed opening object 'sample/@.json' in READ mode: Failed to get summary for object 'sample/@.json': Not Found (404)"
});


//@<> TS_R5_2 - Reading configuration file from custom location
utils.testFailure(["--azureContainerName=something", "--azureConfigFile", "azure_config/.azure/config"], ['AZURE_STORAGE_CONNECTION_STRING='], "The specified container does not exist.",{
    "importTable": "Failed opening object 'path' in READ mode: Failed to get summary for object 'path': Not Found (404)",
    "loadDump": "Failed opening object 'sample/@.json' in READ mode: Failed to get summary for object 'sample/@.json': Not Found (404)"
});

//@<> TS_R6_1-Invalid Username
utils.testFailure(["--azureContainerName=something", "--azureConfigFile", "azure_config/.azure/config", "--azureStorageAccount=jd"], ['AZURE_STORAGE_CONNECTION_STRING='], "The specified Azure Storage Account name is invalid, expected 3 to 24 characters: jd");
utils.testFailure(["--azureContainerName=something", "--azureConfigFile", "azure_config/.azure/config", "--azureStorageAccount=jd-jd"], ['AZURE_STORAGE_CONNECTION_STRING='], "The specified Azure Storage Account name is invalid, expected numbers and lowercase characters: jd-jd");

//@<> TS_R6_1-Empty Username
utils.testFailure(["--azureContainerName=something", "--azureConfigFile", "azure_config/.azure/config", "--azureStorageAccount", ""], ['AZURE_STORAGE_CONNECTION_STRING='], "The specified container does not exist.",{
    "importTable": "Failed opening object 'path' in READ mode: Failed to get summary for object 'path': Not Found (404)",
    "loadDump": "Failed opening object 'sample/@.json' in READ mode: Failed to get summary for object 'sample/@.json': Not Found (404)"
});


//@<> TS_R7_1-Invalid Username
utils.testFailure(["--azureContainerName=something", "--azureConfigFile", "azure_config/.azure/config"], ['AZURE_STORAGE_CONNECTION_STRING=', 'AZURE_STORAGE_ACCOUNT=jd'], "The specified Azure Storage Account name is invalid, expected 3 to 24 characters: jd");
utils.testFailure(["--azureContainerName=something", "--azureConfigFile", "azure_config/.azure/config"], ['AZURE_STORAGE_CONNECTION_STRING=', 'AZURE_STORAGE_ACCOUNT=jd-jd'], "The specified Azure Storage Account name is invalid, expected numbers and lowercase characters: jd-jd");

//@<> TS_R7_1-Empty Username
utils.testFailure(["--azureContainerName=something", "--azureConfigFile", "azure_config/.azure/config"], ['AZURE_STORAGE_CONNECTION_STRING=', 'AZURE_STORAGE_ACCOUNT='], "The specified container does not exist.",{
    "importTable": "Failed opening object 'path' in READ mode: Failed to get summary for object 'path': Not Found (404)",
    "loadDump": "Failed opening object 'sample/@.json' in READ mode: Failed to get summary for object 'sample/@.json': Not Found (404)"
});

//@<> cleanup
utils.cleanupFailureTests();
testutil.rmdir("azure_config", true);
