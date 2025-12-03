
#@<> Invalid format and duplicate entries in include/exclude filters
filters = ["Schemas", "Tables", "Routines", "Triggers", "Events", "Users"]
samples = {"Schemas": "`schema`", "Users": "'sample'@'%'"}

shell.connect(__mysql_uri)

for f in filters:
    sample = samples.get(f, "`schema`.`item`")
    item = "filter" if f == "Triggers"  else f.lower()[0:-1]
    article = "an" if f in ["Events"] else "a"
    #@<> include filter - accepts only array
    EXPECT_THROWS(lambda: util.check_for_server_upgrade(None, {f"include{f}": sample}), f"Argument #2: Option 'include{f}' is expected to be of type Array, but is String")
    #@<> include filter - accepts only array
    if f not in ["Schemas", "Users", "Triggers"]:
        EXPECT_THROWS(lambda: util.check_for_server_upgrade(None, {f"include{f}": ["sample"]}), f"Argument #2: The {item} to be included must be in the following form: schema.{item}, with optional backtick quotes, wrong value: 'sample'.")
    elif f == "Triggers":
        EXPECT_THROWS(lambda: util.check_for_server_upgrade(None, {f"include{f}": ["sample"]}), f"Argument #2: The trigger to be included must be in the following form: schema.table or schema.table.trigger, with optional backtick quotes, wrong value: 'sample'.")
    #@<> exclude filter - accepts only array
    EXPECT_THROWS(lambda: util.check_for_server_upgrade(None, {f"exclude{f}": sample}), f"Argument #2: Option 'exclude{f}' is expected to be of type Array, but is String")
    #@<> include and exclude contain a common entry
    EXPECT_THROWS(lambda: util.check_for_server_upgrade(None, {"include": ["reservedKeywords"], f"include{f}": [sample], f"exclude{f}": [sample]}), "Conflicting filtering options")
    EXPECT_OUTPUT_CONTAINS(f"ERROR: Both include{f} and exclude{f} options contain {article} {item} {sample}.")
    WIPE_OUTPUT()

shell.disconnect()


#@<> Cross fitering issues, attempting to include objects of non included schema
EXPECT_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"], 
    f"includeSchemas": ['included'],
    f"includeTables": ['excluded.table1'],
    f"excludeTables": ['excluded.table2'],
    f"includeTriggers": ['excluded.table.trigger1'],
    f"excludeTriggers": ['excluded.table.trigger2'],
    f"includeRoutines": ['excluded.routine1'],
    f"excludeRoutines": ['excluded.routine2'],
    f"includeEvents": ['excluded.event1'],
    f"excludeEvents": ['excluded.event2']}), "Conflicting filtering options")

EXPECT_OUTPUT_CONTAINS("""ERROR: The includeTables option contains a table `excluded`.`table1` which refers to a schema which was not included.
ERROR: The excludeTables option contains a table `excluded`.`table2` which refers to a schema which was not included.
ERROR: The includeRoutines option contains a routine `excluded`.`routine1` which refers to a schema which was not included.
ERROR: The excludeRoutines option contains a routine `excluded`.`routine2` which refers to a schema which was not included.
ERROR: The includeTriggers option contains a trigger `excluded`.`table`.`trigger1` which refers to a schema which was not included.
ERROR: The excludeTriggers option contains a trigger `excluded`.`table`.`trigger2` which refers to a schema which was not included.
ERROR: The includeTriggers option contains a trigger `excluded`.`table`.`trigger1` which refers to a table which was not included.
ERROR: The excludeTriggers option contains a trigger `excluded`.`table`.`trigger2` which refers to a table which was not included.
ERROR: The includeEvents option contains an event `excluded`.`event1` which refers to a schema which was not included.
ERROR: The excludeEvents option contains an event `excluded`.`event2` which refers to a schema which was not included.
""")
WIPE_OUTPUT()

#@<> Cross fitering issues, attempting to include objects of non included table
EXPECT_THROWS(lambda: util.check_for_server_upgrade(None, {
    "include": ["reservedKeywords"], 
    f"includeSchemas": ['included'],
    f"includeTables": ['included.table1'],
    f"includeTriggers": ['included.table2.trigger1'],
    f"excludeTriggers": ['included.table2.trigger2']}), "Conflicting filtering options")

EXPECT_OUTPUT_CONTAINS("""ERROR: The includeTriggers option contains a trigger `included`.`table2`.`trigger1` which refers to a table which was not included.
ERROR: The excludeTriggers option contains a trigger `included`.`table2`.`trigger2` which refers to a table which was not included.""")
WIPE_OUTPUT()