# Prototype the local MCP round trip

Parent: ../map.md
Type: prototype
Status: open
Blocked by: 06, 21, 23

## Question

What is the thinnest local app-to-Sidecar-to-MCP-client prototype that attaches
to one running application and one explicit document, obtains the chosen
revision-bound Pattern Score Context, reports understandable failures, and
demonstrates where owning-thread dispatch belongs without first implementing
multi-instance discovery, write leases, Operation Receipts, packaging, or a
complete protocol catalogue?

## Comments

The prototype should preserve the chosen process boundary while deliberately
leaving mature-system hardening outside the first round trip.
