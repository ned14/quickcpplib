# Tooling notes

Follow-ups on the review tooling itself, recorded from a run. Not code findings —
those live in `plans/analysis.md`.

## Extend the reviewer budget (noted 2026-10-24, operator request)

The default reviewer budget is 6 minutes per reviewer (`pi-review run` default
`--budget-min 6`) and it is too small for the largest review area.

Evidence from the 10-round review-only run over `15063607c2` (artefacts in
`/workspace/reviews/quickcpplib-llm-fixes-20261024`): the `core-headers` area
(54 tracked headers, template-heavy) hit `STALLED … no output for 600s — killed`
and was recorded as a `stall-no-report` mechanism failure in rounds 2, 4, 5, 8 —
4 of 10 rounds. Each stall costs a full duplicate agent run as the retry, so
roughly 10% of the run's cost bought nothing, and the area that most needs
reviewing is the one most likely to be truncated.

Options when this is picked up: raise `--budget-min`, or size the budget per area
(file count / total bytes in the area's own globs) rather than globally. The
same fix should consider the 600s stall window, which fires before the budget
does for a reviewer that is thinking rather than emitting output.
