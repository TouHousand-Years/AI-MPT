# Pin the Upstream Baseline and source workflow

Parent: ../map.md
Type: research
Status: resolved
Blocked by:

## Question

Which exact OpenMPT revision should become the Upstream Baseline, and what source-import, history, attribution, and selective-sync workflow best supports an independent downstream project given that GitHub is a rebasing mirror of the official SVN repository?

## Comments

### Scope correction — 2026-08-29

The owner no longer requires strict source-version governance, an exact match to
OpenMPT's development environment, or a compatibility promise tied to
`r25644`. The identifiers and imported history below remain useful provenance,
but the current effort treats the checked-in source as a **Starting Snapshot**.
Exact SVN-export verification, selective-sync procedure, and baseline
compatibility are not gates for personal-use development. If public release or
formal upstream intake later becomes a goal, those policies must be reconsidered
in a fresh map rather than silently reactivated.

## Answer

### Decision

Pin the Upstream Baseline to **OpenMPT SVN trunk revision `r25644`**, whose official GitHub-mirror locator observed on 2026-08-26 is **`0eafb124cfd15f94e32302734607e71f2c36c84f`**. The commit records `git-svn-id: https://source.openmpt.org/svn/openmpt/trunk/OpenMPT@25644 ...`, which ties the two identifiers together ([official mirrored commit](https://github.com/OpenMPT/openmpt/commit/0eafb124cfd15f94e32302734607e71f2c36c84f)).

Treat the canonical baseline identity as the tuple:

```text
SVN URL:      https://source.openmpt.org/svn/openmpt/trunk/OpenMPT/
SVN revision: 25644
Git locator:  0eafb124cfd15f94e32302734607e71f2c36c84f
Observed:     2026-08-26
```

The SVN URL and revision are authoritative; the Git SHA is a convenient, immutable locator only inside the mirror history as it existed when pinned. This distinction is necessary because OpenMPT says development occurs in SVN, GitHub is strictly downstream, and GitHub history can be rebased when recent SVN revision properties are corrected ([official contributing guide](https://github.com/OpenMPT/openmpt/blob/master/doc/contributing.md), [official mirror README](https://github.com/OpenMPT/openmpt/blob/master/README.md)).

This baseline is the current trunk state at the planning/development start, matching the requested compatibility target. It is deliberately not a promise to follow subsequent OpenMPT releases.

### Import and history workflow

The following is the downstream workflow decision; it is a recommendation derived from the repository facts above, not an upstream-prescribed workflow.

1. At source-import time, clone the official GitHub mirror with its complete history, check out the pinned Git locator, and create a permanent annotated downstream tag such as `upstream/openmpt-svn-r25644`. Do not squash the imported history. The official project documents both the SVN checkout and Git clone as supported ways to obtain source ([official development page](https://lib.openmpt.org/libopenmpt/development/)); retaining the mirror history gives the downstream useful authorship and change provenance.
2. Before the first downstream code commit, verify that the pinned Git tree matches an `svn export -r 25644` of the canonical URL, excluding only VCS metadata. Record the SVN URL/revision, Git SHA, Git tree ID or deterministic file-manifest hash, import timestamp, and verification result in a tracked `UPSTREAM.md`. This guards against a later mirror rebase without making the moving mirror branch part of the product's identity.
3. Branch the independent project's `main` from the permanent baseline tag. Keep the user's repository as `origin` and configure the official mirror as a read-only remote named `openmpt-upstream`. Never rebase downstream `main` onto the mirror's moving `master`; the downstream's baseline tag and ancestry remain immutable even if the public mirror rewrites its own Git history.
4. Preserve upstream commit authors and messages in the imported ancestry. Use separate downstream commits for the piano roll, capability layer, MCP sidecar, product naming, and later adaptations, so upstream-derived and project-authored work remain distinguishable.

### License and attribution

OpenMPT's root project is BSD-3-Clause, but its README warns that files under `include/` and `contrib/` can have their own licenses ([official README, License](https://github.com/OpenMPT/openmpt/blob/master/README.md#license)). Therefore the import must retain the root `LICENSE`, all copyright headers, and every nested/vendor license and notice; dependency inventory and binary packaging must not assume the root license covers those directories.

The root license requires source redistributions to retain the copyright notice, conditions, and disclaimer; binary redistributions must reproduce them in documentation or other materials; and the OpenMPT project/contributor names may not endorse the derivative without permission ([official LICENSE](https://github.com/OpenMPT/openmpt/blob/master/LICENSE)). Accordingly:

- retain the original OpenMPT notices verbatim and add a clearly separate downstream copyright/attribution notice for new work;
- ship the applicable notices with every source and binary distribution;
- use an independent project name and state that it is derived from OpenMPT, not an official OpenMPT release;
- audit licenses of any newly added MCP/runtime dependencies before distribution.

### Selective upstream synchronization

Do not continuously merge a moving `master`. For each optional sync window:

1. Fetch the official mirror into the read-only remote and identify candidate changes by the canonical SVN revision embedded in each `git-svn-id`, rather than relying only on a Git SHA that a mirror rebase may replace. The official mirrored baseline commit demonstrates this SVN-revision mapping ([official mirrored commit](https://github.com/OpenMPT/openmpt/commit/0eafb124cfd15f94e32302734607e71f2c36c84f)).
2. Apply only selected fixes on a temporary integration branch, in SVN revision order. Cherry-pick when the mirror commit is still reachable; otherwise reproduce the patch from the canonical SVN revision. Add an `Upstream-SVN: rNNNNN` trailer and source URL to each downstream integration commit, plus the observed mirror SHA when available.
3. Run compatibility, playback, file round-trip, and downstream UI/MCP regression tests before merging the integration branch. Resolve conflicts as downstream adaptations rather than rewriting the baseline.
4. Give security, corruption/data-loss, playback correctness, and file-format fixes priority. New upstream features are opt-in; no schedule or blanket compatibility promise is implied.

This yields a reproducible point-in-time OpenMPT derivative with intact provenance, while allowing deliberate upstream fixes without coupling the project to a rebasing mirror or to OpenMPT's future product direction.
