# PlanTurbulenceOpt-MMA: Independent Quasi-2D Turbulent Density Optimizer

**Target folder:** `turbulenceMMAOpt/`  
**Method:** Density-based topology optimization with RAMP/Helmholtz/Heaviside + MMA  
**Target product:** Quasi-2D flow topology for **extruded cold plates**, not a full 3D topology optimization case

---

## 1. Development Position

This document supersedes the earlier "3D organic cold plate" framing.

The intended development is:

- A **standalone optimizer folder** `turbulenceMMAOpt/` with its own `src/` and `app/`
- Initially derived from `laminarOptimizer/`, but evolved independently
- Operated on the same **quasi-2D case setup** style as the reference base solver
- Used to generate a **planar cooling-channel topology** that is later **extruded to a prescribed plate thickness**

This is the correct scope for the current project. Recent heat-sink and cold-plate literature supports pseudo-3D and two-layer reduced-order workflows as practical design routes for extruded electronics-cooling hardware.

---

## 2. Preferred SOTA Formulation for This Branch

For the MMA branch, the preferred formulation is:

1. Density design variable `x`
2. Helmholtz PDE filter `x -> xp`
3. Heaviside projection `xp -> xh`
4. Brinkman-penalized turbulent RANS primal
5. Frozen-Turbulence continuous adjoint
6. MMA design update
7. 2D contour extraction from `xh = 0.5`
8. Downstream extrusion to cold-plate thickness

This is the best match to the current OpenFOAM-6 base framework and the most robust near-term route for turbulent electronics-cooling TO.

### 2.1 Selected Core Choices

| Element | Selected baseline | Why |
|---------|-------------------|-----|
| Design update | MMA | Reuses the current laminar optimizer architecture with minimum structural risk |
| Turbulence model | Standard `kEpsilon` | Strong precedent in turbulent cooling-channel TO and stable for internal recirculating flows |
| Adjoint turbulence treatment | Frozen Turbulence (FT) | Best stability/complexity tradeoff for gray-density intermediate states |
| Wall treatment | Porosity-aware auxiliary wall-distance PDE | Best density-method option for evolving internal solid walls |
| Geometry target | Quasi-2D contour, then extrusion | Matches intended manufacturing workflow and avoids premature full-3D complexity |
| Objective progression | MeanT -> log-MeanT -> KS hotspot or variance control | Best staged path from solver bring-up to electronics-cooling relevance |

---

## 3. Roadblock Resolution Strategy

This section resolves the three critical ambiguities using an ordered preference list. The first option in each subsection is the **selected development baseline**. Lower options remain available through runtime toggles if instability appears.

### 3.1 RAMP Continuation and Penalization

#### Preferred option: split continuation parameters

Do **not** use one shared `qu` for all interpolations.

Use:

- `qAlpha` for Brinkman flow resistance
- `qKappa` for thermal conductivity interpolation
- `qHeat` for design-dependent heat-source interpolation, if used

Selected continuation direction:

- Start with moderate smoothing, e.g. `qAlpha0 = 0.1`, `qKappa0 = 0.1`
- Decrease each parameter toward a sharp final value, e.g. `1e-3`
- Allow separate schedules for flow and thermal interpolation

This is preferred because hydraulic resistance and thermal transport do not need to harden at the same rate. It also resolves the current mismatch with `laminarOptimizer`, where `qu` increases in [update.H](/home/tomathew/work/jobs/chaos/wDir/ChannelsCloud/laminarOptimizer/src/update.H#L28), which is opposite to the desired sharpening behavior.

#### Alternative A: single RAMP parameter, reversed direction

If split continuation causes debugging overhead, keep one `qu` but reverse the schedule:

$$q^{(k+1)} \le q^{(k)}$$

This is acceptable as a short-term simplification, but not preferred for production.

#### Alternative B: SIMP-style penalization

This remains a fallback if RAMP continuation proves hard to stabilize in the chosen implementation. It is not the preferred path because it moves the new branch farther away from the existing laminar framework.

### 3.2 Wall-Distance and Turbulent Wall Treatment

#### Preferred option: porosity-aware wall-distance PDE

For the density branch, the selected baseline is to solve an auxiliary wall-distance field every optimization iteration:

$$-\nabla \cdot ((xh + \epsilon_f)\nabla d_\alpha) = 1, \qquad d_\alpha|_{\partial \Omega} = 0$$

and use `dAlpha` in the wall-function treatment for `kEpsilon`.

This is the best SOTA-aligned option for density-based turbulent TO because standard `wallDist` does not see the evolving internal solid boundaries created by `xh -> 0`.

#### Alternative A: standard `meshWave` wall distance for bring-up only

Allowed only as an early debugging mode. It is useful to isolate whether instability comes from the wall-distance PDE itself, but it is not a valid production formulation.

#### Alternative B: switch turbulence model to Wray-Agarwal

If the `kEpsilon` + `dAlpha` coupling is persistently unstable, the controlled fallback is a wall-distance-free one-equation closure such as Wray-Agarwal. This should remain a fallback, not the first implementation, because the `kEpsilon` route is more aligned with existing turbulent cooling-channel TO literature.

### 3.3 Scope Resolution: quasi-2D extruded cold plate, not full 3D TO

The selected scope is:

- optimize on a planar 2D domain
- treat hydraulic and thermal quantities either per-unit-depth or with a prescribed extrusion thickness
- extract a final 2D contour
- extrude that contour into the target cold-plate thickness

This means:

- no 3D remeshing strategy is needed
- no 3D topological nucleation logic is needed
- no 3D wall-distance construction is needed
- all instability-reduction effort should focus on the 2D optimizer

This is consistent with pseudo-3D and extruded-cold-plate workflows in recent heat-sink literature.

---

## 4. Governing Formulation

### 4.1 Design-Variable Pipeline

Use the established three-field density pipeline:

$$x \rightarrow \tilde{x}=xp \rightarrow \bar{x}=xh$$

with:

- `x` = MMA-updated raw design
- `xp` = Helmholtz-filtered field
- `xh` = projected physical field used in all physics

### 4.2 Helmholtz Filter

$$-r^2 \nabla^2 \tilde{x} + \tilde{x} = x$$

Retain the PDE filter architecture already present in the laminar base solver.

### 4.3 Heaviside Projection

Retain a volume-preserving projection with continuation in `beta`.

Preferred continuation:

- start with low `beta`
- increase gradually
- optionally allow stagnation-triggered increase as a runtime-selectable alternative

### 4.4 Turbulent Primal Momentum

$$({\bf U}\cdot\nabla){\bf U} = -\nabla p + \nabla\cdot[(\nu+\nu_t)(\nabla{\bf U}+(\nabla{\bf U})^T)] - \alpha(xh){\bf U}$$

with RAMP Brinkman interpolation

$$\alpha(xh) = \alpha_{max}\frac{qAlpha(1-xh)}{qAlpha+xh}$$

### 4.5 Turbulence Suppression

The selected baseline is equation-level Brinkman sinks in the turbulence equations:

$$S_k = -\alpha(xh)\rho k, \qquad S_\epsilon = -\alpha(xh)\rho \epsilon$$

This must be implemented through `fvOptions` and remain runtime-switchable.

### 4.6 Thermal Transport

Use separate thermal interpolation:

$$D_T(xh)=\frac{k_s + (k_f-k_s)\,xh\,(1+qKappa)/(qKappa+xh)}{\rho c_p}$$

and, when enabled,

$$D_{T,eff} = D_T + \nu_t/Pr_t$$

with `Prt` configurable.

---

## 5. Adjoint Formulation

### 5.1 Selected baseline

Use two adjoint systems per optimization cycle:

- Flow-power adjoint `(Ua, pa)`
- Thermal-objective adjoint `(Ub, pb, Tb)`

### 5.2 Frozen Turbulence

The selected baseline is:

$$\delta \nu_t = 0$$

That is, use `nuEff = nu + nut` in the adjoint operators while freezing turbulence sensitivity.

This is the preferred baseline because it is substantially more robust than differentiating the full two-equation turbulence model inside density-based gray regions.

### 5.3 Adjoint viscous term

The selected baseline includes the full symmetric adjoint viscous operator:

- implicit Laplacian term with `nuEff`
- explicit transpose-stress correction

This should remain toggleable because it is a common source of implementation bugs.

---

## 6. Objective Stack for Electronics Cooling

### 6.1 Bring-up objective

Start with:

$$J = MeanT$$

for the first stable turbulent-adjoint bring-up.

### 6.2 Preferred production objective

Once the branch is stable, switch to:

$$J_{log} = \ln\left(\frac{MeanT}{T_{ref}}\right)$$

### 6.3 Preferred hotspot-aware objective

For realistic electronics cooling, the preferred advanced objective is KS aggregation:

$$J_{KS} = T_{max}^\star + \frac{1}{\rho_{KS}}\ln\left(\sum_e \exp[\rho_{KS}(T_e-T_{max}^\star)]\right)$$

### 6.4 Alternative objective for distributed loads

If the design target is temperature uniformity across a heated footprint, enable:

$$J_\sigma = \frac{1}{|\Omega_h|}\int_{\Omega_h}(T-\bar{T}_h)^2\,dV$$

### 6.5 Multi-case robustness

Keep this disabled for initial development, but available as a toggle:

$$J_{robust}=\sum_{s=1}^{N_s} w_s J_s$$

---

## 7. Runtime Toggle Philosophy

All **critical formulation choices** must be switchable through `constant/tuneOptParameters`, not hardcoded in C++.

### 7.1 Division of responsibility

- `constant/optProperties`
  - physical constants
  - geometric constants
  - continuation magnitudes
  - baseline constraint values

- `constant/tuneOptParameters`
  - all experimental switches
  - algorithm selection
  - fallback formulations
  - debug-safe simplified modes for physics and optimization, but not for core logging

### 7.2 Required MMA branch switches

The following controls are mandatory:

| Key in `tuneOptParameters` | Purpose |
|----------------------------|---------|
| `useFrozenTurbulenceAdjoint` | Toggle FT adjoint assumption |
| `useKEpsilonModel` | Baseline turbulence-model selection |
| `useWrayAgarwalFallback` | Wall-distance-free fallback |
| `usePorosityWallDistance` | Enable `dAlpha` PDE |
| `useMeshWaveWallDistanceFallback` | Debug fallback |
| `useBrinkmanSinkInKEpsilon` | Enable `k` and `epsilon` sinks |
| `useSplitRAMPControls` | Enable `qAlpha`, `qKappa`, `qHeat` |
| `useSingleQFallback` | Revert to one-parameter RAMP for debugging |
| `useTurbulentThermalDiffusivity` | Toggle `nut/Prt` contribution |
| `useLogMeanTObjective` | Toggle `Jlog` |
| `useKSHotspotObjective` | Toggle `JKS` |
| `useVarianceObjective` | Toggle `Jsigma` |
| `useRobustMultiCaseObjective` | Toggle multi-scenario aggregation |
| `usePowerConstraintRelaxation` | Toggle relaxed early power bound |
| `useGCMMA` | Optional fallback from classical MMA to GCMMA |
| `useStagnationTriggeredBeta` | Alternative projection continuation |
| `useFullAdjointSymmetricStress` | Toggle explicit transpose-stress correction |

### 7.3 Consistency rules for runtime switches

To keep the framework mutually stable and internally consistent, the following runtime rules apply:

- Exactly one turbulence-closure path is active:
  - baseline: `useKEpsilonModel = true`, `useWrayAgarwalFallback = false`
  - fallback: `useKEpsilonModel = false`, `useWrayAgarwalFallback = true`
- Exactly one primary thermal objective is active at a time:
  - baseline bring-up: all advanced objective switches off, which implies `MeanT`
  - advanced mode: one of `useLogMeanTObjective`, `useKSHotspotObjective`, or `useVarianceObjective` may be true
- `useRobustMultiCaseObjective` is a wrapper over the active primary objective, not an additional competing objective
- `useSplitRAMPControls = true` is the baseline; `useSingleQFallback = true` must only be used when split-RAMP mode is disabled
- Logging is not controlled by these switches and remains always enabled during current development

### 7.4 Recommended `tuneOptParameters` layout

```text
frameworkSwitches
{
    useFrozenTurbulenceAdjoint      true;
    useKEpsilonModel                true;
    useWrayAgarwalFallback          false;
    usePorosityWallDistance         true;
    useMeshWaveWallDistanceFallback false;
    useBrinkmanSinkInKEpsilon       true;
    useSplitRAMPControls            true;
    useSingleQFallback              false;
    useTurbulentThermalDiffusivity  true;
    usePowerConstraintRelaxation    true;
    useGCMMA                        false;
    useFullAdjointSymmetricStress   true;
}

objectiveSwitches
{
    useLogMeanTObjective            false;
    useKSHotspotObjective           false;
    useVarianceObjective            false;
    useRobustMultiCaseObjective     false;
}

continuationSwitches
{
    useStagnationTriggeredBeta      false;
}
```

---

## 8. Optimizer Debug Instrumentation

The new optimizer must extend the existing `debugOptimizer` framework and add a dedicated gradient/sensitivity log stream.

### 8.1 Required debug log files

The MMA branch must write, at minimum:

- `debugOptimizer.log` — human-readable optimizer diagnostics
- `debugOptimizer.jsonl` — structured per-iteration diagnostics
- `gradientOpt.log` — dedicated per-iteration gradient and sensitivity diagnostics
- `solverConvergences.log` — linear-solver residual summary

These logs are part of the default optimizer framework and must be enabled unconditionally during current development.

### 8.2 `gradientOpt.log` requirements

`gradientOpt.log` is mandatory and must be written every optimization iteration.

Its purpose is to isolate:

- exploding or vanishing sensitivities
- non-finite values
- sign errors in adjoint gradients
- filter-chain inconsistencies
- dominance of one constraint gradient over the others
- move-limit clipping or asymptote-induced stagnation

For every optimization iteration, log at least the following quantities:

- objective value and active objective label
- constraint values and weighted MMA constraint values
- `qAlpha`, `qKappa`, `qHeat`, `alphaMax`, `beta`
- min/max/L2/non-finite count of raw objective sensitivity
- min/max/L2/non-finite count of raw power-constraint sensitivity
- min/max/L2/non-finite count of raw volume sensitivity
- min/max/L2/non-finite count of filtered objective sensitivity
- min/max/L2/non-finite count of filtered power sensitivity
- min/max/L2/non-finite count of filtered volume sensitivity
- min/max of `drho = d(xh)/d(xp)`
- cosine similarity or normalized dot products between objective and constraint gradients when enabled
- max absolute design change, normalized L2 design change
- number of cells at lower bound, upper bound, and move limit
- gray-volume fraction, solid fraction, fluid fraction
- optional top-N cells or patches by sensitivity magnitude when detailed debug is enabled

The preferred format is structured text with one block per iteration. A JSONL mirror is acceptable as an additional output, but `gradientOpt.log` itself must remain human-readable.

### 8.3 `debugOptimizer.log` runtime-option dump

At optimizer startup, write a complete echo of all active runtime options from:

- `constant/optProperties`
- `constant/tuneOptParameters`

into `debugOptimizer.log`.

This dump must include:

- all physical properties relevant to interpolation and constraints
- all continuation settings
- all framework switches
- all objective switches
- all continuation switches

The startup dump must make it possible to reconstruct the exact optimizer formulation used in a run without reopening the case dictionaries manually.

### 8.4 Per-iteration option echo for changing controls

If any runtime control changes during optimization, also write the active per-iteration values into `debugOptimizer.log` and `gradientOpt.log`, including:

- `alphaMax`
- `qAlpha`, `qKappa`, `qHeat`
- `beta`
- active objective selector
- active wall-distance mode
- active turbulence-model mode

### 8.5 Logging activation policy

For the current development phase:

- `debugOptimizer.log` is always enabled
- `debugOptimizer.jsonl` is always enabled
- `gradientOpt.log` is always enabled
- runtime-option dumping is always enabled

Do not place these core logging features behind `tuneOptParameters` switches for now. If log-volume reduction becomes necessary later, that can be introduced as a future refinement after the optimizer frameworks are stable.

---

## 9. Development Order

### Phase A - branch creation

Create a new independent folder:

- `turbulenceMMAOpt/src`
- `turbulenceMMAOpt/app`

initialized from `laminarOptimizer/`.

### Phase B - turbulent primal bring-up

Implement and verify:

- `kEpsilon` runtime case files
- `k`, `epsilon`, `nut` boundary conditions
- `fvOptions` turbulence sinks
- split `qAlpha/qKappa/qHeat`
- turbulent thermal diffusivity toggle
- startup option dump into `debugOptimizer.log`

### Phase C - wall treatment

Implement:

- porosity-aware wall-distance PDE
- debug fallback to standard `meshWave`
- gradient logging for wall-distance-related sensitivities and mode selection

### Phase D - adjoint bring-up

Implement and verify:

- `nuEff` adjoint operator
- symmetric adjoint viscous correction
- FT toggles
- `gradientOpt.log` entries for raw and filtered sensitivities every iteration

### Phase E - objective upgrades

Promote:

- `MeanT` -> `Jlog`
- `Jlog` -> `JKS` or `Jsigma`
- extend `gradientOpt.log` to objective-specific adjoint-source diagnostics

### Phase F - geometry workflow

Final deliverable of this branch is:

- stable 2D topology
- clean `xh=0.5` contour
- post-processed extrusion to plate thickness

---

## 10. Target File Map

| File | Role |
|------|------|
| `turbulenceMMAOpt/src/turbulenceMMAOpt.C` | Main optimization loop |
| `turbulenceMMAOpt/src/createFields.H` | Base fields + new tuning-switch reads |
| `turbulenceMMAOpt/src/readTransportProperties.H` | Flow properties + split RAMP initialization |
| `turbulenceMMAOpt/src/readThermalProperties.H` | Thermal properties + objective source setup |
| `turbulenceMMAOpt/src/Primal_U.H` | Turbulent primal momentum + Brinkman term |
| `turbulenceMMAOpt/src/Primal_kEpsilon.H` | Explicit `k` and `epsilon` solves or model hooks |
| `turbulenceMMAOpt/src/Primal_T.H` | Energy equation with optional turbulent diffusion |
| `turbulenceMMAOpt/src/AdjointFlow_Ua.H` | Power adjoint with `nuEff` |
| `turbulenceMMAOpt/src/AdjointHeat_Ub.H` | Thermal adjoint momentum |
| `turbulenceMMAOpt/src/AdjointHeat_Tb.H` | Thermal adjoint scalar with MeanT/log/KS/variance sources |
| `turbulenceMMAOpt/src/wallDistanceAlpha.H` | Porosity-aware wall-distance solve |
| `turbulenceMMAOpt/src/filter_x.H` | Helmholtz filter + projection |
| `turbulenceMMAOpt/src/filter_chainrule.H` | Reverse filter chain-rule sensitivities |
| `turbulenceMMAOpt/src/sensitivity.H` | Sensitivities + MMA update |
| `turbulenceMMAOpt/src/update.H` | Split continuation schedules |
| `turbulenceMMAOpt/src/debugOptimizer.H` | Human-readable iteration diagnostics + runtime-option echo |
| `turbulenceMMAOpt/src/gradientOptWrite.H` | Dedicated per-iteration gradient/sensitivity logging |
| `turbulenceMMAOpt/app/constant/tuneOptParameters` | Runtime switches for all critical formulation choices |

---

## 11. Critical References

| Reference | Relevance |
|-----------|-----------|
| Sun et al. (2023) | `kEpsilon` turbulent cooling-channel topology optimization |
| Othmer (2008, 2014) | Continuous adjoint and Frozen-Turbulence practice |
| Wang, Lazarov and Sigmund (2011) | Heaviside projection and volume-preserving thresholding |
| Lazarov and Sigmund (2016) | PDE filter chain rule |
| Kontoleontos et al. (2013) | Density-aware wall-distance treatment |
| Dilgen et al. (2018) | Turbulence suppression via penalization/sinks |
| Haertel et al. (2018) | Reduced-order thermofluid heat-sink optimization |
| Zeng et al. (2019) | Two-layer microchannel heat-sink optimization |
| Huang et al. (2024/2025) | Pseudo-3D heat-sink optimization for extruded devices |
| Alonso et al. (2022) | Wray-Agarwal fallback option |

---

## 12. Final Position

For the intended goal of generating extruded cold plates from a quasi-2D setup, this MMA branch is the **preferred production optimizer**.

It should be developed first, and it should remain conservative in three ways:

- keep the problem quasi-2D
- keep `kEpsilon + FT` as the baseline
- keep every unstable formulation choice behind a `tuneOptParameters` switch

That combination gives the best chance of reaching a robust turbulent optimizer quickly while still retaining controlled escape routes if instability appears.
