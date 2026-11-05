# Launchpad source uploads

Target requested by the owner: **Ubuntu 26.04 LTS (resolute)**.
PPA: `ppa:xtoxicwastx/cuddle`.
Release: **1.4.0**, upload version **1.4.0-1~ppa1~resolute1**.
The root Debian changelog stays on `unstable`; only the isolated PPA overlay uses `resolute`.

## Prepare and review (no signing or uploading)

```bash
bash ~/cuddle/prepare-launchpad.sh
bash ~/cuddle/tools/ppa-upload.sh check
cd ~/cuddle/releases/ppa/latest
sha256sum -c SHA256SUMS
```

The helper builds a source-only `.changes`, `.dsc`, `.orig.tar.xz`, `.debian.tar.xz` and `.buildinfo`, validates their SHA-256/size references, extracts the source again, checks generated room tables, and builds native Debian binaries from that round-trip source. This local build runs on Debian Crostini; the actual Ubuntu build happens on Launchpad.

Only selected project source, documentation and build files enter the upstream archive. Backups, release artifacts, local build trees, agent handoffs, inventory/status reports and generated review patches are excluded. The `.orig.tar.xz` is deterministic and cached under `releases/upstream/`. A different archive cannot silently replace an existing upstream version. After freezing a release, further upstream changes need a new version. A packaging-only retry can use `--revision 2` with the same frozen upstream source.

Original 1.3.0 artifacts and previous source snapshots remain preserved. ABI soversion stays 0 and plugins remain disabled.

## Set up signing from the normal Linux Terminal

```bash
sudo apt install devscripts dput gnupg
 gpg --list-secret-keys --keyid-format LONG
```

The historical key fingerprint is `E7275732ECC72867AE7383960F4CA660B591C0F3`. No keyring/private key is currently available in the agent-accessible Crostini account. Use the machine already holding that key, or set up a new local OpenPGP key and register its public fingerprint with your Launchpad account. Never put a private key or its passphrase in chat or shell arguments.

For a new key, use GPG's interactive flow:

```bash
gpg --full-generate-key
gpg --list-secret-keys --with-fingerprint
```

Publish only the selected **public** key using its full fingerprint:

```bash
gpg --keyserver hkps://keyserver.ubuntu.com --send-keys YOUR_FULL_FINGERPRINT
```

Import that fingerprint at https://launchpad.net/~xtoxicwastx/+editpgpkeys and complete the encrypted-email confirmation. The helper checks this registration again immediately before an upload. An unverified or inaccessible registration stops the upload.

Official instructions: [Import an OpenPGP key](https://ubuntu.com/docs/launchpad/user/how-to/import-openpgp-key/).

## Sign (does not upload)

Keep all files from the source bundle together, including when copying the bundle to the machine with the private key.

```bash
bash ~/cuddle/tools/ppa-upload.sh sign --key YOUR_FULL_FINGERPRINT
```

For a copied bundle, add `--bundle /path/to/copied/bundle`. `debsign` asks through normal GPG pinentry; the helper never accepts a passphrase. It verifies the signed `.changes`, `.dsc` and `.buildinfo` against the chosen fingerprint and refreshes SHA256SUMS.

## Explicitly upload

After reviewing the source bundle, signing, and confirming your key registration:

```bash
bash ~/cuddle/tools/ppa-upload.sh dput --key YOUR_FULL_FINGERPRINT
```

This action alone transfers files. It first checks the selected PPA, registered signing fingerprint, source-only architecture, Ubuntu series, file checksums and signatures. It does not install packages or change system settings.

A successful `dput` transfer is not acceptance or a successful build. Check Launchpad's acceptance email and build results at https://launchpad.net/~xtoxicwastx/+archive/ubuntu/cuddle/+packages. The helper records `transferred; verify acceptance/build on Launchpad` rather than claiming publication.

Official packaging references: [Build a source package](https://ubuntu.com/docs/launchpad/user/reference/packaging/ppas/building-a-source-package/) and [upload to a PPA](https://ubuntu.com/docs/launchpad/user/how-to/packaging/ppa-package-upload/).

Git hosting on Launchpad is separate from publishing a PPA; this workflow handles package publication only.
