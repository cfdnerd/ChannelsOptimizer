# turbulenceMMAOpt Gray-Fraction Debug Experiment Plan

This plan targets the specific failure mode seen in the latest
`turbulenceMMAOpt` run:

- numerically stable iterations
- slowly improving power feasibility
- very weak gray-fraction collapse
- no laminar-style mid-run redesign burst

The goal is to separate three candidate causes:

1. continuation is simply too slow
2. continuation is being gated off too often
3. objective/power sensitivities are too weak to drive topology change

## Run Order

Run the experiments in this order:

1. `baseline`
2. `laminarGrayRamp`
3. `ungatedGrayRamp`
4. `adjointSensitivityProbe`

Only move to the next experiment if the previous one does not produce a clear
gray-collapse trend.

## How To Activate A Profile

Edit [tuneOptParameters](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/turbulenceMMAOpt/app/constant/tuneOptParameters) and set:

```foam
experimentControl
{
    profile                         laminarGrayRamp;
    forceContinuationHardening      false;
}
```

The active profile is echoed into:

- `debugOptimizer.log` runtime dump
- `debugOptimizer.log` interpolation diagnostics
- `debugOptimizer.jsonl` under `interpolation.experimentProfile`

The JSON log now also records:

- `interpolation.powerFeasibilityRatio`
- `interpolation.continuationGateSatisfied`
- `interpolation.forceContinuationHardening`
- `interpolation.hardeningEnabled`
- `sensitivity.objectiveToVolumeL2Ratio`
- `sensitivity.powerToVolumeL2Ratio`

## Experiment Matrix

### 1. `baseline`

Purpose:

- preserve the current turbulent settings
- confirm the current reference behavior before changing anything

Expected reference signals from the latest run:

- gray fraction still around `0.85` by iteration `~78`
- `beta` still below `10`
- `xhStepMax` stays around `O(1e-2)`
- power feasibility slowly improves but does not trigger a redesign burst

### 2. `laminarGrayRamp`

Purpose:

- test whether slow continuation is the main reason gray fraction does not drop

Profile effect:

- `betaIncrement = 0.2`
- `alphaRampEarlySlope = 1/7`
- `alphaRampLateFactor = 1.05`

Interpretation:

- if gray fraction starts dropping much earlier and `xhStepMax` grows strongly,
  the turbulent branch was mainly under-hardened
- if the run still stays gray, continuation speed alone is not the main blocker

Success signs:

- `beta` reaches the low-to-mid teens by iteration `60-80`
- `xhStepMax` rises well above the current `~0.02`
- gray fraction begins a sustained decline instead of drifting slowly

### 3. `ungatedGrayRamp`

Purpose:

- test whether the continuation feasibility gate is suppressing hardening

Profile effect:

- same ramp overrides as `laminarGrayRamp`
- `forceContinuationHardening = true`

Interpretation:

- if this profile collapses gray fraction while `laminarGrayRamp` does not,
  the gate is the dominant blocker
- if both profiles behave similarly, the gate is not the main issue

Key signals to inspect:

- `interpolation.continuationGateSatisfied`
- `interpolation.hardeningEnabled`
- gray fraction and `xhStepMax`

### 4. `adjointSensitivityProbe`

Purpose:

- test whether weak objective/power sensitivities are preventing the laminar
  redesign burst

Profile effect:

- `adjointMomentumSweeps >= 40`
- `useFullAdjointSymmetricStress = true`

Interpretation:

- if objective/power sensitivity ratios rise and topology updates become much
  larger, the current adjoint fidelity is too weak
- if sensitivity ratios remain tiny and gray fraction still stalls, the issue is
  likely deeper than continuation or sweep count alone

Key signals to inspect:

- `sensitivity.objectiveL2`
- `sensitivity.powerConstraintL2`
- `sensitivity.objectiveToVolumeL2Ratio`
- `sensitivity.powerToVolumeL2Ratio`
- `xhStepMax`

## Practical Pass/Fail Heuristics

An experiment is promising if, relative to `baseline`, it shows at least two of
the following:

- earlier gray-fraction drop
- larger `xhStepMax`
- larger objective-to-volume or power-to-volume sensitivity ratio
- `beta` reaching laminar-reference territory sooner
- no loss of solver health

An experiment is not promising if it only increases objective drift while gray
fraction remains nearly flat.

## Laminar Reference Reminder

The laminar reference does not sharpen immediately. Its gray fraction stays
high early, but by roughly iteration `60` it enters a large redesign regime and
then collapses quickly. That means the important comparison is not "is gray high
at iteration 20?" but "does the run ever enter a strong topology-rewrite
phase?"
