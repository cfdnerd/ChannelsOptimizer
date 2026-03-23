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
