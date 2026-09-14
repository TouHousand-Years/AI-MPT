# `get_pattern_order`

Read the current Sequence order without claiming occupancy.

```json
{}
```

No arguments. This is a session-less read: it works before any session, while another connection holds one, and without the Patterns tab open. It never captures, rebinds, or releases the bound Pattern.

## Read the result

- `sequence.index` and `sequence.name`: the current Sequence.
- `entries`: one object per Order position, in order, each with zero-based `order` and `kind`:
  - `pattern`: a valid Pattern; carries `pattern`, `name`, and `rows`.
  - `skip`: the `+++` marker.
  - `stop`: the `---` marker.
  - `invalid`: a reference that is not a valid Pattern.
- `unreferenced_patterns`: valid Patterns the current Sequence never references, each with `pattern`, `name`, and `rows`.

Duplicates are preserved: the same Pattern can appear at several `order` indices, and every occurrence addresses that one Pattern. Use the list to choose a `switch_pattern` target; reading it grants no edit rights by itself.

Completion criterion: the current Sequence, every Order entry with its kind, and the unreferenced Patterns are accounted for before choosing a switch target.
