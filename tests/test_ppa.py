#!/usr/bin/env python3
"""Offline tests for source-upload validation. Never signs or uploads anything."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import ppa

class UploadTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='cuddle-ppa-test-')
        self.root = Path(self.temp.name)
        self.version = '9.8.7-1~ppa1~resolute1'
        self.orig = self.root / 'cuddle_9.8.7.orig.tar.xz'
        self.overlay = self.root / f'cuddle_{self.version}.debian.tar.xz'
        self.dsc = self.root / f'cuddle_{self.version}.dsc'
        self.changes = self.root / f'cuddle_{self.version}_source.changes'
        self.archive(self.orig, 'cuddle-9.8.7/README.md')
        self.archive(self.overlay, 'debian/changelog')
        self.meta = {'changes': self.changes.name, 'upstream': '9.8.7', 'version': self.version,
                     'series': 'resolute', 'upstream_sha256': ppa.sha256(self.orig),
                     'ppa': ppa.DEFAULT_PPA, 'status': 'unsigned; not uploaded'}
        self.refresh()

    def tearDown(self):
        self.temp.cleanup()

    def archive(self, path, name):
        with tarfile.open(path, 'w:xz') as archive:
            info = tarfile.TarInfo(name); info.size = 4
            archive.addfile(info, io.BytesIO(b'test'))

    def checksums(self, paths):
        return ''.join(f' {ppa.sha256(path)} {path.stat().st_size} {path.name}\n' for path in paths)

    def refresh(self, architecture='source', extras=()):
        self.dsc.write_text(f'Format: 3.0 (quilt)\nSource: cuddle\nVersion: {self.version}\nChecksums-Sha256:\n' + self.checksums([self.orig, self.overlay]))
        self.changes.write_text(f'Format: 1.8\nSource: cuddle\nVersion: {self.version}\nArchitecture: {architecture}\nDistribution: resolute\nChecksums-Sha256:\n' + self.checksums([self.orig, self.overlay, self.dsc, *extras]))
        (self.root / 'manifest.json').write_text(json.dumps(self.meta))

    def test_valid_source_and_signed_control_parsing(self):
        ppa.check(self.root)
        self.changes.write_text('-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA256\n\n' + self.changes.read_text() + '\n-----BEGIN PGP SIGNATURE-----\nnot-a-real-signature\n')
        ppa.check(self.root)  # Structural check only; real signing verifies cryptographically.

    def test_changed_archive_is_rejected(self):
        self.orig.write_bytes(self.orig.read_bytes() + b'changed')
        with self.assertRaises(ValueError): ppa.check(self.root)

    def test_binary_architecture_and_binary_artifacts_are_rejected(self):
        self.refresh(architecture='amd64')
        with self.assertRaises(ValueError): ppa.check(self.root)
        binary = self.root / 'cuddle_9.8.7_amd64.deb'; binary.write_bytes(b'fake')
        self.refresh(extras=[binary])
        with self.assertRaises(ValueError): ppa.check(self.root)

    def test_wrong_series_is_rejected(self):
        self.changes.write_text(self.changes.read_text().replace('Distribution: resolute', 'Distribution: noble'))
        with self.assertRaises(ValueError): ppa.check(self.root)

    def test_unsafe_tar_and_artifact_names_are_rejected(self):
        self.archive(self.orig, '../escape')
        self.meta['upstream_sha256'] = ppa.sha256(self.orig)
        self.refresh()
        with self.assertRaises(ValueError): ppa.check(self.root)
        self.archive(self.orig, 'cuddle-9.8.7/README.md')
        self.meta['upstream_sha256'] = ppa.sha256(self.orig)
        self.refresh()
        self.changes.write_text(self.changes.read_text().replace(self.orig.name, '../escape'))
        with self.assertRaises(ValueError): ppa.check(self.root)

    def test_no_upload_or_keyring_creation_for_unsigned_bundle(self):
        with patch.object(ppa, 'registered_key') as network:
            with self.assertRaises(ValueError): ppa.upload(argparse.Namespace(bundle=self.root, key=ppa.DEFAULT_KEY))
            network.assert_not_called()
        missing = self.root / 'no-keyring'
        with patch.dict('os.environ', {'GNUPGHOME': str(missing)}):
            with self.assertRaises(ValueError): ppa.signing_identity(ppa.DEFAULT_KEY)
        self.assertFalse(missing.exists())

    def test_registered_key_requires_match_and_active_archive(self):
        responses = [ {'gpg_keys_collection_link': 'https://api.launchpad.net/keys'},
                      {'entries': [{'fingerprint': ppa.DEFAULT_KEY}]}, {'enabled': True, 'status': 'Active'} ]
        with patch.object(ppa, 'api', side_effect=responses):
            ppa.registered_key(ppa.DEFAULT_PPA, ppa.DEFAULT_KEY)
        with patch.object(ppa, 'api', side_effect=[{}, {'entries': []}]):
            with self.assertRaises(ValueError): ppa.registered_key(ppa.DEFAULT_PPA, ppa.DEFAULT_KEY)

    def test_explicit_dput_profile(self):
        import configparser
        config = configparser.ConfigParser()
        config.read_string(ppa.dput_configuration('ppa:xtoxicwastx/cuddle'))
        target = config['cuddle-launchpad']
        self.assertEqual(target['fqdn'], 'ppa.launchpad.net')
        self.assertEqual(target['incoming'], '~xtoxicwastx/ubuntu/cuddle/')
        self.assertEqual(target['login'], 'anonymous')
        self.assertEqual(target['allow_unsigned_uploads'], '0')
        for invalid in ['ubuntu', 'ppa:owner/archive\nmethod=local', 'ppa:../archive']:
            with self.assertRaises(ValueError): ppa.dput_configuration(invalid)

    def test_frozen_orig_is_deterministic_and_never_overwritten(self):
        root = self.root / 'project'; root.mkdir()
        first = self.root / 'first'; first.mkdir()
        second = self.root / 'second'; second.mkdir()
        source = root / 'README.md'; source.write_text('same release\n')
        with patch.object(ppa, 'ROOT', root), patch.object(ppa, 'upstream_files', return_value=[source]):
            a = ppa.frozen_orig('9.8.7', 123456789, first)
            b = ppa.frozen_orig('9.8.7', 123456789, second)
            self.assertEqual(ppa.sha256(a), ppa.sha256(b))
            source.write_text('changed release\n')
            with self.assertRaises(ValueError): ppa.frozen_orig('9.8.7', 123456789, second)
            self.assertEqual(ppa.sha256(a), ppa.sha256(root / 'releases/upstream' / a.name))

if __name__ == '__main__':
    unittest.main()
