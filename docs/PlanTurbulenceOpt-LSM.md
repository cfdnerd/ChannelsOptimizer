# PlanTurbulenceOpt-LSM: Independent Quasi-2D Turbulent Level-Set Optimizer

**Target folder:** `turbulenceLSMOpt/`  
**Method:** Level-set topology optimization on a quasi-2D domain  
**Target product:** 2D cooling topology for later **extrusion into a cold plate**

---

## 1. Development Position

This branch is to be developed **independently** from the MMA branch in its own folder:

- `turbulenceLSMOpt/src`
- `turbulenceLSMOpt/app`

It may inherit selected utilities from `laminarOptimizer/`, but it is **not** expected to share the same design-update logic.

The intended scope is:

- quasi-2D optimization only
- no full 3D topology optimization
- final 2D contour extraction followed by extrusion

This branch exists because level-set methods can produce cleaner final geometry than density methods, but the LSM branch is still the **higher-risk branch** relative to MMA and should therefore be documented with explicit runtime fallbacks from the start.

---

## 2. Selected SOTA Direction for This Branch

For the intended goal, the preferred LSM direction is:

1. signed-distance level-set field `phiLS`
2. explicit interface-aware turbulent wall treatment
3. level-set-native sensitivity-to-velocity pipeline
4. reaction-diffusion regularization of the interface motion field
5. Hamilton-Jacobi update of `phiLS`
6. contour extraction of `phiLS = 0`
7. extrusion to cold-plate thickness

This is intentionally **not** the same update logic as the MMA branch.

### 2.1 Why this is selected

For a quasi-2D extruded cold-plate workflow:

- the geometry crispness of LSM is useful
- direct interface handling is more valuable than gray-region optimization
- level-set-native motion regularization is more defensible than reusing the density-chain-rule as the primary formulation
- the reduced 2D scope makes explicit interface handling tractable

---

## 3. Roadblock Resolution Strategy

This section resolves the critical ambiguities with an ordered preference list. The first option in each subsection is the selected baseline. Lower options remain runtime-selectable fallbacks if instability appears.

### 3.1 LSM Update Strategy

#### Preferred option: level-set-native interface motion with reaction-diffusion regularization

The selected baseline is:

1. compute a raw interface sensitivity from the turbulent primal + adjoint fields
2. convert that sensitivity into a normal velocity field near the interface
3. regularize that velocity with a reaction-diffusion or Helmholtz-type smoothing step
4. advect `phiLS` with a Hamilton-Jacobi update
5. reinitialize `phiLS` to a signed-distance field

This is the preferred compromise between:

- SOTA LSM practice
- the practical realities of OpenFOAM-6 implementation
- the need for explicit debug fallbacks

It is more level-set-native than routing all sensitivities through the density `x/xp/xh` machinery.

#### Alternative A: pure Hamilton-Jacobi update with direct interface sensitivity

This remains available as a simpler fallback:

$$\frac{\partial \phi}{\partial \tau} + V_n |\nabla \phi| = 0$$

with classical reinitialization and no separate reaction-diffusion smoothing step.

This is acceptable if the reaction-diffusion regularization introduces too much implementation complexity during bring-up.

#### Alternative B: density-assisted debug mode

As a last-resort debugging mode, permit reconstruction of auxiliary `x/xp/xh` fields from `phiLS` and use the existing density-style sensitivity infrastructure to isolate primal/adjoint bugs.

This mode is explicitly a **debug fallback**, not the selected production formulation.

### 3.2 RAMP Continuation Inside the LSM Fictitious Domain

Even though LSM is the design method, the primal equations still use interpolated material properties in the narrow band.

#### Preferred option: split interpolation controls

Use:

- `qAlphaLSM` for Brinkman flow resistance
- `qKappaLSM` for thermal interpolation

with decreasing schedules:

$$qAlphaLSM^{(k+1)} \le qAlphaLSM^{(k)}, \qquad qKappaLSM^{(k+1)} \le qKappaLSM^{(k)}$$

This is selected for the same reason as in the MMA branch: hydraulic hardening and thermal hardening should not be forced to occur on one shared schedule.

#### Alternative A: single `qLSM`

Allowed for simplified debugging only.

#### Alternative B: hard-binary interface forcing

If narrow-band interpolation proves unstable in late iterations, allow a hard-interface experimental mode after convergence of the main topology. This is not the preferred baseline because it is less numerically forgiving.

### 3.3 Turbulent Wall Treatment

#### Preferred option: interface-derived wall distance for the LSM boundary

The selected baseline is to compute an evolving wall distance from the level-set geometry each optimization iteration.

For a reinitialized signed-distance field, the fluid-side distance to the moving interface is already available from `phiLS`. The selected implementation stance is:

$$d_{LS} = \min(\phi^+, d_{fixed})$$

used only after reinitialization has restored the signed-distance property. This gives a practical interface-aware wall distance for the quasi-2D branch.

This is selected because:

- the branch is only quasi-2D
- the interface is explicit
- it avoids building a porous gray-wall model into the LSM branch

#### Alternative A: auxiliary PDE wall distance

If the direct `phiLS` distance proves too noisy, solve a Poisson/heat-method distance field from the reconstructed interface each iteration.

This is the preferred fallback because it remains interface-aware and is still inexpensive in 2D.

#### Alternative B: turbulence-model fallback

If the interface-aware `kEpsilon` wall treatment remains unstable, the controlled fallback is to switch the LSM branch to a simpler closure such as Wray-Agarwal or another wall-distance-light model. This should remain a fallback, not the first implementation.

### 3.4 Scope Resolution: quasi-2D extrusion workflow

The selected workflow is:

- optimize in a 2D planar domain
- treat plate thickness as prescribed
- report hydraulic and thermal quantities either per-unit-depth or using the selected extrusion depth
- export a final 2D contour for extrusion

The LSM branch therefore does **not** need:

- full 3D remeshing
- 3D level-set reinitialization
- 3D topological nucleation logic

---

## 4. Governing Formulation

### 4.1 Level-Set Representation

Use a signed-distance level-set field:

$$|\nabla \phi| = 1$$

with:

$$\phi > 0: \text{fluid}, \qquad \phi < 0: \text{solid}, \qquad \phi = 0: \Gamma$$

### 4.2 Narrow-Band Physical Field Reconstruction

Reconstruct a physical fluid indicator:

$$xh = H_\epsilon(\phiLS)$$

using a smoothed Heaviside over a narrow band.

This field exists only to provide stable coefficients for the primal equations. It is **not** the primary design variable.

### 4.3 Turbulent Primal Momentum

Use a Brinkman-penalized turbulent RANS momentum equation:

$$({\bf U}\cdot\nabla){\bf U} = -\nabla p + \nabla\cdot[(\nu+\nu_t)(\nabla{\bf U}+(\nabla{\bf U})^T)] - \alpha(xh){\bf U}$$

with

$$\alpha(xh) = \alpha_{max}\frac{qAlphaLSM(1-xh)}{qAlphaLSM+xh}$$

### 4.4 Turbulence Suppression

The selected baseline is equation-level damping in the turbulence equations:

$$S_k = -\alpha(xh)\rho k, \qquad S_\epsilon = -\alpha(xh)\rho \epsilon$$

kept behind a runtime switch.

### 4.5 Thermal Transport

Use separate thermal interpolation:

$$D_T(xh)=\frac{k_s+(k_f-k_s)\,xh\,(1+qKappaLSM)/(qKappaLSM+xh)}{\rho c_p}$$

with optional turbulent thermal contribution:

$$D_{T,eff}=D_T+\nu_t/Pr_t$$

---

## 5. Adjoint and Interface Sensitivity

### 5.1 Selected baseline

Use the same dual-adjoint structure as the MMA branch:

- power adjoint `(Ua, pa)`
- thermal adjoint `(Ub, pb, Tb)`

### 5.2 Frozen Turbulence

The selected baseline is Frozen Turbulence:

$$\delta \nu_t = 0$$

for the same stability reasons as the MMA branch.

### 5.3 Interface sensitivity pipeline

Selected baseline:

1. compute raw sensitivity with respect to the reconstructed physical field `xh`
2. map the sensitivity to the interface with the regularized Dirac delta
3. regularize the resulting normal velocity
4. update `phiLS`

$$\frac{dJ}{d\phi} = -\frac{\partial J}{\partial xh}\,\delta_\epsilon(\phi)$$

This is the selected production path.

The density-assisted chain rule remains available only as a debugging switch.

---

## 6. Objective Stack for Electronics Cooling

The LSM branch should use the same objective hierarchy as the MMA branch so that comparisons between branches remain meaningful.

### 6.1 Bring-up objective

$$J = MeanT$$

### 6.2 Preferred production objective

$$J_{log} = \ln\left(\frac{MeanT}{T_{ref}}\right)$$

### 6.3 Preferred hotspot-aware objective

$$J_{KS} = T_{max}^\star + \frac{1}{\rho_{KS}}\ln\left(\sum_e \exp[\rho_{KS}(T_e-T_{max}^\star)]\right)$$

### 6.4 Alternative uniformity objective

$$J_\sigma = \frac{1}{|\Omega_h|}\int_{\Omega_h}(T-\bar{T}_h)^2\,dV$$

### 6.5 Multi-case robustness

Keep disabled initially, but available:

$$J_{robust}=\sum_{s=1}^{N_s} w_s J_s$$

---

## 7. Runtime Toggle Philosophy

Every unstable or experimental LSM feature must be runtime-selectable through `constant/tuneOptParameters`.

### 7.1 Division of responsibility

- `constant/optProperties`
  - physical constants
  - continuation magnitudes
  - extrusion-thickness reference values

- `constant/tuneOptParameters`
  - formulation switches
  - update-model switches
  - turbulence-model fallbacks
  - debug-safe modes for physics and optimizer behavior, but not for core logging

### 7.2 Required LSM branch switches

| Key in `tuneOptParameters` | Purpose |
|----------------------------|---------|
| `useFrozenTurbulenceAdjoint` | Toggle FT adjoint assumption |
| `useKEpsilonModel` | Baseline turbulence model |
| `useWrayAgarwalFallback` | Simplified fallback turbulence closure |
| `useBrinkmanSinkInKEpsilon` | Enable `k` and `epsilon` sinks |
| `useSplitRAMPControlsLSM` | Enable `qAlphaLSM` and `qKappaLSM` |
| `useSingleQLSMFallback` | One-parameter interpolation fallback |
| `useReactionDiffusionLSMUpdate` | Preferred motion regularization |
| `usePureHamiltonJacobiFallback` | Simpler advection fallback |
| `useDensityAssistedLSMDebug` | Last-resort debugging mode |
| `useTopologicalNucleation` | Optional later experimental feature |
| `useInterfaceDerivedWallDistance` | Baseline wall treatment |
| `useAuxiliaryPDEWallDistance` | Fallback wall-distance solve |
| `useMeshWaveWallDistanceFallback` | Debug-only fallback |
| `useTurbulentThermalDiffusivity` | Toggle `nut/Prt` contribution |
| `useLogMeanTObjective` | Toggle `Jlog` |
| `useKSHotspotObjective` | Toggle `JKS` |
| `useVarianceObjective` | Toggle `Jsigma` |
| `useRobustMultiCaseObjective` | Toggle multi-scenario aggregation |
| `useWENO5ForPhiAdvection` | Higher-order advection option |
| `useSussmanReinitialization` | Signed-distance restoration |

### 7.3 Consistency rules for runtime switches

To keep the framework mutually stable and internally consistent, the following runtime rules apply:

- Exactly one turbulence-closure path is active:
  - baseline: `useKEpsilonModel = true`, `useWrayAgarwalFallback = false`
  - fallback: `useKEpsilonModel = false`, `useWrayAgarwalFallback = true`
- Exactly one primary LSM update path is active:
  - baseline: `useReactionDiffusionLSMUpdate = true`, `usePureHamiltonJacobiFallback = false`
  - simplified fallback: `useReactionDiffusionLSMUpdate = false`, `usePureHamiltonJacobiFallback = true`
- `useDensityAssistedLSMDebug` is a debug-only fallback and must not be combined with the production LSM update path for performance claims
- Exactly one wall-distance path is active:
  - baseline: `useInterfaceDerivedWallDistance = true`
  - fallback: `useAuxiliaryPDEWallDistance = true`
  - bring-up/debug only: `useMeshWaveWallDistanceFallback = true`
- Exactly one primary thermal objective is active at a time:
  - baseline bring-up: all advanced objective switches off, which implies `MeanT`
  - advanced mode: one of `useLogMeanTObjective`, `useKSHotspotObjective`, or `useVarianceObjective` may be true
- `useRobustMultiCaseObjective` is a wrapper over the active primary objective, not an additional competing objective
- Logging is not controlled by these switches and remains always enabled during current development

### 7.4 Recommended `tuneOptParameters` layout

```text
frameworkSwitches
{
    useFrozenTurbulenceAdjoint     true;
    useKEpsilonModel               true;
    useWrayAgarwalFallback         false;
    useBrinkmanSinkInKEpsilon      true;
    useSplitRAMPControlsLSM        true;
    useSingleQLSMFallback          false;
    useInterfaceDerivedWallDistance true;
    useAuxiliaryPDEWallDistance    false;
    useMeshWaveWallDistanceFallback false;
    useTurbulentThermalDiffusivity true;
}

lsmSwitches
{
    useReactionDiffusionLSMUpdate  true;
    usePureHamiltonJacobiFallback  false;
    useDensityAssistedLSMDebug     false;
    useTopologicalNucleation       false;
    useWENO5ForPhiAdvection        false;
    useSussmanReinitialization     true;
}

objectiveSwitches
{
    useLogMeanTObjective           false;
    useKSHotspotObjective          false;
    useVarianceObjective           false;
    useRobustMultiCaseObjective    false;
}
```

---

## 8. Optimizer Debug Instrumentation

The LSM branch must extend the `debugOptimizer` framework with dedicated logging for interface sensitivities, normal velocities, and level-set evolution diagnostics.

### 8.1 Required debug log files

The LSM branch must write, at minimum:

- `debugOptimizer.log` — human-readable optimizer diagnostics
- `debugOptimizer.jsonl` — structured per-iteration diagnostics
- `gradientOpt.log` — dedicated per-iteration gradient, interface-sensitivity, and velocity diagnostics
- `solverConvergences.log` — linear-solver residual summary

These logs are part of the default optimizer framework and must be enabled unconditionally during current development.

### 8.2 `gradientOpt.log` requirements

`gradientOpt.log` is mandatory and must be updated every optimization iteration.

Its purpose is to isolate:

- sign errors in the interface sensitivity
- loss of localization of the Dirac-delta band
- noisy or unstable normal velocities
- reinitialization failure
- sensitivity collapse away from the interface
- instability caused by reaction-diffusion regularization or advection

For every optimization iteration, log at least the following quantities:

- objective value and active objective label
- constraint values and weighted constraint values
- `qAlphaLSM`, `qKappaLSM`, `alphaMax`, `epsilonLSM`
- min/max/L2/non-finite count of raw `dJ/dxh`
- min/max/L2/non-finite count of raw constraint sensitivities
- min/max/L2/non-finite count of interface-mapped `dJ/dphi`
- min/max/L2/non-finite count of interface-mapped constraint sensitivities
- min/max/L2/non-finite count of regularized normal velocity `Vn`
- min/max of `delta_epsilon(phi)`
- interface-band volume fraction `|phi| <= epsilonLSM`
- max absolute change and L2 change of `phiLS`
- min/max of `|grad(phiLS)|`
- signed-distance reinitialization residual indicators
- optional top-N interface cells by sensitivity magnitude when detailed debug is enabled

If the branch enters density-assisted debug mode, `gradientOpt.log` must additionally include the auxiliary `x/xp/xh` sensitivity metrics used in that fallback mode.

### 8.3 `debugOptimizer.log` runtime-option dump

At optimizer startup, write a complete dump of all active runtime options from:

- `constant/optProperties`
- `constant/tuneOptParameters`

into `debugOptimizer.log`.

This startup section must include:

- all physical interpolation parameters
- all continuation settings
- all LSM update switches
- all wall-distance switches
- all objective switches

This is mandatory so the exact formulation used in any run can be reconstructed from the logs alone.

### 8.4 Per-iteration option echo for active modes

If any algorithmic mode can change during a run, echo the active mode and current control values into `debugOptimizer.log` and `gradientOpt.log`, including:

- `alphaMax`
- `qAlphaLSM`, `qKappaLSM`
- `epsilonLSM`
- active wall-distance mode
- active LSM update mode
- active turbulence-model mode
- active objective selector

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

Create:

- `turbulenceLSMOpt/src`
- `turbulenceLSMOpt/app`

in the same general style as `laminarOptimizer/`, but without forcing the MMA update logic into this branch.

### Phase B - turbulent primal bring-up

Implement and verify:

- `kEpsilon` case files
- turbulence sinks
- split `qAlphaLSM/qKappaLSM`
- interface reconstruction fields
- startup option dump into `debugOptimizer.log`

### Phase C - wall treatment

Implement:

- interface-derived wall distance
- auxiliary PDE fallback
- logging of active wall-distance mode and wall-distance health indicators

### Phase D - interface evolution

Implement in order:

1. direct sensitivity-to-velocity mapping
2. reaction-diffusion regularization
3. Hamilton-Jacobi update
4. reinitialization

Only after this is stable should higher-order advection or topological nucleation be enabled.

During this phase, `gradientOpt.log` must report:

- raw interface sensitivities
- mapped `dJ/dphi`
- regularized `Vn`
- `phiLS` update norms
- reinitialization quality metrics

### Phase E - objective upgrades

Promote:

- `MeanT` -> `Jlog`
- `Jlog` -> `JKS` or `Jsigma`
- extend `gradientOpt.log` to objective-specific adjoint-source diagnostics

### Phase F - final geometry workflow

Deliver:

- stable 2D zero-contour
- contour smoothing/cleanup if needed
- extrusion to prescribed plate thickness

---

## 10. Target File Map

| File | Role |
|------|------|
| `turbulenceLSMOpt/src/turbulenceLSMOpt.C` | Main optimization loop |
| `turbulenceLSMOpt/src/createFields.H` | Base fields + tuning-switch reads |
| `turbulenceLSMOpt/src/readTransportProperties.H` | Flow properties + `qAlphaLSM` initialization |
| `turbulenceLSMOpt/src/readThermalProperties.H` | Thermal properties + `qKappaLSM` setup |
| `turbulenceLSMOpt/src/Primal_U.H` | Turbulent primal momentum |
| `turbulenceLSMOpt/src/Primal_kEpsilon.H` | Turbulence-equation hooks / sink terms |
| `turbulenceLSMOpt/src/Primal_T.H` | Energy equation |
| `turbulenceLSMOpt/src/AdjointFlow_Ua.H` | Power adjoint |
| `turbulenceLSMOpt/src/AdjointHeat_Ub.H` | Thermal adjoint momentum |
| `turbulenceLSMOpt/src/AdjointHeat_Tb.H` | Thermal adjoint scalar |
| `turbulenceLSMOpt/src/lsmInterfaceReconstruct.H` | `xh = H(phiLS)` and narrow-band coefficients |
| `turbulenceLSMOpt/src/lsmWallDistance.H` | Interface-derived wall distance or PDE fallback |
| `turbulenceLSMOpt/src/lsmSensitivity.H` | Raw interface sensitivity |
| `turbulenceLSMOpt/src/lsmVelocityRegularization.H` | Reaction-diffusion or alternative smoothing |
| `turbulenceLSMOpt/src/lsmAdvection.H` | Hamilton-Jacobi update |
| `turbulenceLSMOpt/src/lsmReinitialize.H` | Signed-distance restoration |
| `turbulenceLSMOpt/src/update.H` | Split continuation schedules |
| `turbulenceLSMOpt/src/debugOptimizer.H` | Human-readable iteration diagnostics + runtime-option echo |
| `turbulenceLSMOpt/src/gradientOptWrite.H` | Dedicated per-iteration gradient/interface-sensitivity logging |
| `turbulenceLSMOpt/app/constant/tuneOptParameters` | Runtime formulation switches |

---

## 11. Critical References

| Reference | Relevance |
|-----------|-----------|
| Kubo et al. (2021) | 2D turbulent level-set TO with immersed-boundary treatment |
| Noel and Maute (2022/2023) | Turbulent CHT level-set topology optimization |
| Wang et al. (2026) | Level-set CHT with turbulence-model simplification |
| Sun et al. (2023) | Turbulent cooling-channel TO baseline for comparison |
| Othmer (2008, 2014) | Continuous adjoint and FT practice |
| Alonso et al. (2022) | Wray-Agarwal fallback option |
| Haertel et al. (2018) | Reduced-order heat-sink optimization |
| Huang et al. (2024/2025) | Pseudo-3D/extruded electronics-cooling optimization scope |

---

## 12. Final Position

For the intended extruded-cold-plate workflow, this LSM branch should be developed as an **independent geometry-focused alternative** to MMA.

Its selected baseline is:

- quasi-2D only
- explicit level-set design variable
- interface-aware wall treatment
- reaction-diffusion-regularized interface motion
- Hamilton-Jacobi advection and reinitialization
- all unstable choices guarded by `tuneOptParameters`

This gives the branch a clear SOTA-informed identity while still preserving debug-safe alternatives if instability appears during development.
