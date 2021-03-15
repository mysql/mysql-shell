//@<> Update sys.path to allow loading modules on this folder
var paths = []
for(p in sys.path) { paths.push(sys.path[p]); }
var script_base = os.path.join(__test_home_path, "scripts", "auto")
var script_base = os.path.join(script_base, os.path.dirname(__script_file), "scripts")
paths.push(script_base)
sys.path = paths
