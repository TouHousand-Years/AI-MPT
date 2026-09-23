# AI-MPT

[简体中文](README_CHN.md) · English

AI-MPT is a music creation project based on [OpenMPT](https://openmpt.org/). See the [official OpenMPT source mirror](https://github.com/OpenMPT/openmpt) and [UPSTREAM.md](UPSTREAM.md) for the source revision used by this repository.

This project adds two features to OpenMPT's Tracker workflow:

- **Piano Roll:** View Pattern notes on a time and pitch grid, select and audition notes, and follow playback. Once experimental editing is enabled, you can insert, move, resize, copy, and delete notes; changes are written back to the Tracker Pattern.
- **AI / MCP collaboration:** A local MCP sidecar lets an Agent read Patterns, prepare candidate edits, manage the current Sequence's Pattern order, and request a different Pattern as its editing target. Edits stay in a proposal until the user reviews and applies the whole proposal in OpenMPT by default. Automatic acceptance is available only when explicitly enabled.

Piano Roll editing is off by default and remains experimental. For requirements, MCP client setup, and full instructions, see the [English User Guide](USER_GUIDE_EN.md).
