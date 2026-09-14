# Touhou-Style Music Composition — Organized Notes

These notes are organized from the transcribed lesson series in this folder (`0.txt`–`6.txt`, with `4a`/`4b` and `5a`/`5b`). The raw transcripts are unedited speech-recognition output: no punctuation, no paragraphs, and a fair amount of mis-heard words. This document keeps their knowledge content, restores punctuation and paragraphing, drops the spoken banter, and groups the material into chapters. Places where the transcription is genuinely ambiguous are listed in [Transcription notes](#transcription-notes) rather than guessed at.

## About this guide

The series is aimed at new and aspiring Touhou-style composers who are struggling with the basics. It is split into two stages:

- **Stage 1** is half music-theory crash course and half application of that theory to Touhou-style composing. Its goal is that you can compose a basic Touhou-style song by the end of it.
- **Stage 2** is much less structured, and mostly covers more advanced techniques and tricks that you can apply to Touhou-style music.

Composing is learned by doing — the same way anything else is. You are encouraged to try making tunes after every lesson, and to start early instead of waiting until you have finished the theory. Even if you already have a background in music, it is worth skimming the theory sections anyway, because they also cover Touhou-specific material.

## 1. Tools

### Choosing a DAW

To make music you need a Digital Audio Workstation (DAW): the program you use to write and arrange the music. Think of DAWs as the equivalent of Paint.NET, GIMP, or Photoshop for image editing. You have probably heard of some commercial DAWs, but free ones exist and work perfectly well.

As with image editors, it does not really matter which DAW you end up using — what matters is that you are used to working with it. Switching later gets difficult once you are comfortable, so choose deliberately at the start. For making Touhou-style music, the two features you mainly want are:

- a **piano roll**, and
- good support for **MIDI editing**.

### Choosing instruments

You will hear a lot of instrument names floating around, and most of them are plug-ins that cost real money. As with DAWs, there are free alternatives that work perfectly well. Unlike DAWs, instruments are much easier to switch later: if you eventually find something that sounds better than what you already have, the change is not painful.

Once you have picked a DAW, get used to using it: learn the piano roll, learn how to load instruments, and read or watch tutorials if you need to. The actual composing starts in the next chapter.

## 2. Notes, Scales, and Keys

### Reading the piano roll

When you open the piano roll, the left side is a keyboard labelled with letter-and-number combinations. Every white key has a letter and a number. Every black key is named after the white key below it, with a sharp sign (`♯`) in between — so the note between C and D is C♯.

Think of C♯ as a genuinely different name from C, in the same way that a letter with an accent is a different letter in languages that use accents. The black-key notes can also be written with a slightly pointed lowercase `b` symbol, which is read as **flat** — so the same black key is also called D♭. They really are the same note with two different names, in the same way that 2 + 1 and 4 − 1 both describe 3.

Some programs have a setting that displays these notes either as sharps or as flats. It does not matter if yours does not: when you are simply discussing music you are not bound to using only sharps or only flats. Whether you write a note as a sharp or a flat depends on context, but that only matters when you are scoring music. With a piano roll, it does not matter at all.

Notice that the letters repeat every 12 notes. That is because those notes are essentially the same notes, only higher or lower. So in practice you are working with 12 categories of note, not with over a hundred unrelated ones.

### Scales

Most songs do not use all 12 categories of pitch. Instead of working with every note from A to G♯, you restrict your choices by picking a **key**. To understand keys, we first need **scales**.

A scale is a sequence of notes produced by starting on some note and then following a rule until you reach a note with the same name. Scales are built from **whole steps** and **half steps** in a specific order, and each scale has its own pattern:

- A **half step** (also called a **semitone**) is the interval between two adjacent notes.
- A **whole step** (also called a **whole tone**) is equal to two semitones.

Every natural minor scale uses the same pattern of whole and half steps: **whole, half, whole, whole, half, whole, whole**. Because the pattern is fixed, you can apply it to *any* starting note and get that note's natural minor scale. Try it for yourself.

Other scales use other patterns. The major scale, for example, is **whole, whole, half, whole, whole, whole, half**. A great many scales exist, but the only ones that matter to most people are the major and minor scales.

### Keys

A song written in a given key uses only the notes of the corresponding scale. Most modern music is written in either a major key or a minor key. **Touhou music is almost exclusively written in minor keys**, so that is where the series focuses.

The lessons do everything in **A minor** for now, mostly because it is the simplest minor key to remember: it is literally all of the white notes.

Which minor key you use does not matter. Remember that the *distances* between notes are what make the song, not the notes themselves — a song can be transposed to any key and still sound basically the same.

One of the most important things to keep in mind is that the less your notes adhere to a key, the more dissonant your music becomes, and the more likely it is to sound bad. Unless you know what you are doing, always try to stick to the notes in the scale. Otherwise your songs may end up sounding wrong in exactly the way you would expect.

## 3. Intervals and Chords

This chapter is theory-heavy. Everything in it is also condensed into a chart in the lesson's accompanying material, so if you already understand all of it you can safely skim.

### Intervals

An **interval** is the distance between two notes. As covered in the previous chapter, a piano roll contains 12 different notes, which also means there are 12 + 1 different intervals (counting the unison at the bottom and the octave at the top).

| Semitones | Interval | Semitones | Interval |
| --- | --- | --- | --- |
| 0 | perfect unison | 7 | perfect fifth |
| 1 | minor second | 8 | minor sixth |
| 2 | major second | 9 | major sixth |
| 3 | minor third | 10 | minor seventh |
| 4 | major third | 11 | major seventh |
| 5 | perfect fourth | 12 | perfect octave |
| 6 | tritone (diminished fifth / augmented fourth) | | |

A good way to measure intervals is to count **semitones**. For intervals of 12 semitones or more, keep subtracting 12 until you reach a value in the table; that is the interval it is equivalent to. For example, 13 semitones is equivalent to 1 semitone, which is a minor second.

There are also alternative names for each interval. A major second, for instance, can also be called a diminished third. Which name is used depends on context, but these alternative names are only a notational concern.

Intervals work from any point on the piano roll. If you want a perfect fourth above E, count five semitones up from E and you have your answer. Get familiar with intervals, because the next topic is built on them.

### Four basic triads

A **chord** consists of multiple notes stacked together and heard simultaneously. The lessons focus on basic **triad** chords.

A triad is made of three notes: a **root**, a **third**, and a **fifth**.

- The **root** is the first note of the chord — its foundation.
- The **third** is an interval of a third above the root, and it can be either a major third or a minor third.
- The **fifth** is an interval of a fifth above the root, and it can be diminished, perfect, or augmented.

| Chord type | Construction | Character |
| --- | --- | --- |
| Major | root + major third + perfect fifth | bright and full |
| Minor | root + minor third + perfect fifth | sad |
| Diminished | root + minor third + diminished fifth | dark and full of tension |
| Augmented | root + major third + augmented fifth | anxious |

The only difference between a major chord and a minor chord is the **third**; the root and fifth stay the same, yet the two chords feel completely different. That is because the third of a chord is the note that determines the chord's quality.

The diminished chord sounds dark and tense because of the **tritone** between its root and its diminished fifth, which wants to resolve somewhere. The augmented chord has an anxious feeling to it, and augmented chords are quite rare — not just in Touhou-style music but in general, because their sound is naturally difficult to turn into something good.

### Diatonic chords of a minor key

Because chords can be built directly from a scale, we can find every chord that "belongs" to a key by stacking thirds using only the notes of that scale. The lessons use the A natural minor scale as the reference: for each of its seven notes, build a triad with that note as the root.

| Degree | Root | Chord tones | Quality |
| --- | --- | --- | --- |
| 1 | A | A–C–E | minor |
| 2 | B | B–D–F | diminished |
| 3 | C | C–E–G | major |
| 4 | D | D–F–A | minor |
| 5 | E | E–G–B | minor |
| 6 | F | F–A–C | major |
| 7 | G | G–B–D | major |

To see where the qualities come from, take the first chord, A–C–E. A to C is three semitones, a minor third; A to E is seven semitones, a perfect fifth. A minor third plus a perfect fifth makes a minor chord, and since the chord begins on A it is called an A minor chord. Try naming the other six the same way.

These seven chords are the **diatonic chords** of the key. In the natural minor scale the sequence of qualities is always **minor, diminished, major, minor, minor, major, major** — that is, three minor chords, three major chords, and one diminished chord. This pattern is not unique to A minor: every minor key has the same shape. If you want to convince yourself, write out the diatonic chords of a different minor key.

### Roman numeral notation

Naming chords by root plus quality is not very useful when you do not know the key, because the same chord can have vastly different uses depending on the key. It is more useful to talk about chords in relation to another note rather than in terms of absolute pitch. That is what **Roman numeral notation** does: the chords of a scale are represented by numerals denoting their scale degree.

- With seven notes in a typical scale, only the numerals **1 to 7** matter.
- **Major** chords are written with a capitalized numeral.
- **Minor** chords are written with a lowercase numeral.
- **Diminished** chords are marked with `dim` or the diminished symbol (`°`) beside the numeral.
- **Augmented** chords are marked with `aug` or a plus sign beside the numeral.

The seven diatonic chords of A minor in Roman numerals are therefore `i`, `ii°`, `III`, `iv`, `v`, `VI`, and `VII`.

Not everyone uses the same system for notating these chords, and the difference matters when you read progressions written by other people:

- **Parallel-major reference (common in the West).** Parallel keys are a major and a minor key that share the same tonic note. Chord names are written relative to the parallel *major* key, so in A minor you would see `i`, `ii°`, `♭III`, `iv`, `v`, `♭VI`, `♭VII`.
- **Relative-major reference (used by the Chinese Touhou-style community).** Relative keys are a major and a minor scale that share the same key signature. The relative major of A minor is C major, so the same seven chords are written as `vi`, `vii°`, `I`, `ii`, `iii`, `IV`, `V`.

In later lessons you may see progressions containing chords that are not diatonic to the minor scale — for example a **major 5** instead of a minor 5. This is called **modal interchange**: borrowing from other scales. In A minor, a major 5 means building a major chord on the fifth degree of the scale, which gives E–G♯–B. Note that G♯ is not part of the A minor scale.

## 4. Chord Progressions

### What a progression does

A **chord progression** is exactly what it sounds like: some chords in an order. In Touhou-style music the chords are played on background instruments such as strings or piano, while the melody is played over them.

Different progressions give off different moods. In fact, the same melody can feel completely different when harmonized with a different progression. For the result to sound good, however, the chords and the melody have to harmonize, which means each chord heavily influences which notes you can place over it. Choosing the right progression is therefore extremely important.

### Chord functions

Before writing progressions, you need to know how each chord functions in its own scale. The diatonic chords of the natural minor scale fall into three functional groups:

- **Tonic** chords have a sense of resolution. It is typical to end a song on a tonic chord, and the 1 chord has the strongest sense of resolution of the three, followed by the 3 and 6 chords. Tonic chords are built on a stable degree of the scale.
- **Dominant** chords have a huge instability, and therefore a need to resolve somewhere — especially to a tonic chord.
- **Predominant** chords lack the stability of tonic chords, and instead drive toward the dominant chords.

These are general tendencies rather than rules. You can, for example, lead a dominant chord to a predominant chord if it works for you.

### The 6–7–1 progression

Now that chords can be named in relation to the active key, we can look at a progression that is quintessential to Touhou: **6–7–1**.

- **6** is treated as a tonic chord with a tendency to move toward other chords, which is what we want at the beginning.
- **7** is a dominant chord that really wants to resolve back to the tonic.
- **1** is the tonic chord with the greatest sense of finality.

In that order, the three chords make a very nice miniature story. But it is only a starting point. There are several ways to extend 6–7–1: you can add extra chords at the beginning, or in between the existing chords, as you see fit.

### Extending and varying 6–7–1

**The anime progression (6–7–minor 5–1).** Inserting a minor 5 before the 1 packs extra emotional impact in just before the tonic resolves. A common shape is one round of 6–7–5–1, followed by a straight 6–7–1 for a clean resolution.

**Defusing the ending (6–7–1–7).** Following the 1 with a 7 removes the sense of finality. This is useful when you do not want the phrase to sound laid back and meandering; in that case the composer loosens the tension through a 6–7–1–7 shape before finishing with 6–7–1 to bring the phrase to a satisfying conclusion. This kind of meandering progression also works in the other direction, leading back into the loop.

### The raised seventh degree

When the natural minor scale was introduced, one detail was left out: **we like to raise the seventh degree of the scale by a semitone.** In A minor the seventh degree is G; raising it gives G♯. Using this raised note in melodies and chords gives a much greater sense of tension that wants to resolve — usually toward the first degree of the scale. (This alteration is what turns the natural minor scale into what is usually called the harmonic minor scale.)

This leads directly to the **major 5** chord. Because the seventh degree is raised, the chord rooted on the fifth degree turns from a minor triad into a major triad. Returning to the anime progression, replacing the minor 5 with a major 5 gives it a more triumphant feel.

You could make roughly ninety percent of all Touhou chord progressions using just **1, 7, 6, minor 5, and major 5**. If you are just starting out, you may want to stick to those chords for now.

Another chord that accepts a raised seventh is the 7 chord, which becomes the **diminished sharp 7** (`♯vii°`). Because it is a diminished chord, it has an even higher sense of tension than a dominant chord, and it often sounds dark or mysterious. Replacing the 7 of the standard Touhou progression with a diminished sharp 7 gives a darker feel. However, the jump from 6 straight to a diminished sharp 7 is quite large, so you can instead insert a 7 in between — 6–7–♯vii° — to smooth out the movement, which gives the progression more of a fantastical feel.

### Ending phrases

You can also swap out the chord that ends a phrase. Instead of the 1, you can try chords such as the 5 or the major 5, which have a more incomplete feeling — useful when you want the phrase to lead smoothly into the next one.

Remember that dominant chords want to resolve to a tonic chord. If you end a progression on the 5 chord, you want the next progression to begin with a tonic chord. The 1 provides the strongest resolution, but the 6 also works.

During sections where there is not much happening developmentally, such as bridges, you might simply want to slowly hop between the 1 and the 6.

### Inversions

When you first construct a chord using the method from the previous chapter, the reference note — the root — sits at the bottom of the chord. Sometimes you will want to move the notes around.

Take the A minor chord, A–C–E. Move the root note to the top of the chord and you have the **first inversion**: A moves up, so C is now the lowest note. Do it again, moving what is now the root to the top, and you have the **second inversion**; the root position's fifth note becomes the second inversion's lowest note. In other words:

| Position | Lowest note | Notes, low to high |
| --- | --- | --- |
| Root position | root | A–C–E |
| First inversion | third | C–E–A |
| Second inversion | fifth | E–A–C |

Inversions are useful for several things:

- **Smoother voice leading.** In a 6–7–major 5–1 progression, putting the major 5 chord in first inversion makes the notes move much less from chord to chord, which sounds smoother.
- **Less boring repetition.** When the same chord repeats back to back, inverting one of the two makes the progression less monotonous.
- **Chromatic bass lines.** This is one of the coolest uses. The main melody of Junko's theme uses inversions to obtain a chromatically descending bass line; if the bass instead played the roots of the chords, the line would be all over the place.
- **Keeping instruments in range.** Most instruments have a set range in which notes sound good. When you change key, notes may end up over range, or the bass may lose its power as everything shifts up. Inverting some of the chords moves the material back into range.

### The Picardy third

A **Picardy third** is when the final 1 chord is replaced with a major 1 by raising its third. It makes the progression end on a more hopeful note, and it is often used at the end of a section, because a major 1 sounds more conclusive than a minor 1.

### Melody first or chords first?

Composers often ask whether to write the melody or the chords first. The answer is: it depends.

- If you already have a melody in mind, start with the melody and find a progression that fits it. Most melodies imply a chord progression, so once you are experienced you tend to write melody and chords at the same time. If you are having trouble, play the melody over a variety of different progressions until you find a combination that sounds good together.
- If you do not have a melody in mind, start by choosing a chord progression and write a melody over it. Each progression places different restrictions on the notes you can use over it. That might sound like a disadvantage, but it actually makes melody writing easier, because you have fewer options to consider.

How to write that melody over a progression is the subject of the next chapter.

## 5. Writing a Melody

### Why melody matters

Touhou-style music is very **melody-focused**, as opposed to ambient. The melody is what determines how memorable the song will be, and it is what every song is immediately judged by. A storm of bullets really only says so much, there is dialogue in only rare, select moments of stages and just before and after boss fights, and that leaves it up to the music to vividly portray what is going on.

As a basic check, ask yourself:

- Am I able to hum this melody to myself?
- Can I see myself listening to it often?

If you cannot answer yes with confidence, the melody is probably not catchy or memorable enough.

This is the part of composing that relies on instinct the most. What follows is a list of do's and don'ts, but ultimately it is up to you to come up with something good. Do not worry if you feel you lack musical instinct: it is something you can develop through experience, and a good way to improve is to **transcribe songs that you like**.

### Starting and ending the phrase

If you came from the chord-progression chapter, you have a progression to work over. The next step is to build **checkpoints**: for each chord, decide which melody note lands on it, then fill in the blanks in between.

- Start the melody on a note from the first chord. If the first chord is a tonic chord, the first degree of the scale is usually your go-to choice.
- Do the same for all the other chords in the progression to create the checkpoints.
- If your final chord is a tonic chord, ending the melody on the first degree will most likely be the most satisfying option.
- If your progression ends on a dominant chord, it is equally satisfying to end on any of that chord's notes — in other words, the 5th, 7th, or 2nd degrees.

Remember that the dominant wants to resolve to the tonic, so it may be in your best interest to think one step ahead and plan both the note you use over the dominant chord *and* the note your next phrase will start with.

### Filling in between the checkpoints

The safe option is to keep using chord tones in between. Using only chord tones is quite limiting, though, not to mention boring — so you will want to add variety by sprinkling in notes that are not in the chord, called **non-chord tones**.

With so many notes to choose from, a good rule of thumb is to **stick with notes from the pentatonic scale**. The strongest notes of the scale come from the corresponding pentatonic scale: in a minor key, the **first, third, fourth, fifth, and seventh scale degrees**. In A minor those are A, C, D, E, and G.

- You can linger on a pentatonic note without the urgent feeling that it wants to move somewhere, which you would get from holding a note outside the pentatonic scale.
- As a bonus, melodies built from these notes tend to harmonize well over the majority of the chords in the key. A lot of catchy pop melody writing leans on this fact heavily; if you have ever wondered why so many of those melodies sound catchy, it is because they are built almost entirely from the minor pentatonic scale.

One note to be extra careful about is the **sixth scale degree**. It is often perfectly fine to use, but it has a quality that makes the melody want to resolve somewhere, so it is best thought of as a **weak note**. The safest way to use it is to avoid making it an important note, unless the underlying chord is a sixth chord; otherwise there is a chance your melody will become dissonant against the progression.

If your melodies feel too weak, try putting more focus on notes from the pentatonic scale.

### Raised notes must be raised in the melody too

As mentioned earlier, the note that is most commonly raised is the seventh, as in the major 5 or the diminished sharp 7 chords. You can think of those chords as borrowing notes from an adjacent scale, and you want your melody to borrow from the same scale. In other words, **for every raised note you use, make sure that note is raised in both your chords and your melody**; otherwise it will sound noticeably dissonant.

### Melody against chords

The rule that you should only use notes from your chosen key, in order to prevent the song from turning into a dissonant mess, has a softer analogue for the melody-versus-chord relationship: it is best if your melody prioritizes notes from the current chord, but when you play a note that does not match the chord, you do not have to worry too much about creating dissonance. There is a fairly low chance of landing on a note that actually clashes.

For example, within a 6–7–1 progression the melody played over 6 can often be repeated over the 7 chord without much problem, even though it does not match the 7. There is a whole body of theory about non-chord tones, but it boils down to this: **as long as it does not sound too dissonant, it is fine.**

The best advice is to use your ears. Listen to your own melody and ask:

- Is this too dissonant?
- If it is, is it because of notes from outside the chord?
- Is there a chord progression that works better with this melody than what I currently have?

Depending on what kind of change you make, you may also have to rewrite adjacent parts of the song so that everything still flows well. If you have a hard time judging whether something is dissonant, ask other people to listen and give feedback — even good composers run into this problem after they have listened to the same song on loop for long enough. When in doubt, err on the side of caution.

### Rhythm

Rhythm is just as important as pitch — in fact it is probably even more important than pitch when it comes to determining the mood of a song. As with everything else here there is no hard and fast rule for writing good rhythms. You generally want some variety in your note lengths, but depending on what the song is trying to do, a melody can still sound good even if it uses mostly one note length.

If you think of writing a melody as telling a story, you want your rhythm to make sense with the taste of the story you are telling. To come up with a rhythm, it helps to be able to count rhythm. A common way to do this is to tap your desk, clap your hands, or tap your feet to the pulse of the background.

Once you have a steady pulse going, you can begin to build your rhythm: tap harder on certain beats to **accent** them, and vary when the accents fall by changing the timing. Experiment with different accents and different intervals between accents.

A simple trick to spice up a rhythm is **syncopation**: placing rhythmic accents where they would not normally occur. A melody whose notes all sit on the main beats will sound dull and plain because of the lack of rhythmic variation, so accenting between the main beats adds spice.

One rhythm in particular is a signature Touhou rhythm. It shows up everywhere in Touhou music, it is great at providing a sense of forward momentum, and it includes a bit of syncopation, with its second note falling between the second and third beats. For extra fun, play it over something with a steady quarter-note pulse to really bring out that syncopation.

A related pattern has cropped up more recently in Touhou soundtracks; it carries a strong forward feeling, and it can be regrouped in a way that makes its structure easier to see. (The exact syllables used in the lesson to describe these two rhythms did not survive transcription — see [Transcription notes](#transcription-notes).)

### Repetition

A useful trick to reduce the amount of work is **repetition**. It also helps to ingrain a particular melody in the listener's mind. Find a melody fragment you like, then copy and paste it: you can keep the same rhythm while changing the pitches, in order to fix whatever problems the original fragment had.

Too much repetition, though, will make a song sound boring, so keep its use under control.

### Melodic contour

The melody in your song is like the plot in a story, whether that story is a book, a movie, a manga, or an anime. The most important principle about writing a good plot is that **good plots move, and they move with purpose**. Do not just let your melody wander around wherever it wants; even the wackiest melodies have a pattern to their movement — a method to their madness.

One way to see that pattern is to picture a **contour line** that roughly outlines which pitch levels your melody takes. The contour line shows you:

- which parts of the melody contain more or less movement,
- the range of your notes, and
- the points at which the melody reaches certain pitch levels.

In general you want the right balance of activity level and melodic range. Too much or too little of either and the listener will have trouble following along. You can view the contour line as a roadmap for your melody: just walking around may leave you lost, so it is good to plan. But just as a roadmap only hints at which streets to walk down and where to turn, you can choose to take any turns in your melody — as long as the melody arrives at your checkpoints and destinations in a timely manner.

Planning too much can make you feel too restricted, which is why most people do not literally compose with a contour line in mind. Once you have enough experience you tend to have a feel for what works and what does not. Until then, you can use the contour-line method to double-check your melodies for awkward points.

## 6. Melody Structure and Song Form

### Antecedent and consequent

In a typical melody structure, the music is separated into two parts: the **antecedent** and the **consequent**, or in plainer terms, a **question** and an **answer**.

**The antecedent** ends by sounding interrupted or unresolved, so as to drive the music forward into the next phrase. This is often done by ending on a fifth-degree note, also known as the **dominant note**, which sounds unresolved. Since dominant chords are likewise very unresolved, they are a good choice at the end of an antecedent.

You do not always have to end an antecedent on a dominant note or chord, though. There are Touhou examples of antecedents ending on a variety of degrees, and you may encounter antecedents that end with a dominant melody note while the underlying chord progression plays a tonic chord.

**The consequent** usually sounds resolved, so as to properly conclude the melody. This is often done by ending on a tonic note over a tonic chord. It is not necessary to resolve at the very end of a consequent, though: when you are trying to lead into a new section, the melody may resolve just a little before the end of the measure, leaving room for an **anacrusis** — in non-fancy terms, a **pickup**: the notes that lead into a melody or section before the section's first measure.

**Worked example.** The series transcribes a melody line from the theme it refers to as "Doremy's theme", in the key of A minor. In A minor the dominant note is E, so the antecedent ends on E, over the dominant chord (major 5), which gives that part of the song an unresolved feel and forward momentum. The consequent then ends with the tonic note A over the tonic chord (1), which brings the melody to a satisfying conclusion.

### Sections and melody forms

A **section** of a song is a group of melodies that are different enough from the surrounding groups that the listener senses the song has moved somewhere.

Suppose your section is 16 measures long. You can split it into four segments, labelled according to their content. For example, if your melody has an antecedent and a consequent that are both four measures long, the melody is eight measures long in total, and you can repeat it as-is to fill the 16-measure section. Labelling the antecedent as **A** and the consequent as **B**, the section you have created has an **A B A B** form.

Various other structures exist. One example is an **A A C**-type form, which is similar to A B A B in that the melody is repeated until partway through, at which point it ends in a completely different way. Looking across these forms, the common factor is that they all contain some amount of repeated material. Repeating part of a melody is good, because it ingrains the melody in the listener's mind; on the other hand, too much repetition will make your song boring, so keep it under control.

### Structuring a whole song

As with writing a single melody, there is no strict formula for linking sections together. There is, however, one big criterion you need to fulfil: **make your song flow well.**

The first step is to categorize your sections into those that should serve as the main **verses**, and those that serve as **bridges** between the verses. Like mixing up moves in a fighting game, you need to mix up your sections by linking verses to bridges, which then link to other verses.

Continuing that analogy: a song that seems to consist of only verses is like trying to win with one string of inputs. It only works for so long before it becomes boring and weak. Your song needs to have **direction as a whole**, not just in its individual melodies; mindlessly linking verses and bridges without regard for flow will make the song sound aimless or lacking in purpose, which goes against the main point of Touhou-style music — maintaining movement in a focused manner.

A workable shape for a song:

1. Designate one section to start the song off; that is the **introduction**. An intro can be either a verse or a bridge — it simply leads into whatever it is not.
2. From there, alternate verses and bridges, paving the road toward the final verse, known as the **climax**.
3. The climax can lead into a lower-activity **outro**, which is entirely optional.
4. Most Touhou songs loop, so pick an appropriate **loop point** to return to after the final section of your song. Loop points tend to be anywhere from the introduction to the first verse.

If you are looking at all of this and wondering where to even start: it is quite simple. **If you do not know what to do for your song structure, rip it from some other song.**

### Spicing up a section by changing key

Suppose you have a section that seems like the right length, but it feels as though it is dragging on for too long. One way to avoid that feeling is to change the key being used, starting from the second half of the section. The theory term for this is **transposition**: moving a set of notes up or down a certain interval. In this case you are transposing an entire half-section's worth of notes.

Changing keys affects the mood of the song, because of the contrast between the previous key and the new one. It gives the feeling of a new section without actually creating one.

There are fancier ways to change keys, which later lessons cover. The most common intervals to transpose by are the **minor second, the minor third, and the major third**, with the minor second especially common in general.

- Transposing a song **upwards** helps create more energy and makes it sound brighter, because the pitch is higher.
- Transposing a song **downwards** has the opposite effect.

The most common place to transpose is when the final iteration of the climax melody begins. You can also transpose at the midpoint of a bridge.

## 7. Harmonizing a Melody

### Harmonizing is not the same as transposing

**Harmonizing** means layering extra notes onto a melody without making a completely new melody. (Writing a second, independent melody over the first is **counterpoint**, which is a separate topic.) Harmonizing is a quick and easy way to give your melody extra punch: it makes the melody sound fuller and helps it stand out more from the other instruments.

A secondary sequence of notes played together with the melody only counts as harmony **if it actually harmonizes with the melody**. If it does not, it is just a distraction.

Unfortunately, many new composers arrive without prior music theory knowledge, so they transpose their melody down by some interval and call it a harmony. That makes the transposed line harmonize with the melody at only one or two notes here and there. Do not just transpose your melody and be done with it: it will sound odd for the majority of the time, and people will notice.

### The harmonization checklist

This is the thought process for harmonizing a melody properly, in order. For each melody note:

1. **Is the note part of the chord?** Harmonize down to the next chord note.
2. **Can you harmonize a third or a sixth down without sounding too dissonant?** Do that.
3. **Is the note a perfect fourth above a note that is in the chord?** Harmonize the fourth down.
4. **None of the above?** Try to harmonize down to the closest chord note.
5. **Still sounds bad?** Make some changes to the melody, and try again.

This later becomes the general yardstick for how well any two notes work together: the further down the list the relationship falls, the less properly the two notes harmonize.

### Walking through an example

Take a melody over a chord progression and work the checklist in two passes.

- **Step one: harmonize chord notes with other chord notes.** While doing this, watch out for intervals that repeat: in the third measure of the series' example, the harmony switches from thirds to sixths specifically to avoid parallel fourths and fifths.
- **Step two: harmonize the non-chord tones.** Harmonize as many of them as possible with thirds or sixths, choosing harmony notes that are also in the scale you are working in, and leave out the ones that sound dissonant. Astonishingly many notes harmonize well this way.
- **Finally, the leftovers.** In the example there are two melody notes remaining whose fourth-down note exists in the underlying chord, so those get harmonized a fourth down.

The result is a melody harmonized in a way that actually sounds good.

### Parallel fourths and fifths

Successive perfect fifth intervals can sound rather bland. The proper term for that is **parallel fifths**, and successive perfect fourths have the same problem. Anyone who has studied four-part harmony will have been taught not to let any two voices contain parallel fifths, and the same applies in Touhou-style composing.

One important disclaimer: **avoiding parallel fourths is more of a personal preference than a hard rule.** It is a good rule to follow if you want your harmony to stand out on its own, and some composers care about that more than others. One of the presenters always harmonizes the closest note possible and does not care whether that creates parallel fourths; the other prefers harmonies that are more buried, and avoiding parallel fourths helps with that. Parallel fifths, in contrast, are usually an indication that something has gone wrong — if you are following the harmonization guide properly, you really should not end up with parallel fifths.

An easy way to avoid parallel fourths and fifths is to employ **contrary motion** between the harmony and the melody, that is, having the harmony move in the opposite direction to the melody:

- If the melody and harmony move **toward** each other, they will likely form a **third** interval.
- If they move **away** from each other, they will likely form a **sixth**.

### The parallel-fourth trap, and how to avoid it

The trap to look out for is when your melody plays the **root** note of a major or minor chord in the progression. The next chord notes below the root are the chord's fifths, so harmonizing a fourth down from the root produces parallel fourths.

Instead, harmonize a **sixth downward**, so that the harmony plays the chord's thirds rather than its fifths. This is the strategy already hinted at in the example above.

### Octaves, seconds, and sevenths

**Octaves.** Harmonizing by octaves is not recommended as a way of supporting the melody, because it adds no additional flavor. However, **doubling a part** up or down an octave is a great way to make your song sound fuller — it just is not harmony in the sense we mean here.

**Seconds and sevenths.** Depending on the context, harmonizing by seconds and sevenths can be situationally good or bad.

- Harmonizing by a **second** works when the interval creates suspense rather than a clash. A melody note of D♯ over a C♯ chord that has no third, harmonized by the second (C♯), creates a suspenseful feeling that is not bad at all; this is most noticeable in the intro of "Pure Furies". By contrast, a melody note of G over a D major chord, harmonized with the second (F♯), creates a clashing sound that does not sound great.
- Harmonizing by a **seventh** works best when your melody and chords imply a seventh chord. In the first melody section of "The Concealed Four Seasons", a C♯ melody note is used over a D major chord, which implies a D7 chord, and harmonizing a seventh down is fine there. If the seventh interval feels too jarring, consider adding a second harmony note in between; the third or the fifth are good suggestions.

If none of that made sense to you, do not worry about it — you should not be harmonizing with seconds and sevenths anyway until you really know what you are doing, and even then it remains quite uncommon.

### Harmonizing above the melody

You can also put harmonies above the melody note. Since melodies usually sit in the higher range, though, there is a chance your harmony ends up outside the acceptable range. That is why it is safer to harmonize with **thirds** more often than with sixths, which reach higher. Other than that, the rules for harmony apply the same way when you harmonize upwards as when you harmonize downwards — including the warning to beware of parallel fourths and fifths.

## 8. Instrumentation and Arrangement

### Working without a fixed sound source

This chapter assumes no specific sound source, so most of the names mentioned are real instruments. In practice:

- If you work with a DAW's built-in patches, the patch list is most likely named directly after the real instruments each patch represents.
- If you use a soundfont or a sound module, the instruments may have less descriptive names. Soundfonts that cover the entire set of General MIDI instruments will almost certainly be ordered the same way you would get when working with MIDI directly.
- Some sound sources contain multiple versions of the same instrument under the same patch number.

### The four groups

At any given point in a song there are four main groups of instruments in use: **melody, progression (chords), bass line, and drums**. These four groups occupy different frequency niches without overlapping too much, and that separation is what keeps the mix readable.

### Melody and harmony instruments

For melodies and harmonies you can use whatever instrument you want. Trumpets and saxophones may be the first thing to come to mind, but square leads and sustained leads are also common choices, and you can try violins, flutes, or glockenspiels as well — it depends on the energy level your melody has. Anything goes, although you do need to make sure the instrument has enough **presence**.

Also consider having more than one instrument play the same melody, to reinforce the strength of the sound.

### String ensembles

String ensembles are sound-filling instruments, and are therefore great for occupying the **mid-range** frequencies of a song. They usually play the chord progression as **block chords**. If three chord tones do not sound thick enough, you can stack one extra chord tone onto the string chord for a total of four notes at once, which thickens the sound.

Make sure the strings do not compete too much with the other instruments frequency-wise, and experiment with different **inversions** to find the range in which the string chords fit best. Between the melody and the bass, you can also distribute notes that would otherwise be played by the string ensemble to other mid-range instruments such as accordions and church organs.

### Arpeggiated chords for intros and climaxes

For comp sections — intros, or climax portions that explode instrumentally later — consider playing an **arpeggiated** version of the chord progression using a piano or a harpsichord.

### Bass

Every Touhou song contains some form of bass. The bass fills in the lower ranges of the song, which gives punch to its sound.

- Play notes from the chord, especially the chord's **root and fifth**.
- During **slow sections**, you can simply hold the root note of the chord.
- During **faster sections**, alternate between the root and the fifth of the chord. This raises the energy level without affecting the harmony too much.

Some synthesizers include a quick way to get a bassline with minimal effort, in the form of **arpeggiator** functionality: by playing a root note in MIDI, the synthesizer plays back an entire pattern for the duration of that note. Ten Desires makes extensive use of this type of bass.

### Drums overview

Percussion has a low pitch content, but it is very important: it provides rhythm, energy, and a strong beat that keeps the whole piece together. Many percussion instruments exist, but for Touhou music the focus is on:

- **kick**
- **snare**
- **hi-hats** (closed, open, and pedal)
- **crash**
- **ride**
- **toms**

How you use percussion is an effective way to raise or lower the energy level of a section. Touhou percussion resembles rock percussion in its role.

**Kick.** The kick gives the main beat rhythm of the music. In a 4/4 song you generally want kicks on the **first and third beats**, which are regarded as the strong beats; the second and fourth beats are the weak beats. Since the bass also serves to keep rhythm, it is a good idea to play your kicks as a supplement to your bass line.

**Snare.** Snares naturally have a powerful sound, so they are good at defining segments that need extra power and energy, and at highlighting. They can overpower your other instruments if they are not well controlled, so be careful. Snares often go on the **second and fourth beats**.

**Hi-hats.** Touhou mainly uses closed and open hi-hats.

- **Closed hi-hats** are often played in quick succession, usually two or four times per beat. Because they sound light, spamming them in that way does not impact the song much, and Touhou does exactly that.
- **Open hi-hats** sound a little stronger, so they are usually only played about as often as snares are.
- The **pedal hi-hat** also exists, but does not show up very often in Touhou music. You can substitute a single closed hi-hat for a pedal hi-hat every so often for variety, especially when playing hi-hats in quick succession.

**Crash.** Crashes sound extremely powerful, so most of the time you only use them on the **first beat of an 8-bar section**, when you are transitioning to the next section. You can also play one on either the first or third beat of the measure immediately before that next section.

**Ride.** Rides are a light, lingering tapping sound, in contrast to the more explosive crash. This means you can use them more often than crashes without the percussion section turning into a mess. Rides are versatile: you can use them like a less intense hi-hat, and a ride can be played alongside every few kicks.

**Toms.** Toms are the drums with the most definite pitch; how many different tom pitches you have depends on the drum set, but generally there will be four. Toms are useful for leading into or ending a high-energy section, for transitioning between sections, and for looping your melody. A flurry of percussion notes is the recommended approach: place a crash at the beginning or middle of the last measure of a melody, and roll the toms in descending pitch, preferably with the highest-pitched tom starting half a beat before the middle of the measure.

### Drumming for different energy levels

**Low-energy sections.** Play fewer drum hits to match. Putting kicks on the first and seventh beats of every two measures is a useful way to keep the energy level low. Snares can be cut down to once per measure, on the third beat. Open hi-hats can also be cut down to once per measure — and since open hi-hats are not as strong as snares, you can keep playing them.

**Rising energy.** Play drum hits more often, which is especially true in the second half of the climax section. Generally you would play kicks and snares twice as quickly. You can choose to play open hi-hats in between every beat, with closed or pedal hi-hats in between the open hi-hats.

### A simple method for slow Touhou-style drums

This recipe builds a slow drum part up in layers.

1. **Start with the skeleton.** Kick and closed hi-hat on each beat, and an open hi-hat spaced evenly between each beat, on the half-beats:

   | | 1 | 1& | 2 | 2& | 3 | 3& | 4 | 4& |
   | --- | --- | --- | --- | --- | --- | --- | --- | --- |
   | Kick | ● | | ● | | ● | | ● | |
   | Closed hi-hat | ● | | ● | | ● | | ● | |
   | Open hi-hat | | ● | | ● | | ● | | ● |

2. **Choose a snare pattern.** The basic slow pattern places snares on beats 2 and 4:

   | | 1 | 1& | 2 | 2& | 3 | 3& | 4 | 4& |
   | --- | --- | --- | --- | --- | --- | --- | --- | --- |
   | Snare | | | ● | | | | ● | |

3. **Fill in more kicks.** This is something you eventually get a feel for: you want the added kicks to fit the song without becoming too overwhelming.
4. **Optionally fill in closed hi-hats.** You can do pretty much whatever you want here. Touhou has been seen filling every sixteenth-note space that is not occupied by an open hi-hat with a closed one; a good alternative is to emphasize the areas that were *not* filled in by the kick drum.
5. **Add in some fills,** and the pattern is done.

### Velocity

Sometimes a song seems to have the right amount of notes but feels too cluttered, while removing notes makes it too empty. **Velocity** lets you preserve the notes and instead fine-tune their presence.

Velocity is the second form of volume control for individual notes, and it represents the force with which the note is played: the higher the velocity, the greater the force and the louder the note sounds. By reducing the velocity of some notes, you leave more room for other sounds.

- Reducing the velocity of your **harmonies** is recommended.
- Some **hi-hat** notes can also be softened this way.
- It is worth playing with the velocity of **snares at the end of sections**.

### Panning

Picture an orchestra: all the instruments are stationed slightly apart from each other. Similarly, you can imagine every instrument positioned somewhere from the left to the right of the listener. By default, a newly created channel is panned to the **center** — which is like having all your musicians stand in a single file during the performance.

Panning an instrument to the left reduces its presence on the right side, and vice versa. In Touhou style we do not often pan instruments too far toward the extremes.

These are guidelines rather than rules; the presenters themselves break them often. If you are deliberately trying out other styles, such as swing, reggae, or Latin, those styles follow their own guidelines. All in all, experiment and use your musical instincts until you get something that sounds good.

## 9. Background Parts, Motifs, and Masking Dissonance

### Filling out the background

Once the melody, chords, bass, and drums are in place, you can add parts in the background, behind the melody. There are several standard kinds of background material, and they tend to live in different frequency ranges.

**Rhythmic material based on chord tones.** The easiest way to fill up the mid frequencies of a song is to introduce rhythmic material, usually based on the chord tones. It comes in many forms, such as arpeggios or strummed chords. The type you should use depends on the mood you want to convey: arpeggios, for example, are more commonly used in light-feeling sections. As always, experiment until you find rhythmic material that fits your particular song.

**Ambient sustained sounds.** A simpler alternative is ambient, sustaining sounds that play every measure or so. These would go on one of the strong beats. Good instruments for this include synth pads and tubular bells. Whereas the rhythmic material above occupies the mid-range, these ambient sounds occupy a **high** frequency range — often even higher than the melody itself.

**Short looping riffs.** A common way to fill up the moderately high frequencies is an instrument playing a short looping riff in the background. These riffs should not fight too much with the main melody, since they sit in roughly the same frequency range. In fact, they are barely noticeable in the final version of the song, but their presence still makes all the difference.

- Background parts at around **two notes per beat** are often simply arpeggiated versions of the underlying chord progression. The series cites a theme as a particularly popular example, though that one is on the complex side (see [Transcription notes](#transcription-notes)).
- **Faster rolling notes** — about four notes per beat — are another option. Acoustic piano is the most popular choice for these; if the rolling notes are meant to stay in the background, violins or harps are better. If the range of the part is narrow, you can use electric pianos or harpsichords instead.

These fast background rolls are often called **note spam**, with alternative names like **piano spam** or **crazy piano**, because they are usually played on a piano. Despite the colloquial name, it is not actually supposed to be as chaotic or random as the word suggests, and it is very much a welcome thing to have in a song.

One last note: the kind of accompaniment being discussed here is meant to be on the **simple** side. In general, the more complex you make your accompaniment, the more it starts to behave like counterpoint, and the more equal in presence you would want the two lines to be.

### Reusing material as a motif

If you are having trouble coming up with something to add as a new layer to a particular section, you can try reusing material that you already introduced earlier in the song as a **motif**. Stage themes in particular are often built this way: a motif introduced early can later be heard being accompanied by the main melody, and a motif can be brought back later in the song for a new section.

A practical method is to create a riff for the intro, then copy and paste the entire riff into a later section. This usually works quite well, though you will often encounter clashes in the rhythm or the notes between the motif and the melody if you insert the motif as-is. That is to be expected: just as a melody will not magically harmonize with itself when it is simply copy-pasted and transposed by some interval, the same applies here. Adjust the motif's notes based on the chord progression of the melody section, and decide whether to use the technique at all based on how workable the result is.

Most importantly, remember that **too many unique sections**, each of them fine in isolation, will only impede the memorability of every individual part — and that is bad for your song. That is why setting a motif is important, especially in longer songs.

Of course, you should avoid writing a song that is long in the first place: when it comes to Touhou-style composing, quality vastly trumps quantity. Take final boss or extra boss themes as an example. They are usually the longest-lasting songs in a soundtrack, at around three minutes to three minutes fifteen seconds in length — but the amount of unique content in such a song only adds up to around two minutes to two and a half minutes at most. That is about a minute or so of reused content. On the off chance that you are required to drag a song out for some reason, having a sense of unity will ultimately be your saving grace. As a bonus, setting a motif early on also gives you a base for coming up with later melodies during the composing process.

### Additional lines must harmonize with both melody and chords

Just as you have to be careful about which notes you use over a chord, the same principle applies to any additional lines you add: your accompaniment needs to harmonize with **both** your main melody and your chords.

That sounds like a lot to keep track of, and it is — but here is the secret: **as long as it does not sound too dissonant, nobody cares.** You are not writing a theory exam, and nobody is marking you down for improper harmony. As long as it sounds consonant enough, you are fine.

In general, the less melodic and the more out of the way your background element is, the less you have to worry about it harmonizing with the melody — though you still need to worry about harmonizing it with the chords. Rhythmic material is a good example: you almost do not have to worry about how it lines up with the melody, unless you made it unusually attention-grabbing for some reason. Reusing a motif at the same time as the melody is the opposite case: it demands a lot of care, because at that point it is basically counterpoint.

To gauge how properly two notes harmonize, refer back to the harmonization checklist from the previous chapter: the further down the list the relationship falls, the less properly the two notes harmonize. And remember that improper harmony is not automatically wrong — it just means it is harder to make sound consonant.

### Masking dissonance through arranging

If something in your song is teetering on the edge of two dissonant, there are ways to **mask** the dissonance — that is, to make it less noticeable than before.

The best way is to change up your composition and remove the dissonances outright. If you are unwilling or unable to do that, you can adopt the following methods, roughly in this order:

1. **Space out your frequencies.** Allocate your melody, chords, bass, and everything else to different octaves. You will inevitably use long chord tones in your melodies, and when the melody sits in the same range as the chords, those long chord tones clash with the progression. Spacing the melodic layers apart reduces the dissonance between them.
2. **Rearrange the voicing of the chords.** If your chords sound dissonant, try different inversions. A seventh chord on D, for example, will sound more dissonant in one voicing order than in another.
3. **Reduce the velocity of the offending notes.** If that does not help much and you do not want to alter the composition, identify the notes causing the dissonance and lower their velocity, which makes them less obvious. The series gives an example in which a song's motif is set against the main melody of "Electric World" and clashes with it; altering the motif would disrupt its flow, so the best solution is to reduce the velocity of the notes causing the dissonances.

Unless the dissonance is super obvious, or your listener has super ears, you do not have to worry about small details like these. Human beings usually listen to a song as a whole rather than focusing on every single aspect.

The **choice of instrument** also affects how noticeable dissonances are. If the instrument used for a motif naturally sounds unusual or abnormal, your listener might simply think the dissonance is part of that instrument's timbre rather than a flaw in your composition. That said, it really depends on how dissonant your composition is: if it is super obviously dissonant, altering the composition is highly recommended; otherwise you can adopt the methods above and shrug the dissonances off.

## 10. Where To Go From Here

That completes stage one. By now the series has covered everything you need in order to start composing your first song.

The best way to get good at music is simply through experience. Once you have composed for long enough, you start to develop an intuition for what works — intuition that is much harder to explain in text than to acquire by doing. So pick up your DAW and start composing.

- **Do not wait on gear.** You do not need to wait for a large sample library to finish downloading or for a hardware sound module to arrive in the mail before you start writing. Arranging can come later; right now you need to develop good compositional skills. If your composition is bad, fancy instruments are not going to save it.
- **Compose for a project to stay motivated.** Tying each song to a pre-existing scenario — for an album, a fan game, or something similar — forces you to do something new and unique for each song. Even if the project itself crashes and burns, you will have gained valuable experience, and possibly some songs you can upload.

Stage two covers topics and techniques that are cool but not absolutely necessary in order to make a good song.

## Transcription notes

The source files are raw speech recognition output, and a few specific names and details did not survive it. Where the intended meaning was clear from context, it has been restored; where it was not, the item is listed here rather than invented.

**Restored with confidence**

- The transcripts' "dot", "duel", and "doll" are all the spoken word **DAW**; "piano raw / role / row" is **piano roll**; "mini editing" is **MIDI editing**.
- "jungkook's theme", "pure fairies", and "the concealed four seasons" are read as the Touhou track names **Junko's theme**, **Pure Furies**, and **The Concealed Four Seasons**, the last two being confirmed by the surrounding descriptions.
- "acquire odds and trick organs" is read as **accordions and church organs**, the best phonetic match for the mid-range instruments being described.

**Uncertain — verify against the original audio if it matters**

- **The two signature rhythms** in Chapter 5. The lesson demonstrates them and speaks syllable patterns aloud, none of which survived transcription. Only their described properties (forward momentum; the second note falling between beats 2 and 3; regrouping) are recorded here.
- **"dori's theme"**, cited as the source of the A-minor antecedent/consequent example. Rendered as "Doremy's theme".
- **The structure example in Chapter 6**, spoken as something like "the Fallen Hero Legend of Sanada". The title and therefore the reference song are unknown.
- **The Touhou game numbers** in the motif examples: "Touhou 6 stage 1" is clear, but the other example ("Touhou 50 stage 4", motif reused 35 seconds in) has an uncertain game number.
- **The arpeggiated-background example** at two notes per beat, spoken as something like "CH Mario's theme".
- **Sound-source patch names**: "the romantic tp from the edro fc series" (a Roland SC-series patch name, rendered generically as a trumpet patch) and "reading mo rounds from the sd80 and sd90" (a saxophone patch name, rendered generically as a saxophone). Only the lesson's point — that cryptic patch names are often ordinary instruments — is recorded.
- **The instrument name** in the Chapter 9 dissonance-masking example, spoken as "incense cone". Its identity is unclear, so the passage is written without it.
- **The low-energy kick placement** in Chapter 8 is recorded exactly as spoken — "the first and seventh beats of every two measures" — which may itself be a mis-transcription.
- **Genre in Chapter 5**: "london music in particular really abuses this fact … why 11 melodies are so catchy" is rendered as "pop music", since the specific genre is unclear.
- **The 4b transcript ends mid-sentence** ("if you change keys in"). That chapter therefore stops at the last complete statement; any content the source video adds after it is not represented here.

