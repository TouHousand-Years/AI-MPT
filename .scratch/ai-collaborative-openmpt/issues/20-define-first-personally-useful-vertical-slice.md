# Define the first personally useful vertical slice

Parent: ../map.md
Type: grilling
Status: resolved
Blocked by:

## Question

What exact owner workflow should count as the first personally useful result,
from opening an existing module through Piano Roll interaction and local AI
collaboration to hearing, accepting, undoing, and saving the result; which
Pattern data and failure cases are essential to that scenario; and which
otherwise desirable capabilities are explicitly absent from this first slice?

## Comments

This ticket fixes the success criterion for the new map before implementation
choices deepen. It must describe an observable owner scenario, not a component
checklist or a release milestone.

## Answer

### Personally useful owner workflow

The first slice is a local, prompt-independent Pattern collaboration loop. The
owner opens a purpose-built but musically credible MPTM test module, works in a
single Pattern through the synchronized Tracker/Piano Roll editor, and asks an
external local Agent to use OpenMPT as an MCP service. The first version proves
both of these paths through the same proposal workflow:

1. the owner edits or selects a main melody and asks the Agent to write harmony;
2. the owner selects harmony and asks the Agent to write a main melody.

The test module uses a fixed meter and tempo with existing piano, bass, and
harmony-oriented sounds. It provides two resettable four-to-eight-bar starting
states: melody with empty harmony voices, and harmony with an empty melody
voice. Each target Pattern appears only once in the unchanged original Order
list, so the first test does not disguise shared-Pattern consequences as
per-Order-instance editing. Both paths must pass the mechanical checks below,
and at least one generated result must be good enough that the owner chooses to
keep it.

The Agent may preserve the same conversation when the owner rejects a result
and gives musical feedback, but every retry creates a new revision-bound
candidate and proposal. A handed-off proposal is immutable and is never edited
or silently rebased in place.

### First-version Pattern mode

The MCP surface exposes only **Pattern editing mode** in the first version.
Order editing remains the original OpenMPT human workflow: there is no new
Order UI and no AI Order mode. Pattern mode binds one Pattern when the proposal
session begins and cannot read or write another Pattern. Human navigation to a
different Pattern does not retarget the Agent's session.

Within the bound Pattern, read commands are not limited by the Piano Roll
selection. They may inspect any rows and channels in that Pattern, plus read-only
summaries and identifiers for existing instruments and samples. They cannot
create, remove, or modify instrument, sample, plugin, routing, tempo, meter, or
project-setup resources.

An explicit `PatternRect` is the initial write boundary. With no selection, the
whole bound Pattern is the initial write boundary. Each candidate-mutation tool
call nevertheless writes exactly one voice: one monophonic musical line bound
to one Tracker channel. A multi-voice harmony therefore uses multiple
single-voice calls that accumulate in one private candidate and one final
proposal.

The Agent may shrink, split, or move among subranges inside the granted write
boundary. Expanding an explicit selection requires human approval in a
non-modal OpenMPT approval surface, while navigation, inspection, and playback
remain available. Approved expansions enlarge a session-local envelope and do
not need repeated approval inside that envelope. With no initial selection,
the whole Pattern is already granted and no expansion approval is needed.

The common AI/MCP settings area provides a global choice between asking for
each write-range expansion and always approving expansions. Asking is the
default. The same focused settings area holds MCP enablement, service status,
and occupancy timeout; this ticket does not require a redesign of OpenMPT's
general settings system.

Pattern reads cover notes, instrument references, volume-column data, effects,
and the row/channel context needed to understand them. The first write surface
covers adding, moving, replacing, and deleting notes; references to already
existing instruments; volume values; and note-off events. Effect commands,
note cuts, complex delays, plugin automation, and other unsupported or
tracker-only content are preserved and never silently approximated or skipped.
A target voice defaults to the existing instrument assigned by the owner or
fixture; a prompt may choose another instrument already present in the module,
but cannot create or alter the instrument resource. The service enforces
structural integrity, not musical taste: the owner judges whether the melody or
harmony follows the prompt.

### Occupancy and candidate state

Every document read or candidate-mutation call automatically takes a short
occupancy for the duration of the call so that it cannot observe or change a
half-mutated document. An `occupy` parameter promotes that short occupancy to a
cross-call occupancy and returns a session token. Once promoted, omission of
the parameter on later calls does not release it; only the explicit ending
call, human override, or timeout does.

The normal first-slice workflow retains occupancy while the Agent evaluates
results and performs its sequence of calls. During retained occupancy, human
project writes, Undo, and Redo are blocked, while navigation, visual inspection,
selection movement, playback, and the occupancy/review UI remain available.
The bound Pattern ID, base revision, and initial write range do not follow later
UI navigation or selection changes.

Each successful single-voice call changes only the private candidate. A failed
call leaves that candidate exactly as it was before the call, so the Agent may
correct the request and retry. A revision mismatch, occupancy loss, explicit
abort, or timeout invalidates the whole candidate rather than rebasing it.

After evaluating the last tool result, the Agent makes one explicit ending
call. `handoff_for_review` freezes an immutable proposal and releases occupancy
atomically; `abort` discards the candidate and releases occupancy atomically.
There is no intermediate state in which an unlocked mutable candidate awaits
handoff.

OpenMPT always shows retained occupancy and offers a high-priority human
release. Human release, including release while a range-expansion approval is
pending, invalidates the token and candidate and causes active or later Agent
calls to fail with occupancy loss. Approval waiting pauses the ordinary
occupancy timeout without blocking human navigation or playback. Outside an
approval wait, normal calls renew a finite timeout; the initial practical
default is five minutes and may be tuned from observed Agent behavior.

The first slice supports one local OpenMPT instance, its current module, one
Agent client, and one active proposal session. Remote access, multiple clients,
multiple simultaneously addressed modules, and the mature lease, receipt, and
distributed recovery contract are deferred.

### Review, hearing, feedback, and saving

Before Apply, the owner sees only the evidence needed to understand the musical
change: the affected range and target voices, the note differences in
synchronized Piano Roll and Tracker projections, the proposal's revision
status, and whole-proposal Accept and Reject actions. Error explanations appear
when an actual validation or unsupported-data problem occurs; the UI does not
accumulate defensive proofs that merely second-guess the Agent. Partial
acceptance is absent.

Apply is a deliberate human action. It revalidates document identity, revision,
format constraints, and the complete proposal, then commits everything or
nothing. One successful Apply advances the document revision once and creates
one ordinary OpenMPT Undo step.

The first slice does not require pre-Apply candidate audition. The owner applies
the proposal, hears it through normal OpenMPT playback, and either keeps it or
uses the single ordinary Undo step. An unsatisfactory result is described back
to the Agent in natural language and becomes a fresh proposal from the then
current revision.

For the successful path, the owner manually uses Save As for the test copy,
closes it, reopens it, and confirms that its MPTM content and playback remain
correct. The Agent receives no save tool.

### Required verification

The vertical slice is not complete until local verification demonstrates:

- both melody-to-harmony and harmony-to-melody workflows through the same MCP
  and proposal loop;
- explicit-selection and no-selection whole-Pattern scopes, single-voice calls,
  multi-call candidate accumulation, and approved range expansion;
- short and retained occupancy, permitted navigation/playback, blocked human
  writes, explicit handoff/abort, timeout, and high-priority human release;
- unchanged candidate state after one failed call and complete invalidation
  after occupancy or revision loss;
- visible failure without document mutation for a stale proposal and for
  unsupported tracker-only content;
- one atomic Apply, one revision increment, one ordinary Undo step, and no
  document change after Reject or failed Apply;
- post-Apply playback, human musical feedback followed by a fresh proposal, and
  successful Save As plus close/reopen verification;
- a human verdict that at least one result is worth retaining for further work.

### Explicitly absent

The first slice does not include Order AI/UI work, cross-Pattern operations,
full-song autonomous composition, new instruments or samples, plugin editing,
Focus layout, partial proposal acceptance, mandatory pre-Apply audition, Mini
Audio Reviewer integration, impact tiers, long-lived receipts, remote or
multi-client MCP operation, release packaging, or public-product readiness.
The Pattern tools and Skills must leave room for later skill-guided independent
composition without making it a first-slice acceptance gate.
