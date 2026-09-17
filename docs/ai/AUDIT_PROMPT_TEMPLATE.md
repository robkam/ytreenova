# Audit Prompt Template

Maintainer instructions:
- Delete the `Audit target:` line that does not apply, then fill in the missing number on the line you keep.
- Do not rewrite the rest unless you intentionally want custom behavior.
- A future maintainer may have only this repository clone. The AI must rely on repo state, git/GitHub state, and any currently-present transient relay files under `.agent/handoffs/`, not on external chat history.

:at c
use skill code-auditor-gate

Audit target: docs/BUGS.md Bug <number>
Audit target: docs/ROADMAP.md Task <number>

Derive the exact title, acceptance target, merged work set, and locked audit scope from the kept `Audit target:` line.

Primary goal:
- Perform one broad, scope-disciplined, adversarial audit of the full locked task surface.
- Optimize for widest credible coverage within scope, not incremental rediscovery.
- Do not turn this into a tiny-slice audit loop.
- Be willing to conclude PASS if the selected task/bug appears completed or fixed satisfactorily within the locked scope. Do not invent findings just to avoid a PASS result.

Scope lock:
- Before auditing the selected work item, inspect `.agent/handoffs/` for leftovers from any other task or bug.
- If unrelated completed-work-item relay files remain and no active PR/branch still depends on them, delete them before continuing so the audit does not preserve cross-task residue.
- Review only the merged diff for this work and the files it directly touched.
- Automatically derive the task's canonical source of truth from the selected `Audit target:` line.
- Cross-check only directly relevant supporting surfaces:
  - docs/ARCHITECTURE.md
  - docs/SPECIFICATION.md
  - any directly relevant handoff/prompt/checkpoint files if present
  - any directly relevant tests or docs touched by this work
- Do NOT expand into unrelated parts of the codebase.
- Do NOT perform a general repository audit.

Audit framing:
Treat the completed work as:
"Does this whole locked task surface work as one coherent, well-integrated machine, or does it contain stray parts, stale paths, partial migrations, divergent sibling behavior, leftover shims, dead code, misleading docs, or root-cause-adjacent defects that remain inside scope?"

Pre-audit inventory rule:
- Before judging the work, build an explicit audit inventory of the full locked scope.
- The inventory must include:
  - all merged PRs/commits that belong to this work if discoverable,
  - all files directly touched by the work,
  - all directly affected runtime surfaces,
  - all directly affected tests,
  - all directly affected docs/spec surfaces,
  - any compatibility shims, replacement paths, sibling paths, fallback paths, or transitional seams touched by the work.
- For bugfixes, also include:
  - the reproducer path,
  - adjacent failure surfaces that could share the same root cause,
  - any stale workaround or duplicate behavior path that may have survived the fix.
- Do not start emitting final conclusions until this audit inventory is explicit enough to prove scope coverage.
- If the audit FAILS, create `/home/rob/ytreenova/.agent/handoffs/` if needed and write the final audit result into both of these handoff files so the next implementation prompt can consume it without maintainer triage:
  - `/home/rob/ytreenova/.agent/handoffs/audit.current.txt`
  - `/home/rob/ytreenova/.agent/handoffs/audit.task-<number>.txt` for roadmap tasks or `/home/rob/ytreenova/.agent/handoffs/audit.bug-<number>.txt` for bugs.
- Overwrite those files with the latest failed audit for the selected work item; repeated FAIL audits of the same task/bug must be able to replace an older verdict cleanly.
- If the audit PASSES and matching relay files for this work item exist, delete them before the final response unless some still-active work item explicitly depends on them.

Coverage rule:
- Audit the entire inventoried scope in one pass.
- Do not stop after the first blocker, first defect family, or first convincing issue.
- Continue until every credible issue in the locked scope has been evaluated.
- If multiple files/lines reflect the same underlying defect family, collapse them into one finding with all affected locations.
- If a suspicious area is in scope and credible, include it even if confidence is lower; label the confidence clearly.
- If findings fall into multiple distinct defect families, you MUST group them yourself inside the audit output and audit handoff. Do not require the maintainer to triage them into next actions.

What to verify:
- Acceptance criteria are actually satisfied.
- No duplicate or parallel paths were left behind.
- No half-migrated replacements remain.
- No divergent sibling semantics were introduced.
- No stale shims, dead code, or misleading docs remain.
- No direct performance, event-loop, state-machine, ownership, or integration regressions were introduced in scope.
- Conceptual integrity, architectural drift/erosion, and architecture conformance within the locked scope.
- Tests and docs directly tied to the work still match the implemented behavior.
- No partially completed cleanup was left behind inside the locked task surface.

Anti-premature-pass rule:
- Do not issue PASS merely because the most visible path looks correct.
- Do not stop because one representative file looks clean.
- Do not assume merged means coherent.
- PASS requires an explicit final sweep across the full audit inventory.
- If no credible in-scope defects remain after that final sweep, say so clearly and return PASS without forcing another iteration.

Output format:
- The maintainer-facing final response must be exactly one of:
  - `PASS`
  - `FAIL saved to <handoff-path>; next use docs/ai/PROMPT_TEMPLATE.md with Work item: <same target>`
- Write the detailed audit record into failed-audit handoff files, not into the maintainer-facing final response, unless the maintainer explicitly asks to see the findings.
- Findings first.
- Severity-ranked.
- For each finding include:
  - severity
  - confidence: high / medium / low
  - defect family
  - file:line
  - evidence
  - impact
  - concrete fix
- Collapse near-duplicate findings into one item with all affected files/lines.

End with:
- explicit PASS/FAIL for the locked task scope
- explicit statement of whether the task/bug appears completed/fixed satisfactorily within the locked scope
- residual risks
- short statement of whether the audit appears exhaustive within the locked scope
- audit inventory reconciliation summary:
  - audited
  - intentionally not audited with reason
  - uncertain with reason
- if FAIL, also include:
  - grouped defect families,
  - the next highest-value coherent family to fix first,
  - the focused validation path for that family.

Audit posture:
- Be adversarial, specific, and scope-disciplined.
- Be exhaustive within scope.
- Do not give a whole-codebase review.
- Include lower-confidence items if credible, but label them clearly.
- Only omit items that are clearly out of scope or not credible.
- Optimize for breadth within the locked scope, not early exit.
- I want the widest credible one-pass dump within the locked scope, not an incremental rediscovery loop.
- If the work is already satisfactory, say PASS plainly and let the relay stop instead of manufacturing more churn.
