//@{__azure_configured}
shell.connect(__mysqluripwd + "/mysql");

var utils = require("_utils")

let WITHOUT_ACCOUNT = undefined
let WITHOUT_KEY = undefined
let WITHOUT_SAS_TOKEN = undefined
let FAKE_ACCOUNT = 'fake'
let FAKE_KEY = 'fake'
let FAKE_SAS_TOKEN = 'fake'
let WITHOUT_CONN_STR = undefined
let FULL_CONN_STR = true
let HALF_CONN_STR = false


function get_env_vars(account, key, connstring, csaccount=undefined, cskey=undefined, sas_token=undefined) {
    vars = []
    if (account !== WITHOUT_ACCOUNT) {
        vars.push(`AZURE_STORAGE_ACCOUNT=${account}`);
    }

    if (key !== WITHOUT_KEY) {
        vars.push(`AZURE_STORAGE_KEY=${key}`);
    }

    if (connstring===WITHOUT_CONN_STR) {
        vars.push("AZURE_STORAGE_CONNECTION_STRING=");
    } else if (connstring == FULL_CONN_STR){
        vars.push(`AZURE_STORAGE_CONNECTION_STRING=${utils.getConnectionString(__azure_endpoint, true, csaccount, cskey)}`);
    } else if (connstring == HALF_CONN_STR) {
        vars.push(`AZURE_STORAGE_CONNECTION_STRING=${utils.getConnectionString(__azure_endpoint, false)}`);
    }

    if (sas_token !== WITHOUT_SAS_TOKEN) {
        vars.push(`AZURE_STORAGE_SAS_TOKEN=${sas_token}`);
    }

    return vars;
}

function update_config(path, account, key, connstring, csaccount=undefined, cskey=undefined, sas_token=undefined) {
    let config = "[storage]\n";
    if (account !== WITHOUT_ACCOUNT) {
        config += `account=${account}\n`;
    }

    if (key !== WITHOUT_KEY) {
        config += `key=${key}\n`;
    }

    if (connstring!==WITHOUT_CONN_STR) {
        if (connstring == FULL_CONN_STR){
            config += `connection_string=${utils.getConnectionString(__azure_endpoint, true, csaccount, cskey)}\n`;
        } else if (connstring == HALF_CONN_STR) {
            config += `connection_string=${utils.getConnectionString(__azure_endpoint, false)}\n`;
        }
    }

    if (sas_token !== WITHOUT_SAS_TOKEN) {
        config += `sas_token=${sas_token}\n`;
    }

    testutil.createFile(path, config);
}


function test_configuration(options, env_vars) {
    utils.testFailure(options, env_vars, "The specified container does not exist.",{
        "importTable": "Failed opening object 'path' in READ mode: Failed to get summary for object 'path': Not Found (404)",
        "loadDump": "Failed opening object 'sample/@.json' in READ mode: Failed to get summary for object 'sample/@.json': Not Found (404)"
    });
}

testutil.mkdir("azure_fallback");
config_file = "azure_fallback/config";

//@<> TS_R8_8, TS_R8_9 - azureStorageSasToken over env and config file {__azure_emulator==false} // The Azure Emulator does not support for SAS Tokens
let env_vars = get_env_vars(FAKE_ACCOUNT, FAKE_KEY, FULL_CONN_STR, FAKE_ACCOUNT, FAKE_KEY, FAKE_SAS_TOKEN);
update_config(config_file, FAKE_ACCOUNT, FAKE_KEY, FULL_CONN_STR, FAKE_ACCOUNT, FAKE_KEY, FAKE_SAS_TOKEN);
test_configuration(["--azureContainerName=something", `--azureConfigFile=${config_file}`, `--azureStorageAccount=${__azure_account}`, "--azureStorageSasToken", `${__azure_account_rwl_sas_token}`], env_vars);

//@<> azureStorageAccount over env and config file
env_vars = get_env_vars(FAKE_ACCOUNT, FAKE_KEY, FULL_CONN_STR, FAKE_ACCOUNT, __azure_key);
update_config(config_file, FAKE_ACCOUNT, FAKE_KEY, FULL_CONN_STR, FAKE_ACCOUNT, FAKE_KEY);
test_configuration(["--azureContainerName=something", `--azureConfigFile=${config_file}`, `--azureStorageAccount=${__azure_account}`], env_vars);

//@<> TS_R8_3, TS_R8_5, TS_R8_7, TS_R8_8, TS_R8_9 - AZURE_STORAGE_SAS_TOKEN over other env vars and config file {__azure_emulator==false} // The Azure Emulator does not support for SAS Tokens
env_vars = get_env_vars(FAKE_ACCOUNT, FAKE_KEY, FULL_CONN_STR, __azure_account, FAKE_KEY, __azure_account_rwl_sas_token);
update_config(config_file, FAKE_ACCOUNT, FAKE_KEY, FULL_CONN_STR, FAKE_ACCOUNT, FAKE_KEY, FAKE_SAS_TOKEN);
test_configuration(["--azureContainerName=something", `--azureConfigFile=${config_file}`, `--azureStorageAccount=${__azure_account}`], env_vars);

//@<> TS_R8_4, TS_R8_6, TS_R8_8, TS_R8_9 - sas_token over other env vars and config file {__azure_emulator==false} // The Azure Emulator does not support for SAS Tokens
env_vars = get_env_vars(FAKE_ACCOUNT, FAKE_KEY, FULL_CONN_STR, __azure_account, FAKE_KEY, WITHOUT_SAS_TOKEN);
update_config(config_file, FAKE_ACCOUNT, FAKE_KEY, FULL_CONN_STR, FAKE_ACCOUNT, FAKE_KEY, __azure_account_rwl_sas_token);
test_configuration(["--azureContainerName=something", `--azureConfigFile=${config_file}`, `--azureStorageAccount=${__azure_account}`], env_vars);

//@<> TS_R8_2 - AZURE_STORAGE_CONNECTION_STRING over other env vars and config file
env_vars = get_env_vars(FAKE_ACCOUNT, FAKE_KEY, FULL_CONN_STR, __azure_account, __azure_key);
test_configuration(["--azureContainerName=something", `--azureConfigFile=${config_file}`], env_vars);


//@<> TS_R8_1 - AZURE_STORAGE_ACCOUNT, AZURE_STORAGE_KEY and partial AZURE_STORAGE_CONNECTION_STRING over config file
env_vars = get_env_vars(__azure_account, __azure_key, HALF_CONN_STR);
test_configuration(["--azureContainerName=something", `--azureConfigFile=${config_file}`], env_vars);


//@<> TS_R8_1 - AZURE_STORAGE_ACCOUNT, AZURE_STORAGE_KEY over config file, connection data fallback to connstring in config file
env_vars = get_env_vars(__azure_account, __azure_key, WITHOUT_CONN_STR);
update_config(config_file, FAKE_ACCOUNT, FAKE_KEY, FULL_CONN_STR, __azure_account, __azure_key);
test_configuration(["--azureContainerName=something", `--azureConfigFile=${config_file}`], env_vars);


//@<> connection_string over account and key
env_vars = get_env_vars(WITHOUT_ACCOUNT, WITHOUT_KEY, WITHOUT_CONN_STR);
update_config(config_file, FAKE_ACCOUNT, FAKE_KEY, FULL_CONN_STR, __azure_account, __azure_key);
test_configuration(["--azureContainerName=something", `--azureConfigFile=${config_file}`], env_vars);


//@<> config account and key as final fallbacks
env_vars = get_env_vars(WITHOUT_ACCOUNT, WITHOUT_KEY, WITHOUT_CONN_STR);
update_config(config_file, __azure_account, __azure_key, HALF_CONN_STR);
test_configuration(["--azureContainerName=something", `--azureConfigFile=${config_file}`], env_vars);

testutil.rmdir("azure_fallback", true);

