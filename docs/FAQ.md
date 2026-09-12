# Frequently Asked Questions

## Development Decisions

### How can I trust a file manager written with AI assistance on my filesystem?

YtreeNova performs standard file operations (rename, move, copy, delete) through normal OS APIs, just like other file managers.

This is the same trust model you should use for any file manager: do not assume zero risk, verify behavior, keep backups, and, at first, try it out in a non-critical directory.

For detailed safeguards, limits, and verification pointers, see [`TRUST.md`](TRUST.md).

### How is AI used in this project?

AI is used as an implementation assistant, not as an autonomous authority.

The human maintainer owns design decisions, architecture constraints, and merge quality. AI helps accelerate coding and refactoring work, but changes are accepted only after manual review plus repository QA gates. This workflow is iterative and often slow: a lot of the effort is in steering, verification, and correction.

For the concrete checks and evidence model, see:
- [`docs/AUDIT.md`](AUDIT.md)
- [`docs/TRUST.md`](TRUST.md)
- [`docs/PR_GATE.md`](PR_GATE.md)

### Why release an alpha before beta?

The alpha is published early so users and contributors can inspect the code, test real workflows, and provide feedback while major design decisions are still adjustable.

In short: the project is usable now, but not stable yet. Expect rough edges, occasional regressions, and evolving UX details until beta and then stable release.

### What is YtreeNova's relationship to Ytree?

YtreeNova started from Werner Bregulla’s Ytree v2.10 codebase, but it is now a separate line with its own name, command, repository, and release history. The aim is to build a Unix-like XTreeGold tribute that keeps the logged-tree workflow and extends it in its own direction.

A clean-slate implementation could have copied the same UI/UX, but it would still have meant rebuilding existing behaviour before adding the missing features. Starting from Ytree preserved a working logged-tree baseline and made it possible to focus on split-screen workflows, integrated preview/autoview, archive-as-directory operations, and a more modular C99/POSIX codebase instead of first rebuilding those foundations from scratch.

Werner’s Ytree continues to value portability across a broad range of Unix systems, including older environments. YtreeNova does not promise the same portability target; it currently aims at contemporary POSIX-style Unix systems and, so far, has mainly been tested only on a small number of recent Linux distributions.

### Why is this not written in Rust?

The primary objective of this phase was architectural cleanup and feature completion in the existing C codebase.

Switching languages immediately would have turned this phase into a total rewrite instead of continuing the existing codebase. However, now that the architecture has been simplified, legacy dependencies removed, and the project stabilized, a port to a modern memory-safe language like Rust is a possibility for a future version.

### Why ncurses, why not termbox2 or notcurses?

YtreeNova only needs fast, reliable text/line-box terminal UI for file and VFS browsing, and ncurses already provides that cleanly, while switching to termbox2 or notcurses would add backend complexity for features outside ytnova’s core scope (like richer in-app media rendering) that are better handled by external helper programs.

---

## Project Philosophy

### How is YtreeNova different from UnixTree?

YtreeNova and UnixTree are separate XTree-inspired projects with different histories, codebases, and design choices. They overlap in several user-facing goals, including logged-tree navigation, tagging, split-screen operation, preview/autoview, and archive handling, but they differ substantially in architecture and maintenance approach.

#### How do the YtreeNova and UnixTree architectures compare?

The difference lies in how they handle system dependencies:

**1. UnixTree: The Self-Contained Framework**
*   **Context:** Built for inconsistent environments (AIX, HP-UX, Solaris).
*   **Approach:** Bundles heavily modified internal libraries (like `libecurses`) to ensure it runs the same everywhere.
*   **Trade-off:** High consistency on legacy systems, but increased code size, complex build requirements, and high maintenance overhead.

**2. YtreeNova: The Modern Approach**
*   **Context:** Built for standardized **POSIX-compliant Unix** systems (Linux, *BSD, macOS).
*   **Approach:** Offloads functionality to shared, well-maintained system libraries:
    *   **Terminal:** `ncurses` (Industry standard).
    *   **Archives:** `libarchive` (Supports a wide variety of formats).
    *   **Input:** GNU `readline`.
*   **Trade-off:** Requires modern dependencies, but yields a significantly smaller, more secure, and maintainable codebase.

---

## Project Relevance

### Is there still a need for a text-mode file manager?

Yes. While graphical file managers are standard for desktop users, TUI (Text User Interface) tools remain vital for specific workflows:
*   **Server Management:** System administrators working over SSH need efficient tools that do not require a graphical environment.
*   **Efficiency:** For power users, keyboard-driven navigation is often significantly faster than dragging and dropping with a mouse.
*   **Minimalism:** Users of tiling window managers and lightweight distributions often prefer low-resource, terminal-based applications.

### Who is the target audience?

YtreeNova specifically targets:
1.  **XTree Veterans:** Users who developed "muscle memory" for the XTree layout and keybindings in the DOS era and find the Midnight Commander style unintuitive.
2.  **Terminal Power Users:** Developers and Admins who want a fast, lightweight file manager that integrates seamlessly with their shell history and standard CLI tools.
3.  **Open Source Archivists:** Those interested in keeping classic Unix tools alive, compilable, and secure on modern hardware.
