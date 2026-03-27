# turbulenceLSMOpt Gray-Fraction And Reopening Experiment Plan

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

Check the active settings in:

- `optimizerlogs/debugOptimizer.log`
- `optimizerlogs/debugOptimizer.jsonl`
- `optimizerlogs/gradientOpt.log`
- `optimizerlogs/optimization.hst`

Most important JSON fields for this ladder:

- `design.xhGrayVolumeFraction`
- `design.interfaceBandVolumeFraction`
- `objective.powerDissipationConstraintValue`
- `lsm.volumePhiShiftRaw`
- `lsm.volumePhiShiftApplied`
- `sensitivity.interfacePowerL2`
- `sensitivity.normalVelocityL2`
- `interpolation.powerFeasibilityRatio`
- `interpolation.continuationGateSatisfied`
- `interpolation.hardeningEnabled`

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

Those probes should be added only after the runtime ladder has been exhausted.
