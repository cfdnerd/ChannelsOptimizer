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
  `power-reopen probe` rerun
- the profile-fallback bug in
  [createFields.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/createFields.H)
  has been fixed and validated by the post-fix reruns
- all completed post-fix reruns respected the explicit case dictionaries:
  `betaIncrement = 0.12`,
  `continuationFeasibilityTol = 1.15`,
  `forceContinuationHardeningUntilIter = 0`, and
  `forceContinuationHardeningUntilBeta = -1`
- the Hamilton-Jacobi rerun respected the intended controls
  `lsmUpdateMode = hamiltonJacobi`,
  `experimentProfile = baseline`,
  `maxVolumePhiShiftFactor = 0.10`,
  `epsilonLSM = 2.5`, and `epsilonLSMMin = 1.0`
- the `power-reopen probe` respected the intended controls
  `experimentProfile = adjointSensitivityProbe`,
  `optPowerConstraintWeight = 5.0`,
  `optPowerViolationScaleExponent = 2.0`,
  `useReactionDiffusionLSMUpdate = false`, and
  `usePureHamiltonJacobiFallback = true`
- the `power-reopen probe` still collapsed on the same `Iter 6 -> 8` window
  and did not materially improve the trapped-regime motion
- the first deeper code probes have now been added in
  [opt_initialization.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/opt_initialization.H),
  [lsmSensitivity.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/lsmSensitivity.H),
  [sensitivity.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/sensitivity.H),
  [gradientOptWrite.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/gradientOptWrite.H),
  and [debugOptimizer.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/debugOptimizer.H)

Therefore Experiments 1 through 6 are complete and the runtime ladder itself is
now exhausted.

Immediate next runs:

1. rerun `power-reopen probe` with the new deeper probes active
2. only then decide whether production logic needs to change

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
- `lsm.powerProjectionMultiplier`
- `lsm.diracSupportVolumeFraction`
- `lsm.meaningfulDiracSupportVolumeFraction`
- `lsm.fluidFractionRecoveredByShift`
- `sensitivity.interfacePowerL2`
- `sensitivity.normalVelocityL2`
- `convergence.stalledLowFluidCandidate`
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
- the trapped snapshot now appears to show a multi-step diagnostic lag around
  the `Iter 6 -> 8` collapse event, where `xhGrayVolumeFraction` collapses
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

- completed
- the early runtime dump still matched the intended case values with
  `experimentProfile = baseline`
- collapse timing did not improve:
  `xhGrayVolumeFraction` stayed near `0.923` through `Iter 4`, then still
  dropped to `9.624e-04` at `Iter 5`
- `PowerDiss` still jumped `10.63 -> 20.21 -> 79.91 -> 382.04` across
  `Iter 4 -> 8`
- `volumePhiShiftRaw` still climbed `0.861 -> 1.149 -> 1.618 -> 3.118` across
  `Iter 4 -> 8`, while the applied shift stayed capped near `0.355`
- the late trapped regime was only modestly softer than Experiment 1:
  `PowerDiss` settled around `335-349` instead of `366-371`, and
  `xhFluidVolumeFraction` rose only to about `0.088`

Interpretation:

- `baseline` is slightly better than `branchRefinement400` in the late trapped
  regime, so it is the sensible profile to retain
- but because collapse still happens on the same `Iter 5 -> 6` window and the
  reopening sensitivities still die, the main blocker is deeper than the
  profile choice itself

### 3. `reduced volume-shift cap`

Purpose:

- test whether the global post-update `phiLS` shift is pushing the design into
  the Heaviside shoulders too early

Change:

- keep `experimentControl.profile baseline;`
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

Current status:

- completed
- the reduced cap was genuinely active from the start:
  `volumePhiShiftApplied` fell to about `0.029` at `Iter 1` and `Iter 2`
  instead of the earlier `~0.145`
- the collapse happened earlier, not later:
  `xhGrayVolumeFraction` was already `9.624e-04` by `Iter 3`
- `PowerDiss` jumped `5.36 -> 15.85 -> 51.86 -> 338.45` across
  `Iter 2 -> 5`
- `volumePhiShiftRaw` still grew quickly, reaching about `1.10` at `Iter 3`
  and `1.55` at `Iter 4`
- the late trapped regime was not meaningfully healthier:
  `PowerDiss` settled around `321-325`,
  `xhFluidVolumeFraction` stayed only about `0.081`,
  and `interfacePowerL2` remained only about `1.0e-03`

Interpretation:

- a smaller cap did not delay collapse or preserve reopening support
- the early lower applied shift appears to remove one of the few mechanisms
  still correcting the design, so the capped global shift is more protective
  than causative in the current branch
- this pushes the ladder away from shift-cap tuning and toward interface-band
  width as the next runtime discriminator

### 4. `wider interface band`

Purpose:

- test whether `xh` becomes effectively binary too early because the active
  interface band is too narrow for the current advection and shift amplitudes

Change:

- restore `maxVolumePhiShiftFactor` to `0.10` so this run isolates band width
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

Current status:

- completed
- the runtime dump confirmed the intended controls were active:
  `experimentControl.profile baseline;`
  `maxVolumePhiShiftFactor = 0.10;`
  `epsilonLSM = 2.5;`
  and `epsilonLSMMin = 1.0;`
- `xhGrayVolumeFraction` stayed near `0.923` through `Iter 5`, then collapsed
  to `9.624e-04` at `Iter 6`
- `interfaceBandVolumeFraction` stayed near `0.923` through `Iter 7`, then
  collapsed to `9.624e-04` at `Iter 8`
- `PowerDiss` still climbed `7.54 -> 9.79 -> 15.93 -> 41.61 -> 334.63` across
  `Iter 4 -> 8`
- the late trapped regime still looked essentially blocked:
  `PowerDiss ~382`,
  `xhFluidVolumeFraction ~0.081`,
  `interfaceBandVolumeFraction ~2.03e-03`,
  and `interfacePowerL2` only `O(1e-04 to 1e-03)`

Interpretation:

- wider band helps, but only modestly
- it delays loss of support rather than curing it
- that makes interface-band width contributory, not dominant, and pushes the
  ladder on to the update-path discriminator

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

Current status:

- completed
- the runtime dump confirmed the intended fallback controls were active:
  `experimentControl.profile baseline;`
  `maxVolumePhiShiftFactor = 0.10;`
  `epsilonLSM = 2.5;`
  `epsilonLSMMin = 1.0;`
- in `lsmSwitches`, the run used
  `useReactionDiffusionLSMUpdate false;` and
  `usePureHamiltonJacobiFallback true;`
- `xhGrayVolumeFraction` still stayed near `0.923` through `Iter 5`, then
  collapsed to `9.624e-04` at `Iter 6`
- `interfaceBandVolumeFraction` still stayed near `0.923` through `Iter 7`,
  then collapsed to `9.624e-04` at `Iter 8`
- `PowerDiss` still climbed `7.54 -> 9.79 -> 15.93 -> 41.61 -> 334.63` across
  `Iter 4 -> 8`
- by `Iter 22 -> 24`, `interfacePowerL2` remained only about
  `1.6e-04 -> 6.9e-04` and `normalVelocityL2` only about
  `1.1e-04 -> 5.3e-04`

Interpretation:

- removing the regularization solve did not materially delay collapse
- the fallback changed the bookkeeping, not the trapped outcome
- this pushes the ladder onward to the reopening-strength probe

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

Current status:

- completed
- the runtime dump confirmed that the active case retained the wider-band plus
  Hamilton-Jacobi settings:
  `useReactionDiffusionLSMUpdate false;`
  `usePureHamiltonJacobiFallback true;`
  `maxVolumePhiShiftFactor = 0.10;`
  `epsilonLSM = 2.5;`
  `epsilonLSMMin = 1.0;`
- the latest run also confirmed:
  `experimentControl.profile adjointSensitivityProbe;`
  `optPowerConstraintWeight = 5.0;`
  `optPowerViolationScaleExponent = 2.0;`
- `xhGrayVolumeFraction` still stayed near `0.923` through `Iter 5`, then
  still dropped to `9.624e-04` at `Iter 6`
- `interfaceBandVolumeFraction` still stayed near `0.923` through `Iter 7`,
  then still dropped to `9.624e-04` at `Iter 8`
- `PowerDiss` still climbed `7.54 -> 9.79 -> 15.93 -> 41.61 -> 334.63` across
  `Iter 4 -> 8`
- `interfacePowerL2` was only `11.05 -> 10.20 -> 8.02 -> 3.37 -> 0.120`
  across `Iter 4 -> 8`
- `normalVelocityL2` was only `21.16 -> 21.20 -> 15.83 -> 5.82 -> 0.194`
  across `Iter 4 -> 8`
- by `Iter 20 -> 22`, `interfacePowerL2` remained only about
  `4.4e-04 -> 1.6e-04`
- by `Iter 20 -> 22`, `normalVelocityL2` remained only about
  `3.2e-04 -> 1.1e-04`

Interpretation:

- stronger power weighting did not materially delay collapse
- stronger power weighting did not materially preserve reopening motion
- the remaining issue lies deeper than simple power-weight tuning
- the next step is now an instrumented rerun, not a seventh runtime-control
  permutation

## Practical Pass/Fail Heuristics

An experiment is promising if, relative to the current failing run, it shows at
least two of the following:

- the runtime dump exactly matches the intended controls
- gray collapse is delayed well beyond the current `Iter 6` event
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

Because none of the six runtime experiments restored a viable reopening path,
the branch now carries the next temporary code probes:

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

Probes 2 through 5 are now available in the current branch through:

- `debugOptimizer.log` row `support`
- `debugOptimizer.log` row `power ctl`
- `debugOptimizer.log` row `Stall Guard` with `lowFluid=yes/no`
- `debugOptimizer.jsonl` fields
  `lsm.diracSupportCellCount`,
  `lsm.diracSupportVolumeFraction`,
  `lsm.meaningfulDiracSupportThreshold`,
  `lsm.meaningfulDiracSupportCellCount`,
  `lsm.meaningfulDiracSupportVolumeFraction`,
  `lsm.powerProjectionMultiplier`,
  `lsm.fluidFractionPreShift`,
  `lsm.fluidFractionPostShift`,
  `lsm.fluidFractionRecoveredByShift`, and
  `convergence.stalledLowFluidCandidate`
- `gradientOpt.log` lines `projection control` and `fluid shift`

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

Second probe result from the `profile baseline` rerun:

- the early collapse geometry stayed effectively the same through
  `Iter 5 -> 6`
- by the late trapped regime, `phiOverEpsAboveTwoFraction` was still about
  `0.919`, while `phiOverEpsFarFraction` was only about `9e-03`
- this remains more consistent with a capped-shift, shoulder/outer-band trap
  than with a wholesale blowout to the far tail

That was the main reason the ladder moved next to the reduced-cap experiment.

Third probe result from the `reduced volume-shift cap` rerun:

- the collapse moved earlier to the `Iter 3 -> 4` window
- by the late trapped regime, `phiOverEpsAboveTwoFraction` was still about
  `0.919`, while `phiOverEpsFarFraction` remained only about `2e-03`
- the run still looks like a shoulder/outer-band trap rather than a far-tail
  blowout, even after the cap reduction

That makes a wider interface band more justified than further shift-cap
tuning for the next rung.

Fourth probe result from the `wider interface band` rerun:

- `xhGrayVolumeFraction` stayed high through `Iter 5`, but collapsed at
  `Iter 6`
- `interfaceBandVolumeFraction` stayed high through `Iter 7`, then collapsed
  at `Iter 8`
- by the late trapped regime, `phiOverEpsAboveTwoFraction` was still about
  `0.919`, while `phiOverEpsFarFraction` remained only about `2e-03`
- `interfacePowerL2` and `normalVelocityL2` remained stronger through
  `Iter 6 -> 7` than in the earlier baseline reference, but still decayed to
  only `O(1e-04 to 1e-03)` late in the run

That means the wider band does preserve support a bit longer, but not enough to
avoid the same shoulder/outer-band trap. The next rung should therefore
interrogate the update path itself rather than tuning the band width further.

Fifth probe result from the `Hamilton-Jacobi fallback` rerun:

- the runtime dump switched to `lsmUpdateMode = hamiltonJacobi`
- `solverConvergences.log` shows `Vn regularize` as `SKIP`, so the fallback was
  genuinely active
- despite that, `xhGrayVolumeFraction` still collapsed at `Iter 6` and
  `interfaceBandVolumeFraction` still caught up only at `Iter 8`
- `interfacePowerL2` still fell to `0.124` at `Iter 8`, then to only
  `O(1e-04 to 1e-03)` by `Iter 22 -> 24`
- `normalVelocityRawL2` and `normalVelocityL2` became identical, as expected,
  but they still decayed to only `O(1e-04 to 1e-03)` in the trapped regime

That makes the Hamilton-Jacobi fallback effectively neutral. The next rung
should therefore test whether the reopening signal itself is too weak, rather
than continuing to tune the update path.

## Current Snapshot Reference Values

These are the trapped-run values that later experiments should compare against:

- collapse window:
  `xhGrayVolumeFraction` collapses at `Iter 6`,
  while `interfaceBandVolumeFraction` does not fully catch up until `Iter 8`
- `PowerDiss`:
  - `7.54` at `Iter 4`
  - `9.79` at `Iter 5`
  - `15.93` at `Iter 6`
  - `41.61` at `Iter 7`
  - `334.63` at `Iter 8`
  - `~387-403` in the late trapped regime covered by the current snapshot
- `xhGrayVolumeFraction`:
  - `0.923` before collapse
  - `9.624e-04` at `Iter 6`
- `interfaceBandVolumeFraction`:
  - `0.923` through `Iter 7`
  - `9.624e-04` at `Iter 8`
- `volumePhiShiftRaw`:
  - `1.087` at `Iter 4`
  - `1.298` at `Iter 5`
  - `1.681` at `Iter 6`
  - `2.328` at `Iter 7`
  - `3.307` at `Iter 8`
  - `~9.05` late in the trapped regime
- `interfacePowerSensitivity L2`:
  - `11.33` at `Iter 4`
  - `10.70` at `Iter 5`
  - `8.51` at `Iter 6`
  - `3.55` at `Iter 7`
  - `0.124` at `Iter 8`
  - `~1.6e-04 -> 6.9e-04` by `Iter 22 -> 24`
- `normalVelocityRaw L2`:
  - `22.78` at `Iter 4`
  - `22.37` at `Iter 5`
  - `18.35` at `Iter 6`
  - `6.55` at `Iter 7`
  - `0.202` at `Iter 8`
  - `~1.1e-04 -> 5.3e-04` by `Iter 22 -> 24`
- `normalVelocity L2`:
  - identical to `normalVelocityRaw L2` throughout the Hamilton-Jacobi run
