# turbulenceMMAOpt Debug Findings

This note records the completed debugging cycle that compared
`turbulenceMMAOpt` against the laminar reference and isolated which runtime
controls most strongly change optimizer evolution.

Reference sources:

- `laminarOptimizer/optimizerlogs_reference/`
- `optimizerlogs/` for the latest isolated run under test
- `turbulenceMMAOpt/app_*_20260323/` isolation cases

## Outcome

The main diagnosis is:

- The turbulence framework itself is **not** the primary reason the optimizer
  failed to reproduce the laminar channel-evolution path.
- The dominant trigger is the **aggressive initial interpolation setting**
  `q = 0.1` in the turbulence branch.
- The original split/decreasing continuation schedule is a **secondary
  amplifier**, not the root cause.

In practice:

- `turbulenceMMAOpt` can track the laminar reference closely in early
  iterations when it is started in a laminar-like interpolation regime.
- `turbulenceMMAOpt` should **not** be expected to reproduce the same design if
  it is run with the original turbulent branch controls that start from
  `qAlpha0=qKappa0=qHeat0=0.1`.

## Tested Isolation Ladder

### 1. Laminar-parity baseline

Case intent:

- fixed `qSingle=0.005`
- `useSingleQFallback=true`
- `useSplitRAMPControls=false`
- `useTurbulentThermalDiffusivity=false`
- `useBrinkmanSinkInKEpsilon=false`

Finding:

- Early history stayed close to the laminar reference.
- This showed that the optimizer machinery can follow the laminar path when the
  interpolation controls are softened.

### 2. Add turbulent thermal diffusivity only

Case change:

- `useTurbulentThermalDiffusivity=true`

Finding:

- Still close to the laminar reference through `Iter 20`.
- This option alone did not recreate the bad divergence.

### 3. Add Brinkman damping in `k-epsilon` only

Case change:

- `useBrinkmanSinkInKEpsilon=true`

Finding:

- Still close to the laminar reference through `Iter 20`.
- This option alone did not recreate the bad divergence.

### 4. Add both turbulence extras together

Case changes:

- `useTurbulentThermalDiffusivity=true`
- `useBrinkmanSinkInKEpsilon=true`

Finding:

- Still close to the laminar reference when `q` stayed fixed at `0.005`.
- Therefore the turbulence extras are not the main source of mismatch.

### 5. Fix high `q=0.1` with continuation disabled

Case intent:

- `qAlpha0=qKappa0=qHeat0=qSingle0=0.1`
- `qSingleMin=0.1`
- `qContinuationStartIter=1000`
- `useSingleQFallback=true`
- `useSplitRAMPControls=false`

Finding:

- The bad trajectory reappeared immediately at `Iter 1`.
- Power dissipation became much larger than the laminar reference before any
  meaningful topology evolution happened.
- The design remained trapped in a very gray state instead of moving toward the
  laminar channel-development path.

Important implication:

- High initial `q=0.1` alone is enough to reproduce the failure mode.

### 6. Original split/decreasing schedule

Case intent:

- original `qAlpha0=qKappa0=qHeat0=0.1`
- `useSplitRAMPControls=true`
- `useSingleQFallback=false`
- `qContinuationStartIter=20`

Finding:

- The run was identical to the fixed-high-`q` case through `Iter 10`.
- At `Iter 20`, after continuation started, the trajectory changed only
  modestly.
- The split schedule worsened the already-bad path slightly, but did not create
  it.

## Root-Cause Decision

The completed test cycle supports the following decision:

1. The key mismatch between `laminarOptimizer` and `turbulenceMMAOpt` is the
   interpolation/control regime, not the mere presence of turbulent physics.
2. Starting from `q=0.1` makes the optimizer enter a very different hydraulic
   and thermal regime from `Iter 1`.
3. Once that happens, MMA follows a different sensitivity landscape and does
   not evolve toward the laminar-style channel pattern.
4. The split/decreasing `qAlpha/qKappa/qHeat` schedule is not the primary bug;
   it only modifies a trajectory that is already off-course.

## Practical Guidance For `turbulenceMMAOpt`

### Parameters that strongly control optimizer evolution

| Control | Main effect on evolution | Debugging guidance |
|---|---|---|
| `qAlpha0`, `qKappa0`, `qHeat0`, `qSingle0` | Most influential startup controls. Large values strongly penalize the interpolated physics from the first solve. | Use low startup `q` when trying to reproduce the laminar branch. |
| `qAlphaMin`, `qKappaMin`, `qHeatMin`, `qSingleMin` | Set the asymptotic sharpening floor. | Keep fixed when isolating startup effects. |
| `qContinuationStartIter`, `qContinuationInterval`, `qContinuationFactor` | Control when and how fast the interpolation hardens or softens. | Disable continuation during root-cause testing. Re-enable only after startup behavior is correct. |
| `useSplitRAMPControls` | Enables independent flow and thermal interpolation controls. | Useful for production tuning, but adds debugging complexity. |
| `useSingleQFallback` | Forces a one-parameter RAMP path. | Best first tool for isolating interpolation behavior. |
| `useTurbulentThermalDiffusivity` | Adds `nut/Prt` to thermal diffusion. | Changes temperature fields, but was not the dominant mismatch in this cycle. |
| `useBrinkmanSinkInKEpsilon` | Suppresses `k` and `epsilon` in porous regions. | Important physically, but not the primary source of divergence here. |
| `beta0`, `betaIncrement`, `betaMax` | Control projection sharpening and the rate at which gray regions collapse. | Do not treat slow early gray behavior as a bug by itself; compare mid-run and late-run collapse. |
| `filterR` | Sets the smoothing radius and therefore the minimum feature scale. | Larger values reduce small-scale branching and delay sharp channel formation. |
| `movlim` | Caps per-iteration design changes. | Useful stability knob if the topology changes too abruptly. |
| `PowerDissMax`, `PowerDissRelax`, `powerConstraintRelaxationRate` | Determine how strongly the run is allowed to violate the power limit early on. | Distinguish normal relaxed early violation from true divergence caused by bad interpolation startup. |

### Switches that matter for solver robustness

| Switch | Importance |
|---|---|
| `useFrozenTurbulenceAdjoint` | Keep enabled for the current branch; this is the stable adjoint baseline. |
| `useKEpsilonModel` | Current production turbulence model. |
| `usePorosityWallDistance` | Important for evolving internal walls in density-based channel generation. |
| `useFullAdjointSymmetricStress` | Influences adjoint fidelity and can matter when validating sensitivities. |

## Recommended Operating Modes

### If the goal is to reproduce laminarOptimizer behavior

Use a laminar-like startup regime:

- low fixed `q` close to `0.005`
- `useSingleQFallback=true`
- `useSplitRAMPControls=false`
- delay `q` continuation until the early trajectory matches the laminar
  reference

Expectation:

- `turbulenceMMAOpt` can closely match the laminar early path under this setup.

### If the goal is a genuinely turbulent production run

Use the full turbulence controls only after startup behavior is verified:

- independent split `qAlpha/qKappa/qHeat`
- optional `nut/Prt` thermal diffusion
- porous damping in `k-epsilon`

Expectation:

- The final design will generally not match the laminar optimum exactly because
  the optimizer is then solving a different problem.

## Recommended Debug Order For Future Work

When `turbulenceMMAOpt` drifts away from expected channel evolution, compare in
this order:

1. Startup interpolation controls: `qAlpha0`, `qKappa0`, `qHeat0`, `qSingle0`
2. Whether split-RAMP is active
3. Whether continuation is already acting too early
4. Only then examine `useTurbulentThermalDiffusivity`
5. Only then examine `useBrinkmanSinkInKEpsilon`

This order reflects the completed isolation cycle and should save time in later
debugging.

## Gray-Fraction Profile Ladder Archive Synthesis

This section records the later gray-fraction debugging cycle that was run after
the startup-`q` root cause had already been isolated.

Primary archive sources:

- `docs/TurbulenceMMAOptGrayFractionExperimentPlan.md`
- `optimizerlogs/` for the currently checked-in run snapshot
- `~/.codex/archived_sessions/rollout-2026-03-24T14-33-42-019d1f15-e5b2-7472-8777-652a5104ff14.jsonl`
- `~/.codex/archived_sessions/rollout-2026-03-24T18-41-46-019d1ff9-028f-7b33-b61b-0a344b11e12c.jsonl`
- `~/.codex/archived_sessions/rollout-2026-03-24T18-28-50-019d1fed-2bc3-72f1-8401-3f7c05813177.jsonl`

### Why This Later Cycle Was Needed

Once the turbulent branch was started in the laminar-like low-`q` regime, the
dominant failure mode changed:

- iterations stayed numerically stable
- the optimizer no longer blew up immediately
- gray fraction still collapsed far more weakly than in the laminar reference
- the turbulent branch never entered the laminar-style mid-run redesign burst

The later profile ladder was therefore aimed at separating three different
causes:

1. continuation was too slow
2. continuation was being gated off too often
3. objective/power sensitivities were too weak to trigger a topology rewrite

### Important Reference Distinction

Two different gray-stall references appeared in the archived discussion and
they should not be mixed together.

#### A. Pre-profile reference run

This was the run that motivated the profile ladder.

Key signals discussed in the archive:

- by about `Iter 78`, gray fraction was still about `0.852`
- `beta` was only about `9.56`
- `xhStepMax` had fallen back to about `0.0195`

Interpretation:

- the run was stable, but still had not entered the strong redesign regime
- this is why the plan document describes a reference case that was still
  mostly gray around iteration `~78`

#### B. Later completed `baseline` profile run

This was a true experiment-profile result, not just the motivating reference.

Key archived findings:

- continuation gate stayed open only through `Iter 34`
- gate closed at `Iter 35` and never reopened
- `beta` froze at `4.4` from `Iter 35` to about `Iter 176`
- gray fraction stayed almost flat: about `0.9230 -> 0.9208`
- `xhStepMax` stayed tiny late in the run, around `0.005-0.009`
- `PD/limit` worsened from about `1.17` at `Iter 40` to about `2.02` at
  `Iter 174`

Interpretation:

- this later `baseline` run gave much stronger evidence than the earlier
  reference that continuation-gate freeze was the dominant blocker

Practical caution:

- the `baseline` expectation text in
  `TurbulenceMMAOptGrayFractionExperimentPlan.md` reflects the earlier
  pre-profile reference more than the later completed `baseline` profile result

### Completed Profile Results From The Archive

#### 1. `baseline`

Status:

- completed and interpreted in the archive

Observed behavior:

- hardening gate froze at `Iter 35`
- `beta` froze at `4.4`
- gray fraction stayed near `0.92`
- no redesign burst appeared

Decision:

- confirms that early gate closure alone can suppress gray collapse

#### 2. `laminarGrayRamp`

Status:

- profile exists in the plan and code
- no completed archived run summary was found during the archive sweep

Current inference:

- still an open diagnostic gap
- continuation speed alone was proposed as a test, but the available archive
  material does not provide a completed result for it

#### 3. `relaxedGateGrayRamp`

Status:

- completed and interpreted in the archive

Observed behavior:

- gate stayed open until about `Iter 62` instead of closing at `35`
- `beta` reached `12.8` by about `Iter 63`
- gray fraction dropped from `0.923` to about `0.8517` by `Iter 75`
- `xhStepMax` grew into the `0.05-0.06` range
- gate closed again at about `Iter 63` when `PD/limit` passed about `2.5`
- after that, hardening shut off and the run drifted again

Decision:

- substantially better than `baseline`
- still not enough to reach the true collapse/rewrite phase
- proves that loosening the gate helps, but does not solve the problem by
  itself

#### 4. `betaFloorGrayRamp`

Status:

- completed archived run exists through `Iter 88`
- also has a newer checked-in partial run snapshot in `optimizerlogs/`

Archived completed-run behavior:

- hardening floor stayed active through `Iter 80`
- after the floor shut off at `Iter 81`, the feasibility gate stayed open on
  its own
- `beta` rose to `17.8` by `Iter 88`
- gray fraction dropped from `0.9230` to `0.7346` by `Iter 88`
- intermediate archived checkpoints:
  - `Iter 60`: gray fraction about `0.8646`
  - `Iter 80`: gray fraction about `0.7878`
  - `Iter 88`: gray fraction about `0.7346`
- `xhStepMax` rose to about `0.097`
- objective and hotspot penalty worsened during sharpening:
  - `MeanT` rose from about `2.51` early to about `3.79` by `Iter 88`
  - `MaxT` rose from about `5.4` to about `14.9`

Decision:

- best-controlled experiment so far
- first profile that clearly crossed the "hardening remains active after the
  floor" threshold
- archived recommendation was explicitly: do not retune yet; run the same
  profile longer first

#### 5. `ungatedGrayRamp`

Status:

- completed and interpreted in the archive

Observed behavior:

- `beta` rose from `0.4` at `Iter 1` to `35.4` by about `Iter 176`
- gray fraction collapsed from about `0.923` to about `0.0038`
- archived intermediate gray checkpoints also included:
  - about `0.9269`
  - about `0.2204`
  - about `0.0451`
  - about `0.0195`
- large late topology changes occurred
- archive notes mention very large `xhStepMax` in the `Iter 120-160` window
- objective spiked into the `~20` range
- brief positive volume overshoots occurred before recovery

Decision:

- strongest proof that the turbulent branch is not missing a gray-collapse
  mechanism
- also strongest proof that fully bypassing the gate is too violent for a
  production-like setting

#### 6. `adjointSensitivityProbe`

Status:

- profile exists in the plan and code
- no completed archived run summary was found during the archive sweep

Current inference:

- still an open secondary diagnostic
- the archive strongly suggests it only becomes the next priority if continued
  hardening is already working but topology rewrite remains too weak

### Cross-Profile Conclusions From The Archive

The later archive sweep supports these conclusions:

1. The turbulent branch is not missing the mathematical machinery for gray
   collapse.
2. The primary blocker in the later runs was continuation gating, not the old
   startup-`q` issue.
3. `ungatedGrayRamp` proves that if hardening is allowed to continue, gray can
   collapse dramatically.
4. `relaxedGateGrayRamp` proves that a looser gate helps but can still freeze
   too early.
5. `betaFloorGrayRamp` is the best compromise observed so far because it kept
   hardening alive long enough for the gate to remain open after the floor.
6. Sensitivity weakness is still a plausible secondary limitation, but the
   archive does not yet contain a completed `adjointSensitivityProbe` result.

### Secondary Sensitivity Inference

Before the profile ladder was added, the archive already noted that the
turbulent branch was missing the laminar-style redesign burst even in a stable,
low-`q` run.

Archived laminar-vs-turbulent comparison around the gray-stall regime:

- turbulent run at about `Iter 78`:
  - gray fraction about `0.852`
  - `beta` about `9.56`
  - `xhStepMax` about `0.0195`
- laminar reference at about `Iter 78`:
  - gray fraction about `0.350`
  - `beta` about `15.6`
  - `xhStepMax` about `0.384`
- laminar reference around `Iter 60` already showed a very strong redesign
  regime with `xhStepMax` about `0.607`

Interpretation:

- the turbulent branch was not only under-hardened
- it was also receiving much weaker topology-changing pushes than the laminar
  reference
- therefore sensitivity weakness remains the most credible secondary hypothesis
  after continuation gating

### Latest Stored Run Snapshot

The currently stored `optimizerlogs/` directory now contains a much longer
`betaFloorGrayRamp` run than the earlier partial snapshot.

Current stored-run facts:

- `tuneOptParameters` still selects `profile betaFloorGrayRamp`
- `debugOptimizer.jsonl` reaches `Iter 149`
- `optimization.hst` reaches `Iter 148`
- `debugOptimizer.log` lags behind the JSON tail
- the run stopped at `Iter 149` with `reason=adjointRunaway`

Observed behavior:

- the run successfully moved far beyond the old gray-stall regime
- archived promising checkpoints were matched and then exceeded:
  - `Iter 60`: gray fraction about `0.859`, `beta=12.2`,
    `xhStepMax~0.049`
  - `Iter 88`: gray fraction about `0.666`, `beta=17.8`,
    `xhStepMax~0.112`
- after the floor shut off at `Iter 81`, the feasibility gate stayed open on
  its own through `Iter 148`
- gray fraction fell below `0.10` at about `Iter 126`
- gray fraction fell below `0.05` at about `Iter 127`
- minimum gray fraction was about `0.0179` at `Iter 142`
- maximum `xhStepMax` reached about `0.604` at `Iter 142`
- objective deterioration became severe in the late stage:
  - `MeanT` reached about `15.09` at `Iter 130`
  - `MaxT` continued rising into the `30-40` range
- volume became slightly positive around `Iter 119-130`, then oscillated back
  near active

Late-run failure mode:

- at `Iter 149`, `Ua/U` stayed reasonable at about `4.0`
- but `Ub/U` jumped to about `1.94e6`
- `pbMax` jumped to about `1.04e6`
- stop reason became `adjointRunaway`

Interpretation:

- `betaFloorGrayRamp` is now proven capable of driving strong gray collapse
- the primary remaining issue is no longer "insufficient hardening"
- the new issue is late over-hardening, followed by heat-adjoint runaway after
  the design is already nearly binary
- this late behavior is closer to the violent `ungatedGrayRamp` regime than to
  the earlier archived "best-controlled so far" interpretation of the
  `Iter 88` snapshot

### `pa` Solver Crosscheck Status

The archive and current log review also show that `pa adjoint pressure` should
be treated as a secondary robustness issue, not the primary explanation for
gray-collapse failure.

Archive-supported inference:

- gray reduction improved substantially once continuation hardening was kept
  alive, even before `pa` was fully cleaned up
- therefore `pa` is not the first-order cause of the gray-collapse stall

Earlier caveat:

- the current checked-in `fvSolution` file now contains:
  - `pa` solver `PCG`
  - `pa` preconditioner `DIC`
  - `pa` `maxIter = 10000`
  - `pa` relaxation `0.1`

Latest stored-run validation:

- the latest `solverConvergences.log` now shows that the `pa` patch really did
  improve the pressure-adjoint solve
- `pa adjoint pressure` is reported as `OK`, not `WARN`
- `pa` no longer hits the old `5000` iteration cap
- latest summary from the stored run:
  - average `pa` solve iterations about `270`
  - maximum `pa` iterations about `980`
  - zero `5000`-cap hits in the run

Updated interpretation:

- the earlier `pa` cap-hit problem is no longer the active first-order blocker
  in this stored run
- the stop at `Iter 149` was caused by the heat-adjoint branch (`Ub/pb`), not
  by `pa`
- future debugging should not attribute the latest runaway to the old `pa`
  issue

Additional caveat:

- `solverConvergences.log` still reports `Ub` and `pb` as `OK` right up to the
  failing iteration because the linear residuals remain small
- the actual stop is triggered by adjoint field magnitude ratios in
  `debugOptimizer`, not by linear residual health alone
- therefore late-stage crosschecks must use both solver convergence logs and the
  field-magnitude diagnostics in `debugOptimizer.jsonl`

### Current Tuning Patch After The Latest Run

To address the newly observed late-stage failure mode, a continuation ceiling
was added for the `betaFloorGrayRamp` path.

New control:

- `experimentControl.stopContinuationHardeningBelowGrayFraction`

Current tuning choice:

- for `betaFloorGrayRamp`, if the user does not override it explicitly, the
  solver now defaults this threshold to `0.05`

Effect:

- once the gray volume fraction falls below the threshold, continuation
  hardening is disabled
- this freezes further `beta` and `alphaMax` growth even if the feasibility
  gate remains open
- the purpose is to preserve the already-achieved gray collapse while avoiding
  the late over-hardening regime that drove the `Ub/pb` runaway

Logging support:

- runtime dump now records `stopContinuationHardeningBelowGrayFraction`
- `debugOptimizer.log` now reports whether a hardening ceiling is active
- `debugOptimizer.jsonl` now records:
  - `interpolation.continuationCeilingActive`
  - `interpolation.stopContinuationHardeningBelowGrayFraction`

### Verified Ceiling-Controlled Run Through `Iter 200`

The latest complete `optimizerlogs/` set now verifies that the ceiling-controlled
`betaFloorGrayRamp` path can run cleanly through the current `200`-iteration
budget without the old late `Ub/pb` runaway.

Validated run facts:

- `debugOptimizer.jsonl` reaches `Iter 200`
- `solverConvergences.log` reaches `Optimization Iteration : 200`
- `runLOG` ends with `Finalising parallel run`
- the run does not stop with `adjointRunaway`
- the intermediate dimension-mismatch abort seen during one patching attempt is
  not the latest state and should not be used as the current reference run

Observed behavior:

- gray fraction still collapses strongly:
  - `Iter 1`: gray about `0.923`
  - `Iter 60`: gray about `0.537`
  - `Iter 88`: gray about `0.388`
  - `Iter 126`: gray about `0.209`
  - `Iter 132`: gray about `0.0236`
  - minimum gray about `0.0147` at `Iter 165`
  - `Iter 200`: gray about `0.0163`
- the hardening ceiling activates as intended at `Iter 132`
- after `Iter 132`, `beta` stays frozen at `26.6`
- after `Iter 132`, `hardeningEnabled=0` while the feasibility gate remains open
- once the design is already nearly binary, thermal performance starts to
  recover instead of blowing up:
  - `MeanT` drops from about `53.9` at `Iter 132` to about `42.1` at `Iter 200`
  - `MaxT` drops from about `116.3` at `Iter 132` to about `93.3` at `Iter 200`
- power feasibility remains healthy and continues improving:
  - `PDCon` drops to about `1.58` by `Iter 200`
- the heat adjoint stays bounded:
  - maximum observed `Ub/U` in the run is only about `1.79e-01`
  - this is far below the runaway limit of `1e6`
- `Tb adjoint thermal` remains solver-healthy in the late run and is still `OK`
  at `Iter 200`

Interpretation:

- the gray-suppression goal is now achieved on the current best candidate path
- the late-stage ceiling does not merely delay the old failure mode; it
  successfully prevents the previous heat-adjoint runaway through the full
  current run horizon
- the main unresolved question is no longer whether gray fraction can collapse
- the next refinement question is whether the post-ceiling nearly-binary regime
  can improve objective quality faster, or reduce the residual gray plateau near
  `~0.016`, without reintroducing late instability
- according to the experiment plan, this result does not justify moving on to
  `ungatedGrayRamp`; the correct course remains to refine the
  `betaFloorGrayRamp` path

### Latest `betaFloorGrayRamp` Follow-Up Run With Robust `Tb` Solve

The latest complete `optimizerlogs/` set continues to support the same
plan-level conclusion: the gray-area suppression debugging should remain on the
`betaFloorGrayRamp` path rather than advancing to `ungatedGrayRamp`.

Validated run facts:

- `debugOptimizer.jsonl` reaches `Iter 200`
- `runLOG` again ends with `Finalising parallel run`
- `optimization.hst` reports `SolverStat=OK` through `Iter 200`
- the active profile remains `betaFloorGrayRamp`
- the late-run `Tb` solve stays healthy with the current robust `GAMG`
  settings:
  - `Tb adjoint thermal` is `OK` at `Iter 200`
  - `Tb` final residual is about `9.615e-11`
  - `Tb` solve count is about `196` iterations, not the earlier
    pathological `O(1e4-1e5)` behavior

Observed behavior:

- gray suppression still succeeds:
  - `Iter 200` gray is about `1.96e-02`
  - the design remains mostly binary by volume split:
    - solid about `0.590`
    - fluid about `0.390`
- the continuation ceiling is active in the late run:
  - `ceiling=yes`
  - `gray<=5.0e-02`, actual gray about `1.96e-02`
  - `beta` is frozen at `25.6`
  - `hardeningEnabled=0` while the gate remains open
- thermal performance in this run is much lower than the earlier archived
  `Iter 200` reference:
  - `MeanT` finishes near `10.47`
  - `MaxT` finishes near `24.30`
- solver health remains good at the end of the run:
  - `T thermal` is `OK`
  - `Tb adjoint thermal` is `OK`
  - `Ub`, `pb`, `Ua`, and `pa` are also `OK`

New caveat relative to the archived best reference:

- the run finishes with a small power-constraint violation instead of a fully
  clean feasible end state
- at `Iter 200`, `PD/limit` is about `1.011`
- the dashboard therefore marks constraints as `WARN` even though solver health
  remains `OK`
- sensitivity ratios also remain small in the late nearly-binary regime:
  - `objectiveToVolumeL2Ratio` about `5.66e-03`
  - `powerToVolumeL2Ratio` about `1.37e-02`

Interpretation:

- this follow-up run does not change the main experiment-plan verdict:
  `betaFloorGrayRamp` still clearly produces the required gray-collapse trend,
  so the plan does not justify moving on to `ungatedGrayRamp`
- the current debugging focus should remain refinement of the post-ceiling
  regime on `betaFloorGrayRamp`
- the two main follow-up questions are now:
  - how to recover full late-run power feasibility without losing the achieved
    gray suppression
  - whether weak late objective/power sensitivity ratios justify the planned
    `adjointSensitivityProbe` as a secondary diagnostic after the current
    `betaFloorGrayRamp` tuning is stabilized

### Current `betaFloorGrayRamp` Refinement Patch

To support the next round of post-ceiling tuning, the solver now includes an
optional lagging-gray-collapse throttle for the `betaFloorGrayRamp` profile.

New controls:

- `experimentControl.laggingGrayCollapseStartIter`
- `experimentControl.laggingGrayCollapseAboveGrayFraction`
- `experimentControl.laggingGrayCollapseBelowXhStepMax`
- `experimentControl.laggingGrayCollapseMinGrayDrop`
- `experimentControl.laggingGrayCollapsePauseInterval`

Default tuning now applied for `betaFloorGrayRamp` unless overridden:

- start checking at `Iter 120`
- only activate while gray fraction is still at least `0.20`
- require the previous `xhStepMax` to be at most `0.05`
- require gray-fraction drop from the previous iteration to be at most `0.01`
- allow one continuation-hardening pulse every `5` iterations while the lag
  condition remains active

Effect:

- this throttle is only relevant after the forced hardening floor has ended
- if gray is still high but topology motion has become weak and gray is no
  longer dropping much, continuation hardening is temporarily paused
- the periodic pulse prevents the continuation schedule from freezing
  permanently under a long stagnant plateau
- the existing late-stage ceiling remains independent: once gray is already low
  enough, hardening still shuts off completely

Logging support:

- the runtime option dump now records all five
  `laggingGrayCollapse*` parameters
- `debugOptimizer.log` now reports a `lag throttle` row with:
  - whether the lag condition is active
  - the latest gray-fraction drop
  - the previous `xhStepMax`
  - whether the current iteration is a pulse iteration
- `debugOptimizer.jsonl` now records:
  - `interpolation.laggingGrayCollapseDetected`
  - `interpolation.laggingGrayCollapsePulseIter`
  - `interpolation.grayVolumeFractionDrop`
  - `interpolation.previousXhStepMax`

### Fast Triage Rules For Future `optimizerlogs`

When reviewing future runs, use this order of interpretation:

1. Confirm which profile actually ran from `debugOptimizer.jsonl` under
   `interpolation.experimentProfile`.
2. Check whether the current run is tracking the archived promising
   `betaFloorGrayRamp` path by about `Iter 40-60`.
3. If `floor=yes` but gray remains near `0.92` and `xhStepMax` keeps fading,
   the run is already underperforming before gate re-entry even matters.
4. After the floor ends, check whether hardening stays on without the floor:
   this is the decisive success signal for the mid-run `betaFloorGrayRamp`
   diagnosis.
5. Once gray becomes small, check `interpolation.continuationCeilingActive`:
   the new ceiling should freeze hardening before the nearly binary regime
   becomes violently unstable.
6. If hardening continues deep into low-gray territory while objective and
   volume swing violently, the run is drifting toward `ungatedGrayRamp`
   behavior.
7. If hardening continues but `objectiveToVolumeL2Ratio`,
   `powerToVolumeL2Ratio`, and `xhStepMax` all stay small, prioritize
   `adjointSensitivityProbe` as the next diagnostic.

### Current Best Working Hypothesis

For future debugging, the best evidence-backed hypothesis is:

- the old startup-`q` issue was the earlier root cause
- the later gray-fraction stall was primarily caused by hardening being gated
  off too early
- `betaFloorGrayRamp` successfully solves the "no gray collapse" problem, but
  in its unbounded late form it can over-harden into a heat-adjoint runaway
- the new late-stage hardening ceiling is now validated through `Iter 200` as
  the current best corrective tuning direction
- sensitivity weakness remains the next unresolved secondary hypothesis
- completed archived evidence is still missing for both `laminarGrayRamp` and
  `adjointSensitivityProbe`
