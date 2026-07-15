# Copyright (c) 2025, 2026, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

import re
import urllib.request
import xml.etree.ElementTree as ET
import typing
import json
import os

INITIAL_VERSION = [0, 0, 0]
MINIMAL_VERSION = [5, 7, 0]
BASEDIR = os.path.dirname(__file__)
REPO_ROOT = os.path.dirname(os.path.dirname(BASEDIR))

# These variables are documented as strings, but in reality they are sets.
FORCE_TYPES={
    "ssl_cipher": "set",
    "admin_ssl_cipher": "set",
    "tls_ciphersuites": "set",
    "admin_tls_ciphersuites": "set",
}

# Set-typed variables whose elements are separated by characters other than the
# default space/comma. The upgrade checker splits the value on ANY of these
# characters before validating each element against the allowed list. Cipher
# lists are colon-separated but also accept comma/space, so they carry " ,:".
FORCE_SEPARATORS={
    "ssl_cipher": " ,:",
    "admin_ssl_cipher": " ,:",
    "tls_ciphersuites": " ,:",
    "admin_tls_ciphersuites": " ,:",
}

class Sysvar_definition_error(Exception):
    def __init__(self, *args: object) -> None:
        super().__init__(*args)


class Sysvar_out_of_scope(Exception):
    def __init__(self, *args: object) -> None:
        super().__init__(*args)


def node_attribute(node: ET.Element, target: str, default:str = None):
    return default if node is None or target not in node.attrib else node.attrib[target]


def node_version(node: ET.Element, target: str):
    version = node_attribute(node, target)
    return None if version is None else version.split('-')[0]


def node_text(node: ET.Element):
    return None if node is None else node.text


def is_subset(a: typing.List, b: typing.List):
    for element in a:
        if element not in b:
            return False
    return True

def fix_text_value(text : str) -> str:
    return re.sub(' {2,}', ' ', text)

def fix_value(value):
    if type(value) is list:
        result = []
        for val in value:
            result.append(fix_text_value(val))
        return result
    if type(value) is str:
        return fix_text_value(value)
    return value

def fix_dict_values(values : dict) -> dict:
    result = {}
    for key, val in values.items():
        result[key] = fix_value(val)
    return result

SCALES = {
    'bytes':(1024, 'KB'),
    'KB':(1024, 'MB'),
    'MB':(1024, 'GB'),
    'GB':(1024, 'TB'),
    'TB':(1024, 'PB'),
    'PB':(1024, 'EB'),
    'EB':(1024, 'ZB'),
    'ZB':(1024, 'YB'),
    'YB':(None, None),
    'microseconds':(1000, 'milliseconds'),
    'milliseconds':(1000, 'seconds'),
    'seconds':(60, 'minutes'),
    'minutes':(60, 'hours'),
    'hours':(24, 'days'),
    'days':(365, 'years'),
    'years':(None, None),
}

NO_ALLOWED_VALUES_SYSVARS = [
    'secure_file_priv'
]

def get_human_value(value, units:str):
    if units is None:
        return value

    if units not in SCALES:
        return f"{value} ({units.capitalize()})"

    try:
        int_value = int(value)
    except ValueError:
        # No OP: This could be the case of a value that is not a pure
        # integer value but already has the human readable part in it
        return f"{value} ({units.capitalize()})"

    limit, next = SCALES[units]

    while int_value != 0 and limit is not None:
        if (int_value % limit) != 0:
            break

        int_value = int_value // limit
        units = next

        limit, next = SCALES[units]

    if len(units) > 2:
        units = units.capitalize()
        if units.endswith("s") and int_value == 1:
            units = units[:-1]

    if int_value == int(value):
        return f"{value} ({units})"
    else:
        return f"{value} ({int_value} {units})"

        


class Platform_value:
    def __init__(self) -> None:
        self.bitsize = None

        # These map values to architecture: all, 32, 64
        self.default = {}
        self.allowed = {}
        self.vartype = ""

        # NOTE: Forbidden values can only come from sysvar_defaults.json
        # so it doesn't need to be supported in all the functions
        self.forbidden = {}

    def parse_identifier_values(self, s):
        pattern = r"(\w+)\s*=\s*\{([^}]*)\}"
        match = re.match(pattern, s)
        if match:
            identifier = match.group(1)
            values = [value.strip() for value in match.group(2).split("|")]
            return identifier, values
        else:
            return s, None

    def set_value(self, default, allowed, bitsize, units, vartype):
        self.vartype = vartype

        arch_default = node_attribute(default, 'default')
        arch_default = get_human_value(arch_default, units)
        arch_allowed = []
        for item in allowed:
            id, values = self.parse_identifier_values(node_attribute(item, 'value'))
            if values:
                for value in values:
                    arch_allowed.append(f"{id}={value}")
            else:
                arch_allowed.append(id)

        if arch_default is not None:
            self.default[bitsize] = arch_default

        if len(arch_allowed) > 0:
            self.allowed[bitsize] = arch_allowed

    def shell_config(self, force_type=None):
        config = {}

        if force_type:
            config['vartype'] = force_type
        elif self.vartype == "set":
            config['vartype'] = self.vartype

        if len(self.default) > 0:
            config['default'] = fix_dict_values(self.default)

        if len(self.allowed) > 0:
            config['allowed'] = fix_dict_values(self.allowed)

        if len(self.forbidden) > 0:
            config['forbidden'] = fix_dict_values(self.forbidden)

        return config

    def load(self, other):
        if 'default' in other:
            self.default = other['default']

        if 'allowed' in other:
            self.allowed = other['allowed']

        if 'forbidden' in other:
            self.forbidden = other['forbidden']

    def is_allowed_subset(self, other):
        if len(self.allowed) == 0 and len(other.allowed) > 0:
            return False

        for x, v in self.allowed.items():
            if x in other.allowed:
                if not is_subset(v, other.allowed[x]):
                    return False

        return True

    def __eq__(self, value: object) -> bool:
        return value is not None and self.default == value.default and self.allowed == value.allowed and self.vartype == value.vartype

    def has_allowed(self):
        return len(self.allowed) > 0

    def clear_allowed(self):
        self.allowed.clear()

    def purge(self):
        if '32' in self.default and '64' in self.default:
            # If specific values are defined for the platforms delete all if present
            if 'all' in self.default:
                del self.default['all']

            # OTOH if both values are the same, then delete them and use all
            # This is the case when different limits are used in different archs
            # Since we are ignoring the limits, we may end up on this scenario
            if self.default['32'] == self.default['64']:
                self.default = {'all': self.default['64']}

        if '32' in self.default and 'all' in self.default and self.default['32'] == self.default['all']:
            del self.default['32']

        if '64' in self.default and 'all' in self.default and self.default['64'] == self.default['all']:
            del self.default['64']

        # Same cleanup is done on the allowed list
        if '32' in self.allowed and '64' in self.allowed:
            if 'all' in self.allowed:
                del self.allowed['all']

            if self.allowed['32'] == self.allowed['64']:
                self.allowed = {'all': self.allowed['64']}

        if '32' in self.allowed and 'all' in self.allowed and self.allowed['32'] == self.allowed['all']:
            del self.allowed['32']

        if '64' in self.allowed and 'all' in self.allowed and self.allowed['64'] == self.allowed['all']:
            del self.allowed['64']


class Xml_value(object):
    def __init__(self) -> None:
        self.all: Platform_value = None
        self.windows: Platform_value = None
        self.unix: Platform_value = None
        self.linux: Platform_value = None
        self.macos: Platform_value = None

    def load(self, other):
        if 'all' in other:
            if self.all is None:
                self.all = Platform_value()
            self.all.load(other['all'])
        elif 'windows' in other:
            if self.windows is None:
                self.windows = Platform_value()
            self.windows.load(other['windows'])
        elif 'unix' in other:
            if self.unix is None:
                self.unix = Platform_value()
            self.unix.load(other['unix'])
        elif 'linux' in other:
            if self.linux is None:
                self.linux = Platform_value()
            self.linux.load(other['linux'])
        elif 'macos' in other:
            if self.macos is None:
                self.macos = Platform_value()
            self.macos.load(other['macos'])

    def set_value(self, value, allowed, platform, bitsize, units, vartype):
        if platform == 'all':
            if self.all is None:
                self.all = Platform_value()
            self.all.set_value(value, allowed, bitsize, units, vartype)
        elif platform == 'windows':
            if self.windows is None:
                self.windows = Platform_value()
            self.windows.set_value(value, allowed, bitsize, units, vartype)
        elif platform == 'unix':
            if self.unix is None:
                self.unix = Platform_value()
            self.unix.set_value(value, allowed, bitsize, units, vartype)
        elif platform == 'linux':
            if self.linux is None:
                self.linux = Platform_value()
            self.linux.set_value(value, allowed, bitsize, units, vartype)
        elif platform == 'macos':
            if self.macos is None:
                self.macos = Platform_value()
            self.macos.set_value(value, allowed, bitsize, units, vartype)
        elif platform == "other":
            # Sometimes used to define the default value similar to all
            if self.all is None:
                self.all = Platform_value()

            self.all.set_value(value, allowed, bitsize, units, vartype)
        else:
            raise Sysvar_definition_error(
                f"Unexpected platform '{platform}' for value {value}")

    def shell_config(self, force_type=None):
        config = {}
        if not self.all is None:
            config["all"] = self.all.shell_config(force_type)

        if not self.windows is None:
            config["windows"] = self.windows.shell_config(force_type)

        # Based on the code in the server "unix" means NOT WINDOWS
        if not self.unix is None:
            config["unix"] = self.unix.shell_config(force_type)

        if not self.linux is None:
            config["linux"] = self.linux.shell_config(force_type)

        if not self.macos is None:
            config["macos"] = self.macos.shell_config(force_type)

        return config

    def __is_allowed_subset(self, local, other):
        if local is None:
            return other is None
        elif other is None:
            return False
        else:
            return local.is_allowed_subset(other)

    def is_allowed_subset(self, other):
        if other is None:
            return False

        return self.__is_allowed_subset(self.all, other.all) \
            and self.__is_allowed_subset(self.windows, other.windows) \
            and self.__is_allowed_subset(self.unix, other.unix) \
            and self.__is_allowed_subset(self.linux, other.linux) \
            and self.__is_allowed_subset(self.macos, other.macos)

    def __has_allowed(self, attribute):
        return attribute is not None and attribute.has_allowed()

    def has_allowed(self):
        return self.__has_allowed(self.all) \
            or self.__has_allowed(self.windows) \
            or self.__has_allowed(self.linux) \
            or self.__has_allowed(self.macos) \
            or self.__has_allowed(self.unix)

    def clear_allowed(self):
        if self.all is not None:
            self.all.clear_allowed()
        if self.windows is not None:
            self.windows.clear_allowed()
        if self.linux is not None:
            self.linux.clear_allowed()
        if self.macos is not None:
            self.macos.clear_allowed()
        if self.unix is not None:
            self.unix.clear_allowed()

    def purge(self):
        if self.all is not None:
            self.all.purge()
        if self.windows is not None:
            self.windows.purge()
        if self.linux is not None:
            self.linux.purge()
        if self.macos is not None:
            self.macos.purge()
        if self.unix is not None:
            self.unix.purge()

        # Now that values are purged, we need to purge at platform level
        if self.unix is not None:
            # Unix means non windows
            # Delete macos if same as unix
            if self.macos is not None and self.macos == self.unix:
                self.macos = None

            # Delete linux if same as unix
            if self.linux is not None and self.linux == self.unix:
                self.linux = None

        # If the values for linux and macos are the same they get merged into unix
        elif self.macos is not None and self.linux is not None and self.macos == self.linux:
            self.unix = self.linux
            self.linux = None
            self.macos = None

        if self.all is not None:
            # Delete windows if same as all
            if self.windows is not None and self.windows == self.all:
                self.windows = None

            # Delete macos if same as all
            if self.macos is not None and self.macos == self.all:
                self.macos = None

            # Delete linux if same as all
            if self.linux is not None and self.linux == self.all:
                self.linux = None

            # Delete unix if same as all
            if self.unix is not None and self.unix == self.all:
                self.unix = None

        # Merge windows and unix into all if they are the same
        if self.unix is not None and self.windows is not None and self.unix == self.windows:
            self.all = self.unix
            self.windows = None
            self.unix = None

    def __eq__(self, value: object) -> bool:
        return self.all == value.all and self.linux == value.linux and self.macos == value.macos and self.windows == value.windows and self.unix == value.unix


class Xml_values(object):
    def __init__(self, introduced: str, warning_cb) -> None:
        self.introduced: str = introduced
        self.version_values = {}
        self.warning_cb = warning_cb

    def load_xml(self, values: typing.List[ET.Element]):
        introduced_version = INITIAL_VERSION if self.introduced is None else list(
            map(int, self.introduced.split('.')))

        units = None
        for value in values:
            default = value.find('value[@default]')
            allowed = value.findall('choice')

            # Sometimes the units are only defined in one of the values
            if units is None:
                units = node_version(value, 'units')

            if default is not None or len(allowed) > 0:
                # Only default value is needed, if not present, we ignore it
                # if default is not None:
                platform = node_attribute(value, 'platform', 'all')
                bitsize = node_attribute(value, 'bitsize', 'all')
                inversion = node_version(value, 'inversion')
                vartype = node_attribute(value, 'vartype', '')

                if inversion is None:
                    out_version = node_version(value, 'outversion')
                    if out_version is not None:
                        out_version = list(map(int, out_version.split('.')))

                    if out_version is not None and out_version < introduced_version:
                        inversion = INITIAL_VERSION
                    else:
                        inversion = introduced_version
                else:
                    try:
                        inversion = list(map(int, inversion.split('.')))
                    except Exception:
                        value = node_attribute(default, 'default')
                        self.warning_cb(
                            f"Ignoring value '{value}': platform: '{platform}', version: '{inversion}'")
                        continue

                # IGNORE THESE VALUES THEY ARE NOT REFLECTED IN THE DOCS
                if inversion >= [6, 0, 0] and inversion < [8, 0, 0]:
                    continue

                self.set_value(default, allowed, inversion, platform, bitsize, units, vartype)

        # Purge the allowed values if they are present on the variable
        self.__purge_values()
        if self.version_values and next(iter(self.version_values.values())).has_allowed():
            self.__purge_allowed_values()

        self.__purge_no_changes()

    def load(self, other):
        for change in other:
            try:
                version = change.pop('version')
            except NameError:
                version = '0.0.0'

            if version not in self.version_values:
                self.version_values[version] = Xml_value()

            self.version_values[version].load(change)

    def set_value(self, default, allowed, version, platform, bitsize, units, vartype):
        target: Xml_value = None

        str_version = ".".join(map(str, version))
        if version < MINIMAL_VERSION and len(self.version_values) == 1:
            _, target = self.version_values.popitem()
            self.version_values[str_version] = target
        else:
            if str_version not in self.version_values:
                self.version_values[str_version] = Xml_value()

            target = self.version_values[str_version]

        target.set_value(default, allowed, platform, bitsize, units, vartype)

    def __purge_no_changes(self):
        """
        Purges registered value changes that are irrelevant

        Some value entries in the XML are to define different value ranges,
        In cases like that, the default value and allowed values don't change
        at all, this function eliminates those irrelevant value changes entries
        """
        # Purges the values
        initial_values = self.version_values
        self.version_values = {}

        # Gets the all the versions sorted in numeric order
        versions = sorted([list(map(int, version.split('.')))
                          for version in initial_values.keys()])

        initial = False
        last_version_added = ""
        for version in versions:
            str_version = ".".join(map(str, version))
            if initial:
                if initial_values[str_version] != self.version_values[last_version_added]:
                    # if not self.version_values[last_version_added].is_subset(initial_values[str_version]):
                    self.version_values[str_version] = initial_values[str_version]
                    last_version_added = str_version
                else:
                    self.warning_cb(
                        f"Ignoring irrelevat value, version: '{str_version}'")
            else:
                initial = True
                last_version_added = str_version
                self.version_values[str_version] = initial_values[str_version]

    def __purge_values(self):
        # Gets the all the versions sorted in numeric order
        versions = sorted([list(map(int, version.split('.')))
                          for version in self.version_values.keys()])

        for version in versions:
            # Performs value isolated cleanup
            self.version_values[".".join(map(str, version))].purge()

    def __purge_allowed_values(self):
        """
        Purges registered value changes that only add new values to the allowed list.

        If the list of allowed values in a version is equal or a subset of the allowed
        values in future versions (for all the versions), then we don't need to verify
        the allowed values, as in an upgrade operation, they will never represent a
        failure
        """
        # Gets the all the versions sorted in numeric order
        versions = sorted([list(map(int, version.split('.')))
                          for version in self.version_values.keys()])

        last_value: Xml_value = None
        all_subsets = True
        for version in versions:
            str_version = ".".join(map(str, version))
            if last_value is None:
                last_value = self.version_values[str_version]
            else:
                if last_value.is_allowed_subset(self.version_values[str_version]):
                    last_value = self.version_values[str_version]
                else:
                    all_subsets = False
                    break

        if all_subsets:
            self.warning_cb("Removing irrelevant allowed values")
            for _, value in self.version_values.items():
                value.clear_allowed()

    def has_changes(self):
        return len(self.version_values) > 1

    def shell_config(self, force_type=None):
        changes = []

        # Gets the all the versions sorted in numeric order
        versions = sorted([list(map(int, version.split('.')))
                          for version in self.version_values.keys()])

        for version in sorted(versions):
            str_version = ".".join(map(str, version))
            # for key, val in self.changes.items():
            config = {}
            if version != INITIAL_VERSION and str_version != self.introduced:
                config["version"] = str_version

            config.update(self.version_values[str_version].shell_config(force_type))
            changes.append(config)

        return changes

    def clear_allowed(self):
        for _, values in self.version_values.items():
            values.clear_allowed()


PREDEFINED = {}
with open(os.path.join(BASEDIR, 'sysvar_defaults.json'), "r+") as file:
    variables = json.load(file)
    for var in variables:
        PREDEFINED[var['name']] = var


class Xml_sysvar(object):
    def __init__(self, source) -> None:
        self.name = ""
        self.sysvar = False
        self.warnings = []
        self.removed = None
        self.deprecated = None
        self.introduced = None
        self.values = None
        self.replacement = None

        if isinstance(source, dict):
            self.load(source)
        else:
            self.load_xml(source)

    def load_xml(self, node: ET.Element):
        self.name = node_text(node.find('name'))

        # Identify if it is a system variable
        types = node.find('types')
        self.sysvar = types.find('system') is not None

        if self.is_sysvar():
            versions = node.find('versions')
            self.removed = node_version(
                versions.find('removed'), 'version')

            self.introduced = node_version(
                versions.find('introduced'), 'version')

            if not self.removed is None and self.name not in PREDEFINED:
                removed_version = list(map(int, self.removed.split('.')))
                if removed_version < MINIMAL_VERSION:
                    raise Sysvar_out_of_scope(
                        f"The variable '{self.name}' was removed at {self.removed}, ignoring it")
                if self.introduced is not None:
                    introduced_version = list(
                        map(int, self.introduced.split('.')))
                    if introduced_version >= [6, 0, 0] and introduced_version < [8, 0, 0] and removed_version < [8, 0, 0]:
                        raise Sysvar_out_of_scope(
                            f"The variable '{self.name}' was introduced at {self.introduced} and removed at {self.removed}, ignoring it")

            self.deprecated = node_version(
                versions.find('deprecated'), 'version')

            self.values = Xml_values(self.introduced, self.__on_warning_cb)

            self.values.load_xml(node.findall('values'))

    def __on_warning_cb(self, warning):
        self.warnings.append(warning)

    def is_sysvar(self):
        return self.sysvar

    def is_deprecated(self):
        return self.deprecated is not None

    def is_removed(self):
        return self.removed is not None

    def has_replacement(self):
        return self.replacement is not None

    def has_changes(self):
        return self.values is not None and self.values.has_changes()

    def should_be_included(self):
        if self.name in PREDEFINED:
            return (True, "Included in pre-defined list")

        if not self.is_sysvar():
            return (False, "Not a sysvar")

        reasons = []
        if self.has_changes():
            reasons.append('Has Changes')

        if self.is_deprecated():
            reasons.append('Deprecated')

        if self.is_removed():
            reasons.append('Removed')

        if reasons:
            return (True, ", ".join(reasons))
        else:
            return (False, "Boring")

    def load(self, value):
        self.name = value['name']

        if 'introduced' in value:
            self.introduced = value['introduced']

        if 'deprecated' in value:
            self.deprecated = value['deprecated']

        if 'removed' in value:
            self.removed = value['removed']

        if 'replacement' in value:
            self.replacement = value['replacement']

        all_changes = []
        if 'initial' in value:
            initial_version = self.introduced if self.introduced is not None else '0.0.0'
            initial_change = value['initial']
            initial_change['version'] = initial_version
            all_changes.append(initial_change)

        if 'changes' in value:
            all_changes = all_changes + value['changes']

        if len(all_changes) > 0:
            if self.values is None:
                self.values = Xml_values(self.introduced, self.__on_warning_cb)
            self.values.load(all_changes)

    def shell_config(self):
        config = {}
        config["name"] = self.name

        separator = FORCE_SEPARATORS.get(self.name)
        if separator is not None:
            config["separator"] = separator

        if self.introduced:
            config["version"] = self.introduced

        if self.is_deprecated():
            config["deprecation"] = self.deprecated

        if self.is_removed():
            config["removal"] = self.removed

        if self.has_replacement():
            config["replacement"] = self.replacement

        if self.has_changes():
            changes = self.values.shell_config(FORCE_TYPES.get(self.name))
            if 'version' not in changes[0]:
                config["initial"] = changes[0]
                changes = changes[1:]

            config["changes"] = changes

        return config

    def clear_allowed(self):
        if self.values:
            self.values.clear_allowed()


# Download the system variables xml file
sysvar_xml_file = os.path.join(BASEDIR, "mysqld.xml")

sysvar_configuration_file = os.path.join(REPO_ROOT, "res", "upgrade_checker", "sysvars.json")
urllib.request.urlretrieve("https://mydocs.mysql.oraclecorp.com/shell-files/mysqld.xml", sysvar_xml_file)

# Parse the system variables file
tree = ET.parse(sysvar_xml_file)

# Deletes the xml file
os.remove(sysvar_xml_file)
root = tree.getroot()


included_count = 0
excluded_count = 0
sysvars = {}
for mysqldoption in root.iter('mysqloption'):
    try:
        option = Xml_sysvar(mysqldoption)

        if option.name in NO_ALLOWED_VALUES_SYSVARS:
            option.clear_allowed()

        included, reason = option.should_be_included()

        if included:
            included_count += 1
            sysvars[option.name] = option
        else:
            excluded_count += 1

        if len(option.warnings):
            print(f"WARNINGS processing '{option.name}':")
            for warning in option.warnings:
                print(f"- {warning}")

    except Sysvar_out_of_scope as err:
        print(err)
        excluded_count += 1

for name, value in PREDEFINED.items():
    if name in sysvars:
        sysvars[name].load(value)
    else:
        var = Xml_sysvar(value)
        sysvars[name] = var

varlist = []
for name, option in sysvars.items():
    varlist.append(option.shell_config())


with open(sysvar_configuration_file, 'w') as file:
    file.write(json.dumps(varlist, indent=2))

print(f"Total Variables: {included_count + excluded_count}")
print(f"Included: {included_count}")
print(f"Excluded: {excluded_count}")
