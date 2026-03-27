# turbulenceLSMOpt Debug Findings

This note records the current debugging position for `turbulenceLSMOpt` and
summarizes the failure mechanisms visible in the latest `optimizerlogs/`
snapshot.

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
- The turbulence physics backbone is probably not the primary blocker, because
  `turbulenceMMAOpt` already works with the same low-`q` turbulent baseline.
- The main suspects are LSM-specific:
  - hidden runtime-control overrides
  - global post-update `phiLS` shifts that stay permanently capped
  - loss of usable interface support for the power sensitivity after collapse
  - continuation logic that becomes irrelevant once the design has already
    solidified too far

## Latest-Run Failure Sequence

### 1. Iterations 1-4: stable but drifting toward closure

Observed in the latest logs:

- `xhGrayVolumeFraction` stays near `0.923`
- `PowerDiss` rises from `3.83` to `13.09`
- `volumePhiShiftRaw` grows from `0.176` to `0.873`
- `volumePhiShiftApplied` is capped every iteration
- solver health remains `OK`

Interpretation:

- the run is still numerically healthy
- but the interface is already being pushed hard by the global volume
  correction before power feasibility is recovered

### 2. Iteration 5: abrupt projection collapse

At `Iter 5`:

- `xhGrayVolumeFraction` drops from `0.923` to `9.624e-04`
- `xhSolidVolumeFraction` jumps to about `0.922`
- `PowerDiss` jumps to `27.49`
- `volumePhiShiftRaw` grows to `1.17` while `volumePhiShiftApplied` remains
  capped at `0.349`

Interpretation:

- the design effectively snaps into an over-solid configuration
- the run has not crashed, but it has already left the viable
  channel-evolution path

### 3. Iterations 6-17: power violation explodes, but reopening dies

After collapse:

- `PowerDiss` jumps to `128.7`, then `467`, then stays near `460`
- `interfacePowerSensitivity L2` falls from `15.66` at `Iter 4` to `1.77` at
  `Iter 6`, then to about `5e-04` by `Iter 16`
- `normalVelocity L2` falls from `3.09e+01` at `Iter 4` to `2.88` at `Iter 6`,
  then to about `4e-04` by `Iter 16`
- `volumePhiShiftRaw` climbs to about `5.36` while the applied shift stays
  fixed at the cap
- the objective changes only weakly once the channel is blocked

Interpretation:

- once the interface-support region collapses, the power adjoint no longer
  generates enough motion to reopen the flow path
- the run becomes dynamically trapped even though the power constraint is badly
  violated

## Completed Code-Level Finding

### Profile overrides were stronger than the case dictionaries

The latest run's runtime dump showed:

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

## Remaining Primary Findings

### 1. Low startup `q` is already in place

Unlike the original MMA failure, the current LSM run is already using:

- `qAlpha = qKappa = qHeat = 0.005`
- `useSingleQFallback = true`

Therefore the dominant LSM failure is not the same startup-interpolation
problem that dominated `turbulenceMMAOpt`.

### 2. The LSM branch is failing in its update path, not mainly in its physics solves

Evidence:

- `solverConvergences.log` stays healthy
- no `NaN` or divergence cascade appears
- the objective improves smoothly until the topology blocks the channel

This points the investigation toward:

- interface sensitivity mapping
- velocity regularization
- advection and reinitialization
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
