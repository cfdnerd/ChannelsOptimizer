# turbulenceMMAOpt Tuning Guide

This guide summarizes the optimizer controls exposed through:

- `turbulenceMMAOpt/app/constant/optProperties`
- `turbulenceMMAOpt/app/constant/tuneOptParameters`

The goal is quick tuning, not solver internals.

## Profiles

`experimentControl.profile` supports:

- `baseline`: use dictionary values as written.
- `laminarGrayRamp`: faster laminar-style beta/alpha hardening.
- `relaxedGateGrayRamp`: laminar-style hardening with looser power-feasibility gating.
- `betaFloorGrayRamp`: relaxed gate plus forced hardening until a beta/iteration floor.
- `branchRefinement400`: keep strong early topology birth, then switch into earlier late-stage carving to trim oversized ribs without over-hardening the design.
- `ungatedGrayRamp`: bypass the continuation gate and always harden.
- `adjointSensitivityProbe`: increase adjoint fidelity to test weak-sensitivity hypotheses.

## `optProperties`

| Key | Scope / values | Intended goal |
| --- | --- | --- |
| `raa0` | positive scalar | MMA regularization; raise to damp noisy or erratic updates. |
| `voluse` | typically `0..1` | Target fluid-volume fraction. |
| `mma_init` | positive scalar | Initial MMA asymptote width; higher is more aggressive early. |
| `mma_dec` | `0..1` usually | Shrinks MMA asymptotes after oscillation. |
| `mma_inc` | `>1` usually | Expands MMA asymptotes when updates stay consistent. |
| `movlim` | positive scalar, often `0..1` | Caps per-iteration design motion in `x`. Lower for stability, higher for faster redesign. |
| `filterR` | positive scalar | PDE filter radius. Larger suppresses small features; smaller allows finer channels. |
| `PowerDiss0` | positive scalar | Power normalization reference used in logs and constraint scaling. |
| `PowerDissMax` | positive scalar | Final power-dissipation limit after relaxation. |
| `PowerDissRelax` | positive scalar, usually `>= PowerDissMax` | Relaxed initial power limit. |
| `powerConstraintRelaxationRate` | nonnegative scalar | Per-iteration tightening rate from relaxed to final power limit. |
| `continuationFeasibilityTol` | positive scalar | Power-feasibility tolerance used to allow/deny hardening. |
| `GeoDim` | usually `2` | Geometric dimension of the case. |
| `fluid_area` | `yes` / `no` | Fix prescribed fluid cells to `x=1`. |
| `solid_area` | `yes` / `no` | Fix prescribed solid cells to `x=0`. |
| `test_area` | `yes` / `no` | Legacy debug switch; normally keep `no`. |
| `Pnorm` | positive scalar | Sharpness for KS / p-norm style hotspot aggregation. |
| `objFunction` | legacy numeric selector | Legacy objective hook; primary objective mode is now controlled elsewhere. |
| `qAlpha0`, `qKappa0`, `qHeat0` | positive scalar | Initial split-RAMP interpolation curvature controls. |
| `qAlphaMin`, `qKappaMin`, `qHeatMin` | positive scalar | Lower bounds for split-RAMP continuation. |
| `qSingle0` | positive scalar | Initial shared-q value when single-q mode is used. |
| `qSingleMin` | positive scalar | Lower bound for shared-q continuation. |
| `qContinuationStartIter` | integer `>= 0` | Iteration to begin q continuation. |
| `qContinuationInterval` | integer `> 0` to enable | Apply q continuation every N iterations. |
| `qContinuationFactor` | positive scalar, usually `<1` | Multiplicative q reduction per continuation step. |
| `beta0` | positive scalar | Initial projection sharpness. |
| `betaMax` | positive scalar | Global maximum projection sharpness. |
| `betaIncrement` | positive scalar | Default beta increase per allowed hardening step. |
| `alphaRampEarlySlope` | nonnegative scalar | Early additive alpha ramp slope. |
| `alphaRampLateFactor` | nonnegative scalar | Late multiplicative alpha ramp after Iter 100. |
| `dAlphaPorosityEps` | small positive scalar | Floor to avoid singular porosity-gradient evaluations. |

## `tuneOptParameters`

### `frameworkSwitches`

| Key | Scope / values | Intended goal |
| --- | --- | --- |
| `useFrozenTurbulenceAdjoint` | `true` / `false` | Must currently stay `true`; non-frozen mode is not implemented. |
| `useKEpsilonModel` | `true` / `false` | Must currently stay `true`; this is the implemented turbulence branch. |
| `useWrayAgarwalFallback` | `true` / `false` | Not implemented; keep `false`. |
| `usePorosityWallDistance` | `true` / `false` | Use porosity-aware wall distance inside evolving solid/fluid topology. |
| `useMeshWaveWallDistanceFallback` | `true` / `false` | Fallback wall-distance mode if needed. |
| `useBrinkmanSinkInKEpsilon` | `true` / `false` | Adds porous damping to turbulence equations. |
| `useSplitRAMPControls` | `true` / `false` | Enable separate `qAlpha/qKappa/qHeat` continuation. |
| `useSingleQFallback` | `true` / `false` | Enable shared-q continuation instead of split-q. |
| `useTurbulentThermalDiffusivity` | `true` / `false` | Include turbulent thermal diffusion in heat transport. |
| `usePowerConstraintRelaxation` | `true` / `false` | Use the relaxed-to-final power limit schedule. |
| `useGCMMA` | `true` / `false` | Not implemented; keep `false`. |
| `useFullAdjointSymmetricStress` | `true` / `false` | Stronger adjoint stress model for sensitivity-fidelity testing. |

Notes:

- Exactly one of `useSplitRAMPControls` or `useSingleQFallback` must be `true`.
- `useGCMMA=true`, `useWrayAgarwalFallback=true`, and `useFrozenTurbulenceAdjoint=false` are not supported in this solver.

### `objectiveSwitches`

| Key | Scope / values | Intended goal |
| --- | --- | --- |
| `useLogMeanTObjective` | `true` / `false` | Optimize `log(mean T)` instead of `mean T`. |
| `useKSHotspotObjective` | `true` / `false` | Bias the optimizer toward hotspot suppression. |
| `useVarianceObjective` | `true` / `false` | Bias toward thermal uniformity. |
| `useRobustMultiCaseObjective` | `true` / `false` | Reserved for robust multi-case optimization; not implemented. |

Notes:

- At most one of `useLogMeanTObjective`, `useKSHotspotObjective`, or `useVarianceObjective` may be `true`.
- `useRobustMultiCaseObjective=true` is not implemented.

### `continuationSwitches`

| Key | Scope / values | Intended goal |
| --- | --- | --- |
| `useStagnationTriggeredBeta` | `true` / `false` | Harden beta only after convergence/stagnation events instead of every allowed iteration. |
| `useLateStageFilterRCap` | `true` / `false` | Enable or disable the late-stage `filterR` cap feature without changing its numeric settings. |

### `experimentControl`

| Key | Scope / values | Intended goal |
| --- | --- | --- |
| `profile` | one of the supported profile names | Apply a predefined continuation strategy. |
| `forceContinuationHardening` | `true` / `false` | Ignore the feasibility gate and always harden. |
| `forceContinuationHardeningUntilIter` | integer `>= 0` | Force hardening until this iteration. |
| `forceContinuationHardeningUntilBeta` | `-1` or scalar `>= 0` | Force hardening until beta reaches this value. |
| `stopContinuationHardeningBelowGrayFraction` | `-1` or scalar in `[0,1]` | Full hardening stop once gray fraction is already low. |
| `lateStageStrictGateBelowGrayFraction` | `-1` or scalar in `[0,1]` | Switch to a stricter feasibility gate below this gray level. |
| `lateStageContinuationFeasibilityTol` | `-1` or scalar `> 0` | Late-stage replacement tolerance for `PowerDiss / activePowerLimit`. |
| `lateStageRefinementStartIter` | integer `>= 0` | Begin late refinement taper at this iteration. |
| `lateStageRefinementBelowGrayFraction` | `-1` or scalar in `[0,1]` | Also trigger late refinement when gray falls below this level. |
| `lateStageBetaRampFactor` | `-1` or scalar `>= 0` | Multiply `betaIncrement` by this factor during refinement. |
| `lateStageAlphaRampFactor` | `-1` or scalar `>= 0` | Replace late `alphaRampLateFactor` during refinement. |
| `lateStageMaxBeta` | `-1` or scalar `>= 0` | Cap beta during refinement. |
| `lateStageFilterRStartIter` | integer `>= 0` | Keep the dictionary `filterR` through this iteration, then allow the late filter cap to take over if `useLateStageFilterRCap=true`. |
| `lateStageFilterRCap` | `-1` or scalar `> 0` | Late-stage cap applied to `filterR`; the effective value becomes `min(filterR, lateStageFilterRCap)` when the feature is enabled. |
| `laggingGrayCollapseStartIter` | integer `>= 0` | Start checking for stalled gray collapse from this iteration. |
| `laggingGrayCollapseAboveGrayFraction` | `-1` or scalar in `[0,1]` | Only call it stalled while gray remains above this value. |
| `laggingGrayCollapseBelowXhStepMax` | `-1` or scalar `>= 0` | Stagnation trigger when topology motion is below this threshold. |
| `laggingGrayCollapseMinGrayDrop` | `-1` or scalar `>= 0` | Stagnation trigger when gray reduction per step is too small. |
| `laggingGrayCollapsePauseInterval` | integer `> 0` | Allow one hardening pulse every N steps while throttled. |
| `overactiveTopologyStartIter` | integer `>= 0` | Start checking for overly violent late topology changes. |
| `overactiveTopologyBelowGrayFraction` | `-1` or scalar in `[0,1]` | Only apply overactive throttling below this gray level. |
| `overactiveTopologyAboveXhStepMax` | `-1` or scalar `>= 0` | Pause hardening when recent `xhStepMax` exceeds this limit. |

Usage patterns:

- Raise hardening aggressiveness with `forceContinuationHardening*`, higher `lateStageContinuationFeasibilityTol`, or more aggressive profiles.
- Protect fine sub-branching with `lateStageRefinement*`, lower `lateStageMaxBeta`, and earlier `overactiveTopology*`.
- Keep large smooth trunks early and finer carving late with `useLateStageFilterRCap=true` plus an earlier `lateStageFilterRStartIter` and a smaller `lateStageFilterRCap`.
- Stop late speckle/noise by lowering `stopContinuationHardeningBelowGrayFraction` only if gray collapse is already achieved.

### `adjointControl`

| Key | Scope / values | Intended goal |
| --- | --- | --- |
| `adjointMomentumSweeps` | integer `> 0` | More sweeps improve adjoint fidelity but cost more time. |
| `adjointExplicitStressScale` | scalar in `[0,1]` | Scale the explicit full symmetric-stress correction; lower it if the adjoints spike or run away. |
| `resetAdjointsEachIteration` | `true` / `false` | Reset adjoints each loop for robustness. |
| `stopOnAdjointRunaway` | `true` / `false` | Abort if adjoint magnitudes become unstable. |

### `convergenceControl`

| Key | Scope / values | Intended goal |
| --- | --- | --- |
| `optMinIterations` | integer `>= 0` | Earliest iteration at which early stopping is allowed. |
| `optStallWindow` | integer `> 0` | Consecutive near-stalled iterations required before stopping. |
| `optObjectiveTol` | nonnegative scalar | Objective-drift tolerance for early stop. |
| `optConstraintTol` | nonnegative scalar | Constraint-violation tolerance for early stop. |
| `optStepTol` | nonnegative scalar | Step-size tolerance for early stop. |
| `optPowerConstraintWeight` | positive scalar | Relative optimizer weight on power-constraint violation. |
| `optPowerViolationScaleExponent` | scalar `>= 0` | Nonlinear scaling of power-violation severity. |
| `optVolumeConstraintWeight` | positive scalar | Relative optimizer weight on volume-constraint violation. |

## Quick Tuning Heuristics

- If channels are too thick and small branches never appear: lower `filterR`, reduce late `beta` growth, or start `lateStageRefinement*` earlier.
- If solid blocks or ribs stay too large after the main topology has formed: enable `useLateStageFilterRCap`, start the cap earlier, and pair it with a lower `lateStageMaxBeta`.
- If the design stays gray too long: increase `betaIncrement`, use a more aggressive profile, or relax the continuation gate.
- If late iterations become speckled or noisy: lower `lateStageMaxBeta`, reduce `lateStageBetaRampFactor`, or tighten `lateStageStrictGate*`.
- If topology motion dies too early while gray is still high: loosen `laggingGrayCollapse*` or allow stronger early hardening.
- If power feasibility dominates too early: increase `PowerDissRelax`, reduce `powerConstraintRelaxationRate`, or use `usePowerConstraintRelaxation=true`.
- If late sensitivities look too weak: increase `adjointMomentumSweeps` gradually and only raise `adjointExplicitStressScale` if the adjoints stay bounded.
- If `Ua/U` or `Ub/U` spikes appear: cut `adjointExplicitStressScale`, then reduce `adjointMomentumSweeps` if needed.
