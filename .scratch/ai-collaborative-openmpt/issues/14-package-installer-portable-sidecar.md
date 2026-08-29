# Package the installer, portable application, and MCP Sidecar

Parent: ../map.md
Type: task
Status: resolved
Blocked by: 06, 11, 13

## Question

Which per-user installer technology and portable layout can ship one version-locked Release Unit, provide stable and client-specific Sidecar launch configuration, isolate channels and installation instances, migrate or roll back configuration atomically, and uninstall without touching protected user data?

## Comments

### Closed outside the current destination — 2026-08-29

The owner does not currently need an installer, portable Release Unit, channel
isolation, atomic package migration, or uninstall behavior. The local Sidecar
round trip is planned independently of packaging.

## Answer

Closed without resolving the packaging question. It may be reconsidered only
when the owner explicitly chooses to distribute the application.
