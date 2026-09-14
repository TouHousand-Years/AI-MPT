---
name: touhou-style-music
description: Make the musical decisions for Touhou-style composition — key, chords, melody, form, harmonization, bass, drums, background layers, and note placement — and write them into a DAW, tracker, or MIDI sequence. Use when the task is to compose, extend, or repair Touhou-style (Touhou-like, 东方风格/东方风) music and the question is what to write — choosing chords, writing or reworking a melody, harmonizing a line, arranging a section, structuring a song, or judging whether a written passage sounds dissonant — including when reviewing existing material for musical mistakes. Do not use for the mechanics of a particular editor — how to enter, navigate, or edit notes in your target DAW or tracker — or for genres other than Touhou-style.
---

# Touhou-Style Music

This skill supplies the musical rules a Touhou-style passage has to satisfy and the choices that make it sound like Touhou rather than generic minor-key music. It says nothing about how to operate a particular editor: whether your tools are a tracker MCP, a DAW scripting bridge, a plugin API, or the UI driven through computer control, the rules below are the same and you translate them into whatever controls that editor exposes. Read the target editor's own capability description before writing, and do not assume a control set, a command name, or a valid range that you have not confirmed there.

The working principle behind almost every rule below: Touhou music is minor-key, melody-focused, and driven by forward movement. Each rule exists to keep a line moving toward a resolution without producing dissonance the listener will notice.

## Work order

Establish these in order; each one constrains the next.

1. **Key and scale** — default to A natural minor (all white keys).
2. **Chord progression**, section by section.
3. **Melody** over it, phrase by phrase (antecedent, then consequent).
4. **Song form** — intro, alternating verse/bridge, climax, optional outro, plus the loop point.
5. **Harmonization** of the melody.
6. **Parts** — string chords, bass, drums, background layers.
7. **Placement and mix** — octave spacing, velocity, panning.

## Non-negotiables

Violating any of these is what makes output sound wrong rather than merely plain.

- **Stay in the key.** Notes outside the scale are the main source of "this sounds bad". Every chromatic note must be a deliberate borrow (see the raised 7th), never a slip.
- **A raised note must be raised in the melody too.** When a chord raises the 7th (major 5, or the diminished sharp 7), the melody over it must use the raised note. Chords and melody taking the 7th differently is clearly audible.
- **Never treat a transposed copy as a harmony.** Transposing the melody down by an interval harmonizes with it at only one or two notes; the rest sounds wrong and listeners notice. Use the harmonization checklist below.
- **Avoid parallel fifths.** Successive perfect fifths mean something went wrong. Parallel fourths are only a preference, not an error.
- **The melody is the deliverable.** The game context carries almost no meaning — the tune is what makes the song memorable. A melody that cannot be hummed should be rewritten, not decorated.

## Pitch material

Twelve pitch classes. Notes are named letter plus number, with black keys named from the white key below and a sharp (♯); the same key can be written as a flat (♭) of the key above. Which name is used is a notational matter and does not affect what is written.

- Natural minor is **whole, half, whole, whole, half, whole, whole** from any root. Major is whole, whole, half, whole, whole, whole, half. Touhou is almost always minor; A natural minor is A B C D E F G.
- The **7th degree is commonly raised** — G becomes G♯ in A minor. That raised note is the tension that wants to resolve to the 1st degree, and it is the origin of the major 5 and diminished sharp 7 chords.
- Measure and reason with intervals:

| semitones | interval | semitones | interval |
| --- | --- | --- | --- |
| 0 | perfect unison (P1) | 7 | perfect fifth (P5) |
| 1 | minor second (m2) | 8 | minor sixth (m6) |
| 2 | major second (M2) | 9 | major sixth (M6) |
| 3 | minor third (m3) | 10 | minor seventh (m7) |
| 4 | major third (M3) | 11 | major seventh (M7) |
| 5 | perfect fourth (P4) | 12 | perfect octave (P8) |
| 6 | tritone (TT) | | |

For an interval of 12 semitones or more, subtract 12 until it is in range — 13 semitones is a minor second. Intervals measure from any starting note.

## Chords

A triad is root + third + fifth, and the **third decides the quality**.

| type | stack | sound |
| --- | --- | --- |
| major | M3 + P5 | bright, full |
| minor | m3 + P5 | sad |
| diminished | m3 + diminished 5th | dark, tense — the tritone wants to resolve |
| augmented | M3 + augmented 5th | anxious; rare in this style, only use deliberately |

Diatonic triads of A minor. Every minor key has this same pattern of qualities:

| degree | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| chord | Am | Bdim | C | Dm | Em | F | G |
| quality | minor | diminished | major | minor | minor | major | major |
| roman | i | ii° | III | iv | v | VI | VII |

Three minor, three major, one diminished, always in that order. Notation systems differ — some writers reference the parallel major (`♭III`, `♭VI`, `♭VII`), the Chinese Touhou community references the relative major (`vi`, `vii°`, `I`, `ii`, …) — but **throughout this skill the numbers are scale degrees of the current minor key**. "5" is the chord on the fifth degree; "major 5" is a major triad built there (E–G♯–B in A minor).

## Chord progressions

Functions, as tendencies rather than rules:

- **Tonic** resolves. The 1 chord resolves most strongly, then 3 and 6.
- **Dominant** is unstable and wants the tonic — 5 and 7. With the 7th raised, the 5 becomes a major 5.
- **Predominant** lacks tonic stability and drives toward the dominant.

What to actually write:

- **Core Touhou progression: 6–7–1.** Treat 6 as the moving start, 7 as dominant, 1 as the finality. Most sections are built as extensions of this.
- **The 90% set: 1, 7, 6, minor 5, and major 5.** Roughly ninety percent of Touhou progressions can be made from these alone; when unsure, stay inside it.
- **Insertions.** A minor 5 before the 1 gives extra emotional payload before the tonic resolves (the "anime progression": 6-7-5-1, often followed by a plain 6-7-1). Following a 1 with a 7 defuses the finality and loosens the phrase (6-7-1-7). For low-development bridge sections, simply hop between 1 and 6.
- **Darker color.** Replace the 7 with a **diminished sharp 7** for a darker, more mysterious tension. The leap from 6 to it is large, so smooth it with 6-7-♯vii°.
- **Phrase endings.** End on 1 for resolution, or on a Picardy third (major 1, third raised) for a conclusive, hopeful close at a section end. End on 5 or major 5 for an incomplete feel that carries into the next phrase. If a progression ends on the dominant, the next one should open on the tonic — 1 resolves best, 6 is acceptable.
- **Inversions.** Root position, first inversion (third in the bass), second inversion (fifth in the bass). Use them to smooth voice leading between chords, to vary a repeated chord, to build a chromatically descending bass line, and to bring parts back into instrument range after a key change.
- **Modal interchange.** A major 5 in a minor key is a deliberate borrow: E–G♯–B in A minor, and G♯ is not in the scale. Keep the melody aligned with the borrow.

## Melody

Write phrase by phrase. Each phrase is an **antecedent** (question) followed by a **consequent** (answer).

**Checkpoints.** Place a chord tone on each chord.

- Start the melody on a note of the first chord; if it is tonic, degree 1 is the usual choice.
- If the phrase ends on a tonic chord, end the melody on degree 1. If it ends on a dominant chord, end on any chord tone — degrees 5, 7, or 2.
- Plan the note over a dominant together with the note the next phrase begins on, because the dominant resolves into it.

**Fill between checkpoints with chord tones** — but only using chord tones is limiting and boring.

- **Safe filler: the minor pentatonic — degrees 1, 3, 4, 5, 7** (A C D E G in A minor). These notes can be held without an urgent pull toward elsewhere, and they fit the majority of chords in the key. Most of the catchy material in this style comes from here.
- **Degree 6 is a weak note.** It pulls toward a resolution, so do not give it an important position unless the underlying chord is a sixth chord.
- **Non-chord tones rarely clash.** A line over 6 can often repeat unchanged over 7. The rule is that if it does not sound too dissonant, it is fine — and when it does, fix the melody rather than the theory.

**Rhythm.** Use variety in note lengths, accents, and syncopation. Notes sitting only on the main beats sound flat; accents placed between the beats create drive. The lesson's two named "signature Touhou rhythms" were lost in transcription, so build rhythm from the accents the section needs instead of from a remembered canonical figure.

**Repetition and contour.** Copy-pasting a fragment and changing its pitches is an efficient way to fix a weak spot; too much repetition is boring. Track the melody's pitch contour as a roadmap: it should move with purpose, reach the checkpoints, and balance activity against range — too much or too little of either loses the listener.

**Resolving.** The antecedent ends unresolved (often on the dominant note, degree 5) to drive forward. The consequent resolves (tonic note over a tonic chord), and may resolve slightly before the bar ends to leave room for a pickup into the next section.

## Song form

- Alternate **verses** (the main material) with **bridges**. A song made only of verses has no direction; the music has to move as a whole, not just phrase by phrase.
- Shape: an **intro** (either a verse or a bridge, leading into whatever it is not), then alternating verses and bridges, then the **climax** (the final verse), then an optional lower-activity **outro**.
- Within a section, label the antecedent A and the consequent B. An eight-bar melody (four plus four) repeated to fill a sixteen-bar section gives AABB. An AAC-style form repeats the opening but ends differently. Keep some repetition, and control it.
- **Loops.** Most Touhou songs loop. Choose the loop point between the intro and the first verse.
- **Key change to fix a dragging section.** Transpose from the midpoint of the section. A minor 2nd up is the most common move and sounds brighter and more energetic; transposing down has the opposite effect. The minor 3rd and major 3rd are the other common intervals. The usual place is the final iteration of the climax melody, or the midpoint of a bridge.
- **Motifs.** When a layer or a section needs material, reuse an earlier idea: state a riff in the intro and restate it later under the melody, re-voicing its notes to the new section's chords. Clashes in a raw copy-paste are expected — they mean the motif needs re-voicing, not that the technique failed. Unique content does not scale: a final-boss theme runs about 3:00–3:15 but holds only roughly 2:00–2:30 of unique material, so unity matters more as a song gets longer.

## Harmonizing

Work this checklist per melody note, in order. It is also the general test for any two simultaneous lines: the further down the list a relationship falls, the less well the two notes harmonize.

1. Note is a chord tone → harmonize down to the next chord tone.
2. A third or sixth down works without clashing → use it.
3. Note is a perfect fourth above a chord tone → harmonize a fourth down.
4. Otherwise → harmonize to the closest chord tone below.
5. Still bad → change the melody and start over.

- **The root trap.** When the melody plays the root of a chord, a fourth-down harmony lands on the fifth and produces parallel fourths. Harmonize a **sixth down** instead, so the harmony takes the chord's third.
- **Parallel motion.** Avoid parallel fifths. **Contrary motion** — the harmony moving opposite to the melody — avoids both parallel fourths and fifths: converging lines tend to form a third, diverging lines a sixth.
- **Octaves** add fullness but are not harmony. **Seconds and sevenths** only work situationally: a second over a chord missing its third creates suspense, and a seventh works where the chord implies a seventh. Leave them out unless the case is clear.
- When harmonizing **above** the melody, prefer thirds to sixths, because the melody already sits high and a sixth up can leave the instrument's range.

## Parts and frequency layout

Four groups are active at any moment — **melody, chord progression, bass, drums** — and each needs its own frequency niche.

- **Lead.** Any instrument with enough presence, chosen for the section's energy. Doubling the line with a second instrument reinforces it.
- **Strings.** Block chords in the mid-range. Add a fourth chord tone if three sounds thin, and try inversions to find the register where they sit best. Keep them from competing with other parts.
- **Bass.** Chord tones, especially root and fifth. In slow sections hold the root; in fast sections alternate root and fifth, which raises energy without changing the harmony.
- **Background.** Rhythmic chord-tone material (arpeggios, strums) at around two notes per beat fills the mid-range — arpeggios suit lighter sections. Ambient sustained hits on a strong beat roughly once per bar (pads, tubular bells) fill the top, above the melody. Short looping riffs sit near the melody's own band and must stay unobtrusive. Fast rolls — "note spam", about four notes per beat, usually piano — add motion to a section.
- The more melodic a part becomes, the more it behaves like counterpoint and the more care it needs.

## Drums

Aim at rock-like Touhou percussion.

| element | default placement | role |
| --- | --- | --- |
| kick | beats 1 and 3 (the strong beats) | main pulse; supplements the bass |
| snare | beats 2 and 4 | power and section definition; easy to overdo |
| closed hat | two to four per beat, freely | light texture |
| open hat | about as often as snares | stronger accent |
| crash | first beat of an eight-bar section, or beat 1/3 of the bar before a change | marks transitions |
| ride | as a milder hat, e.g. alongside every few kicks | sustained light tapping |
| toms | into or out of high-energy sections; a descending roll into the last bar of a melody, top tom starting half a beat before mid-bar | fills and transitions |

Energy scaling:

- **Low energy:** sparse hits. Kicks once per two bars, snare once per bar on beat 3, open hat once per bar. (The transcription's exact low-energy kick position is unreliable — write what sounds right.)
- **High energy:** roughly double the kick and snare density, and put open hats on every beat with closed or pedal hats between them. The second half of the climax is the clearest case.
- **Building a slow section:** kick and closed hat on every beat, open hat on every half-beat, snares on 2 and 4 — then add extra kicks to taste, optionally fill remaining sixteenth-note spaces with closed hats, then add fills.

**Velocity** is the per-note force control and is the second form of volume control: lowering it on harmonies, on some hats, and on section-ending snares makes room without deleting notes. **Panning** is the left-to-right placement of a part, defaulting to centre in most editors, and Touhou-style music rarely pans far from it. Both are guidelines, not rules. How you reach them depends entirely on the editor and the file format — the control may be a column, a lane, a plugin parameter, a scripting call, or an effect command with a different name and range in every format — so look up the target's own controls instead of carrying over a name or a number from somewhere else.

## Dissonance triage

When something clashes, work in this order:

1. **Change the composition** to remove the offending notes. This is the best fix.
2. **Space the parts across octaves.** Melody, chords, and bass sharing one register is the usual cause of clashes on long notes.
3. **Re-voice the chords** using a different inversion.
4. **Lower the velocity** of the clashing notes so they are less audible; an instrument whose own sound is unusual can also mask small clashes.

If the dissonance is obvious, fix the composition. If it is marginal, masking is legitimate — listeners hear the whole song, not individual intervals.

## Putting it into the editor

Everything above is editor-independent. Before writing, find the target's own equivalents, and keep the four groups — melody, chords, bass, drums — on separate channels or tracks, so the frequency layout stays controllable later.

| musical decision | what it becomes |
| --- | --- |
| pitch, intervals | note data: a tracker's note column, a piano-roll note, a MIDI note number. Pitch class plus octave, and since intervals are counted in semitones, a transposition is a constant offset on that value |
| key | the set of pitches you allow. If the editor has a scale lock or key setting, treat it as a guard rail, not as a substitute for the rules above |
| degree or Roman numeral | not a stored value — resolve it to a concrete pitch in the current key before writing |
| chord | several note events on one channel starting at the same time position |
| inversion | the same chord tones with a different lowest note; nothing special is stored |
| section | pattern, clip, region, or block — whatever the editor's unit of repeatable material is |
| song form, loop point | the playback-order structure (sequence order, arrangement timeline, loop markers), not the note data |
| velocity | the editor's per-note force or volume-lane value. Ranges and defaults vary, so read them before mapping a dynamic plan onto numbers |
| panning | the editor's per-channel or per-note pan control |

Two habits that prevent most mechanical mistakes:

- Re-read the current state of the region you are about to change, and write complete content for it. A sparse write can mean "empty" rather than "leave unchanged", and which of the two it means differs between editors.
- Keep the musical model and the editor's notation separate in your head. A tracker names pitches like `C-5`, a MIDI pipeline counts note numbers, a piano roll shows vertical position. Decide in pitch-class and degree terms first, then encode.

