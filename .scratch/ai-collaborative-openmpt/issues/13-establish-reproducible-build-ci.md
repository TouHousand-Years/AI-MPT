# Establish the reproducible build and CI baseline

Parent: ../map.md
Type: task
Status: resolved
Blocked by: 12

## Question

Which pinned Visual Studio, MSVC, Windows SDK, MFC, dependency, helper-architecture, and clean-runner configuration reproducibly builds the x64 application, required Plugin Bridges, MCP Sidecar, tests, and package inputs from the imported source tree?

## Comments

### Closed outside the current destination — 2026-08-29

Pinned toolchains, clean-runner reproducibility, helper-architecture matrices,
and CI are public/distributed-product concerns. The current map needs only the
smallest build-and-run loop on the owner's machine, tracked separately by
[Establish the smallest local build-and-run loop](21-establish-smallest-local-build-run-loop.md).

## Answer

Closed without resolving the original release-oriented question. Reconsider it
in a fresh publication or team-development map if reproducible distribution
becomes a real goal.
