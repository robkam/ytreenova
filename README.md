# **YtreeNova: A File Manager for Unix-like Systems**

**YtreeNova** is a keyboard-first file manager for Linux and BSD with fast multi-volume logging and navigation.

> [!IMPORTANT]
> **STATUS: ALPHA (v1.0.0-alpha)**
> `ytnova` is in active alpha development. It is stable and usable, but expect rough edges, incomplete behaviour, and occasional regressions. Interfaces, key bindings, and configuration details may change before the first stable release.
>
> Before opening an issue or suggesting a feature, please check [BUGS.md](docs/BUGS.md) and [ROADMAP.md](docs/ROADMAP.md) first, so as not to create a duplicate if it is already listed.

## Quick links

- [CHANGELOG.md](docs/CHANGELOG.md) — release history and notable shipped milestones
- [USAGE.md](docs/USAGE.md) — user guide and key reference
- [quickstart.md](docs/quickstart.md) — developer setup
- [CONTRIBUTING.md](docs/CONTRIBUTING.md) — contribution guidelines
- See the [Documentation Guide](#documentation-guide) below for the full project documentation set

<p align="center">
  <img src="docs/screenshots/split_archive_home.png" alt="YtreeNova F8 split screen: left panel browsing /home/rob/scripts.tar.gz, right panel showing the ~ tree with scripts inactive and its files in a small window." width="84%">
</p>

YtreeNova logs filesystem hierarchies into memory, so you can work across whole volumes, not just one directory at a time. Use tree + file-list views, Showall/Global views, tagging, fast filtering, split-screen workflows, preview, and archive-as-directory operations (including in-archive copy/move/rename/delete/mkdir where supported).

## Background

YtreeNova is a Unix-like XTreeGold tribute project with Ytree v2.10 lineage. It began from Werner Bregulla's Ytree codebase in October 2025 and now has its own name, command (`ytnova`), repository, and release history.

It remains in the [XTree](https://www.xtreefanpage.org/lowres/x10dirja.htm)/Ytree family of full-tree logging and tagging file managers: scan a hierarchy first, then navigate, filter, tag, view, and operate across the logged set as a whole. YtreeNova focuses on feature completeness for Unix power users, with split-screen work, integrated preview/autoview, archive-as-directory operations, and contemporary POSIX-oriented internals rather than old-system portability.

Many file managers today function as "browsers"—they look at one directory at a time and rely on the OS to fetch files on demand. YtreeNova keeps the logger model: it scans ("logs") entire drive hierarchies into memory. This treats the filesystem as a database, allowing you to **Show All** files in a flat view, filter across thousands of subdirectories instantly, and perform bulk operations on tagged files regardless of their location.

## Development Methodology

This project is an experiment in AI-assisted systems engineering. The codebase was not simply "ported"; it has been reworked toward feature completeness and maintainability.

In practice, the human maintains design ownership and quality control, while AI is used as an implementation assistant. This requires substantial iteration, verification, and architectural guardrails for each meaningful change. The goal is to show that LLM-assisted development can still meet normal project standards when the process is disciplined, specification-driven, and strongly validated.

Quality claims are backed by repository-visible gates and documentation:
- [AUDIT.md](docs/AUDIT.md): required QA/audit loop and merge gates.
- [TRUST.md](docs/TRUST.md): safety posture, limits, and code-level evidence pointers.
- [PR_GATE.md](docs/PR_GATE.md): governance and merge-readiness checks.

## Why release alpha now?

v1.0.0-alpha is being published early so people can use the program, inspect the code, and evaluate the direction before beta. It is usable today, but still in active development: expect rough edges, occasional UX/workflow bugs, and ongoing refinement of some features.

## Features (v1.0.0-alpha)

*   **Classic XTree Interface:** Directory Tree + File List layout.
*   **Split Screen Mode (F8):** Manage two independent file panels side-by-side.
*   **File Preview (F7):** Instant view of file contents without launching external tools.
*   **Multi-Volume Support:** Log multiple drives or archives simultaneously and switch instantly.
*   **Archive Handling:** Read archives and, where supported, write/update archive contents via `libarchive`.
*   **Advanced Filtering:** Filter by RegEx, Attribute, Date, and Size.
*   **Configurable Color Themes:** Customize UI colors to match workflow preferences.
*   **Modern Architecture:** Clean C99, strict context-passing design — no global mutable state. See [ARCHITECTURE.md](docs/ARCHITECTURE.md).
*   **Auto-Refresh:** Inotify integration for live directory updates.
*   **External Viewers:** Associate specific file extensions with external programs (images, PDFs, etc.).
*   **User Commands:** Bind keys to custom shell commands/scripts for infinite extensibility.

## Installation

### Prerequisites

*   **C Compiler** (GCC or Clang; Clang is required for fuzz targets)
*   **ncurses** (libncurses-dev / ncurses-devel)
*   **readline** (libreadline-dev / readline-devel)
*   **libarchive** (libarchive-dev / libarchive-devel)
*   **lcov** (for baseline coverage reports)
*   **llvm-symbolizer** (recommended for sanitizer/fuzz stack traces)

### Build from Source

```bash
# Clone the repository
git clone https://github.com/robkam/ytreenova.git
cd ytreenova

# Compile (Optimized Release Build)
make

# Install
sudo make install

# Uninstall
sudo make uninstall
```

*Note: Developers can compile with AddressSanitizer enabled by running `make DEBUG=1`.*

## Compiles and Runs On

> **Note on Development:**
> YtreeNova is developed using **VScodium** with the **Codex extension**, targeting **WSL Ubuntu 24.04.4 LTS** and **GCC 13.3.0**.

**Verified Platforms (via WSL):**
- Arch Linux, GCC 16.1.1
- Fedora Linux 45 (WSL Prerelease), GCC 16.1.1
- Ubuntu 26.04 LTS, GCC 15.2.0

## Documentation Guide

The project documentation lives under `docs/` and is split into focused user, developer, governance, and AI workflow references.

| Document | Purpose |
| :--- | :--- |
| **[USAGE.md](docs/USAGE.md)** | **User Guide**: How to navigate, tag, and use command keys. (Generated from `ytnova.1.md`). |
| **[quickstart.md](docs/quickstart.md)** | **Developer Setup**: How to set up the environment, run tests, and work on the codebase. |
| **[BUGS.md](docs/BUGS.md)** | **Known Issues**: Current defects, reproductions, and fix status. |
| **[CONTRIBUTING.md](docs/CONTRIBUTING.md)** | **Developer Setup**: How to set up the environment, run tests, and submit code. |
| **[PR_GATE.md](docs/PR_GATE.md)** | **PR Governance**: Required PR gate checks and triage rules needed for merge readiness. |
| **[AUDIT.md](docs/AUDIT.md)** | **QA Workflow**: The mandatory safety/integrity checks for every PR (Valgrind, ASan, etc). |
| **[SPECIFICATION.md](docs/SPECIFICATION.md)** | **Behavioral Contract**: UI layout, navigation protocols, and design philosophy. |
| **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** | **System Design**: Core technical principles (DRY, SRP, Context-passing) and data hierarchy. |
| **[TRUST.md](docs/TRUST.md)** | **Trust & Safety**: Safety claims and where to verify them in the codebase. |
| **[ROADMAP.md](docs/ROADMAP.md)** | **Future Plans**: Pending milestones and prioritized delivery backlog. |
| **[CHANGELOG.md](docs/CHANGELOG.md)** | **Changelog**: Detailed history of YtreeNova feature delivery, architecture work, and updates. |
| **[ai/WORKFLOW.md](docs/ai/WORKFLOW.md)** | **AI Workflow**: Persona routing, agent workflow, semantic-tool usage, and maintainer prompts. |

## Project Structure

```text
ytreenova/
├── src/                 C source files
├── include/             C headers
├── tests/               pytest/pexpect test suite
├── scripts/             helper scripts (build and tooling)
├── docs/                user, developer, and process documentation
├── etc/                 manpage source and related assets
├── build/               build artifacts
├── obj/                 object files
├── coverage/            coverage outputs
├── Makefile             primary build/test targets
├── README.md            project overview
└── AGENTS.md            Codex/agent discovery stub
```

---

## Reporting Issues

If you find anything amiss, you can report it using [GitHub Issues](https://github.com/robkam/ytreenova/issues).

For security-sensitive bugs, report privately via [SECURITY.md](SECURITY.md).

Feature requests and enhancement ideas are welcome too. Suggestions are appreciated, but not every request can be implemented.

It will help us to address the issue if you include the following:
*   **OS & Configuration:** (Distro, Terminal type, etc.)
*   **YtreeNova version:**
*   **Steps to Reproduce:**
*   **Expected Behavior:**
*   **Actual Behavior:**

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](docs/CONTRIBUTING.md) for guidelines. See [SPECIFICATION.md](docs/SPECIFICATION.md) for behavioral requirements, and [ARCHITECTURE.md](docs/ARCHITECTURE.md) to understand the system design before submitting code.
Contributions do not need to be low-level C internals: bug reports, documentation updates, typo fixes, wording/UX clarifications, and translations are all valuable.

## License

YtreeNova is free software distributed under the GPL. See the [LICENSE.md](LICENSE.md) file for details.

## Contributors

For detailed authorship, see [AUTHORS.md](docs/AUTHORS.md).
