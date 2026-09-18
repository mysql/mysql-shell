Copyright (c) 2025, 2026, Oracle and/or its affiliates.

## MySQL Shell sysvar.json generation tool

### Description:

This tool creates a <PROJECT-ROOT>/res/upgrade_checker/sysvar.json
file, that is used as an sys var database for "sysvar" check in
Upgrade Checker utility.

### Requirements:

- Python at least 3.x
- Access to https://mydocs.mysql.oraclecorp.com/shell-files/mysqld.xml
- A valid sysvar_defaults.json file

### Usage:

From within its folder (<PROJECT-ROOT>/internal/upgrade_checker):

python update_sysvar_configuration.py

### Remarks:

File sysvar_defaults.json contains sysvar definitions that will
override/add specific entries to resulting sysvar.json file.
This can be used to add undocumented variables, or remove
specific values from processing by "sysvar" check. Downside of
sysvar_defaults.json is that it will replace whole parts of
definition, that now needs to be maintained manually.

File sysvar_defaults.json is required when adding sys var
replacement by another variable information, because mysqld.xml
does not contain a dedicated field for that.

The generator reads the complete set of `manual`, `introduced`, `deprecated`,
and `removed` nodes as independent facts. Their child order and adjacency do
not pair lifecycle events. Manuals establish the documentation scope; they do
not imply that a variable persists at runtime after removal.

One supported-release policy filters both lifecycle events and versioned
values, excluding the discontinued 6.0 development line. The generated
`deprecation` and `removal` fields are strings for one lifecycle version and
sorted arrays for multiple versions.

Do not edit the generated `res/upgrade_checker/sysvars.json` file directly.
Update the generator and regenerate the file instead.

Inside update_sysvar_configuration.py is a variable called
"NO_ALLOWED_VALUES_SYSVARS", that contains a list of sys vars,
whose allowed values list should be wiped, while maintaining
the rest of the definition. This is a multiple platform/version
solution, that would be troublesome to maintain using the
sysvar_defaults.json file.
