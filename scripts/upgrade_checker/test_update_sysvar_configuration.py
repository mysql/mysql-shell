# Copyright (c) 2026, Oracle and/or its affiliates.
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
# Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

import copy
import dataclasses
import importlib.util
import os
import sys
import unittest
import xml.etree.ElementTree as ET
from unittest import mock


def load_generator_module():
    module_name = "update_sysvar_configuration_import_safety"
    module_path = os.path.join(
        os.path.dirname(__file__), "update_sysvar_configuration.py")
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    previous_module = sys.modules.get(module_name)
    sys.modules[module_name] = module

    try:
        spec.loader.exec_module(module)
    finally:
        if previous_module is None:
            del sys.modules[module_name]
        else:
            sys.modules[module_name] = previous_module

    return module


def mysqloption(name, facts, values=()):
    option = ET.Element('mysqloption')
    ET.SubElement(option, 'name').text = name
    ET.SubElement(ET.SubElement(option, 'types'), 'system')
    versions = ET.SubElement(option, 'versions')
    for kind, version in facts:
        ET.SubElement(versions, kind, version=version)

    for attributes, default, choices in values:
        value = ET.SubElement(option, 'values', attributes)
        if default is not None:
            ET.SubElement(value, 'value', default=default)

        for choice in choices:
            ET.SubElement(value, 'choice', value=choice)

    return option


class ImportSafetyTest(unittest.TestCase):
    def test_import_has_no_download_parse_delete_or_write_side_effects(self):
        with mock.patch("urllib.request.urlretrieve") as retrieve, \
                mock.patch("xml.etree.ElementTree.parse") as parse, \
                mock.patch("os.remove") as remove, \
                mock.patch("builtins.open", mock.mock_open(read_data="[]")) as open_file:
            load_generator_module()

        retrieve.assert_not_called()
        parse.assert_not_called()
        remove.assert_not_called()
        open_file.assert_not_called()


class ReleasePolicyTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.generator = load_generator_module()

    def test_release_classification(self):
        cases = (
            ('5.2.4', False, False, False),
            ('5.6.7', False, False, False),
            ('5.7.0', False, True, True),
            ('6.0.14', True, True, False),
            ('8.0.3', False, True, True),
            ('8.4.11', False, True, True),
            ('9.7.2', False, True, True),
            ('26.7.0', False, True, True),
        )
        policy = self.generator.RELEASE_POLICY
        for version, excluded, at_baseline, relevant in cases:
            with self.subTest(version=version):
                self.assertEqual(policy.is_excluded(version), excluded)
                self.assertEqual(
                    policy.is_at_or_after_baseline(version), at_baseline)
                self.assertEqual(policy.is_relevant_manual(version), relevant)

    def test_series_are_distinct_and_sort_numerically(self):
        versions = ['26.7', '8.4', '5.7', '9.7', '8.0']
        expected = [(5, 7), (8, 0), (8, 4), (9, 7), (26, 7)]
        for ordered in (versions, list(reversed(versions))):
            self.assertEqual(sorted({self.generator.version_series(version)
                                     for version in ordered}), expected)

    def test_parser_pads_comparisons_and_strips_suffixes(self):
        cases = (
            ('8', (8, 0, 0)),
            ('8.4', (8, 4, 0)),
            ('8.4.11', (8, 4, 11)),
            ('8.4.11-commercial', (8, 4, 11)),
        )
        for version, expected in cases:
            with self.subTest(version=version):
                self.assertEqual(self.generator.parse_version(version), expected)

    def test_malformed_versions_report_original_input(self):
        for version in ('', 'a', '8.a', '8.4.a', '8.4.1.2', '8..1',
                        '8.', '-8', '8.+4', ' 8', '8.4.bad-commercial'):
            with self.subTest(version=version):
                with self.assertRaises(
                        self.generator.Sysvar_definition_error) as caught:
                    self.generator.parse_version(version)

                self.assertIn(f"'{version}'", str(caught.exception))

    def test_policy_is_configurable_and_copies_exclusions(self):
        excluded = {(8, 0)}
        policy = self.generator.Release_policy((8, 4, 1), excluded)
        excluded.clear()
        self.assertTrue(policy.is_excluded('8.0.3'))
        self.assertFalse(policy.is_excluded('6.0.14'))
        self.assertTrue(policy.is_relevant_manual('8.4'))
        self.assertFalse(policy.is_at_or_after_baseline('8.4'))


class XmlVersionHistoryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.generator = load_generator_module()

    def history(self, facts):
        versions = ET.Element('versions')
        for kind, version in facts:
            ET.SubElement(versions, kind, version=version)

        return self.generator.Xml_version_history.from_xml(versions)

    def resolve(self, facts, name='example', predefined=False):
        return self.history(facts).resolve(
            self.generator.RELEASE_POLICY, name, predefined)

    def test_complete_history_is_normalized_deduplicated_and_order_independent(self):
        facts = [
            ('manual', '26.7'), ('deprecated', '26.7.0'),
            ('introduced', '8.0.27'), ('manual', '8.0'),
            ('deprecated', '8.4.11-commercial'), ('manual', '8.4'),
            ('deprecated', '9.7.2'), ('manual', '9.7'),
            ('deprecated', '8.4.11'), ('removed', '27.0.1'),
            ('removed', '27.0.1-commercial'),
        ]
        expected = self.generator.Xml_version_history(
            ('8.0', '8.4', '9.7', '26.7'), ('8.0.27',),
            ('8.4.11', '9.7.2', '26.7.0'), ('27.0.1',))
        for ordered in (facts, list(reversed(facts)), facts[::2] + facts[1::2]):
            self.assertEqual(self.history(ordered), expected)
            self.assertEqual(self.resolve(ordered),
                             self.generator.Resolved_version_history(
                                 '8.0.27', expected.deprecations, expected.removals))

    def test_normalized_source_spelling_is_preserved(self):
        result = self.resolve([
            ('manual', '8'), ('introduced', '8-commercial'),
            ('deprecated', '8.4-commercial'), ('removed', '9.7.2-commercial')])
        self.assertEqual(result.introduced, '8')
        self.assertEqual(result.deprecations, ('8.4',))
        self.assertEqual(result.removals, ('9.7.2',))

    def test_equal_numeric_versions_have_deterministic_source_spelling_order(self):
        facts = [('manual', '8.0'), ('deprecated', '8.4.0'),
                 ('deprecated', '8.4'), ('deprecated', '8.4-commercial')]
        for ordered in (facts, list(reversed(facts))):
            self.assertEqual(self.history(ordered).deprecations, ('8.4', '8.4.0'))

    def test_histories_are_immutable(self):
        history = self.history([('manual', '5.7')])
        resolved = history.resolve(self.generator.RELEASE_POLICY, 'example')
        with self.assertRaises(dataclasses.FrozenInstanceError):
            history.manuals = ()

        with self.assertRaises(dataclasses.FrozenInstanceError):
            resolved.introduced = '5.7.1'

    def test_histories_without_relevant_manuals_are_out_of_scope(self):
        cases = (
            ('optimizer_use_mrr', [('manual', '5.2'), ('introduced', '5.2.4'),
                                   ('manual', '6.0'), ('introduced', '6.0.3'),
                                   ('removed', '6.0.14')]),
            ('sql_log_update', [('manual', '5.5'), ('deprecated', '5.0.0'),
                                ('removed', '5.5.3'), ('manual', '6.0'),
                                ('removed', '6.0.14')]),
            ('table_lock_wait_timeout', [('manual', '5.0'), ('introduced', '5.0.10'),
                                         ('manual', '5.5'), ('removed', '5.5.3'),
                                         ('manual', '6.0'), ('removed', '6.0.14')]),
            ('no_manual', [('introduced', '8.0.1')]),
        )
        for name, facts in cases:
            with self.subTest(name=name):
                with self.assertRaises(self.generator.Sysvar_out_of_scope) as caught:
                    self.resolve(facts, name)

                self.assertIn(name, str(caught.exception))

    def test_predefined_variable_can_have_no_relevant_manual(self):
        result = self.resolve([('manual', '6.0'), ('introduced', '6.0.3'),
                               ('removed', '6.0.14')], predefined=True)
        self.assertEqual(result, self.generator.Resolved_version_history(None, (), ()))

    def test_thread_pool_size_ignores_excluded_events(self):
        result = self.resolve([
            ('removed', '6.0.14'), ('manual', '8.0'), ('introduced', '8.0.11'),
            ('manual', '6.0'), ('introduced', '6.0.4'), ('deprecated', '6.0.5'),
            ('manual', '5.7'), ('introduced', '5.7.9'),
            ('manual', '5.5'), ('introduced', '5.5.16')], 'thread_pool_size')
        self.assertEqual(result, self.generator.Resolved_version_history('5.5.16', (), ()))

    def test_multi_range_count_carries_prebaseline_deprecation(self):
        result = self.resolve([
            ('manual', '5.0'), ('introduced', '5.0.3'), ('removed', '5.2.4'),
            ('manual', '5.7'), ('deprecated', '5.6.7'), ('removed', '8.0.3'),
            ('manual', '6.0'), ('removed', '6.0.3')], 'multi_range_count')
        self.assertEqual(result, self.generator.Resolved_version_history(
            '5.0.3', ('5.6.7',), ('8.0.3',)))

    def test_prebaseline_introduction_requires_relevant_manual_evidence(self):
        facts = [('introduced', '5.2.4'), ('manual', '5.2')]
        with self.assertRaises(self.generator.Sysvar_out_of_scope):
            self.resolve(facts)

        self.assertEqual(self.resolve(facts + [('manual', '5.7')]).introduced, '5.2.4')

    def test_only_excluded_introductions_do_not_create_version_marker(self):
        result = self.resolve([('introduced', '6.0.3'), ('manual', '5.7'),
                               ('manual', '8.0'), ('removed', '6.0.14')])
        self.assertIsNone(result.introduced)

    def test_later_branch_introduction_cannot_replace_unknown_baseline_existence(self):
        facts = [('manual', '5.7'), ('manual', '8.0'), ('introduced', '8.0.11')]
        for introductions in ([], [('introduced', '6.0.3')]):
            with self.subTest(introductions=introductions):
                for ordered in (facts + introductions,
                                list(reversed(facts + introductions))):
                    self.assertIsNone(self.resolve(ordered).introduced)

    def test_supported_deprecations_are_preserved(self):
        result = self.resolve([
            ('manual', '26.7'), ('deprecated', '26.7.0'),
            ('manual', '8.4'), ('deprecated', '8.4.11'),
            ('manual', '9.7'), ('deprecated', '9.7.2'),
            ('manual', '8.0'), ('introduced', '8.0.27')],
            'group_replication_communication_stack')
        self.assertEqual(result.deprecations, ('8.4.11', '9.7.2', '26.7.0'))

    def test_removal_without_matching_manual_is_retained(self):
        result = self.resolve([('manual', '5.7'), ('removed', '8.0.3')])
        self.assertEqual(result.removals, ('8.0.3',))

    def test_parallel_branch_removals_are_preserved(self):
        facts = [
            ('manual', '4.1'), ('manual', '5.0'), ('manual', '5.1'),
            ('manual', '5.4'), ('manual', '5.5'), ('manual', '5.6'),
            ('manual', '5.7'), ('deprecated', '5.7.38'), ('removed', '5.7.39'),
            ('manual', '8.0'), ('deprecated', '8.0.29'), ('removed', '8.0.30'),
            ('manual', '6.0'),
        ]
        for ordered in (facts, list(reversed(facts))):
            self.assertEqual(self.resolve(ordered, 'myisam_repair_threads'),
                             self.generator.Resolved_version_history(
                                 None, ('5.7.38', '8.0.29'), ('5.7.39', '8.0.30')))

    def test_conflicting_versions_report_variable_and_both_versions(self):
        cases = (
            ([('manual', '8.0'), ('removed', '8.0.3'), ('introduced', '8.0.4')],
             ('8.0.3', '8.0.4')),
        )
        for facts, conflicts in cases:
            with self.subTest(facts=facts):
                with self.assertRaises(self.generator.Sysvar_definition_error) as caught:
                    self.resolve(facts, 'conflicting_variable')

                message = str(caught.exception)
                for expected in ('conflicting_variable',) + conflicts:
                    self.assertIn(expected, message)

    def test_later_introduction_restores_presence_for_subsequent_manuals(self):
        result = self.resolve([
            ('introduced', '5.6.1'), ('removed', '5.7.2'),
            ('manual', '8.4'), ('introduced', '8.4.1'), ('manual', '9.7')])
        self.assertEqual(result.introduced, '8.4.1')
        self.assertEqual(result.removals, ())

    def test_manual_covers_presence_within_its_series(self):
        result = self.resolve([('manual', '8.0'), ('introduced', '8.0.2'),
                               ('removed', '8.0.3')])
        self.assertEqual(result.introduced, '8.0.2')
        self.assertEqual(result.removals, ('8.0.3',))

    def test_invalid_lifecycle_and_manual_versions_fail_parsing(self):
        for kind in ('manual', 'introduced', 'deprecated', 'removed'):
            with self.subTest(kind=kind):
                with self.assertRaises(self.generator.Sysvar_definition_error) as caught:
                    self.history([(kind, '8.bad-commercial')])

                self.assertIn('8.bad-commercial', str(caught.exception))


class XmlSysvarIntegrationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.generator = load_generator_module()

    def generate(self, option, predefined=None, policy=None):
        root = ET.Element('mysqloptions')
        root.append(option)
        return self.generator.generate_configuration(
            root, {} if predefined is None else predefined,
            self.generator.RELEASE_POLICY if policy is None else policy)

    def test_optimizer_switch_hypergraph_values_follow_release_boundaries(self):
        values = [
            ('5.7.33', ['index_merge={on|off}', 'legacy=off']),
            ('8.0.21', ['index_merge={on|off}']),
            ('8.0.22', ['index_merge={on|off}']),
            ('8.2.0', ['hash_set_operations={on|off}', 'index_merge={on|off}']),
            ('9.0.0', ['hash_set_operations={on|off}',
                       'hypergraph_optimizer=off', 'index_merge={on|off}']),
            ('9.3.0', ['hash_set_operations={on|off}',
                       'hypergraph_optimizer=off', 'index_merge={on|off}']),
            ('9.3.1', ['hash_set_operations={on|off}',
                       'hypergraph_optimizer=on', 'index_merge={on|off}']),
            ('9.7.0', ['hash_set_operations={on|off}',
                       'hypergraph_optimizer={on|off}', 'index_merge={on|off}']),
            ('26.7.0', ['hash_set_operations={on|off}',
                        'hypergraph_optimizer={on|off}', 'index_merge={on|off}',
                        'new_flag={on|off}']),
        ]
        base = ['index_merge=on', 'index_merge=off']
        hash_set = ['hash_set_operations=on', 'hash_set_operations=off']
        hypergraph = ['hypergraph_optimizer=on', 'hypergraph_optimizer=off']
        expected = {
            '5.7.33': base + ['legacy=off'],
            '8.0.21': base,
            '8.0.22': ['hypergraph_optimizer=off'] + base,
            '8.2.0': hash_set + ['hypergraph_optimizer=off'] + base,
            '9.0.0': hash_set + hypergraph + base,
            '26.7.0': hash_set + hypergraph + base + ['new_flag=on', 'new_flag=off'],
        }

        for ordered in (values, list(reversed(values))):
            option = mysqloption('optimizer_switch', [
                ('introduced', '5.1.34'), ('manual', '5.7'), ('manual', '8.0'),
                ('manual', '8.4'), ('manual', '9.7'), ('manual', '26.7')], [
                    ({'inversion': version, 'vartype': 'set'}, None, choices)
                    for version, choices in ordered])
            original = ET.tostring(option)

            for run in range(2):
                with self.subTest(order=ordered[0][0], run=run):
                    records = self.generate(option).records
                    self.assertEqual(len(records), 1)
                    self.assertEqual({
                        change['version']: change['all']['allowed']['all']
                        for change in records[0]['changes']}, expected)
                    self.assertEqual(ET.tostring(option), original)

    def test_hypergraph_adjustment_does_not_change_other_variables(self):
        result = self.generate(mysqloption('other_switch', [
            ('manual', '8.0'), ('manual', '8.4'), ('manual', '9.7')], [
                ({'inversion': '8.0.22', 'vartype': 'set'}, None, ['old']),
                ({'inversion': '9.0.0', 'vartype': 'set'}, None, ['new'])]))
        self.assertEqual(result.records, [{
            'name': 'other_switch', 'changes': [
                {'version': '8.0.22', 'all': {
                    'vartype': 'set', 'allowed': {'all': ['old']}}},
                {'version': '9.0.0', 'all': {
                    'vartype': 'set', 'allowed': {'all': ['new']}}}]}])

    def test_complete_lifecycle_output_is_independent_of_xml_child_order(self):
        fixtures = (
            ('optimizer_use_mrr', [
                ('manual', '5.2'), ('introduced', '5.2.4'),
                ('manual', '6.0'), ('introduced', '6.0.3'),
                ('removed', '6.0.14')], [], []),
            ('thread_pool_size', [
                ('manual', '5.5'), ('introduced', '5.5.16'),
                ('manual', '5.6'), ('introduced', '5.6.10'),
                ('manual', '5.7'), ('introduced', '5.7.9'),
                ('manual', '8.0'), ('introduced', '8.0.11'),
                ('manual', '9.5'), ('manual', '6.0'),
                ('introduced', '6.0.4'), ('removed', '6.0.14')], [
                    ({'outversion': '5.7.99'}, '16', []),
                    ({'inversion': '6.0.4'}, '20', []),
                    ({'inversion': '8.0.11'}, '16', []),
                    ({'inversion': '8.0.19'}, '16', []),
                    ({'inversion': '9.5.0'}, 'MIN(VCPU/2, 16)', [])], [{
                        'name': 'thread_pool_size', 'version': '5.5.16',
                        'initial': {'all': {'default': {'all': '16'}}},
                        'changes': [{'version': '9.5.0', 'all': {
                            'default': {'all': 'MIN(VCPU/2, 16)'}}}]}]),
            ('multi_range_count', [
                ('manual', '5.0'), ('introduced', '5.0.3'),
                ('manual', '5.1'), ('manual', '5.2'), ('removed', '5.2.4'),
                ('manual', '5.4'), ('manual', '5.5'), ('manual', '5.6'),
                ('deprecated', '5.6.7'), ('manual', '5.7'), ('manual', '8.0'),
                ('removed', '8.0.3'), ('manual', '6.0'), ('removed', '6.0.3')],
                [({}, '256', [])], [{
                    'name': 'multi_range_count', 'version': '5.0.3',
                    'deprecation': '5.6.7', 'removal': '8.0.3'}]),
            ('sql_log_update', [
                ('manual', '4.1'), ('manual', '5.0'), ('deprecated', '5.0.0'),
                ('manual', '5.1'), ('manual', '5.4'), ('manual', '5.5'),
                ('removed', '5.5.3'), ('manual', '6.0'), ('removed', '6.0.14')],
                [], []),
            ('table_lock_wait_timeout', [
                ('manual', '5.0'), ('introduced', '5.0.10'),
                ('manual', '5.1'), ('manual', '5.4'), ('manual', '5.5'),
                ('removed', '5.5.3'), ('manual', '6.0'), ('removed', '6.0.14')],
                [({}, '50', [])], []),
            ('group_replication_communication_stack', [
                ('manual', '8.0'), ('introduced', '8.0.27'),
                ('manual', '8.4'), ('deprecated', '8.4.11'),
                ('manual', '9.7'), ('deprecated', '9.7.2'),
                ('manual', '26.7'), ('deprecated', '26.7.0')], [
                    ({'inversion': '8.0.27', 'outversion': '9.7.99'},
                     'XCOM', ['XCOM', 'MYSQL']),
                    ({'inversion': '26.7.0'}, 'MYSQL', ['XCOM', 'MYSQL'])], [{
                        'name': 'group_replication_communication_stack',
                        'version': '8.0.27',
                        'deprecation': ['8.4.11', '9.7.2', '26.7.0'],
                        'initial': {'all': {'default': {'all': 'XCOM'}}},
                        'changes': [{'version': '26.7.0', 'all': {
                            'default': {'all': 'MYSQL'}}}]}]),
            ('myisam_repair_threads', [
                ('manual', '5.7'), ('deprecated', '5.7.38'),
                ('removed', '5.7.39'), ('manual', '8.0'),
                ('deprecated', '8.0.29'), ('removed', '8.0.30')], [], [{
                    'name': 'myisam_repair_threads',
                    'deprecation': ['5.7.38', '8.0.29'],
                    'removal': ['5.7.39', '8.0.30']}]),
        )
        for name, facts, values, expected in fixtures:
            for reverse in (False, True):
                with self.subTest(name=name, reverse=reverse):
                    ordered = list(reversed(facts)) if reverse else facts
                    result = self.generate(mysqloption(name, ordered, values))
                    self.assertEqual(result.records, expected)
                    self.assertEqual(result.included_count, len(expected))
                    self.assertEqual(result.excluded_count, 1 - len(expected))

    def test_reintroduced_variable_serializes_only_current_history_removals(self):
        facts = [('introduced', '5.6.1'), ('removed', '5.7.2'),
                 ('manual', '8.4'), ('introduced', '8.4.1'), ('manual', '9.7')]
        values = [({'inversion': '8.4.1'}, 'restored', []),
                  ({'inversion': '9.7.1'}, 'current', [])]
        for removals in ([], ['9.7.2'], ['9.7.2', '26.7.0']):
            history = facts + [('removed', version) for version in removals]
            expected = {
                'name': 'example', 'version': '8.4.1',
                'initial': {'all': {'default': {'all': 'restored'}}},
                'changes': [{'version': '9.7.1',
                             'all': {'default': {'all': 'current'}}}]}
            if removals:
                expected['removal'] = removals[0] if len(removals) == 1 else removals

            for order, ordered in enumerate((
                    history, list(reversed(history)), history[::2] + history[1::2])):
                with self.subTest(removals=removals, order=order):
                    result = self.generate(mysqloption('example', ordered, values))
                    self.assertEqual(result.records, [expected])
                    self.assertEqual((result.included_count, result.excluded_count), (1, 0))

    def test_excluded_value_cannot_hide_identical_supported_transition(self):
        option = self.generator.Xml_sysvar(mysqloption('example', [
            ('manual', '5.7'), ('manual', '8.0')], [
                ({}, 'old', []),
                ({'inversion': '6.0.4-commercial'}, 'new', []),
                ({'inversion': '8.0.11-commercial'}, 'new', [])]))
        self.assertEqual(option.shell_config(), {
            'name': 'example', 'initial': {'all': {'default': {'all': 'old'}}},
            'changes': [{'version': '8.0.11', 'all': {'default': {'all': 'new'}}}]})

    def test_out_of_scope_values_cannot_resurrect_variable(self):
        result = self.generate(mysqloption('example', [('manual', '6.0')], [
            ({}, 'old', []), ({'inversion': '8.0.11'}, 'new', [])]))
        self.assertEqual(result.records, [])
        self.assertEqual((result.included_count, result.excluded_count), (0, 1))

    def test_injected_policy_controls_history_and_value_exclusions(self):
        policy = self.generator.Release_policy((5, 7, 0), {(6, 0), (8, 0)})
        result = self.generate(mysqloption('example', [
            ('manual', '5.7'), ('manual', '8.4'),
            ('deprecated', '8.0.11'), ('deprecated', '8.4.1')], [
                ({}, 'old', []), ({'inversion': '8.0.11'}, 'excluded', []),
                ({'inversion': '8.4.1'}, 'new', [])]), policy=policy)
        self.assertEqual(result.records, [{
            'name': 'example', 'deprecation': '8.4.1',
            'initial': {'all': {'default': {'all': 'old'}}},
            'changes': [{'version': '8.4.1', 'all': {'default': {'all': 'new'}}}]}])

    def test_value_order_preserves_prebaseline_state_and_supported_transitions(self):
        facts = [('manual', '5.7'), ('manual', '8.0'),
                 ('manual', '8.4'), ('manual', '9.0')]
        values = [
            ({'inversion': '5.6.1'}, 'oldest', []),
            ({'inversion': '5.6.2'}, 'old', []),
            ({'inversion': '5.7.1'}, 'newer', []),
            ({'inversion': '6.0.4'}, 'excluded', []),
            ({'inversion': '8.0.11'}, 'initial', []),
            ({'inversion': '8.4.1'}, 'new', []),
            ({'inversion': '9.0.1'}, 'newest', []),
        ]
        cases = (
            (self.generator.RELEASE_POLICY, [
                {'version': '5.6.2', 'all': {'default': {'all': 'old'}}},
                {'version': '5.7.1', 'all': {'default': {'all': 'newer'}}},
                {'version': '8.0.11', 'all': {'default': {'all': 'initial'}}},
                {'version': '8.4.1', 'all': {'default': {'all': 'new'}}},
                {'version': '9.0.1', 'all': {'default': {'all': 'newest'}}}]),
            (self.generator.Release_policy((8, 4, 0), {(6, 0)}), [
                {'version': '8.0.11', 'all': {'default': {'all': 'initial'}}},
                {'version': '8.4.1', 'all': {'default': {'all': 'new'}}},
                {'version': '9.0.1', 'all': {'default': {'all': 'newest'}}}]),
        )
        orders = (values, list(reversed(values)), values[::2] + values[1::2])
        for policy, changes in cases:
            for order, ordered in enumerate(orders):
                with self.subTest(baseline=policy.minimum_version, order=order):
                    result = self.generate(
                        mysqloption('example', facts, ordered), policy=policy)
                    self.assertEqual(result.records, [
                        {'name': 'example', 'changes': changes}])
                    self.assertEqual((result.included_count, result.excluded_count), (1, 0))

    def test_value_order_preserves_architecture_merging_and_purging(self):
        values = [
            ({'inversion': '5.6.1'}, '1', []),
            ({'inversion': '8.0.11', 'bitsize': '32'}, '16', ['16', '32']),
            ({'inversion': '8.0.11', 'bitsize': '64'}, '16', ['16', '32']),
            ({'inversion': '8.4.1', 'bitsize': '32'}, '32', ['16', '32']),
            ({'inversion': '8.4.1', 'bitsize': '64'}, '32', ['16', '32']),
        ]
        policy = self.generator.Release_policy((8, 4, 0), {(6, 0)})
        for order, ordered in enumerate((
                values, list(reversed(values)), values[::2] + values[1::2])):
            with self.subTest(order=order):
                result = self.generate(mysqloption(
                    'example', [('manual', '8.4')], ordered), policy=policy)
                self.assertEqual(result.records, [{
                    'name': 'example', 'changes': [
                        {'version': '8.0.11', 'all': {'default': {'all': '16'}}},
                        {'version': '8.4.1', 'all': {'default': {'all': '32'}}}]}])
                self.assertEqual((result.included_count, result.excluded_count), (1, 0))

    def test_prebaseline_architecture_fallbacks_follow_version_order(self):
        specific_values = [
            ({'inversion': '5.6.1', 'outversion': '5.6.1', 'bitsize': '32'}, 'A', []),
            ({'inversion': '5.6.1', 'outversion': '5.6.1', 'bitsize': '64'}, 'B', []),
        ]
        common_value = ({'inversion': '5.6.2'}, 'C', [])
        cases = (
            (specific_values + [common_value], '5.6.2', {'all': 'C'}),
            (specific_values + [common_value,
                                ({'inversion': '5.6.2', 'bitsize': '64'}, 'D', [])],
             '5.6.2', {'all': 'C', '64': 'D'}),
            (specific_values + [common_value,
                                ({'inversion': '5.6.3', 'bitsize': '64'}, 'D', [])],
             '5.6.3', {'all': 'C', '64': 'D'}),
        )
        for values, baseline_version, expected in cases:
            values = values + [({'inversion': '8.0.11'}, 'E', [])]
            for order, ordered in enumerate((
                    values, list(reversed(values)), values[::2] + values[1::2])):
                with self.subTest(baseline=expected, order=order):
                    result = self.generate(mysqloption(
                        'example', [('manual', '5.7'), ('manual', '8.0')], ordered))
                    self.assertEqual(result.records, [{
                        'name': 'example', 'changes': [
                            {'version': baseline_version,
                             'all': {'default': expected}},
                            {'version': '8.0.11',
                             'all': {'default': {'all': 'E'}}}]}])

    def test_prebaseline_allowed_architecture_fallbacks_follow_version_order(self):
        values = [
            ({'inversion': '5.6.1', 'bitsize': '32'}, None, ['A']),
            ({'inversion': '5.6.1', 'bitsize': '64'}, None, ['B']),
            ({'inversion': '5.6.2'}, None, ['C', 'retired']),
            ({'inversion': '5.6.2', 'bitsize': '64'}, None, ['D', 'retired']),
            ({'inversion': '5.6.3', 'bitsize': '64'}, None, ['F', 'retired']),
            ({'inversion': '8.0.11'}, None, ['E']),
        ]
        for order, ordered in enumerate((
                values, list(reversed(values)), values[::2] + values[1::2])):
            with self.subTest(order=order):
                result = self.generate(mysqloption(
                    'example', [('manual', '5.7'), ('manual', '8.0')], ordered))
                self.assertEqual(result.records, [{
                    'name': 'example', 'changes': [
                        {'version': '5.6.3', 'all': {'allowed': {
                            'all': ['C', 'retired'], '64': ['F', 'retired']}}},
                        {'version': '8.0.11',
                         'all': {'allowed': {'all': ['E']}}}]}])

    def test_unknown_introduction_keeps_unversioned_initial_values(self):
        result = self.generate(mysqloption('example', [
            ('manual', '5.7'), ('manual', '8.0'), ('introduced', '8.0.11')], [
                ({}, 'old', []), ({'inversion': '8.0.11'}, 'new', [])]))
        self.assertEqual(result.records, [{
            'name': 'example', 'initial': {'all': {'default': {'all': 'old'}}},
            'changes': [{'version': '8.0.11', 'all': {'default': {'all': 'new'}}}]}])

    def test_short_introduction_still_identifies_initial_values(self):
        option = self.generator.Xml_sysvar(mysqloption('example', [
            ('manual', '8.0'), ('introduced', '8')], [
                ({}, 'old', []), ({'inversion': '8.0.11'}, 'new', [])]))
        self.assertEqual(option.shell_config(), {
            'name': 'example', 'version': '8',
            'initial': {'all': {'default': {'all': 'old'}}},
            'changes': [{'version': '8.0.11', 'all': {'default': {'all': 'new'}}}]})

    def test_predefined_override_keeps_out_of_scope_variable(self):
        for deprecated, removed in (('8.4.1', '9.7.1'),
                                    (['8.4.1', '9.7.1'], ['9.7.2', '26.7.0'])):
            with self.subTest(deprecated=deprecated, removed=removed):
                predefined = {'example': {
                    'name': 'example', 'introduced': '8.0.1',
                    'deprecated': deprecated, 'removed': removed,
                    'initial': {'all': {'default': {'all': 'old'}}},
                    'changes': [{'version': '8.0.11', 'all': {
                        'default': {'all': 'new'}}}]}}
                result = self.generate(mysqloption('example', [
                    ('manual', '6.0'), ('introduced', '6.0.3'),
                    ('removed', '6.0.14')]), predefined)
                self.assertEqual(result.records, [{
                    'name': 'example', 'version': '8.0.1',
                    'deprecation': deprecated, 'removal': removed,
                    'changes': [
                        {'version': '8.0.1', 'all': {'default': {'all': 'old'}}},
                        {'version': '8.0.11', 'all': {'default': {'all': 'new'}}}]}])
                self.assertEqual((result.included_count, result.excluded_count), (1, 0))

    def test_predefined_only_record_uses_scalar_and_list_lifecycle(self):
        predefined = {'example': {
            'name': 'example', 'introduced': '8.0.1',
            'deprecated': ['8.4.1', '9.7.1'], 'removed': '26.7.0'}}
        result = self.generator.generate_configuration(
            ET.Element('mysqloptions'), predefined, self.generator.RELEASE_POLICY)
        self.assertEqual(result.records, [{
            'name': 'example', 'version': '8.0.1',
            'deprecation': ['8.4.1', '9.7.1'], 'removal': '26.7.0'}])
        self.assertEqual((result.included_count, result.excluded_count), (0, 0))

    def test_predefined_generation_is_repeatable_and_preserves_input(self):
        predefined = {
            'example': {
                'name': 'example', 'introduced': '8.0.1',
                'deprecated': ['8.4.1', '9.7.1'],
                'removed': ['26.7.0', '27.0.0'],
                'initial': {'all': {'default': {'all': 'old'},
                                    'allowed': {'all': ['old', 'new']}}},
                'changes': [{'version': '8.0.11', 'all': {
                    'default': {'all': 'new'}, 'allowed': {'all': ['new']},
                    'forbidden': {'all': ['legacy']}}}]},
            'predefined_only': {
                'name': 'predefined_only',
                'initial': {'windows': {'default': {'32': 'old'}}},
                'changes': [{'version': '8.0.11',
                             'windows': {'default': {'32': 'new'}}}]},
        }
        snapshot = copy.deepcopy(predefined)
        option = mysqloption('example', [('manual', '8.0'), ('introduced', '8.0.1')])
        first = self.generate(option, predefined)
        self.assertEqual(predefined, snapshot)
        second = self.generate(option, predefined)
        self.assertEqual(first.records, second.records)
        self.assertEqual(predefined, snapshot)
        self.assertEqual([record['name'] for record in first.records],
                         ['example', 'predefined_only'])

    def test_predefined_loading_detaches_nested_values_and_lifecycle_lists(self):
        predefined = {
            'name': 'example', 'deprecated': ['8.4.1', '9.7.1'],
            'removed': ['26.7.0', '27.0.0'],
            'initial': {'all': {'default': {'all': 'old'},
                                'allowed': {'all': ['old', 'new']},
                                'forbidden': {'all': ['legacy']}}},
            'changes': [{'version': '8.0.11', 'all': {
                'default': {'all': 'new'}, 'allowed': {'all': ['new']},
                'forbidden': {'all': ['legacy']}}}],
        }
        snapshot = copy.deepcopy(predefined)
        option = self.generator.Xml_sysvar(predefined)
        option.deprecated.append('26.7.0')
        option.removed.append('28.0.0')
        for value in option.values.version_values.values():
            value.all.default['all'] = 'changed'
            value.all.allowed['all'].append('changed')
            value.all.forbidden['all'].append('changed')

        option.clear_allowed()
        self.assertEqual(predefined, snapshot)

    def test_predefined_change_without_version_uses_initial_values(self):
        predefined = {'example': {
            'name': 'example', 'changes': [
                {'all': {'default': {'all': 'old'}}},
                {'version': '8.0.11', 'all': {'default': {'all': 'new'}}}]}}
        snapshot = copy.deepcopy(predefined)
        result = self.generator.generate_configuration(
            ET.Element('mysqloptions'), predefined, self.generator.RELEASE_POLICY)
        self.assertEqual(result.records, [{
            'name': 'example', 'initial': {'all': {'default': {'all': 'old'}}},
            'changes': [{'version': '8.0.11', 'all': {'default': {'all': 'new'}}}]}])
        self.assertEqual(predefined, snapshot)
