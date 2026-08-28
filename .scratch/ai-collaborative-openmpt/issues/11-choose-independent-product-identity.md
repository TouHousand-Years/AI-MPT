# Choose the independent product identity

Parent: ../map.md
Type: grilling
Status: resolved
Blocked by: 09

## Question

Which legally and practically available independent name, executable and package namespace, GitHub organization/repository, visual identity, data-directory identity, and public attribution language should replace the current development placeholders without implying endorsement by OpenMPT?

## Comments

The product identity was grilled and confirmed by the human collaborator on 2026-08-28. The project is first a personal-use effort: an enhanced OpenMPT experience for its owner. The initial implementation therefore freezes only the minimum engineering identity needed to keep work coherent. Final public branding and open-source release concerns are intentionally inactive until the owner considers a version satisfactory for publication.

## Answer

### Product role and identity boundary

This project is an independent OpenMPT-derived tracker intended first for the owner's own use. Its goal is to provide a more useful OpenMPT experience by adding the approved Piano Roll and AI-collaboration capabilities while preserving the tracker's fundamental identity, canonical Tracker Pattern model, and baseline behavior.

The project does not seek upstream acceptance as a condition of success. The development project and its eventual product must not imply that they are an official OpenMPT release or that OpenMPT or its contributors endorse them.

### Initial engineering identity

The following names are stable working identifiers for the initial implementation, not a final public brand:

| Surface | Initial identity | Boundary |
| --- | --- | --- |
| Working product name | `OpenMPT for AI` | Provisional development name only; it is not the final public product name. |
| Repository | `OpenMPT-for-AI` | Retain the current repository identity; do not create a new organization or rename the repository for this issue. |
| Engineering prefix | `OpenMPTForAI` | Use for executable, package, runtime, and other implementation identifiers until a later owner decision. |
| Installed user data | `%LOCALAPPDATA%\\OpenMPTForAI\\` | Keep configuration, logs, cache, runtime records, and temporary resources separate from original OpenMPT data. |
| Portable user data | `UserData` beside the portable program | Follow issue 09's portable boundary; do not write product metadata into module files or module directories. |

Executable names, package identifiers, file associations, installation namespaces, and runtime discovery identifiers should derive from the engineering prefix and remain centralized so that a future product-name change does not require a scattered string rewrite. The source-tree directory `openmpt-original_ref` remains provenance for the imported upstream implementation and is not itself a user-facing product identity.

### Attribution

README, About, and other unavoidable product-information surfaces use a short source and independence statement:

> This is an independent derivative of OpenMPT based on the project's pinned Upstream Baseline. It is not affiliated with, sponsored by, or endorsed by OpenMPT or its contributors.

This is the minimum attribution decision for current engineering work. It does not attempt to settle a complete release notice, trademark clearance, or other open-source publication question.

### Visual presentation

The initial implementation uses the working name and a simple placeholder/text treatment. A complete logo, icon family, visual identity system, marketing language, and final public name are not part of this issue. UI work must remain usable if those elements are replaced later.

### Stage boundary before open-source publication

Until the owner considers a version satisfactory and ready to open source, the project does not actively consider or develop:

- final public brand selection or trademark clearance;
- complete release-license and redistribution auditing;
- code signing, publisher identity, or release-security hardening;
- public release channels, update branding, or other open-source publication concerns.

These are not current implementation tasks and should not be used to expand issue 11 or block personal-use development. The minimal source attribution above remains part of the engineering identity. When the owner decides that a version is ready for open-source publication, the applicable release work can be reconsidered as a new gate rather than assumed by this issue.

### Consequences and handoff

- Issue 12 may bootstrap the pinned source tree under the current working/repository identity.
- Issue 14 may use `OpenMPTForAI` for the initial application, package, Sidecar, and data/runtime namespaces, while treating the name as replaceable.
- Original OpenMPT settings and data remain isolated; any future import is explicit and itemized as required by issue 09.
- No final public name, public GitHub organization, full visual identity, or release-ready legal/security package is implied by resolving this issue.
