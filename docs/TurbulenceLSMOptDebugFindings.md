# turbulenceLSMOpt Debug Findings

This note records the current debugging position for `turbulenceLSMOpt` and
summarizes the failure mechanisms visible in the latest `optimizerlogs/`
snapshot, which is now the tightened-trap-guard `power-reopen probe` rerun.

Reference sources:

- `optimizerlogs/`
- [createFields.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/createFields.H)
- [sensitivity.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/sensitivity.H)
- [lsmSensitivity.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/lsmSensitivity.H)
- [update.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/update.H)
- [TurbulenceMMAOptDebugFindings.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/TurbulenceMMAOptDebugFindings.md)

## Outcome

Current diagnosis:

- The latest LSM run is numerically stable. The linear solvers are not the main
  failure mechanism.
- The dominant failure is an early level-set collapse followed by an inability
  to reopen blocked channels under severe power violation.
- The tightened-trap-guard `power-reopen probe` rerun still does not materially
  change the collapse timing or trapped-regime behavior relative to the
  `Hamilton-Jacobi fallback` reference, but it confirms that the new guard can
  stop the branch shortly after the trapped regime becomes unmistakable.
- The turbulence physics backbone is probably not the primary blocker, because
  `turbulenceMMAOpt` already works with the same low-`q` turbulent baseline.
- The main suspects are LSM-specific:
  - loss of meaningful `diracPhiLS` support once the design starts collapsing
  - a power-projection path that may still be too weak after closure even when
    the power weighting is increased
  - global post-update `phiLS` shifts that stay permanently capped while
    recovering only a small amount of fluid volume
  - continuation and stall logic that becomes too lenient once the branch falls
    into a low-fluid trapped state

## Current Debugging Status

Completed in the current cycle:

- examined the latest `optimizerlogs/` snapshot and confirmed that it is the
  tightened-trap-guard `power-reopen probe` rerun
- confirmed from `solverConvergences.log` that the LSM branch is not failing by
  linear-solver blow-up
- identified a code-level control issue where `branchRefinement400` was
  overriding explicit case settings in
  [createFields.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/createFields.H)
- corrected that profile behavior so explicit case values now win
- captured a clean `case-respected baseline` rerun and verified from
  `optimizerlogs/` that the intended case values were active in the early
  iterations
- verified from the current runtime dump that the Hamilton-Jacobi fallback was
  genuinely active from the start of the latest rerun:
  `lsmUpdateMode = hamiltonJacobi`,
  `useReactionDiffusionLSMUpdate = false`,
  `usePureHamiltonJacobiFallback = true`,
  `experimentProfile = adjointSensitivityProbe`,
  `optPowerConstraintWeight = 5.0`,
  `optPowerViolationScaleExponent = 2.0`,
  `epsilonLSM = 2.5`, and
  `epsilonLSMMin = 1.0`
- confirmed from `solverConvergences.log` that the fallback path skipped the
  `Vn regularize` solve entirely, while the primary and adjoint systems stayed
  healthy
- confirmed that the `power-reopen probe` did not materially move the collapse
  window or the trapped-regime sensitivity decay relative to the
  `Hamilton-Jacobi fallback` reference
- confirmed from the new probe fields that meaningful `diracPhiLS` support
  collapses before the broad `|phiLS| <= epsilonLSMActive` band catches up:
  at `Iter 7`,
  `meaningfulDiracSupportVolumeFraction` is already `9.624e-04` while
  `interfaceBandVolumeFraction` is still `0.923`
- confirmed that `powerProjectionMultiplier` only acts briefly:
  it is `0.341` at `Iter 6`,
  `0.187` at `Iter 7`,
  and already back to `0` from `Iter 8` onward
- confirmed that the capped global shift becomes almost useless immediately
  after collapse:
  `fluidFractionRecoveredByShift` falls
  `3.81e-02 -> 1.14e-03 -> 2.06e-04` across `Iter 6 -> 8`,
  and remains only about `2.5e-04 -> 3.0e-04` late in the run
- added the first deeper LSM code probes in
  [opt_initialization.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/opt_initialization.H),
  [lsmSensitivity.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/lsmSensitivity.H),
  [sensitivity.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/sensitivity.H),
  [gradientOptWrite.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/gradientOptWrite.H),
  and [debugOptimizer.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/debugOptimizer.H)
  so the next rerun exposes:
  meaningful `diracPhiLS` support,
  `powerProjectionMultiplier`,
  pre/post-shift fluid fractions, and
  a low-fluid stalled-infeasible flag
- tightened the temporary low-fluid stalled-infeasible guard in
  [debugOptimizer.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/debugOptimizer.H)
  so it now keys off
  low fluid fraction,
  lost meaningful `diracPhiLS` support, and
  negligible shift recovery,
  instead of waiting for `designStepFrozen`
- captured the tightened-guard rerun and confirmed that
  `stalledLowFluidCandidate` switches on at `Iter 7` and the run stops at
  `Iter 11` with `reason = stalledLowFluidInfeasible`
- added the next reopening-path debug change in
  [lsmSensitivity.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/lsmSensitivity.H),
  [opt_initialization.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/opt_initialization.H),
  [gradientOptWrite.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/gradientOptWrite.H),
  and [debugOptimizer.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/debugOptimizer.H):
  a debug-scoped power-support floor for `adjointSensitivityProbe` that keeps
  the power sensitivity alive across the remaining interface band once the
  power constraint is active
- documented the first LSM-specific runtime experiment ladder in
  [TurbulenceLSMOptCollapseRecoveryExperimentPlan.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/TurbulenceLSMOptCollapseRecoveryExperimentPlan.md)

Still pending:

- rerun the active `power-reopen probe` case with the broadened power-support
  fallback
- inspect whether `lsm.powerSupportFloorVolumeFraction`,
  `sensitivity.interfacePowerL2`, and `sensitivity.normalVelocityL2`
  stay materially higher through `Iter 7 -> 8`

Practical meaning:

- the current logs now include valid post-fix references for
  `case-respected baseline`, `profile baseline`,
  `reduced volume-shift cap`, `wider interface band`, and
  `Hamilton-Jacobi fallback`, and `power-reopen probe`
- they show that fixing the profile override was necessary, but not sufficient,
  and that changing from `branchRefinement400` to `baseline` is also not
  sufficient, to prevent the early LSM collapse
- they also show that reducing the shift cap makes the collapse earlier,
  widening the interface band only delays collapse modestly, switching to
  Hamilton-Jacobi does not materially improve reopening, and strengthening the
  power weighting also does not materially improve reopening
- they now also show that the broad `|phi| <= epsilon` band overstates usable
  reopening support after collapse, because meaningful `diracPhiLS` support is
  already gone by `Iter 7`
- they also show that the tightened trapped-state guard now stops the run on
  the expected schedule, so the next useful discriminator is reopening support
  itself rather than more stop-logic tuning

## First Ladder Result

### `case-respected baseline` passed the control-parity check but still failed

The new rerun confirms that the explicit case settings are now actually active
before collapse:

- `betaIncrementActive = 0.12`
- `continuationFeasibilityTolActive = 1.15`
- `forceContinuationHardeningUntilIter = 0`
- `forceContinuationHardeningUntilBeta = -1`

That means the old hidden-profile bug is no longer contaminating the ladder.

However, the behavioral outcome did not materially improve:

- `xhGrayVolumeFraction` stayed near `0.923` through `Iter 4`, then dropped to
  `9.624e-04` at `Iter 5`
- `PowerDiss` still jumped `10.63 -> 20.21 -> 79.91` across `Iter 4 -> 6`
- `volumePhiShiftRaw` still climbed `0.861 -> 1.149 -> 1.618` across
  `Iter 4 -> 6`
- by the late trapped regime, `PowerDiss` was still about `366-371`

Practical conclusion:

- Experiment 1 is complete
- the next required discriminator at that point was `profile baseline`
- if that run still collapsed on nearly the same schedule, the dominant
  blocker is deeper than the `branchRefinement400` profile alone

### The new `phi/eps` probe points to shoulder saturation, not immediate far-tail blowout

The added `phiLS / epsilonLSMActive` histogram gives a more specific picture of
the collapse geometry:

- at `Iter 4`, most of the still-gray region already lies in
  `-1 < phiLS / epsilonLSMActive <= -0.5`
- at `Iter 5`, `xhGrayVolumeFraction` collapses while
  `interfaceBandVolumeFraction` still shows the old high value for one
  iteration
- by `Iter 6`, almost all non-fluid volume has moved into
  `1 < |phiLS / epsilonLSMActive| <= 2`
- the far-tail fraction `|phiLS / epsilonLSMActive| > 2` is still essentially
  zero at the actual collapse event

Implication:

- the current failure looks more like projection-shoulder saturation and loss
  of usable narrow-band support than an immediate explosion to very large
  `|phi| / epsilon` values
- this kept Experiments 2 through 4 in the same order and increased the value
  of comparing `baseline` against `branchRefinement400` before moving on to cap
  or band-width changes

## Second Ladder Result

### `profile baseline` did not move the collapse window

The `profile baseline` rerun confirms that the profile switch was genuinely
applied:

- `experimentProfile = baseline`
- `betaIncrementActive = 0.12`
- `continuationFeasibilityTolActive = 1.15`
- `forceContinuationHardeningUntilIter = 0`
- `forceContinuationHardeningUntilBeta = -1`

But the key failure timing stayed the same:

- `xhGrayVolumeFraction` stayed near `0.923` through `Iter 4`
- `xhGrayVolumeFraction` still collapsed to `9.624e-04` at `Iter 5`
- `PowerDiss` still jumped `10.63 -> 20.21 -> 79.91 -> 382.04` across
  `Iter 4 -> 8`
- `volumePhiShiftRaw` still climbed `0.861 -> 1.149 -> 1.618 -> 3.118` across
  `Iter 4 -> 8` while the applied shift remained capped near `0.355`

That means `branchRefinement400` is not the dominant reason the optimizer
falls into the trapped regime.

### `profile baseline` only softened the late trapped regime modestly

Relative to the earlier `branchRefinement400` trapped reference, the late
baseline-profile state is somewhat less blocked, but not enough to count as a
clear recovery:

- late `PowerDiss` fell only to about `335-349`, not to anything near the
  feasible regime
- `xhFluidVolumeFraction` improved only to about `0.088`
- `interfaceBandVolumeFraction` remained only about `2.18e-03`
- `interfacePowerL2` remained around `1.3e-03`
- `normalVelocityL2` remained around `8e-04`
- `volumePhiShiftRaw` still sat near `5.46` while the applied shift stayed
  capped at about `0.355`

Practical conclusion:

- `baseline` is the slightly better profile and should be retained
- but at that point the next useful discriminator was no longer another
  profile tweak
- at that point the strongest remaining runtime suspect still looked like the
  capped global post-update `phiLS` shift

### The late `phi/eps` histogram still supports the shift-cap hypothesis

By the late trapped regime in the baseline-profile run:

- `phiOverEpsAboveTwoFraction` was still about `0.919`
- `phiOverEpsFarFraction` was only about `9e-03`

So even after many trapped iterations, the state still looks more like a
shoulder/outer-band trap than a far-tail blowout. That was the main reason the
ladder moved next to the reduced-cap experiment rather than skipping straight
to interface-band or wall-distance surgery.

## Third Ladder Result

### `reduced volume-shift cap` made the collapse earlier

The reduced-cap rerun kept the intended post-fix control surface:

- `experimentProfile = baseline`
- `betaIncrementActive = 0.12`
- `continuationFeasibilityTolActive = 1.15`
- `forceContinuationHardeningUntilIter = 0`
- `forceContinuationHardeningUntilBeta = -1`

But the cap change itself was strongly visible in the early iterations:

- `volumePhiShiftApplied` fell to about `0.029` at `Iter 1` and `Iter 2`
  instead of the earlier `~0.145`

That change did not help. It made the run fail sooner:

- `xhGrayVolumeFraction` was already `9.624e-04` by `Iter 3`
- `PowerDiss` rose `5.36 -> 15.85 -> 51.86 -> 338.45` across `Iter 2 -> 5`
- `volumePhiShiftRaw` still grew quickly to about `1.10` at `Iter 3`
  and `1.55` at `Iter 4`

Practical conclusion:

- the capped global post-update shift is not the main trigger of the collapse
- in the current branch, the shift cap appears more compensatory than
  destabilizing
- reducing it removes recovery capacity and lets the design lock shut sooner

### The reduced-cap run still died as a narrow-band/outer-band trap

Even in the reduced-cap run, the geometry of failure did not look like a
far-tail blowout:

- the usual one-iteration lag remained around the collapse event, with
  `xhGrayVolumeFraction` collapsing before `interfaceBandVolumeFraction`
  catches up
- by the late trapped regime, `phiOverEpsAboveTwoFraction` was still about
  `0.919`
- `phiOverEpsFarFraction` remained only about `2e-03`
- `interfacePowerL2` remained only about `1.05e-03`
- `normalVelocityL2` remained only about `7.6e-04`

That keeps the strongest remaining runtime hypothesis on interface-band width
and loss of usable support rather than on excessive global shift amplitude.

## Fourth Ladder Result

### `wider interface band` delayed collapse modestly but did not restore reopening

The wider-band rerun confirmed that the intended controls were genuinely
active:

- `experimentProfile = baseline`
- `maxVolumePhiShiftFactor = 0.10`
- `epsilonLSM = 2.5`
- `epsilonLSMMin = 1.0`

Relative to the earlier baseline-profile reference, the wider band did buy a
small amount of extra survival time:

- `xhGrayVolumeFraction` stayed near `0.923` through `Iter 5`, then dropped to
  `9.624e-04` at `Iter 6`
- `interfaceBandVolumeFraction` stayed near `0.923` through `Iter 7`, then
  dropped to `9.624e-04` at `Iter 8`
- `PowerDiss` rose `7.54 -> 9.79 -> 15.93 -> 41.61 -> 334.63` across
  `Iter 4 -> 8`
- `volumePhiShiftRaw` rose `1.087 -> 1.298 -> 1.681 -> 2.328 -> 3.307` across
  `Iter 4 -> 8` while the applied shift stayed capped near `0.589`
- `interfacePowerL2` stayed at `11.33` at `Iter 4`, `10.70` at `Iter 5`,
  `8.51` at `Iter 6`, and `3.55` at `Iter 7`, before falling to `0.124` at
  `Iter 8`

Practical conclusion:

- the wider band delayed the snap-to-solid event, but only by about one
  optimizer iteration for `xh`
- it preserved usable interface support slightly longer, but not long enough to
  reopen the blocked channels
- interface-band width therefore matters, but it is not the dominant remaining
  blocker

### The wider-band late state is still the same trapped outer-band regime

By the late trapped regime in the wider-band run:

- `PowerDiss` still sat near `382`
- `xhFluidVolumeFraction` stayed only about `0.081`
- `interfaceBandVolumeFraction` remained only about `2.03e-03`
- `interfacePowerL2` remained only about `1.9e-04` to `7.5e-04`
- `normalVelocityL2` remained only about `1.3e-04` to `5.9e-04`
- `volumePhiShiftRaw` still sat near `9.05` while the applied shift stayed
  capped near `0.589`

Implication:

- the extra band width delays the loss of usable support, but the run still
  falls into the same nearly motionless over-solid trap

## Fifth Ladder Result

### `Hamilton-Jacobi fallback` did not move the collapse window

The Hamilton-Jacobi rerun confirmed that the fallback path was genuinely
active:

- `lsmUpdateMode = hamiltonJacobi`
- `useReactionDiffusionLSMUpdate = false`
- `usePureHamiltonJacobiFallback = true`
- `experimentProfile = baseline`
- `epsilonLSM = 2.5`
- `epsilonLSMMin = 1.0`

But the key failure timing stayed effectively the same as the wider-band run:

- `xhGrayVolumeFraction` stayed near `0.923` through `Iter 5`, then dropped to
  `9.624e-04` at `Iter 6`
- `interfaceBandVolumeFraction` stayed near `0.923` through `Iter 7`, then
  dropped to `9.624e-04` at `Iter 8`
- `PowerDiss` still rose `7.54 -> 9.79 -> 15.93 -> 41.61 -> 334.63` across
  `Iter 4 -> 8`
- `volumePhiShiftRaw` still rose `1.087 -> 1.298 -> 1.681 -> 2.328 -> 3.307`
  across `Iter 4 -> 8`, while the applied shift stayed capped near `0.589`

Practical conclusion:

- removing the reaction-diffusion regularization solve does not delay the snap
  to an over-solid state
- the dominant blocker is therefore earlier than the regularized update path
  itself

### `Hamilton-Jacobi fallback` only changed the bookkeeping, not the trapped regime

The fallback path did change one expected implementation detail:

- `normalVelocityRawL2` and `normalVelocityL2` became identical
- `solverConvergences.log` shows `Vn regularize` as `SKIP` throughout the run

But the trapped regime itself did not improve materially:

- by `Iter 8`, `interfacePowerL2` had still fallen to `0.124`
- by `Iter 9`, `interfacePowerL2` was already down near `2.7e-04`
- by `Iter 22 -> 24`, `interfacePowerL2` remained only about
  `1.6e-04 -> 6.9e-04`
- by `Iter 22 -> 24`, `normalVelocityL2` remained only about
  `1.1e-04 -> 5.3e-04`
- `PowerDiss` still sat around `387-403` in the late trapped state covered by
  the current snapshot

Implication:

- the simplified advection fallback is not what restores reopening
- the remaining runtime discriminator should now target whether the power
  constraint signal is simply too weak

### The next clean discriminator is `power-reopen probe`

Because the Hamilton-Jacobi fallback was effectively neutral, the next
experiment should retain the wider-band plus Hamilton-Jacobi settings and
strengthen the reopening signal itself:

- set `experimentControl.profile adjointSensitivityProbe;`
- keep `useReactionDiffusionLSMUpdate false;`
- keep `usePureHamiltonJacobiFallback true;`
- increase `optPowerConstraintWeight` from `2.0` to `5.0`
- increase `optPowerViolationScaleExponent` from `1.0` to `2.0`

This isolates whether the remaining failure is mainly an underpowered
reopening sensitivity rather than an update-path defect.

## Sixth Ladder Result

### `power-reopen probe` did not move the collapse window

The latest rerun confirmed that the intended controls were genuinely active:

- `experimentProfile = adjointSensitivityProbe`
- `optPowerConstraintWeight = 5.0`
- `optPowerViolationScaleExponent = 2.0`
- `lsmUpdateMode = hamiltonJacobi`
- `useReactionDiffusionLSMUpdate = false`
- `usePureHamiltonJacobiFallback = true`
- `epsilonLSM = 2.5`
- `epsilonLSMMin = 1.0`

But the physical failure timing still matched the previous rung:

- `xhGrayVolumeFraction` stayed near `0.923` through `Iter 5`, then still
  dropped to `9.624e-04` at `Iter 6`
- `interfaceBandVolumeFraction` stayed near `0.923` through `Iter 7`, then
  still dropped to `9.624e-04` at `Iter 8`
- `PowerDiss` still rose `7.54 -> 9.79 -> 15.93 -> 41.61 -> 334.63` across
  `Iter 4 -> 8`
- `volumePhiShiftRaw` still rose `1.087 -> 1.298 -> 1.681 -> 2.328 -> 3.307`
  across `Iter 4 -> 8`, while the applied shift stayed capped near `0.589`

Practical conclusion:

- stronger power weighting does not delay the early snap-to-solid event
- the main blocker is therefore deeper than simple
  `optPowerConstraintWeight` or `optPowerViolationScaleExponent` tuning

### `power-reopen probe` also failed to preserve reopening motion after closure

The early post-closure motion was not materially stronger than in the
Hamilton-Jacobi reference:

- `interfacePowerL2` was `11.05` at `Iter 4`, `10.20` at `Iter 5`,
  `8.02` at `Iter 6`, `3.37` at `Iter 7`, and `0.120` at `Iter 8`
- `normalVelocityL2` was `21.16` at `Iter 4`, `21.20` at `Iter 5`,
  `15.83` at `Iter 6`, `5.82` at `Iter 7`, and `0.194` at `Iter 8`
- by `Iter 20 -> 22`, `interfacePowerL2` was still only about
  `4.4e-04 -> 1.6e-04`
- by `Iter 20 -> 22`, `normalVelocityL2` was still only about
  `3.2e-04 -> 1.1e-04`
- `PowerDiss` still sat near `392` in the late trapped regime covered by the
  latest snapshot

Implication:

- the stronger reopening weight does not keep the interface motion alive after
  the blocked state has formed
- the six-rung runtime ladder is now exhausted
- the next run should be the same case with the tightened low-fluid trap
  guard, not another runtime-control permutation

### The instrumented rerun sharpens the trapped-state signature

The new deeper probes show why the branch stops reopening even though the broad
band diagnostics still look alive for one extra step:

- at `Iter 6`, `diracSupportVolumeFraction` and
  `meaningfulDiracSupportVolumeFraction` are still both `0.923`, and
  `powerProjectionMultiplier` reaches `0.341`
- at `Iter 7`, `diracSupportVolumeFraction` is still `0.923`, but
  `meaningfulDiracSupportVolumeFraction` has already collapsed to
  `9.624e-04`
- at the same `Iter 7`, `powerProjectionMultiplier` has already softened to
  `0.187`, and `fluidFractionRecoveredByShift` is only `1.14e-03`
- by `Iter 8`, `diracSupportVolumeFraction` itself has collapsed to
  `9.624e-04`, `powerProjectionMultiplier` is back to `0`, and
  `fluidFractionRecoveredByShift` is only `2.06e-04`

Implication:

- the usable interface support dies before the older band metric fully admits
  it
- the power projection only has a two-iteration window in which it can still
  act
- once that window closes, the capped global shift is no longer recovering a
  meaningful amount of fluid volume

### The tightened trap guard now trips on the intended trapped regime

The new rerun with the revised low-fluid guard confirms that the stop logic is
now tracking the trapped state rather than waiting indefinitely:

- `stalledLowFluidCandidate` is still `0` through `Iter 6`
- it switches to `1` at `Iter 7` exactly when meaningful support has already
  collapsed to `9.624e-04`
- it stays `1` through `Iter 11`
- the run stops at `Iter 11` with
  `reason = stalledLowFluidInfeasible`

Implication:

- the temporary debug stop logic is now behaving as intended
- the remaining problem is not how long the branch runs after trapping
- the next code change needs to target reopening support before `Iter 8`, not
  stop detection after `Iter 8`

### The next reopening-path probe is broader power support

Because the newer logs show that the broad interface band still exists at
`Iter 7` even after meaningful `diracPhiLS` support has already collapsed, the
next debug change keeps the power sensitivity alive across that remaining band
for `adjointSensitivityProbe`:

- apply a power-support floor of `0.1 / epsilonLSMActive`
- only apply it when the power constraint is active
- only apply it to the power sensitivity, not to the objective or volume
  sensitivities

Target outcome for the next rerun:

- `lsm.powerSupportFloorVolumeFraction` stays large at `Iter 7`
- `sensitivity.interfacePowerL2` remains materially above the old
  `3.37 -> 0.120` drop across `Iter 7 -> 8`
- `sensitivity.normalVelocityL2` remains materially above the old
  `5.82 -> 0.194` drop across `Iter 7 -> 8`

## Latest-Run Failure Sequence

### 1. Iterations 1-5: stable but drifting toward closure

Observed in the latest logs:

- `xhGrayVolumeFraction` stays near `0.923`
- `PowerDiss` rises from `3.19` to `9.79`
- `volumePhiShiftRaw` grows from `0.028` to `1.298`
- `volumePhiShiftApplied` is cap-limited from `Iter 2` onward and reaches
  about `0.59` by `Iter 4 -> 5`
- solver health remains `OK`

Interpretation:

- the run is still numerically healthy
- but the interface is already being pushed hard by the global volume
  correction before power feasibility is recovered

### 2. Iteration 6: `xh` collapses while the band diagnostics still lag

At `Iter 6`:

- `xhGrayVolumeFraction` drops from `0.923` to `9.624e-04`
- `interfaceBandVolumeFraction` still reports the old high value for the
  second lagged iteration
- `PowerDiss` crosses into hard violation at `15.93`
- `volumePhiShiftRaw` grows to `1.681` while `volumePhiShiftApplied` remains
  capped at about `0.589`

Interpretation:

- the design effectively snaps into an over-solid configuration
- the run has not crashed, but it has already left the viable
  channel-evolution path

### 3. Iterations 7-24: band support briefly lingers, then reopening dies

After collapse:

- `interfaceBandVolumeFraction` stays high for `Iter 7`, then collapses to
  `9.624e-04` at `Iter 8`
- `PowerDiss` jumps to `41.61` at `Iter 7`, then `334.63` at `Iter 8`, then
  stays around `392-404` in the trapped regime covered by the current snapshot
- `interfacePowerSensitivity L2` falls from `3.37` at `Iter 7` to `0.120` at
  `Iter 8`, then to `O(1e-04)` late in the run
- `normalVelocity L2` falls from `5.82` at `Iter 7` to `0.194` at `Iter 8`,
  then to `O(1e-04)` late in the run
- `volumePhiShiftRaw` climbs to about `9.05` while the applied shift stays
  fixed at the cap
- the objective changes only weakly once the channel is blocked

Interpretation:

- the wider band prolongs the interface-support region slightly, but not long
  enough to recover a viable reopening path
- once that support finally collapses, the power adjoint no longer generates
  enough motion to reopen the flow path
- the run becomes dynamically trapped even though the power constraint is badly
  violated

## Completed Code-Level Finding

### Profile overrides were stronger than the case dictionaries

The pre-fix failing run's runtime dump showed:

- `betaIncrement = 0.2`
- `alphaRampEarlySlope = 1/7`
- `continuationFeasibilityTol = 2.0`
- forced hardening until `Iter 80` and `beta = 16`

Those values were harsher than the checked-in case dictionaries. The cause was
that `branchRefinement400` in
[createFields.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceLSMOpt/src/createFields.H)
was overwriting explicit case settings instead of acting as a fallback.

This has now been corrected so that explicit case values win.

Implication:

- every future LSM debug run must first verify the runtime option dump
- otherwise the experiment being tested may not be the experiment actually run

Observed mismatch in the pre-fix logs:

- runtime dump showed `betaIncrement = 0.2` instead of case `0.12`
- runtime dump showed `alphaRampEarlySlope = 1/7` instead of case `0.08`
- runtime dump showed `continuationFeasibilityTol = 2.0` instead of case `1.15`
- runtime dump showed hardening-floor values that were not intended by the case

## Remaining Primary Findings

### 1. Low startup `q` is already in place

Unlike the original MMA failure, the current LSM run is already using:

- `qAlpha = qKappa = qHeat = 0.005`
- `useSingleQFallback = true`

Therefore the dominant LSM failure is not the same startup-interpolation
problem that dominated `turbulenceMMAOpt`.

### 2. The LSM branch is failing in its reopening path, not mainly in its physics solves

Evidence:

- `solverConvergences.log` stays healthy
- no `NaN` or divergence cascade appears
- the objective improves smoothly until the topology blocks the channel

This points the investigation toward:

- interface sensitivity mapping
- power-weighting and reopening strength
- global `phiLS` shift correction
- continuation and stall logic after collapse

### 3. The post-update global `phiLS` shift is saturated almost immediately

Observed behavior:

- `volumePhiShiftRaw` grows monotonically
- `volumePhiShiftApplied` stays clamped at the cap
- the cap remains hit for many consecutive iterations

Interpretation:

- the run is repeatedly asking for a large global recovery shift
- but the capped correction is too small to restore the target fluid fraction
- once the interface-support region is weak, this correction no longer repairs
  the topology
- in the current trapped logs, `volumePhiShiftRaw` reaches about `9.05` while
  the applied shift remains near `0.589`

### 4. Volume control is split across two inconsistent mechanisms

Current LSM behavior combines:

- a global post-update `phiLS` shift that tries to steer back toward `voluse`
- an optimizer constraint `gx[1] = V` that still behaves like an upper-bound
  form `V <= 0`

Practical consequence:

- once the applied global shift saturates, low-fluid states can remain
  "feasible enough" from the optimizer perspective while still being physically
  unusable for flow
- this is one important difference from `turbulenceMMAOpt`, where MMA carries
  the volume state directly inside the optimizer step

### 5. Power-reopening sensitivity is too weak after closure

By mid-run:

- `interfacePowerSensitivity` becomes extremely small
- `objectiveToVolumeL2Ratio` and `powerToVolumeL2Ratio` remain around
  `O(1e-2)` to `O(1e-1)`
- `normalVelocity` nearly vanishes

Interpretation:

- the blocked design is not receiving a strong enough reopening signal from the
  power constraint
- this is the most important remaining functional blocker once the early
  collapse has happened

### 6. The broad interface-band metric overstates usable support after collapse

In the current snapshot:

- at `Iter 6`, `xhGrayVolumeFraction` already says the design is almost binary
- but `interfaceBandVolumeFraction` still reports the old high-band value
  through `Iter 7`
- at `Iter 7`, `meaningfulDiracSupportVolumeFraction` is already only
  `9.624e-04` even though `interfaceBandVolumeFraction` is still `0.923`
- by `Iter 8`, all three fields agree that the usable support is essentially
  gone

Interpretation:

- the older `|phi| <= epsilon` band metric is too generous for diagnosing the
  actual reopening window
- the more important signal is now meaningful `diracPhiLS` support rather than
  the broad band fraction alone
- this is not just bookkeeping noise; it changes which late-collapse signals
  should drive the next code changes

### 7. The first low-fluid trap guard was still too lenient

In the instrumented snapshot:

- `volumeShiftCapped` is already `yes` from the early run onward
- `V` is already about `-0.322` from `Iter 8` onward
- `stalledLowFluidCandidate` still remains `0` throughout the current
  `Iter 1 -> 26` snapshot

Interpretation:

- waiting for `designStepFrozen` is too strict for this LSM branch
- the trapped regime still takes large capped `phiLS` steps, so step-size
  freezing is not the right stop criterion
- that is why the debug guard has now been changed to use
  low fluid,
  lost meaningful support, and
  negligible shift recovery instead

### 8. The next likely blocker is strict power localization, not stop logic

In the tightened-guard rerun:

- `stalledLowFluidCandidate` now behaves correctly
- but the collapse still reaches the same `Iter 7 -> 8` no-support window
  before the run stops
- at `Iter 7`, the broad band is still present while meaningful support is not

Interpretation:

- the next change should act earlier than the stop logic
- the clearest remaining candidate is the strict use of `diracPhiLS` in
  `interfacePowerSensitivity`, which is now being relaxed in a debug-scoped
  way for the next rerun

## Comparison To `turbulenceMMAOpt`

[TurbulenceMMAOptDebugFindings.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/TurbulenceMMAOptDebugFindings.md)
established that the turbulence backbone can behave well when startup controls
are reasonable.

That means the most useful branch-to-branch comparison is now:

- not "laminar versus turbulent physics"
- but "working MMA design evolution versus failing LSM design evolution"

The shared pieces are already good enough to reuse:

- primal turbulent solves
- frozen-turbulence adjoint
- low-`q` interpolation regime
- structured debug logs

The non-shared pieces are where the current LSM risk sits:

- `phiLS -> xh` reconstruction behavior
- interface sensitivity support
- global `phiLS` volume correction
- LSM transport and reinitialization

## Practical Guidance For Future LSM Debugging

Focus on these signals first:

| Signal | Why it matters |
|---|---|
| runtime option dump | verifies the case was not silently altered by a profile |
| `xhGrayVolumeFraction` and `interfaceBandVolumeFraction` | shows whether the interface support is collapsing too early |
| `volumePhiShiftRaw`, `volumePhiShiftApplied`, `volumePhiShiftCap` | shows whether global volume recovery is saturated |
| `interfacePowerSensitivity L2` | shows whether the power constraint can still reopen channels |
| `normalVelocity L2` | shows whether the LSM update still has meaningful motion |
| `lsm.diracSupportVolumeFraction` and `lsm.meaningfulDiracSupportVolumeFraction` | shows whether the `diracPhiLS` support is collapsing before `|phi| <= epsilon` fully disappears |
| `lsm.powerProjectionMultiplier` and `lsm.fluidFractionRecoveredByShift` | shows whether the stronger power weighting or the capped global shift is actually moving the trapped state |
| `powerFeasibilityRatio` and `continuationGateSatisfied` | shows whether continuation logic still matters after collapse |
| `pMax`, `Umax` | shows whether the blocked channel is driving hydraulic blow-up |

## Recommended Debug Order

When `turbulenceLSMOpt` fails, inspect in this order:

1. Verify the runtime dump matches the intended dictionaries.
2. Check whether global `phiLS` shift saturation starts before the first power
   blow-up.
3. Check whether `xh` collapses to near-binary before `PowerDiss` becomes
   feasible.
4. Check whether `interfacePowerSensitivity` and `normalVelocity` survive after
   the first blockage event.
5. Only then tune continuation or late-stage hardening.
6. Only after the above should wall-distance and adjoint-fidelity probes become
   the main focus.

This order should prevent LSM debugging from getting stuck in the same
parameter-chasing loops that were only appropriate for the MMA branch.

## Immediate Next Action

The current ladder position is:

1. code-level profile-fallback bug fixed
2. `case-respected baseline` rerun captured and characterized
3. `profile baseline` rerun captured and characterized
4. `reduced volume-shift cap` rerun captured and characterized
5. `wider interface band` rerun captured and characterized
6. `Hamilton-Jacobi fallback` rerun captured and characterized
7. `power-reopen probe` rerun captured and characterized
8. first deeper code probes added to the branch
9. instrumented `power-reopen probe` rerun captured and characterized
10. low-fluid trapped-state guard tightened in the debug logic
11. tightened-guard `power-reopen probe` rerun captured and characterized
12. broader power-support fallback added for the next rerun

The next action is to rerun the active `power-reopen probe` case with the
broader power-support fallback and inspect:

- `lsm.diracSupportCellCount`
- `lsm.meaningfulDiracSupportCellCount`
- `lsm.powerSupportFloorValue`
- `lsm.powerSupportFloorVolumeFraction`
- `lsm.powerProjectionMultiplier`
- `lsm.fluidFractionPreShift`
- `lsm.fluidFractionPostShift`
- `lsm.fluidFractionRecoveredByShift`
- `convergence.stalledLowFluidCandidate`

If that rerun still reaches the same support-collapse signature, the next code
step should move from support broadening to a stronger reopening formulation
change.
