# Upstream Baseline

Label: triage
Status: pending-verification

This file records the provenance boundary for the OpenMPT-derived project. It is
not a substitute for importing the upstream history or for the byte-for-byte
verification described below.

## Canonical baseline

The project baseline is the following fixed OpenMPT source revision:

```text
SVN URL:       https://source.openmpt.org/svn/openmpt/trunk/OpenMPT/
SVN revision:  25644
Git locator:   0eafb124cfd15f94e32302734607e71f2c36c84f
Observed:      2026-08-26
```

The SVN URL and revision are authoritative. The Git SHA is an immutable
locator in the OpenMPT GitHub mirror as observed on the date above; it is not a
replacement for the canonical SVN identity.

The official mirror commit records the same SVN revision in its
`git-svn-id` metadata:

<https://github.com/OpenMPT/openmpt/commit/0eafb124cfd15f94e32302734607e71f2c36c84f>

## Repository topology

The independent application and its imported upstream history belong to one
project Git repository. This is a project-release decision, not a requirement
that the upstream project use the same repository. The official OpenMPT GitHub
mirror and SVN repository remain read-only upstream sources, configured as an
upstream remote when the import is performed.

The imported upstream source uses the existing provenance prefix:

```text
openmpt-original_ref/
```

The final import must preserve the complete reachable upstream history rather
than adding only a source snapshot. The permanent annotated tag is:

```text
upstream/openmpt-svn-r25644
```

The project must not use a floating submodule, a nested `.git` directory, or a
separate source repository as the release's authoritative source. A temporary
clone may be used to acquire and inspect upstream history, but the release
repository must retain the imported ancestry and all downstream changes.

## Current repository audit

The original snapshot is evidence for the import. The source prefix was
corrected from the pinned Git tree on 2026-08-29; the remaining verification
gate is the canonical SVN export comparison.

| Item | Current value | Interpretation |
| --- | --- | --- |
| Source path | `openmpt-original_ref/` | One downstream source prefix already exists. |
| Source-adding commit | `b42d5c34ddc825a06bd070ee004f9c5d42df285b` | A project commit, not upstream ancestry. |
| Historical source tree object | `91900a5706884d9b972c55f7f637000a65c3425d` | Provisional snapshot tree before correction. |
| Historical snapshot files | 7,373 | Provisional project snapshot before correction. |
| Current source prefix files | 7,539 | Exact path count from `upstream/openmpt-svn-r25644` after correction. |
| Current source-prefix tree | `713efc62513515497d701c64ca2728175f8f5acb` | Exact match with the official baseline tree. |
| Official baseline tree | `713efc62513515497d701c64ca2728175f8f5acb` | Tree ID of official Git commit `0eafb...`. |
| Official baseline paths | 7,539 | Tracked paths in the checked-out `r25644` commit. |
| Historical missing project paths | 166 | Officially tracked paths omitted from the provisional snapshot; restored by the correction. |
| Historical shared blob differences | 1,524 | Raw Git blob IDs differed before correction; many were line-ending normalization. |
| Historical shared mode differences | 712 | Executable bits were not preserved by the provisional snapshot. |
| Upstream history | Imported into Git object database | 17,432 commits and 185,790 reachable object IDs checked; 0 missing objects. |
| Project tag | Created | `upstream/openmpt-svn-r25644` points to the official r25644 commit and tree. |
| Upstream remote | Configured read-only | `openmpt-upstream` fetches from GitHub; its push URL is `DISABLED`. |
| Exact SVN comparison | Not run | `svn` and `svnversion` are unavailable in the current environment. |

The snapshot reports OpenMPT version `1.33.00.25`, and
`soundlib/SampleFormats.cpp` contains the three type-explicit `std::clamp`
changes shown by the pinned GitHub commit. These spot checks are consistent
with the pinned revision but do not establish that every file and byte matches
the baseline.

The raw comparison of the provisional snapshot found two representative
shared-path differences: `include/lame/include/lame.def` differed by
line-ending representation, while `mptrack/View_smp.cpp` contained an
additional sample-path restoration change absent from `r25644`. The latter's
provenance was unknown and was not silently carried into the corrected
upstream prefix. The corrected prefix now has the official Git tree ID above.

## Previous snapshot omissions and ignored files

Before correction, the provisional snapshot omitted 166 paths that are tracked
by the pinned upstream commit. The local snapshot import was affected by the
upstream `.gitignore`, which excludes the VS2017 XP project directory and
nested dependency build files even though those paths are tracked upstream.
On 2026-08-29, the old leftovers inside `openmpt-original_ref/` were removed
with a target-scoped cleanup, and the prefix was repopulated with
`git read-tree --prefix=openmpt-original_ref/` from the immutable baseline tag.
The current prefix contains all 7,539 baseline paths; no hand-copy or
force-add was used.

Any truly generated or VCS metadata excluded from a clean export must be
identified separately in the verification manifest. Issue 13 may regenerate
build outputs when its toolchain and build policy are implemented, but it must
not redefine which tracked source files belong to the r25644 baseline.

## Required bootstrap verification

Issue 12 is complete only after all of the following are true:

1. [x] A full-history checkout of the official mirror is obtained; the project
   repository now retains 17,432 commits and 185,790 reachable object IDs with
   no missing objects, and the pinned Git locator carries SVN revision
   `r25644`.
2. [x] The exact upstream Git tree is installed under the agreed source prefix
   without squashing authorship or commit messages. The full history is
   retained in this repository through the immutable baseline tag, and the
   current `openmpt-original_ref/` tree matches its official baseline tree.
3. [x] A permanent annotated tag named `upstream/openmpt-svn-r25644` identifies
   the imported official baseline commit and tree.
4. A clean `svn export -r 25644` from the canonical URL is compared with the
   imported tree after documented VCS and generated-file exclusions.
5. [ ] The comparison records the source path set, deterministic byte-manifest
   hash, imported tree ID, Git locator, SVN URL/revision, export timestamp, and
   result in this file. The Git-side manifest is already recorded by the
   7,539-path tree and deterministic SHA-256
   `ED8A7F57A29588A16D2F9F2CD424016309B8179A01BF6D1169ADFDBEC6D28106`; the
   SVN export timestamp and comparison result remain pending.
6. Downstream project commits remain distinguishable from upstream commits;
   build and release verification continue in issues 13 and 15.

Until the SVN export comparison and its manifest/result are complete, this
document and issue 12 must remain open and must not claim a fully verified
upstream baseline. The full-history Git import and exact Git-tree installation
are complete.
