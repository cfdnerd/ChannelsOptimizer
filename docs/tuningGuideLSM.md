# turbulenceLSMOpt Tuning Guide

This guide summarizes the optimizer controls exposed through:

- `turbulenceLSMOpt/app/constant/optProperties`
- `turbulenceLSMOpt/app/constant/tuneOptParameters`

It focuses on the hybrid MMA-to-LSM handover and the Stage-2 level-set controls. The "valid values" below follow the current runtime checks in `turbulenceLSMOpt/src/createFields.H`. The "practical bands" are starting heuristics inferred from the present implementation in:

- `turbulenceLSMOpt/src/update.H`
- `turbulenceLSMOpt/src/updateLevelSet.H`
- `turbulenceLSMOpt/src/lsmSensitivity.H`
- `turbulenceLSMOpt/src/lsmGeometryMetrics.H`
- `turbulenceLSMOpt/src/levelSetWallDistance.H`

## Profiles

`experimentControl.profile` supports:

- `baseline`: use dictionary values as written.
- `laminarGrayRamp`: faster density-stage beta and alpha hardening.
- `relaxedGateGrayRamp`: density-stage hardening with a looser feasibility gate.
- `betaFloorGrayRamp`: relaxed gate plus forced hardening until an iteration or beta floor.
- `branchRefinement400`: strong early topology formation, then gentler late density refinement before handover.
- `ungatedGrayRamp`: always allow density-stage hardening.
- `adjointSensitivityProbe`: raise adjoint fidelity for sensitivity debugging.

These profiles mainly affect Stage 1. They matter for LSM because they determine when the density design becomes crisp, connected, and safe to hand over.

## `optProperties`

These controls are shared with the MMA branch, but they still affect the LSM workflow because Stage 2 only begins after Stage 1 satisfies the handover gate.

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `raa0` | positive scalar; start near `1e-5` and raise by `10x` if MMA updates become erratic | Damp noisy density-stage updates before the handover. |
| `voluse` | typically `0..1` | Target fluid-volume fraction used throughout the hybrid run. |
| `mma_init` | positive scalar, often `0.1..0.5` | Controls early MMA asymptote width. Higher values explore faster but can hand over a less-settled design. |
| `mma_dec` | usually `0..1` | Shrinks asymptotes after oscillation. Lower values are safer. |
| `mma_inc` | usually `>1` | Grows asymptotes when updates stay consistent. |
| `movlim` | positive scalar, often `0.1..0.5` | Caps density-step motion in `x`. Lower it if Stage 1 is too violent to meet the LSM switch gate. |
| `filterR` | positive scalar; often `4..12` cells depending on desired feature scale | Density-stage PDE filter radius. Higher values favor thicker trunks; lower values allow finer branches before handover. |
| `PowerDiss0` | positive scalar | Power normalization reference for logs and constraint scaling. |
| `PowerDissMax` | positive scalar | Final power-dissipation limit that must still be respected after LSM handover. |
| `PowerDissRelax` | positive scalar, usually `>= PowerDissMax` | Early relaxed power limit to let Stage 1 form topology before tightening. |
| `powerConstraintRelaxationRate` | nonnegative scalar | Tightening rate from `PowerDissRelax` toward `PowerDissMax`. Slower tightening can make the LSM switch easier to reach. |
| `continuationFeasibilityTol` | positive scalar; often `1.0..2.5` depending on profile | Density-stage hardening gate on `PowerDiss / activePowerLimit`. |
| `GeoDim` | usually `2` | Geometric dimension of the case. |
| `fluid_area` | `yes` / `no` | Keep prescribed fluid cells fixed during density updates. |
| `solid_area` | `yes` / `no` | Keep prescribed solid cells fixed during density updates. |
| `test_area` | `yes` / `no`; usually `no` | Legacy debug switch. |
| `Pnorm` | positive scalar; often `50..500` | Hotspot objective sharpness when hotspot-style objectives are active. |
| `objFunction` | legacy numeric selector | Legacy hook; primary thermal objective is controlled elsewhere. |
| `qAlpha0`, `qKappa0`, `qHeat0` | positive scalar, often `1e-3..1e-1` | Initial split-RAMP curvature controls. Higher values keep interpolation softer longer. |
| `qAlphaMin`, `qKappaMin`, `qHeatMin` | positive scalar | Lower bounds for split-RAMP continuation. |
| `qSingle0` | positive scalar | Initial shared-q value when single-q mode is active. |
| `qSingleMin` | positive scalar | Lower bound for shared-q continuation. |
| `qContinuationStartIter` | integer `>= 0` | Iteration at which q continuation begins. |
| `qContinuationInterval` | integer `> 0` to enable | Apply q continuation every `N` iterations. |
| `qContinuationFactor` | positive scalar, usually `<1` | Multiplicative q reduction per continuation step. |
| `beta0` | positive scalar; often `0.2..1.0` | Initial projection sharpness. |
| `betaMax` | positive scalar | Global upper cap on density-stage projection sharpness. |
| `betaIncrement` | positive scalar; often `0.05..0.25` | Per-hardening beta increase during Stage 1. |
| `alphaRampEarlySlope` | nonnegative scalar | Early additive alpha ramp. |
| `alphaRampLateFactor` | nonnegative scalar | Late multiplicative alpha ramp. |
| `dAlphaPorosityEps` | small positive scalar | Floor that avoids singular porosity-gradient evaluations. |

## `tuneOptParameters`

### `frameworkSwitches`

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `useFrozenTurbulenceAdjoint` | `true` / `false`; currently must stay `true` | Use the implemented frozen-turbulence adjoint path. |
| `useKEpsilonModel` | `true` / `false`; currently must stay `true` | Use the implemented `k-epsilon` turbulence branch. |
| `useWrayAgarwalFallback` | `true` / `false`; currently must stay `false` | Reserved fallback turbulence path; not implemented. |
| `usePorosityWallDistance` | `true` / `false` | Use porosity-aware wall distance in the density-stage and baseline LSM flow path. |
| `useMeshWaveWallDistanceFallback` | `true` / `false` | Keep a non-porosity wall-distance fallback available. At least one wall-distance path must stay enabled. |
| `useBrinkmanSinkInKEpsilon` | `true` / `false` | Include porous Brinkman damping in turbulence equations. |
| `useSplitRAMPControls` | `true` / `false` | Enable separate `qAlpha`, `qKappa`, and `qHeat` continuation. |
| `useSingleQFallback` | `true` / `false` | Use one shared q continuation parameter instead. |
| `useTurbulentThermalDiffusivity` | `true` / `false` | Include turbulent thermal diffusivity in heat transport. |
| `usePowerConstraintRelaxation` | `true` / `false` | Use the relaxed-to-final power schedule before and during handover gating. |
| `useGCMMA` | `true` / `false`; currently must stay `false` | Reserved optimizer path; not implemented. |
| `useFullAdjointSymmetricStress` | `true` / `false` | Enable the stronger adjoint stress term when probing sensitivity fidelity. |

Notes:

- Exactly one of `useSplitRAMPControls` or `useSingleQFallback` must be `true`.
- `useGCMMA=true`, `useWrayAgarwalFallback=true`, and `useFrozenTurbulenceAdjoint=false` are not supported in this branch.

### `objectiveSwitches`

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `useLogMeanTObjective` | `true` / `false` | Optimize `log(mean T)` instead of `mean T`. |
| `useKSHotspotObjective` | `true` / `false` | Bias the run toward hotspot suppression. |
| `useVarianceObjective` | `true` / `false` | Bias the run toward thermal uniformity. |
| `useRobustMultiCaseObjective` | `true` / `false`; currently must stay `false` | Reserved robust multi-case objective path. |

Notes:

- Only one primary thermal objective may be active at a time.
- `useRobustMultiCaseObjective=true` is not implemented.

### `continuationSwitches`

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `useStagnationTriggeredBeta` | `true` / `false` | Increase beta only after convergence/stagnation events instead of every allowed step. |
| `useLateStageFilterRCap` | `true` / `false` | Let late density-stage refinement cap the effective `filterR` without changing the dictionary base value. |

### `experimentControl`

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `profile` | one of the supported profile names | Apply a predefined density-stage continuation strategy. |
| `forceContinuationHardening` | `true` / `false` | Ignore the feasibility gate and always harden density continuation. |
| `forceContinuationHardeningUntilIter` | integer `>= 0` | Force hardening until this iteration. |
| `forceContinuationHardeningUntilBeta` | `-1` or scalar `>= 0` | Force hardening until beta reaches this value. |
| `stopContinuationHardeningBelowGrayFraction` | `-1` or scalar in `[0,1]` | Fully stop density hardening once gray fraction is already low enough. |
| `lateStageStrictGateBelowGrayFraction` | `-1` or scalar in `[0,1]` | Tighten the density continuation gate once gray falls below this level. |
| `lateStageContinuationFeasibilityTol` | `-1` or scalar `> 0` | Replacement feasibility tolerance used after the strict gate activates. |
| `lateStageRefinementStartIter` | integer `>= 0` | Start gentler late density refinement from this iteration onward. |
| `lateStageRefinementBelowGrayFraction` | `-1` or scalar in `[0,1]` | Also trigger late density refinement when gray falls below this level. |
| `lateStageBetaRampFactor` | `-1` or scalar `>= 0` | Multiply `betaIncrement` by this factor during late density refinement. |
| `lateStageAlphaRampFactor` | `-1` or scalar `>= 0` | Replace late `alphaRampLateFactor` during late density refinement. |
| `lateStageMaxBeta` | `-1` or scalar `>= 0` | Cap beta during late density refinement. |
| `lateStageFilterRStartIter` | integer `>= 0` | Keep the base `filterR` through this iteration, then allow a late cap to take over. |
| `lateStageFilterRCap` | `-1` or scalar `> 0` | Late-stage cap applied to `filterR` when the cap feature is enabled. |
| `laggingGrayCollapseStartIter` | integer `>= 0` | Start detecting stalled gray collapse from this iteration. |
| `laggingGrayCollapseAboveGrayFraction` | `-1` or scalar in `[0,1]` | Only call it stalled while gray remains above this level. |
| `laggingGrayCollapseBelowXhStepMax` | `-1` or scalar `>= 0` | Stagnation trigger when density topology motion is below this threshold. |
| `laggingGrayCollapseMinGrayDrop` | `-1` or scalar `>= 0` | Stagnation trigger when gray reduction per step is too small. |
| `laggingGrayCollapsePauseInterval` | integer `> 0` | Allow one hardening pulse every `N` steps while throttled. |
| `overactiveTopologyStartIter` | integer `>= 0` | Start detecting overly violent late density topology motion from this iteration. |
| `overactiveTopologyBelowGrayFraction` | `-1` or scalar in `[0,1]` | Only apply the overactive throttle once gray is already low. |
| `overactiveTopologyAboveXhStepMax` | `-1` or scalar `>= 0` | Pause hardening when recent density motion exceeds this threshold. |

These controls do not move `phi` directly, but they strongly influence whether Stage 2 receives a clean topology or a still-gray, still-exploring design.

### `adjointControl`

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `adjointMomentumSweeps` | integer `> 0`; often `20..40` | More sweeps improve adjoint fidelity but cost more runtime. |
| `adjointExplicitStressScale` | scalar in `[0,1]`; often `0.2..0.4` when reduced from full scale | Scale the explicit full symmetric-stress contribution. Lower it if adjoints spike. |
| `resetAdjointsEachIteration` | `true` / `false` | Reinitialize adjoints each loop for robustness. |
| `stopOnAdjointRunaway` | `true` / `false` | Abort if adjoint magnitudes become unstable. |

## LSM-Focused Controls

### `hybridStageControl`

These are the main handover controls from density Stage 1 into LSM Stage 2.

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `useHybridMMAtoLSM` | `true` / `false` | Enable the staged density-to-level-set workflow at all. Set `false` for density-only debugging. |
| `lsmSwitchGrayFraction` | valid range `[0,1]`; practical start `0.15..0.25` | Require the density design to be sufficiently crisp before handover. Lower values delay the switch; higher values switch earlier. |
| `lsmSwitchPowerFeasibility` | valid range `> 0`; practical start `1.02..1.10` | Require near-feasible power dissipation before Stage 2 begins. Lower values are safer. |
| `lsmSwitchMinBeta` | valid range `>= 0`; practical start `10..15` | Require enough projection sharpness before creating `phi`. Higher values give a crisper density contour at handover. |
| `lsmSwitchMinIterations` | valid range `>= 0`; practical start `80..150` | Prevent an early switch even if the other metrics look acceptable. |
| `lsmSwitchRequireConnectivity` | `true` / `false`; usually keep `true` | Require a connected thresholded inlet-to-outlet fluid path before handover. |
| `lsmBlendIterations` | valid range `>= 0`; practical start `3..10` | Blend density and level-set reconstructions over the first Stage-2 iterations to soften the physics jump. |
| `lsmRollbackEnabled` | `true` / `false`; strongly recommended `true` | Allow rollback to the density checkpoint if Stage 2 becomes unstable or repeatedly rejects steps. |
| `requireRestartableLSMCheckpoint` | `true` / `false`; usually keep `true` when hybrid mode is enabled | Refuse handover unless rollback support is available. |
| `allowTopologyChangeInLSM` | `true` / `false`; baseline is `false` | When `false`, Stage 2 behaves like geometry refinement. When `true`, Stage 2 may accept larger topological changes, but this is more experimental. |

Notes:

- The switch gate also requires solver health to be acceptable.
- With `allowTopologyChangeInLSM=false`, Stage 2 rejects steps that break connectivity or collapse width or rib metrics below `50%` of the corresponding minimum target.
- Two rejected LSM steps in a row trigger rollback when `lsmRollbackEnabled=true`.

### `lsmControl`

These controls govern the signed-distance update itself.

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `useLevelSetStage` | `true` / `false` | Enable Stage-2 level-set refinement machinery. Keep `true` for the hybrid workflow. |
| `phiBandWidthCells` | valid range `> 0`; practical start `1.0..2.0` cells | Smooth-Heaviside half-width used to reconstruct `xh` from `phi`. Lower values sharpen the interface; higher values soften it. |
| `narrowBandHalfWidthCells` | valid range `> 0`; practical start `3..6` cells | Restrict Hamilton-Jacobi updates to a narrow band around the interface. Wider bands are more forgiving but touch more cells each update. |
| `phiReinitInterval` | valid range `>= 0`; use `0` to disable interval-based reinit, practical start `3..10` | Reinitialize the signed-distance field every `N` Stage-2 iterations. Lower values keep `|grad(phi)|` closer to one. |
| `phiGradDriftTol` | valid range `>= 0`; use `0` to disable drift-triggered reinit, practical start `0.1..0.3` | Reinitialize when signed-distance drift becomes too large. Lower values reinitialize earlier. |
| `lsmPseudoDt` | valid range `>= 0`; `0` means CFL-only stepping | Fixed pseudo-time cap for `phi` advection. When positive, it is still clamped by the CFL limit. Use `0` for adaptive stepping first. |
| `lsmCfl` | valid range `> 0`; practical start `0.25..0.75` | CFL-like cap on the LSM pseudo-step. Lower values are safer; higher values move the interface faster. |
| `useVelocityHelmholtz` | `true` / `false`; usually keep `true` | Smooth the total interface normal velocity before updating `phi`. |
| `velocityFilterRadius` | valid range `> 0`; practical start `1.5..3.0` cells | Helmholtz smoothing radius for interface velocity regularization. Larger values suppress jagged motion. |
| `useCurvaturePenalty` | `true` / `false` | Add curvature-based smoothing to interface motion. Useful when Stage 2 produces serrated walls. |
| `curvaturePenaltyWeight` | valid range `>= 0`; practical start `0.01..0.10` | Strength of curvature smoothing. Higher values favor cleaner walls but can slow local carving. |
| `useReactionDiffusionComplexityControl` | `true` / `false`; baseline is `false` | Apply an extra reaction-diffusion smoothing pass after advection. Treat this as an experimental extra regularizer. |

Notes:

- `phiBandWidthCells` is also used when initializing `phi` from the density design.
- `velocityFilterRadius` is reused by the optional reaction-diffusion path.

### `lsmSensitivityControl`

These controls define how density-based sensitivities become Stage-2 interface velocities.

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `useApproximateNarrowBandSensitivityBridge` | `true` / `false`; normally keep `true` | Reuse `xh` sensitivities in a narrow band around the interface. If set `false`, the objective, power, and volume interface velocities are no longer populated by the current bridge path. |
| `lsmSensitivitySmoothingRadius` | valid range `> 0`; practical start `1.0..3.0` cells | Smoothing radius for geometry-control extension fields such as width and rib velocities. |
| `normalizeInterfaceVelocity` | `true` / `false`; usually keep `true` | Normalize objective, power, volume, geometry, and total interface-velocity components before combining them. |
| `finiteDifferenceCheckStage2` | `true` / `false`; normally `false` | Keep extra Stage-2 interface velocity fields for manual finite-difference validation and debugging. |

Notes:

- `normalizeInterfaceVelocity=true` makes the different velocity components comparable in scale.
- `finiteDifferenceCheckStage2=true` is for diagnostics, not production runs.

### `geometryControl`

These controls are the main reason to use the LSM stage in this branch: channel width, rib thickness, curvature, and regionwise geometry cleanup.

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `useChannelWidthControl` | `true` / `false` | Enable channel-width-based correction velocities. |
| `channelWidthMin` | valid range `>= 0`; practical start `2..6` cell lengths | Minimum channel width target. Raise it to stop over-thinning. |
| `channelWidthMax` | valid range `<= 0` to effectively disable max-width trimming, otherwise `>= channelWidthMin`; practical start `8..20` cell lengths when enabled | Maximum channel width target. Lower it to split or slim overly fat trunks. |
| `useRegionwiseWidthTargets` | `true` / `false` | Promote width targets from global values to per-cell-zone targets. |
| `useBranchwiseWidthTargets` | `true` / `false`; currently must stay `false` | Reserved branchwise width targeting path; not implemented. |
| `useRibThicknessControl` | `true` / `false` | Enable rib-thickness correction velocities. |
| `ribThicknessMin` | valid range `>= 0`; practical start `2..5` cell lengths | Minimum rib thickness target. Raise it to protect structural walls. |
| `useRibThicknessMaxControl` | `true` / `false` | Enable maximum rib-thickness trimming. |
| `ribThicknessMax` | when max control is enabled, must be `>= ribThicknessMin`; practical start `8..15` cell lengths | Maximum rib thickness target. Lower it to trim bulky internal ribs. |
| `useRegionwiseRibTargets` | `true` / `false` | Promote rib targets from global values to per-cell-zone targets. |
| `useBranchwiseRibTargets` | `true` / `false`; currently must stay `false` | Reserved branchwise rib targeting path; not implemented. |
| `useProtectedGeometryMasks` | `true` / `false` | Activate protected region masks so selected zones are excluded from width or rib corrections. |
| `applyRibTargetsToInternalRibsOnly` | `true` / `false`; usually keep `true` | Prevent rib-max trimming from eroding boundary-attached walls. |
| `useHydraulicDiameterReporting` | `true` / `false` | Report hydraulic-diameter metrics derived from the local width metric and the supplied extrusion thickness. |
| `hydraulicDiameterExtrusionThickness` | valid range `> 0`; use the intended extruded plate thickness | Extrusion thickness used for hydraulic-diameter reporting. |
| `ksRhoGeom` | valid range `> 0`; practical start `10..30` | KS sharpness used to aggregate geometry violations. Higher values focus more on worst-case violations. |
| `sizeConstraintWeightMin` | valid range `>= 0`; practical start `0.5..2.0` | Weight multiplier for minimum-size corrections. |
| `sizeConstraintWeightMax` | valid range `>= 0`; practical start `0.5..2.0` | Weight multiplier for maximum-size corrections. |
| `widthConstraintWeight` | valid range `>= 0`; practical start `0.5..2.0` | Overall weight for channel-width correction velocities. |
| `ribConstraintWeight` | valid range `>= 0`; practical start `0.5..2.0` | Overall weight for rib-thickness correction velocities. |
| `protectedSolidRegionNames` | `wordList` of cell-zone names | Mark solid zones that should be excluded from rib corrections when protected masks are enabled. |
| `protectedFluidRegionNames` | `wordList` of cell-zone names | Mark fluid zones that should be excluded from width corrections when protected masks are enabled. |
| `regionwiseChannelWidthMin` | dictionary of `cellZoneName value;` entries | Override the global minimum channel width on specific cell zones. |
| `regionwiseChannelWidthMax` | dictionary of `cellZoneName value;` entries | Override the global maximum channel width on specific cell zones. |
| `regionwiseRibThicknessMin` | dictionary of `cellZoneName value;` entries | Override the global minimum rib thickness on specific cell zones. |
| `regionwiseRibThicknessMax` | dictionary of `cellZoneName value;` entries | Override the global maximum rib thickness on specific cell zones. |

Notes:

- Regionwise and protected-geometry dictionaries are resolved against `mesh.cellZones()`.
- Width and rib rollback protection is based on observed Stage-2 metrics, not only on requested targets.
- `channelWidthMetric`, `ribThicknessMetric`, `hydraulicDiameterMetric`, `lsmWidthVelocity`, and `lsmRibVelocity` are useful Stage-2 fields to inspect.

### `wallDistanceControl`

These controls tune how Stage 2 feeds wall distance back into the porous physics path.

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `lsmWallDistanceMode` | one of `porosity`, `blend`, or `phi` | Choose how wall distance is formed during Stage 2. `porosity` keeps the baseline path, `blend` transitions toward `phi`, and `phi` uses the `phi`-based distance directly. |
| `useLSMWallDistance` | `true` / `false`; baseline is `false` | Turn on the Stage-2 LSM wall-distance path. If `false`, the solver keeps the porosity-style wall-distance baseline. |
| `useHeatMethodWallDistanceFallback` | `true` / `false`; baseline is `false` | Reserved for future wall-distance experiments. The current baseline only keeps porosity or `phi`-based distance. |
| `lsmWallDistanceBlendIterations` | valid range `>= 0`; practical start `3..10` | Number of early Stage-2 iterations over which `blend` transitions from porosity distance toward `phi`-based distance. |

Notes:

- The LSM wall-distance path is only active outside the density stage, with `useLevelSetStage=true`, `useLSMWallDistance=true`, and `lsmWallDistanceMode != porosity`.
- `phi` switches immediately to the `phi`-based distance.
- `blend` starts with the porosity distance and gradually shifts toward the `phi`-based distance over `lsmWallDistanceBlendIterations`.

### `convergenceControl`

| Key | Usage / values | Intended goal |
| --- | --- | --- |
| `optPowerConstraintWeight` | positive scalar; practical start `1..5` | Relative penalty weight on power-constraint violation when Stage-2 interface velocities are combined. |
| `optPowerViolationScaleExponent` | scalar `>= 0`; practical start `1.0` | Nonlinear scaling of power-violation severity in the Stage-2 velocity weighting. |

## Stage-2 Outputs Worth Watching

When Stage 2 is active, the most informative written fields are:

- `phiLevelSet`: signed-distance geometry field.
- `xhLevelSet`: smooth-Heaviside reconstruction from `phi`.
- `phiInterfaceDelta`: interface-band weight used during sensitivity mapping.
- `channelWidthMetric` and `ribThicknessMetric`: local geometry metrics.
- `hydraulicDiameterMetric`: hydraulic-diameter report when enabled.
- `lsmObjectiveVelocity`, `lsmPowerVelocity`, `lsmVolumeVelocity`, `lsmGeometryVelocity`: decomposed interface velocities.
- `lsmNormalVelocity` and `lsmNormalVelocityRaw`: total interface motion before and after regularization.
- `lsmCurvature`: curvature field used by the curvature penalty.

## Quick Tuning Heuristics

- If Stage 2 starts too early and destabilizes: lower `lsmSwitchGrayFraction`, lower `lsmSwitchPowerFeasibility`, raise `lsmSwitchMinBeta`, or raise `lsmSwitchMinIterations`.
- If the handover itself is too abrupt: increase `lsmBlendIterations`, keep `lsmRollbackEnabled=true`, and start with `lsmWallDistanceMode blend` instead of `phi`.
- If Stage 2 walls look jagged or noisy: keep `useVelocityHelmholtz=true`, increase `velocityFilterRadius`, enable `useCurvaturePenalty`, or raise `curvaturePenaltyWeight` gradually.
- If Stage 2 is too sluggish to clean geometry: lower `velocityFilterRadius`, lower `curvaturePenaltyWeight`, or raise `lsmCfl` carefully.
- If channels or ribs collapse too much: raise `channelWidthMin` or `ribThicknessMin`, increase `widthConstraintWeight` or `ribConstraintWeight`, and keep `allowTopologyChangeInLSM=false`.
- If thick trunks are not slimming enough: enable `channelWidthMax` trimming, enable `useRibThicknessMaxControl` only where appropriate, and consider regionwise targets instead of stronger global penalties.
- If protected manifolds or boundary walls are being disturbed: use `useProtectedGeometryMasks=true`, populate `protected*RegionNames`, and keep `applyRibTargetsToInternalRibsOnly=true`.
- If Stage-2 sensitivities look unbalanced: keep `normalizeInterfaceVelocity=true` and only disable it when you intentionally want raw magnitude differences to dominate.
- If you only want geometry cleanup and not objective-driven interface motion: turning off `useApproximateNarrowBandSensitivityBridge` effectively leaves Stage 2 driven by geometry-control velocities only in the current implementation.
