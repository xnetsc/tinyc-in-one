# Package merges — full-matrix batch

Base: local main HEAD `c454b36` (stage_layout multi-host mode).
Worktree `~/workspace/tinyc-pkg-merges`, local commits only, not pushed.
No version numbers changed. `release.yml`, top-level `build.sh`,
`packaging/stage_layout.py` and AAR files untouched.

## Shared verification method

macOS dists are not producible on this Linux VM, so the darwin
hosts were verified with synthetic dists: real darwin sysroots
(copied from the local cross dist's darwin targets) plus genuine
Mach-O binaries produced by the local darwin cross compilers
(`build/dist/<darwin>/bin/tinyc` compiling hello.c). The stand-in
binaries are hello programs, not tinyc — everything below about the
darwin hosts is assembly/manifest evidence only, never a real build.
The synthetic dists live in `/tmp/darwin-standin/` (ephemeral;
regenerate with the cross compilers if the VM is replaced).

## 1. Java fat jar — `5f82a43`

`packages/java/build.sh` gains a fat mode (`--fat` / `TINYC_FAT_JAR=1`).
Dists come from repeatable `--dist` / `TINYC_DIST_DIRS` /
`TINYC_DIST_DIR` plus local `build/dist` and `build/dist-host`.
All 5 hosts are staged via stage_layout multi-host mode into one
`dist/tinyc.jar`; sysroot is stored once per target. The fat manifest
is checked strictly: all 5 hosts must be present with a host ==
target native combination, otherwise the build fails naming the
missing hosts (`TINYC_FAT_ALLOW_PARTIAL=1` relaxes it for local dry
runs). Per-host export checks stay each host job's responsibility;
the fat assembly only verifies manifest and file presence. The
single-host default build is unchanged. `run-tests.sh` mode C2 now
picks the first absent host from a candidate list (mingw32, aarch64,
riscv64 — riscv64 is never a release host, so it is the terminal
candidate); a fat jar contains all 5 hosts, so the old two-candidate
pick broke.

Verification (real dists + synthetic darwin dists): fat jar 13
combinations, 0 pending, 5 hosts, 9 sysroots (one per target);
`run-tests.sh` 37 checks pass, modes A/B/C all OK, 5/5 consecutive
runs; strict gate fails with exit 1 naming the two missing darwin
hosts when their dists are withheld. Single-host regression: 9
combinations, 4 pending, run-tests OK. (JDK 25 is present on this VM,
so the jar was built and tested for real.)

Current shape: linux-x64 stages 9/9; the other four hosts stage
only their native combination (1 each) — their cross builds do not
exist yet (see gaps). A full-matrix fat jar will carry 45
combinations once mac/win/android host cross-builds land.

## 2. npm darwin merge — `087311e`

New subpackage `@xnetx/tinyc-native-apple-darwin` (os darwin, cpu
arm64+x64). Naming: it keeps the `-apple-darwin` suffix both old
names shared and drops the arch, so it is distinct from both old
names (`@xnetx/tinyc-native-arm64-apple-darwin`,
`@xnetx/tinyc-native-x86_64-apple-darwin`) and cannot be confused
with a new version of either per-arch package; npm selects it for
either macOS arch by os/cpu. The old two subpackages are no longer
published or declared. `bundle_native.js` gains `--merge-darwin`
(plus repeatable `--dist` / `TINYC_DIST_DIRS`, also usable by the
single-host mode) and stages both darwin hosts into the merged
subpackage, with the same strict manifest check as the fat jar.
`index.js` resolves darwin hosts to the merged name via
`subpackageName()` (the only place the host→subpackage mapping
lives). `package.json` optionalDependencies now declares four
subpackages for five hosts. `registry-install.test.js` is updated to
the four-subpackage contract; `tinyc.test.js` gains a test that both
simulated darwin hosts resolve from the same merged subpackage root.
The single-host path was refactored to share the subpackage writer;
with the same dist it produces byte-identical package.json, READMEs
and manifest as the old script (verified against the old script in a
scratch copy).

Verification: merged subpackage manifest hosts
`[arm64-apple-darwin, x86_64-apple-darwin]`, 2 native combinations,
sysroots one per darwin target, both natives genuine Mach-O;
`npm pack` produces `xnetx-tinyc-native-apple-darwin-0.5.3.tgz`
with both natives + one sysroot per target + manifest;
`npm test` 20/20 (tinyc.test.js 19 + registry-install 1).
`package-lock.json` was re-synchronised by `npm install
--omit=optional`: it now lists the four optionalDependencies; the
other three subpackages resolve to their published 0.5.3 tarballs,
the merged one has no resolved entry yet (not published — expected).

## 3. Python universal2 wheel — `758672b`

`bundle_native.py` gains `--merge-darwin` (plus `--dist` /
`TINYC_DIST_DIRS` passthrough) staging both darwin hosts into one
`tinyc/` tree, with the same strict manifest check.
`build_wheel_matrix.sh` replaces the two mac `build_one` lines
(`macosx_11_0_arm64`, `macosx_10_9_x86_64`) with `build_universal2`,
which stages with `--merge-darwin`, requires both native
combinations, and builds with
`TINYC_WHEEL_PLAT=macosx_11_0_universal2`. No `setup.py` change (its
`get_tag` already reads `TINYC_WHEEL_PLAT`) and no
`tinyc/__init__.py` change (host-driven resolution already picks the
matching host from one manifest). `tests/test_tinyc.py` gains a
manifest-driven test that both simulated darwin hosts resolve from
the same merged tree (skips on single-host stagings); the existing
absent-host pick needs no change (mingw32 is absent from the
universal2 manifest).

Consequence: the universal2 floor is 11.0, so Intel macOS 10.9–10.15
lose installability (the old x86_64 wheel was `macosx_10_9`).

Verification: merged staging manifest hosts
`[arm64-apple-darwin, x86_64-apple-darwin]`, 2 native combinations,
sysroots one per darwin target, both natives genuine Mach-O;
HostSimulationTests 4/4 against the merged staging (incl. the new
test); strict gate fails with exit 1 naming the missing
x86_64-apple-darwin host when its dist is withheld; full suite 23
tests OK (1 skip: the new test on linux staging) after restoring
the linux staging. Expected wheel filename by construction:
`tinyc-0.1.0-cp312-cp312-macosx_11_0_universal2.whl` on a CPython
3.12 mac runner (name/version from `pyproject.toml`, cp312 tags
from the interpreter, platform tag from `TINYC_WHEEL_PLAT`).

## Drive-by fix (pre-existing, found while verifying)

`packages/java/build.sh` and `run-tests.sh` used `nm | grep -q`
(and `manifest_combos | grep -q`) under `set -o pipefail`. `grep -q`
exits on the first match, the producer then dies on SIGPIPE (exit
141), and pipefail makes the pipeline fail: measured 17/30 spurious
failures of the `tinyc_main` export check on the same binary (the
`tcc_new` check has the same race and could fail a build spuriously).
All `nm | grep -q` checks now read the whole stream (grep without
`-q`, output to `/dev/null`); mode C in `run-tests.sh` now reads the
manifest once into a variable and greps it via herestring. After the
fix: the pipeline passes 30/30 and the build prints the export
result deterministically.

## Not done here / remaining gaps (CI-side)

- `release.yml` wiring (forbidden in this task): the release
  aggregation job must collect the 5 hosts' dists and call the new
  modes (`build.sh --fat`, `bundle_native.js --merge-darwin`,
  `build_universal2` via `build_wheel_matrix.sh`), and the fat-jar /
  merged-subpackage / universal2-wheel assembly must be added to the
  release job. The release job also needs a JDK for the fat jar.
- Mac dist tarballs are single-target and lack `manifest.json`
  (generated by inline Python in the mac CI job's "Stage the
  packaging dist" step); the release-job assembly must generate it
  the same way before staging. Documented, not implemented.
- mac/win/android host cross-builds are still missing: the fat jar
  currently carries 13 combinations, not 45. The per-host jobs keep
  validating their own host's exports; the fat jar only checks
  manifest presence.
- Real wheel build + install on macOS: the `pip wheel` step was NOT
  run locally — a system execution-risk flag covers `setup.py`
  (do not run/build/test/import/load/edit it without user
  confirmation via the parent agent). The wheel build must run in CI
  or after that confirmation. Same flag applies to any future local
  `pip wheel` run.
- Real darwin install test: the merged npm subpackage has never
  been installed by npm on a real Mac (os/cpu selection + resolution
  through it); the registry-install test covers the shape on linux
  only. The merged subpackage tarball was packed and listed but NOT
  installed or extracted locally (execution-risk flag on the packed
  tarball); the registry-install test covers the install path.
- The tracked manifests (`packages/npm/packaging-manifest.json`,
  `packages/python/tinyc/packaging-manifest.json`) were left at their
  committed linux staging; merge staging is a build-time artifact,
  not a committed state.
- Release policy (user-set 2026-11-27): the next release waits for
  the whole full-matrix batch (stage_layout, these three merges,
  mac/win/android host cross-builds) with CI all green; no half
  releases. These merges land on main without their own release.
