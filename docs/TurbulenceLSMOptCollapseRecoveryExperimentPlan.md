# turbulenceLSMOpt Collapse-Recovery Experiment Plan

This plan targets the specific failure mode seen in the latest
`turbulenceLSMOpt` run:

- numerically stable iterations
- early collapse from a mostly gray design to an over-solid design
- permanently capped global `phiLS` volume correction
- severe power violation after blockage
- almost no interface-driven reopening motion once the collapse has happened

The goal is to separate six candidate causes:

1. the run is not actually using the intended case controls
2. the adaptive `branchRefinement400` logic is too aggressive for the LSM path
3. the global `phiLS` shift is forcing the design into the Heaviside shoulders
4. the LSM interface band is too narrow and becomes effectively binary too fast
5. the reaction-diffusion update path is damping away reopening motion
6. the power-reopening sensitivity is too weak even when the adjoint is healthy

## Current Ladder Status

As of the current debugging cycle:

- the latest analyzed `optimizerlogs/` snapshot is now the
  `case-respected baseline` rerun
- the profile-fallback bug in
  [createFields.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/createFields.H)
  has been fixed and validated by the rerun
- the early runtime dump now respects the case dictionaries:
  `betaIncrement = 0.12`,
  `continuationFeasibilityTol = 1.15`,
  `forceContinuationHardeningUntilIter = 0`, and
  `forceContinuationHardeningUntilBeta = -1`
- despite that fix, the rerun still collapses between `Iter 5` and `Iter 6`
  and remains trapped afterward

Therefore Experiment 1 is complete and the ladder is now positioned at
Experiment 2 in practice.

Immediate next runs:

1. `profile baseline`
2. `reduced volume-shift cap`

Do not jump ahead to later experiments until the `profile baseline` run is
archived and reviewed.

## Run Order

Run the experiments in this order:

1. `case-respected baseline`
2. `profile baseline`
3. `reduced volume-shift cap`
4. `wider interface band`
5. `Hamilton-Jacobi fallback`
6. `power-reopen probe`

Only move to the next experiment if the previous one does not produce a clear
change in collapse timing or reopening behavior.

## Where To Edit The Case

Use these files:

- [tuneOptParameters](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/app/constant/tuneOptParameters)
- [optProperties](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/app/constant/optProperties)

For each rung in this ladder:

- inspect the latest `optimizerlogs/` snapshot first
- then modify the active case dictionaries in `turbulenceLSMOpt/app/constant/`
  directly for the next run
- record in the run notes exactly which dictionary values were changed before
  rerunning

Check the active settings in:

- `optimizerlogs/debugOptimizer.log`
- `optimizerlogs/debugOptimizer.jsonl`
- `optimizerlogs/gradientOpt.log`
- `optimizerlogs/optimization.hst`

Most important JSON fields for this ladder:

- `design.xhGrayVolumeFraction`
- `design.interfaceBandVolumeFraction`
- `objective.powerDissipationConstraintValue`
- `objective.volumeConstraintValue`
- `objective.volumeConstraintMargin`
- `lsm.volumePhiShiftRaw`
- `lsm.volumePhiShiftApplied`
- `sensitivity.interfacePowerL2`
- `sensitivity.normalVelocityL2`
- `interpolation.powerFeasibilityRatio`
- `interpolation.continuationGateSatisfied`
- `interpolation.hardeningEnabled`

## Interpretation Guardrails

Keep the following findings from
[TurbulenceLSMOptDebugFindings.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/TurbulenceLSMOptDebugFindings.md)
in mind while reading the ladder results:

- volume control is currently split across two mechanisms:
  the optimizer-side volume constraint and the capped global post-update
  `phiLS` shift. A run can therefore look "feasible enough" to the optimizer
  while still being too over-solid to reopen flow.
- the trapped snapshot appears to show a one-iteration diagnostic lag around
  the `Iter 5 -> 6` collapse event, where `xhGrayVolumeFraction` collapses
  before `interfaceBandVolumeFraction` fully catches up. Read those two signals
  together instead of over-interpreting a single iteration in isolation.
- wall-distance and adjoint-fidelity probes are still considered lower-priority
  than the six runtime experiments below. They should only move to the front of
  the queue after the current ladder has been exhausted.

## Experiment Matrix

### 1. `case-respected baseline`

Purpose:

- verify that the run is actually using the explicit case settings after the
  recent profile-fallback fix
- establish the new post-fix reference before tuning anything else

Use the current case settings and confirm the runtime dump now shows the
explicit dictionary values rather than the old profile-forced values.

Key values that should match the dictionaries:

- `betaIncrement = 0.12`
- `alphaRampEarlySlope = 0.08`
- `alphaRampLateFactor = 1.0`
- `continuationFeasibilityTol = 1.15`
- `forceContinuationHardeningUntilIter = 0`
- `forceContinuationHardeningUntilBeta = -1`

Interpretation:

- if the runtime dump still shows `0.2`, `1/7`, `2.0`, or forced hardening to
  `Iter 80`, the experiment setup is still wrong
- if the runtime dump matches the dictionaries, the profile-override bug is no
  longer contaminating the ladder

Success signs:

- collapse is delayed relative to the old `Iter 5` failure
- `volumePhiShiftRaw` grows more slowly
- `PowerDiss` does not jump two orders of magnitude immediately after the first
  blockage

Current status:

- completed
- `Iter 1 -> 4` confirmed the intended case values were active
- `betaIncrementActive = 0.12` and `continuationFeasibilityTolActive = 1.15`
  before collapse
- the collapse still happened on the old schedule:
  `xhGrayVolumeFraction` stayed near `0.923` through `Iter 4`, then dropped to
  `9.624e-04` at `Iter 5`
- `volumePhiShiftRaw` still climbed `0.861 -> 1.149 -> 1.618` across
  `Iter 4 -> 6`
- `PowerDiss` still jumped `10.63 -> 20.21 -> 79.91` across `Iter 4 -> 6`

### 2. `profile baseline`

Purpose:

- isolate whether `branchRefinement400` itself is destabilizing the LSM branch
  even after explicit case values are respected

Change:

- set `experimentControl.profile baseline;`
- keep the other current case values unchanged

Interpretation:

- if the collapse is delayed or softened, the adaptive hardening logic is still
  too aggressive for LSM
- if the run behaves almost the same, the main problem is deeper in the LSM
  update path

Key signals to inspect:

- runtime dump parity
- `xhGrayVolumeFraction`
- `interfaceBandVolumeFraction`
- `volumePhiShiftRaw`
- `PowerDiss`

Current status:

- this is now the active next run
- its purpose is to tell us whether the adaptive profile is still a major
  destabilizer once the explicit case values are respected

### 3. `reduced volume-shift cap`

Purpose:

- test whether the global post-update `phiLS` shift is pushing the design into
  the Heaviside shoulders too early

Change:

- keep the previous best profile choice
- reduce `maxVolumePhiShiftFactor` from `0.10` to `0.02`

Interpretation:

- if collapse is delayed and the interface support stays alive longer, the
  global shift is a major trigger
- if collapse timing is unchanged, the advection or sensitivity path is the
  stronger suspect

Key signals to inspect:

- `volumePhiShiftRaw / volumePhiShiftApplied`
- `xhGrayVolumeFraction`
- `interfaceBandVolumeFraction`
- `interfacePowerL2`

Optional follow-up if this experiment is strongly positive:

- run one pure diagnostic case with `maxVolumePhiShiftFactor = 0.0`
- treat that case as debugging-only, not a production candidate

### 4. `wider interface band`

Purpose:

- test whether `xh` becomes effectively binary too early because the active
  interface band is too narrow for the current advection and shift amplitudes

Change:

- increase `epsilonLSM` from `1.5` to `2.5`
- increase `epsilonLSMMin` from `0.5` to `1.0`

Interpretation:

- if `xhGrayVolumeFraction` and `interfaceBandVolumeFraction` stay alive much
  longer, the failure is strongly tied to projection-shoulder collapse
- if the run still snaps closed quickly, the root cause is more likely the
  velocity field or the global shift logic

Key signals to inspect:

- `xhGrayVolumeFraction`
- `interfaceBandVolumeFraction`
- `normalVelocityL2`
- `interfacePowerL2`

### 5. `Hamilton-Jacobi fallback`

Purpose:

- isolate whether the reaction-diffusion regularization path is damping away
  reopening motion or distorting the update direction

Change:

- in `lsmSwitches` set:
  - `useReactionDiffusionLSMUpdate false;`
  - `usePureHamiltonJacobiFallback true;`

Interpretation:

- if reopening motion survives much longer, the regularization path needs
  closer inspection
- if the collapse still occurs at almost the same time, the main issue is
  earlier in the sensitivity or global-shift path

Key signals to inspect:

- `normalVelocityRawL2`
- `normalVelocityL2`
- `phiStepL2OverSqrtN`
- `interfacePowerL2`

### 6. `power-reopen probe`

Purpose:

- test whether the power constraint is simply too weak to reopen blocked
  channels even when the adjoint remains numerically healthy

Change:

- set `experimentControl.profile adjointSensitivityProbe;`
- increase `optPowerConstraintWeight` from `2.0` to `5.0`
- increase `optPowerViolationScaleExponent` from `1.0` to `2.0`

Interpretation:

- if `interfacePowerL2`, `normalVelocityL2`, and channel reopening all increase
  materially, the reopening path was underpowered
- if the interface sensitivity remains tiny, the problem lies deeper than
  simple power-weight tuning

Key signals to inspect:

- `sensitivity.interfacePowerL2`
- `sensitivity.powerToVolumeL2Ratio`
- `sensitivity.normalVelocityL2`
- `objective.powerDissipationConstraintValue`

## Practical Pass/Fail Heuristics

An experiment is promising if, relative to the current failing run, it shows at
least two of the following:

- the runtime dump exactly matches the intended controls
- gray collapse is delayed well beyond the old `Iter 5` event
- `interfaceBandVolumeFraction` stays clearly above `O(1e-2)` after the first
  power violation
- `interfacePowerL2` remains well above the near-zero values seen after the old
  collapse
- `normalVelocityL2` does not decay to near zero while the power constraint is
  still badly violated
- `volumePhiShiftRaw` is no longer permanently much larger than the applied
  shift

An experiment is not promising if it only changes the objective mildly while
the design still collapses early and the reopening sensitivity still dies.

## If The Runtime Ladder Still Fails

If none of the six runtime experiments restores a viable reopening path, the
next cycle should add temporary code probes:

1. log a histogram of `phiLS / epsilonLSMActive` to distinguish true narrow-band
   collapse from shoulder saturation
2. log the number of cells with meaningful `diracPhiLS` support, not only
   `|phi| <= epsilon`
3. log the power-projection multiplier in
   [lsmSensitivity.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/lsmSensitivity.H)
4. log pre-shift and post-shift fluid fractions to measure how much the capped
   global correction is actually recovering
5. temporarily flag large negative `V` as a stall condition in the debug logic,
   since the current optimizer constraint treats low-fluid states too leniently

Probe 1 is now available in the current branch through:

- `debugOptimizer.log` rows `phi/eps agg`, `phi/eps bins1`, `phi/eps bins2`,
  and `phi/eps bins3`
- `debugOptimizer.jsonl` fields `design.phiOverEpsCoreFraction`,
  `design.phiOverEpsShoulderFraction`,
  `design.phiOverEpsAboveTwoFraction`, and
  `design.phiOverEpsHistogram`

The remaining probes should still be added only after the runtime ladder has
been exhausted.

First probe result from the `case-respected baseline` rerun:

- at `Iter 4`, most of the still-gray design already sits in the negative
  inner shoulder `-1 < phiLS / epsilonLSMActive <= -0.5`
- at `Iter 5`, `xhGrayVolumeFraction` collapses before
  `interfaceBandVolumeFraction` catches up
- by `Iter 6`, almost all non-fluid cells sit in the shoulder range
  `1 < |phiLS / epsilonLSMActive| <= 2`, not in the far-tail `|.| > 2` range

That makes the current collapse look more like projection-shoulder saturation
and narrow-band loss than an immediate blowout to very large `|phi| / epsilon`
values.

## Current Snapshot Reference Values

These are the trapped-run values that later experiments should compare against:

- collapse window: `Iter 4 -> 6`
- `PowerDiss`:
  - `10.63` at `Iter 4`
  - `20.21` at `Iter 5`
  - `79.91` at `Iter 6`
  - `~376-382` in the late trapped regime
- `xhGrayVolumeFraction`:
  - `0.923` before collapse
  - `9.624e-04` immediately after collapse
- `volumePhiShiftRaw`:
  - `0.861` at `Iter 4`
  - `1.149` at `Iter 5`
  - `1.618` at `Iter 6`
  - `~5.47` late in the trapped regime
- `interfacePowerSensitivity L2`:
  - `16.10` at `Iter 4`
  - `11.04` at `Iter 5`
  - `2.71` at `Iter 6`
  - `O(1e-04 to 1e-03)` late in the trapped regime
- `normalVelocityRaw L2`:
  - `30.92` at `Iter 4`
  - `22.89` at `Iter 5`
  - `4.60` at `Iter 6`
  - `O(1e-04 to 1e-03)` late in the trapped regime
