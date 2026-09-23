# OpenMPT Starting Snapshot

Label: triage
Status: reference-only

This file records where the checked-in OpenMPT source came from and what
historical import work was performed. The source is now a Starting Snapshot for
personal-use development, not an active compatibility target or a byte-for-byte
verification commitment.

## Recorded source identity

The imported source was taken from the following OpenMPT revision:

```text
SVN URL:       https://source.openmpt.org/svn/openmpt/trunk/OpenMPT/
SVN revision:  25644
Git locator:   0eafb124cfd15f94e32302734607e71f2c36c84f
Observed:      2026-08-26
```

The SVN URL/revision and observed Git locator identify provenance. They do not
require the downstream project to preserve this revision's behavior, build
environment, file handling, or future compatibility.

The official mirror commit records the same SVN revision in its
`git-svn-id` metadata:

<https://github.com/OpenMPT/openmpt/commit/0eafb124cfd15f94e32302734607e71f2c36c84f>

## Repository topology

The independent application and its imported upstream history currently live in
one project Git repository. The official OpenMPT GitHub mirror and SVN
repository remain read-only reference sources.

The imported upstream source currently lives at:

```text
openmpt-src/
```

The completed historical import retained reachable upstream history and created
this annotated tag:

```text
upstream/openmpt-svn-r25644
```

These topology choices describe work already done. They are not continuing
release gates or a requirement that future development preserve exact upstream
ancestry.

## Current repository audit

The original snapshot is evidence for the import. The source prefix was
corrected from the recorded Git tree on 2026-08-29. A canonical SVN export
comparison was not performed and is no longer required for the current effort.

| Item | Current value | Interpretation |
| --- | --- | --- |
| Source path | `openmpt-src/` | Current downstream source location; renamed from `openmpt-original_ref/` after the import. |
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
| Exact SVN comparison | Not run | Reference-only missing evidence; it does not block personal-use development. |

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
The prefix contained all 7,539 baseline paths after the correction; no hand-copy
or force-add was used. It was later renamed to `openmpt-src/` because the source
is now the project's development tree, not an untouched reference copy.

Generated build outputs may be created as needed by the local development loop.
No manifest or exact baseline path-set proof is required by the current map.

## Current scope status

- The full-history Git import, annotated tag, and installation of the recorded
  Git tree under the former `openmpt-original_ref/` prefix are complete historical facts.
- The canonical SVN export comparison and deterministic cross-system manifest
  were not completed. They are deliberately abandoned as current gates.
- Local development may change, remove, reorganize, or wrap the Starting
  Snapshot wherever the personal-use vertical slice benefits.
- Tests should protect the workflows and personal project data actually touched
  by downstream changes; they do not have to prove general `r25644`
  compatibility.
- If public distribution, formal upstream intake, or a compatibility promise is
  later desired, it requires a fresh decision map rather than reactivating the
  historical checklist by default.
