\py
#@ Fail test
import os
path=os.path.join(__test_home_path, "scripts/auto/py_shell/scripts")
\source <<<path>>>/cmd_source_test.js
#@ Success test
\source --js <<<path>>>/cmd_source_test.js
