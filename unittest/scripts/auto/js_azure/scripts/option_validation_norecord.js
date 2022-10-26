//@{__azure_configured}
shell.connect(__mysqluripwd + "/mysql");

var utils = require("_utils")
utils.setupFailureTests();

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
