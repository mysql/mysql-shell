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
import dataclasses
import copy
import json
import os

INITIAL_VERSION = [0, 0, 0]
BASEDIR = os.path.dirname(__file__)
REPO_ROOT = os.path.dirname(os.path.dirname(BASEDIR))
XML_URL = "https://mydocs.mysql.oraclecorp.com/shell-files/mysqld.xml"
SYSVAR_XML_FILE = os.path.join(BASEDIR, "mysqld.xml")
SYSVAR_CONFIGURATION_FILE = os.path.join(
    REPO_ROOT, "res", "upgrade_checker", "sysvars.json")
SYSVAR_DEFAULTS_FILE = os.path.join(BASEDIR, "sysvar_defaults.json")

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


Version_tuple = typing.Tuple[int, int, int]
Version_series = typing.Tuple[int, int]


def parse_version(version: str) -> Version_tuple:
    normalized = version.split('-', 1)[0]
    components = normalized.split('.')
    if not 1 <= len(components) <= 3 or any(
            not part.isascii() or not part.isdecimal() for part in components):
        raise Sysvar_definition_error(f"Invalid version: '{version}'")

    try:
        parts = tuple(int(part) for part in components)
    except ValueError as error:
        raise Sysvar_definition_error(
            f"Invalid version: '{version}'") from error

    return typing.cast(Version_tuple, (parts + (0, 0, 0))[:3])


def version_series(version: str) -> Version_series:
    parsed = parse_version(version)
    return parsed[0], parsed[1]


def is_ndb_version(version: typing.Optional[str]) -> bool:
    return version is not None and '-ndb-' in version


class Release_policy:
    def __init__(self, minimum_version: Version_tuple,
                 excluded_series: typing.AbstractSet[Version_series]) -> None:
        self.minimum_version = minimum_version
        self.minimum_series = minimum_version[:2]
        self.excluded_series = frozenset(excluded_series)

    def is_excluded(self, version: str) -> bool:
        return version_series(version) in self.excluded_series

    def is_at_or_after_baseline(self, version: str) -> bool:
        return parse_version(version) >= self.minimum_version

    def is_relevant_manual(self, version: str) -> bool:
        series = version_series(version)
        return series >= self.minimum_series and series not in self.excluded_series


RELEASE_POLICY = Release_policy((5, 7, 0), {(6, 0)})


@dataclasses.dataclass(frozen=True)
class Resolved_version_history:
    introduced: typing.Optional[str]
    deprecations: typing.Tuple[str, ...]
    removals: typing.Tuple[str, ...]


@dataclasses.dataclass(frozen=True)
class Xml_version_history:
    manuals: typing.Tuple[str, ...]
    introductions: typing.Tuple[str, ...]
    deprecations: typing.Tuple[str, ...]
    removals: typing.Tuple[str, ...]

    @classmethod
    def from_xml(cls, versions: ET.Element) -> 'Xml_version_history':
        return cls(*(tuple(node_versions(versions, kind)) for kind in (
            'manual', 'introduced', 'deprecated', 'removed')))

    def resolve(self, policy: Release_policy, name: str,
                predefined: bool = False) -> Resolved_version_history:
        manuals = tuple(version for version in self.manuals
                        if policy.is_relevant_manual(version))
        if not manuals and not predefined:
            raise Sysvar_out_of_scope(
                f"The variable '{name}' has no relevant manual, ignoring it")

        introductions = tuple(version for version in self.introductions
                              if not policy.is_excluded(version))
        deprecations = tuple(version for version in self.deprecations
                            if not policy.is_excluded(version))
        all_removals = tuple(version for version in self.removals
                             if not policy.is_excluded(version))
        removals = tuple(version for version in all_removals
                         if policy.is_at_or_after_baseline(version))

        for removed in all_removals:
            for introduced in introductions:
                if version_series(introduced) == version_series(removed) and \
                        parse_version(introduced) > parse_version(removed):
                    raise Sysvar_definition_error(
                        f"The variable '{name}' is introduced at {introduced} "
                        f"after its removal at {removed} in the same series")

        # Manuals may continue documenting a variable after its removal and do
        # not require a later introduction in that documentation's series.

        # An introduction must explain the earliest relevant documented
        # presence. A later branch's introduction cannot replace an unknown
        # introduction in an earlier manual series.
        removal_series = {version_series(removed) for removed in removals}
        first_manual_series = version_series(manuals[0]) if manuals else None
        candidates = tuple(
            introduced for introduced in introductions
            if first_manual_series is not None and
            version_series(introduced) <= first_manual_series and (
                first_manual_series in removal_series or not any(
                    parse_version(introduced) <= parse_version(removed) and
                    version_series(removed) < first_manual_series
                    for removed in removals)))
        introduced = candidates[0] if candidates else None

        # Removals before a selected reintroduction belong to the old history.
        # Keeping them would make the consumer skip even reintroduced sources.
        if introduced is not None:
            removals = tuple(removed for removed in removals
                             if parse_version(removed) >= parse_version(introduced))

        return Resolved_version_history(introduced, deprecations, removals)


def node_attribute(node: ET.Element, target: str, default:str = None):
    return default if node is None or target not in node.attrib else node.attrib[target]


def node_version(node: ET.Element, target: str):
    version = node_attribute(node, target)
    return None if version is None else version.split('-')[0]


def node_versions(node: ET.Element, target: str):
    versions = set()
    for child in node.findall(target):
        version = node_attribute(child, 'version')
        if version is not None:
            parse_version(version)
            versions.add(version.split('-', 1)[0])

    return sorted(versions, key=lambda version: (parse_version(version), version))


def normalize_versions(versions):
    if isinstance(versions, list):
        return list(versions)

    return [versions]


def serialize_versions(versions):
    if len(versions) == 1:
        return versions[0]

    return versions


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


def fix_optimizer_switch_values(
        values: typing.List[ET.Element]) -> typing.List[ET.Element]:
    """Fill gaps in the documented hypergraph_optimizer choices (BUG#39875290)."""
    values = copy.deepcopy(values)

    for value in values:
        try:
            version = parse_version(node_version(value, 'inversion') or '0.0.0')
        except Sysvar_definition_error:
            # Leave malformed versions for the normal value parser to report.
            continue

        if version < (8, 0, 22):
            continue

        # The switch exists since 8.0.22; HeatWave supports enabling it since 9.0.
        allowed = 'off' if version < (9, 0, 0) else '{on|off}'
        prefix = 'hypergraph_optimizer='

        for choice in value.findall('choice'):
            if node_attribute(choice, 'value', '').startswith(prefix):
                value.remove(choice)

        # Keep the choice order consistent so identical snapshots are removed.
        position = next((index for index, child in enumerate(value)
                         if child.tag == 'choice' and
                         node_attribute(child, 'value', '') > prefix),
                        len(value))
        value.insert(position, ET.Element(
            'choice', value=f'hypergraph_optimizer={allowed}'))

    return values


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
            self.default = copy.deepcopy(other['default'])

        if 'allowed' in other:
            self.allowed = copy.deepcopy(other['allowed'])

        if 'forbidden' in other:
            self.forbidden = copy.deepcopy(other['forbidden'])

    def merge(self, other: 'Platform_value') -> None:
        self.vartype = other.vartype

        for attribute in ('default', 'allowed'):
            current = getattr(self, attribute)
            incoming = getattr(other, attribute)
            if 'all' in incoming:
                current.clear()

            current.update(incoming)

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

    def merge(self, other: 'Xml_value') -> None:
        for platform in ('all', 'windows', 'unix', 'linux', 'macos'):
            incoming = getattr(other, platform)
            if incoming is None:
                continue

            current = getattr(self, platform)
            if current is None:
                setattr(self, platform, incoming)
            else:
                current.merge(incoming)

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
    def __init__(self, introduced: typing.Optional[str], warning_cb,
                 release_policy: Release_policy = RELEASE_POLICY) -> None:
        self.introduced = introduced
        self.version_values = {}
        self.warning_cb = warning_cb
        self.release_policy = release_policy

    def load_xml(self, values: typing.List[ET.Element]):
        introduced_version = INITIAL_VERSION if self.introduced is None else list(
            parse_version(self.introduced))

        units = None
        versioned_values = []
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
                        out_version = list(parse_version(out_version))

                    if out_version is not None and out_version < introduced_version:
                        inversion = INITIAL_VERSION
                    else:
                        inversion = introduced_version
                else:
                    try:
                        inversion = list(parse_version(inversion))
                    except Sysvar_definition_error:
                        value = node_attribute(default, 'default')
                        self.warning_cb(
                            f"Ignoring value '{value}': platform: '{platform}', version: '{inversion}'")
                        continue

                if self.release_policy.is_excluded('.'.join(map(str, inversion))):
                    continue

                versioned_values.append(
                    (inversion, default, allowed, platform, bitsize, vartype))

        # Assemble each version before folding pre-baseline state, so a newer
        # common value replaces older architecture-specific values without
        # losing specific overrides from the same version.
        for inversion, default, allowed, platform, bitsize, vartype in versioned_values:
            self.set_value(default, allowed, inversion, platform, bitsize, units, vartype)

        self.__fold_prebaseline_values()

        # Purge the allowed values if they are present on the variable
        self.__purge_values()
        if self.version_values and next(iter(self.version_values.values())).has_allowed():
            self.__purge_allowed_values()

        self.__purge_no_changes()

    def load(self, other):
        for change in other:
            version = change.get('version', '0.0.0')

            if version not in self.version_values:
                self.version_values[version] = Xml_value()

            self.version_values[version].load(change)

    def set_value(self, default, allowed, version, platform, bitsize, units, vartype):
        str_version = ".".join(map(str, version))
        if str_version not in self.version_values:
            self.version_values[str_version] = Xml_value()

        self.version_values[str_version].set_value(
            default, allowed, platform, bitsize, units, vartype)

    def __fold_prebaseline_values(self) -> None:
        resolved_values = {}
        for version in sorted(self.version_values, key=parse_version):
            value = self.version_values[version]
            if resolved_values and \
                    parse_version(version) < self.release_policy.minimum_version:
                _, previous = resolved_values.popitem()
                previous.merge(value)
                value = previous

            resolved_values[version] = value

        self.version_values = resolved_values

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
        introduced_version = None if self.introduced is None else list(
            parse_version(self.introduced))

        # Gets the all the versions sorted in numeric order
        versions = sorted([list(map(int, version.split('.')))
                          for version in self.version_values.keys()])

        for version in sorted(versions):
            str_version = ".".join(map(str, version))
            # for key, val in self.changes.items():
            config = {}
            if version != INITIAL_VERSION and version != introduced_version:
                config["version"] = str_version

            config.update(self.version_values[str_version].shell_config(force_type))
            changes.append(config)

        return changes

    def clear_allowed(self):
        for _, values in self.version_values.items():
            values.clear_allowed()


class Xml_sysvar(object):
    def __init__(self, source, release_policy: Release_policy = RELEASE_POLICY,
                 predefined: typing.Optional[typing.Mapping[str, dict]] = None) -> None:
        self.name = ""
        self.sysvar = False
        self.warnings = []
        self.removed = []
        self.deprecated = []
        self.introduced = None
        self.values = None
        self.replacement = None
        self.release_policy = release_policy
        self.predefined = {} if predefined is None else predefined

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
            history = Xml_version_history.from_xml(node.find('versions'))
            resolved = history.resolve(
                self.release_policy, self.name, self.name in self.predefined)
            self.introduced = resolved.introduced
            self.deprecated = list(resolved.deprecations)
            self.removed = list(resolved.removals)

            self.values = Xml_values(
                self.introduced, self.__on_warning_cb, self.release_policy)

            values = node.findall('values')
            # Some options contain both Server and NDB value histories. NDB
            # versions must not override the corresponding Server defaults.
            if node_attribute(node, 'section') != 'ndb':
                values = [
                    value for value in values
                    if not any(is_ndb_version(
                        node_attribute(value, attribute))
                        for attribute in ('inversion', 'outversion'))
                ]

            if self.name == 'optimizer_switch':
                values = fix_optimizer_switch_values(values)

            self.values.load_xml(values)

    def __on_warning_cb(self, warning):
        self.warnings.append(warning)

    def is_sysvar(self):
        return self.sysvar

    def is_deprecated(self):
        return len(self.deprecated) > 0

    def is_removed(self):
        return len(self.removed) > 0

    def has_replacement(self):
        return self.replacement is not None

    def has_changes(self):
        return self.values is not None and self.values.has_changes()

    def should_be_included(self):
        if self.name in self.predefined:
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
            self.deprecated = normalize_versions(value['deprecated'])

        if 'removed' in value:
            self.removed = normalize_versions(value['removed'])

        if 'replacement' in value:
            self.replacement = value['replacement']

        all_changes = []
        if 'initial' in value:
            initial_version = self.introduced if self.introduced is not None else '0.0.0'
            initial_change = dict(value['initial'])
            initial_change['version'] = initial_version
            all_changes.append(initial_change)

        if 'changes' in value:
            all_changes = all_changes + value['changes']

        if len(all_changes) > 0:
            if self.values is None:
                self.values = Xml_values(
                    self.introduced, self.__on_warning_cb, self.release_policy)
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
            config["deprecation"] = serialize_versions(self.deprecated)

        if self.is_removed():
            config["removal"] = serialize_versions(self.removed)

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


@dataclasses.dataclass(frozen=True)
class Generation_result:
    records: typing.List[dict]
    included_count: int
    excluded_count: int


def load_predefined(path: str) -> typing.Dict[str, dict]:
    """Read sysvar_defaults.json and index records by variable name."""
    with open(path, "r") as file:
        variables = json.load(file)

    return {variable['name']: variable for variable in variables}


def generate_configuration(
        root: ET.Element, predefined: typing.Mapping[str, dict],
        release_policy: "Release_policy") -> Generation_result:
    """Resolve all mysqloption nodes and return JSON-ready records."""
    included_count = 0
    excluded_count = 0
    sysvars = {}
    for mysqldoption in root.iter('mysqloption'):
        try:
            option = Xml_sysvar(mysqldoption, release_policy, predefined)

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

    for name, value in predefined.items():
        if name in sysvars:
            sysvars[name].load(value)
        else:
            var = Xml_sysvar(value, release_policy, predefined)
            sysvars[name] = var

    records = []
    for name, option in sysvars.items():
        records.append(option.shell_config())

    return Generation_result(records, included_count, excluded_count)


def update_configuration(xml_url: str, xml_path: str, output_path: str,
                         defaults_path: str,
                         release_policy: "Release_policy") -> None:
    """Download source XML and write the generated configuration."""
    predefined = load_predefined(defaults_path)
    urllib.request.urlretrieve(xml_url, xml_path)
    tree = ET.parse(xml_path)
    os.remove(xml_path)

    result = generate_configuration(
        tree.getroot(), predefined, release_policy)

    with open(output_path, 'w') as file:
        file.write(json.dumps(result.records, indent=2) + '\n')

    print(f"Total Variables: {result.included_count + result.excluded_count}")
    print(f"Included: {result.included_count}")
    print(f"Excluded: {result.excluded_count}")


def main() -> None:
    update_configuration(XML_URL, SYSVAR_XML_FILE,
                         SYSVAR_CONFIGURATION_FILE,
                         SYSVAR_DEFAULTS_FILE, RELEASE_POLICY)


if __name__ == "__main__":
    main()
