# V1 release line

The `1.0.0-beta` release shipped on 2026-09-14. The roadmap stays intact while this document describes the remaining path to stable v1.

This file exists only to answer two practical questions:

1. What is the smallest honest line from the current beta to a stable v1 release?
2. What is still worth doing later, without pretending it all has to happen first?

It does not replace `docs/ROADMAP.md`, and it does not change the roadmap.

Deferring work here means "keep it on the roadmap and do it later", not "cut it".

## At what point can you do the stable release?

The next release decision is when the project is ready to move from `1.0.0-beta` to stable `1.0.0`.

### Beta release baseline

The beta milestone used this credibility line:

- Make Contextual F1 Help Accurate, Direct, and Task-Focused done
- Bring Manpage/Usage Reference Prose in Line with Unix Manpage Conventions done
- Externalize UI Strings with GNU gettext (i18n Foundation) done, if you want the i18n foundation in beta
- the Security Risk Gate (Audit + Detect + Block) family done
- Remove Temporary Compatibility Shims (Global Cleanup Gate) done
- Bounded Consistency Audit (Docs and UX Terminology) done
- one bounded adversarial pass done
- no known high-severity trust or corruption issues

### A stable v1.0

Do not tie v1.0 to finishing the whole roadmap.

Tie it to this instead:

- the program is useful for real work
- it is stable in common use
- no known serious safety or security defects
- no known serious corruption failures
- no known architecture-shim debt left in core paths
- docs, help, and footer guidance broadly truthful
- no daily-use trust failures
- the remaining bugs are mostly irritations, not disasters
- a release candidate survives a bounded hostile review without blocker findings

A beta is only useful if you still mean this:

> Core behaviour may still be unstable enough that I do not want to call this stable yet.

If that no longer describes the program, there is no obligation to stop at beta first.

### Release-validation note

Before cutting v1, use the existing release-depth checks from `docs/AUDIT.md` instead of inventing a new one-off ritual.

In particular:

- run the bounded blocker-only adversarial audit in this file
- run the deep Valgrind pass already documented by the repo, `make qa-valgrind-full`
- if release-risk runtime work changed recently, include the complementary deep runtime checks already called out in `docs/AUDIT.md`

The point is confidence, not infinite delay. Use the existing deep checks to look for serious memory-safety or runtime problems, then make the release decision.

## Part A. Minimum line for a v1 release

The point of this line is to stop widening scope and get a stable first release out to real users.

Call it v1 when these things are true:

- the program is useful in normal work
- common paths are trustworthy
- there are no known serious crash, corruption, security, or invariant failures
- the docs and help are good enough not to mislead
- the remaining known bugs are mostly smaller trust irritants, such as cursor placement or other UI or UX awkwardness

Minimum order from the current state:

1. Run `make qa-valgrind-full` as the documented deep Valgrind release check.
2. Run one bounded blocker-only adversarial audit.
3. Release v1 unless those release-depth checks find something truly bad.

What this line does not require:

- finishing the whole roadmap
- waiting for the codebase to stop having small repetitive issues
- waiting for an adversarial audit to stop finding nits
- another grand cleanup phase

### Blocker-only audit rule

The audit is a release gate, not an endless purification ritual.

Run one pass and time-box it. Treat the release as blocked only by findings in one of these classes:

- serious security trouble
- data loss or corruption
- major invariant breakage
- common-path behavior that is so misleading or unstable that calling the program stable would feel dishonest

If the audit finds only smaller inconsistencies, polish issues, wording problems, or familiar UI or UX annoyances, log them and release anyway.

### Ready-to-paste audit prompt

This is the release-candidate prompt for the bounded audit. It is intentionally narrower than the general adversarial audit prompt because the goal here is to decide whether v1 is unsafe, not to create an endless cleanup list.

```text
:at code_auditor
use skill code-quality

Audit ytnova as a release-candidate gate for v1.

This is a bounded blocker-only audit. The question is not "what small things are still wrong?" The question is "is there any serious reason this should not be released as v1 yet?"

Be adversarial, but stay evidence-based:
- prioritize serious defects over style nits
- look for security trouble, data loss, corruption, invariant breakage, major common-path fragility, and any behavior that would make calling the program stable feel dishonest
- reason from first principles about how the code could fail in normal paths, degraded states, and edge conditions
- look for root-cause problems, not isolated trivia
- do not invent findings; mark uncertainty explicitly

Do not turn this into a backlog-grooming exercise.
Ignore or briefly bucket non-blocking polish, wording, and familiar minor UI or UX annoyances unless they reveal a deeper release-blocking defect.

Return findings only.

For each blocking or near-blocking finding include:
- Severity: blocker | high
- File:line
- Evidence
- Impact
- Concrete fix
- Minimal trigger or failure path, if inferable

Also include:
- the single most dangerous real finding, if any
- the single most likely release-blocking real-world failure mode, if any
- final gate status: PASS FOR V1 or FAIL FOR V1

If there are no blocker or high findings, say so plainly and stop.
Do not praise the code.
Do not pad the result with balance, encouragement, or generic commentary.
```

## Part B. Worth doing after the minimum line

These items are still worth doing. They are simply not part of the smallest sane path to v1.

### Strongly worth doing soon

The strongest remaining candidate after the minimum line, or soon after v1 if energy is limited, is Task 39, the multi-round adversarial security review. Keep it time-boxed and severity-driven.

### Pull these forward only if current use still feels fragile

Do these sooner only if day-to-day use still exposes real trust problems in the current build:

- Task 23.1, input loop determinism and event-priority contract
- Task 23.2, non-blocking FD multiplexing implementation
- Task 6.1, unify stats and main-pane frame redraw contract
- Task 6.2, footer-aware redraw synchronization contract
- Task 5.1, keep progress indicators from clobbering footer, prompt, or F1 guidance

### Safe to defer without cutting

These are real tasks. They just are not good reasons to delay v1 by themselves.

#### Feature expansion

- Task 29, advanced batch rename
- Task 30, unify copy semantics and add directory sync
- Task 31, extension surface contract
- Task 32, shared provider registry
- Task 33, optional background app execution
- Task 34, F7 preview helper pipeline

#### Configurability and later polish

- Task 35, propagate active theme to terminal helpers
- Task 36, configurable keymap
- Task 37, UI or UX snappiness polish
- Task 38, modal window shadows

#### UI nice-to-haves

- Task 3, manual file-column width controls
- Task 4, adjustable list or preview width in F7 mode
- Task 24, configurable bypass for external viewers
- Task 25, auto-execute on command termination
- Task 26, standardize internal viewer layout
- Task 27, nested archive traversal
