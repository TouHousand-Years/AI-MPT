# Bootstrap the pinned upstream source tree

Parent: ../map.md
Type: task
Status: resolved
Blocked by: 01, 09

## Question

How should the complete OpenMPT history at canonical SVN `r25644` be imported and verified, permanently tagged, documented in `UPSTREAM.md`, and turned into the monorepo base for the independent application's first downstream code?

## Comments

### Scope correction — 2026-08-29

The imported source now serves as a Starting Snapshot, not a compatibility
baseline. The existing Git tree is sufficient to begin local development. The
canonical SVN export comparison described below was not run and is no longer a
closure gate; reproducing OpenMPT's exact development environment and proving
byte-for-byte `r25644` fidelity are outside the current destination.

### Repository topology decision — 2026-08-29

The independent application and the imported OpenMPT history will share one
project Git repository. This is needed by the project's Release Unit and
provenance contract so that one release tag identifies the application,
upstream-derived source, downstream adaptations, and supporting documentation
together. It does not mean that the official OpenMPT repository is merged into
the upstream project or that upstream must share this repository.

The official OpenMPT GitHub mirror and canonical SVN repository remain read-only
upstream sources. A temporary clone may acquire history, but the project will
not use a floating submodule, nested `.git`, or a separate source repository as
the release's authoritative source. The imported source keeps the existing
`openmpt-original_ref/` provenance prefix, and the permanent baseline tag is
`upstream/openmpt-svn-r25644`.

### Current audit — 2026-08-29

The repository already contains a 7,373-file source snapshot under
`openmpt-original_ref/`, added by downstream commit
`b42d5c34ddc825a06bd070ee004f9c5d42df285b`. It has no imported upstream
ancestry, upstream remote, baseline tag, or `UPSTREAM.md` before this change.
The snapshot is therefore provisional evidence, not completion of this issue.

The official `r25644` tree has 7,539 tracked paths, so the snapshot omits 166
upstream-tracked paths. The local import also changes 1,524 shared blob IDs and
712 file modes; among the content differences, one is a line-ending change and
`mptrack/View_smp.cpp` contains an unproven substantive downstream change. The
snapshot must be replaced by a history-preserving import rather than patched
into pretending to be the baseline. See the root
[`UPSTREAM.md`](../../../UPSTREAM.md) for the current audit and verification gate.

The official GitHub baseline was fetched into a temporary blob-filtered clone;
its complete reachable commit graph currently counts 17,432 commits, and the
baseline commit records SVN revision `r25644`. The clone is suitable for
provenance and path comparison, but it is not itself the final release import
until all required blobs are retained in the project repository.

The fixed commit and its complete reachable objects have now been fetched into
the project repository. The annotated tag
`upstream/openmpt-svn-r25644` points to the official commit and tree; the
repository check found 185,790 reachable object IDs and no missing objects.
The `openmpt-upstream` remote is configured for fetch only, with its push URL
set to `DISABLED`. The provisional source snapshot has since been replaced
with the tree read from that immutable tag: `openmpt-original_ref/` now has
7,539 tracked paths and its tree ID exactly matches
`713efc62513515497d701c64ca2728175f8f5acb`. The remaining closure gate is the
canonical SVN export comparison, which cannot yet run because `svn` and
`svnversion` are unavailable in the current environment.

## Answer

Use one project Git repository for the imported upstream history and all
downstream work, while keeping the official repositories as read-only remotes.
This is not a universal Git requirement; it is the appropriate choice for this
project because the Release Unit, source distribution, provenance, and future
selective upstream integration must be identifiable from one repository and
one release tag.

The Git history import and exact source-prefix installation are complete and
more than sufficient for the current personal-use effort. The checked-in tree
under `openmpt-original_ref/` is the development starting point. Its provenance
and the unperformed canonical SVN comparison are recorded in
[`UPSTREAM.md`](../../../UPSTREAM.md); that comparison is reference-only and
does not block subsequent work.
