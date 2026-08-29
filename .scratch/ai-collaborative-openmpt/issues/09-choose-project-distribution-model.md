# Choose the independent project's distribution model

Parent: ../map.md
Type: grilling
Status: resolved
Blocked by:

## Question

What project identity, licensing and attribution presentation, executable and sidecar packaging, user-data boundaries, module-file compatibility policy, and upstream-change intake process should define this independent OpenMPT-derived open-source project?

## Comments

### Scope correction — 2026-08-29

This distribution contract is no longer active in the current map. The owner is
building for personal use and does not currently need public packaging,
publication-grade version governance, strict `r25644` compatibility, CI release
gates, or upstream intake policy. User-data isolation and preservation of
existing license notices remain useful safety boundaries. Everything else below
is retained as historical design material and must be reconsidered in a fresh
publication map if the owner later chooses to release the project.

The distribution design was grilled in nine rounds and confirmed by the human collaborator on 2026-08-28. It applies the pinned provenance and selective-sync policy from issue 01 and the local, client-supervised sidecar topology from issue 06. During review, the first release was deliberately narrowed to an unsigned Developer Preview: signing, supply-chain attestations, automated updating, security maintenance, and mature package channels remain later work rather than functional prerequisites.

## Answer

### Independent identity, source, and licensing

The public product uses an independent name that does not contain OpenMPT. `OpenMPT for AI`, `AI-Collaborative OpenMPT`, the current repository name, executable names, and paths are development placeholders only. The public name, product IDs, executable names, icons, GitHub organization/repository, installation namespace, data directories, file associations, and update source are frozen together by a later naming task. Every public surface says that the product is derived from OpenMPT and is not an official OpenMPT release.

The project is a public monorepo rooted in the complete imported OpenMPT history described by issue 01. The derived application, MCP Sidecar, protocol schemas, packaging configuration, tests, and documentation live in that repository so that one release tag identifies the complete source of one distribution. New project-authored code uses BSD-3-Clause. Upstream file headers, the root license, nested and vendored licenses, and separate downstream copyright notices remain intact.

Every source and binary distribution contains offline license and attribution material covering the files actually shipped. Permissive dependencies such as BSD, MIT, ISC, Apache-2.0, and Zlib remain preferred, but license-name matching never replaces a per-dependency redistribution review. Copyleft, non-commercial, source-unknown, or otherwise restricted dependencies require an explicit decision. Missing provenance, license text, or redistribution rights blocks the entire release.

### First-release promise and version identity

The first public release is an **unsigned x64 Windows Developer Preview**, not a production-stability promise. Preview versions use explicit prerelease identities such as `0.1.0-preview.1` and matching immutable-in-meaning tags such as `v0.1.0-preview.1`. Every public artifact increments the version; a tag or asset is never silently replaced under an existing version.

Only the x64 host application is supported initially. The package still includes any x86 Plugin Bridge or helper that the imported baseline requires for supported 32-bit plugins. ARM64, an x86 host application, and legacy-Windows packages are deferred. The exact minimum Windows version is published only after the imported source, chosen toolchain, MFC runtime, helpers, and packaged application have passed real-system tests.

Application and Sidecar share the product version. IPC, capability, and data contracts retain independent major/minor schema versions and negotiate as required by issue 06. About, Sidecar diagnostics, release metadata, and package manifests identify the product version, Git commit, build configuration, architecture, Upstream Baseline, and applicable schema versions.

### Release Unit and package forms

One Release Unit contains the main application, MCP Sidecar, required architecture helpers and Plugin Bridges, redistributable runtime components, default resources, protocol metadata, notices, and concise MCP setup documentation. These components are version-locked and released, installed, upgraded, and removed as one unit; users are not expected or supported to assemble or independently replace them.

**Plugin Bridge/helper binaries, portable archive construction, common packaged resources, and upstream license/notice collection reuse the original OpenMPT implementation and package layout as their starting point.** The project adapts that implementation for the independent product identity and adds the Sidecar, schemas, runtime metadata, version-lock checks, and new dependency notices. Original OpenMPT's administrator-required install policy and automatic-update behavior are not inherited.

Each preview publishes three corresponding artifacts from the same fixed source:

- a per-user Windows installer that requires no administrator rights and supports unattended install, upgrade, and uninstall;
- a portable ZIP with the same features, protocols, helpers, and compatibility behavior;
- a buildable source archive or verified tagged source containing all required vendored/submodule sources, licenses, `UPSTREAM.md`, and build instructions.

The installer technology is selected only after an implementation spike. The contract is a self-contained per-user EXE installer; MSI, MSIX, Store packaging, and per-machine deployment are not first-release requirements. The installer and portable package may differ only in installation behavior, data placement, shortcuts, and optional file associations. Neither package downloads a runtime or functional dependency during first launch.

Installation and upgrade refuse to replace files while the application, Sidecar, or required helper is running. Upgrade is atomic from the user's perspective: cancellation or failure leaves the prior complete Release Unit usable, never a mixture of versions. Configuration migration is schema-versioned, backs up the prior state, and rolls back on failure. Preview and future stable channels remain separate; preview supports only its current release, while no fixed long-term-support period is promised.

### Sidecar launch, client configuration, and instance isolation

An installed channel exposes a Sidecar launcher at a stable path that survives patch and minor upgrades. The application provides an AI-integration settings page that generates client-specific stdio configuration snippets, displays the absolute command, and offers diagnostics. It never edits a third-party MCP client's configuration without a separate, explicit user action. Portable configuration records its current absolute path and must be regenerated after the directory moves.

No Windows-wide or MCP-wide server registration mechanism is assumed: the MCP stdio contract defines client-launched child processes but no universal operating-system registry. Optional MCP Registry or package metadata may be added later only as another discovery aid.

The product ID, release channel, protocol major, and installation-instance identity namespace runtime registration and participate in handshake validation. A Sidecar discovers only matching product and compatible protocol identities by default. Preview, stable, portable, and side-by-side installations do not silently attach to each other's app processes; any exceptional cross-instance selection must be explicit and still pass protocol negotiation.

### User-data and portability boundaries

The installed application stores machine-local state below a current-user Local App Data product namespace, divided into configuration, logs, cache, runtime registration, and temporary resources. It never treats a module's directory as an implicit metadata store. Portable Mode uses a visible `UserData` directory beside the portable program while continuing to use a protected current-user runtime or temporary directory for security- and lifecycle-sensitive IPC and render resources. A read-only portable location produces an explicit error or documented no-persistence mode; it never silently switches to installed-mode storage.

**Portable-mode detection, install-relative path handling, and portable configuration primitives reuse the original OpenMPT implementation where they satisfy this boundary.** The independent product namespace, visible `UserData` layout, protected runtime directory, read-only behavior, and separation of configuration/log/cache/runtime data are downstream adaptations.

Settings, plugin paths, logs, caches, runtime records, update preferences, and MCP configuration never cross Windows-user boundaries. Runtime records and incomplete renders are removed on normal completion and swept after abandonment; caches and logs are bounded and safely clearable. Diagnostics retain issue 06's content-minimizing rules. Exact sizes and retention periods are implementation-tested limits, not part of this distribution decision.

Modules, automatic saves, backups, and explicit exports are user data rather than cache. Uninstall retains them by default and never touches original OpenMPT data or another installation instance. A removal option distinguishes disposable cache/log/runtime data, opt-in settings removal, and protected automatic saves/backups/exports. The latter remain preserved unless the user separately and unmistakably selects them.

The first release does not automatically import original OpenMPT settings, plugin paths, or file associations. It does not bundle third-party plugins, commercial instruments, or source-unclear samples. Any example music or redistributable content is separately licensed. A future import wizard must be explicit and itemized.

Temporary Context Packages, Audio Previews, Reviewer opinions, unhanded-off candidate state, and uncommitted proposals expire according to their resource/session lifecycle. Agent Edit Sessions, write leases, document-lifetime receipts, and their runtime identities never recover across a crash. A user may explicitly export review evidence with revision, range, provenance, and privacy warnings, but no AI or workspace artifact is silently written beside or inside a module.

### Module compatibility and upstream intake

The project initially promises compatibility with the formats and behavior of Upstream Baseline `r25644`, not with every later OpenMPT release. Opening a module must not mutate it. For data that remains representable in the target format, Tracker, Piano Roll, and AI edits preserve baseline read, playback, save, reopen, and interoperability behavior. The first release writes no project-private MPTM chunk or other module extension.

**Module loading, playback, format validation, saving, compatibility conversion, and offline rendering reuse the original OpenMPT implementation from the pinned baseline.** New Piano Roll and AI paths must invoke those implementations through the shared capability boundary rather than introducing independent format or playback engines.

When a target format cannot express current data or save would make an irreversible conversion, direct overwrite is blocked by default. The user receives a concrete loss report and explicitly chooses Save As, a different target format, or confirmed conversion. A generic warning is insufficient.

Compatibility is verified with a fixed baseline corpus across structural loading, playback or bounded offline-render comparison, no-change opening, edit-save-reopen round trips, baseline OpenMPT interoperability, plugin/sample failure cases, and both installer and portable packages. Any tolerance for non-bit-identical audio must be specified before results are evaluated.

Subsequent upstream changes remain opt-in and are integrated by canonical SVN revision under issue 01's provenance and regression rules. Security, data-corruption, playback-correctness, and format fixes receive priority but never bypass source recording, review, relevant tests, or human release approval. Selective fixes do not change the project's `r25644` origin or automatically establish a new compatibility baseline; a baseline uplift requires its own explicit decision and complete regression evidence.

### Distribution, updates, and deferred hardening

The single first-release authority is the public project's fixed GitHub Releases location, chosen after the formal identity is settled. A project site may link to it but does not mirror artifacts. The application remains fully useful offline. Only an explicit **Check for Updates** action reads the matching preview or stable release stream, presents version and release information, and opens the browser for manual download. Failure to check has no local effect. There is no background check, download, installation, rollback, user-supplied mirror, or custom update server.

Code signing is not functionally required for the first Developer Preview. The unsigned release explains that Windows may show an unknown-publisher warning and supplies SHA-256 values for manual integrity checking. Authenticode, timestamping, signed tags and update manifests, SBOMs, build-provenance attestations, immutable hosting controls, downgrade resistance, a private vulnerability-reporting program, and a supported security-maintenance policy are deliberately deferred to post-preview hardening. WinGet, Microsoft Store, other package managers, and self-hosted download services are likewise deferred.

This deferral does not relax functional, compatibility, privacy, or licensing gates. CI produces candidates; one authorized maintainer deliberately promotes a fixed commit after all required tests and artifact inspections pass. Unofficial nightly artifacts use distinct non-release identities, short retention, and prominent warnings, and never enter an update channel.

### Release gates and implementation handoff

A Developer Preview is all-or-nothing. Release is blocked if any package is incomplete or version-mixed; application, Sidecar, or helper cannot start offline; install, migration, upgrade, cancellation, or uninstall damages the old installation or protected user data; installer and portable features diverge; baseline compatibility regresses; license provenance is incomplete; MCP configuration cannot establish the intended stdio connection; or one installation can attach to the wrong app instance.

Issue 09 freezes this distribution contract, not its implementation. The formal name, exact installer framework, minimum Windows version, toolchain lock, and measured retention/resource values remain evidence-driven follow-up work. Issue 10 is additionally constrained by this issue: Piano Roll workspace persistence must honor the module-file and user-data boundaries above.

Implementation is split into issues 11 through 18 for product identity, source import, reproducible build/CI, packaging and MCP setup, licensing/source distribution, compatibility/lifecycle verification, Developer Preview publication, and post-preview signing/security maintenance.
