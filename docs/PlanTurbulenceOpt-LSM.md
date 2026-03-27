# PlanTurbulenceOpt-LSM: Independent Quasi-2D Turbulent Level-Set Optimizer

**Target folder:** `turbulenceLSMOpt/`  
**Method:** Level-set topology optimization on a quasi-2D domain  
**Target product:** 2D cooling topology for later extrusion into a cold plate

See also:

- [PlanTurbulenceOpt-MMA.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/PlanTurbulenceOpt-MMA.md) for the working turbulence density-optimizer architecture that this branch should reuse wherever possible.
- [TurbulenceMMAOptDebugFindings.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/TurbulenceMMAOptDebugFindings.md) for the completed 2026-03 debugging cycle that identified which runtime controls actually govern optimizer evolution.
- [TurbulenceLSMOptDebugFindings.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/TurbulenceLSMOptDebugFindings.md) for the current LSM-specific failure diagnosis from the latest structured log snapshot.
- [TurbulenceLSMOptGrayFractionExperimentPlan.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/TurbulenceLSMOptGrayFractionExperimentPlan.md) for the current LSM experiment ladder that targets early collapse and failed channel reopening.
- [tuningGuideMMA.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/tuningGuideMMA.md) for the current control-surface philosophy that has already been proven useful in practice.

---

## 1. Development Position

This branch should still be developed independently from the MMA branch in its
own folder:

- `turbulenceLSMOpt/src`
- `turbulenceLSMOpt/app`

But the implementation plan must now be grounded in the current repository
state:

- `turbulenceLSMOpt` is **not yet a true LSM solver**
- the current source still follows the laminar/MMA density workflow with
  `x/xp/xh`
- the current update path still uses `MMA`, `filter_x.H`, `filter_chainrule.H`,
  and `MMAsolver`
- `tuneOptParameters` in the LSM app is currently only a minimal convergence
  stub

Therefore this document is not a greenfield formulation note. It is a **refactor
and replacement plan** for converting the current density-style scaffold into a
working turbulent level-set optimizer while preserving the parts of
`turbulenceMMAOpt` that are already known to work.

The intended scope remains:

- quasi-2D optimization only
- no full 3D topology optimization
- final 2D contour extraction followed by extrusion

The LSM branch is still the higher-risk branch relative to MMA, so the selected
strategy is:

1. reuse the working turbulence primal/adjoint/debug backbone from MMA
2. replace only the design representation and update path with LSM-native logic
3. keep explicit debug fallbacks that can isolate physics, adjoint, and update
   failures independently

---

## 2. Repository-Backed Lessons That Must Transfer From `turbulenceMMAOpt`

The completed MMA debugging cycle already established several facts that must be
treated as design constraints for the LSM branch.

### 2.1 Controls that already proved decisive

From [TurbulenceMMAOptDebugFindings.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/TurbulenceMMAOptDebugFindings.md):

- the main early-run failure trigger was **aggressive startup interpolation**,
  especially `q=0.1`
- the turbulence extras were secondary relative to startup interpolation
- low-startup-`q` operation was necessary to reproduce healthy channel
  evolution
- continuation should be debugged only after startup behavior is correct

This means the LSM branch must **not** be brought up with an aggressive
hard-binary fictitious-domain regime. The signed-distance representation does
not remove the need for soft startup in the primal equations.

### 2.2 Infrastructure that should be reused unchanged

The following elements from `turbulenceMMAOpt` are already valuable and should
be reused rather than reinvented:

- turbulent primal/adjoint structure
- frozen-turbulence adjoint baseline
- `kEpsilon` baseline with optional porous damping in `k` and `epsilon`
- split hydraulic and thermal interpolation controls
- power-constraint relaxation and feasibility gating
- late-stage hardening throttles based on gray-fraction and step-size health
- startup runtime-option dump
- always-on `debugOptimizer.log`, `debugOptimizer.jsonl`,
  `gradientOpt.log`, and `solverConvergences.log`
- adjoint runaway detection and structured solver-health logging
- the staged objective stack:
  `MeanT -> log(MeanT) -> KS hotspot or variance`

### 2.3 Naming and dictionary philosophy

Where the concept is shared between MMA and LSM, the same runtime key should be
used unless there is a strong reason not to.

Prefer:

- `useFrozenTurbulenceAdjoint`
- `useKEpsilonModel`
- `useBrinkmanSinkInKEpsilon`
- `useTurbulentThermalDiffusivity`
- `usePowerConstraintRelaxation`
- `useSplitRAMPControls`

Avoid introducing LSM-specific suffixes for shared controls unless the
underlying meaning truly changes. This keeps the tuning surface easier to audit
across branches and makes the debug logs easier to compare.

---

## 3. Selected SOTA Direction For This Branch

For the intended quasi-2D extruded cold-plate workflow, the preferred LSM
direction remains:

1. signed-distance level-set field `phiLS`
2. reconstructed physical indicator `xh = H_epsilon(phiLS)` for the primal and
   adjoint equations
3. turbulent primal and adjoint systems reused from the MMA branch as far as
   possible
4. interface-native sensitivity mapping to a normal velocity field
5. reaction-diffusion or Helmholtz regularization of that velocity
6. Hamilton-Jacobi advection of `phiLS`
7. periodic signed-distance reinitialization
8. contour extraction of `phiLS = 0`
9. downstream extrusion to plate thickness

This is the correct compromise between:

- SOTA LSM practice
- the current OpenFOAM-6 codebase reality
- the need to build a working optimizer quickly from a known-good turbulence
  backbone

### 3.1 What is SOTA-compliant here

The LSM branch should be considered SOTA-aligned only if all of the following
hold:

- the design variable is `phiLS`, not MMA-updated density `x`
- the production update law is interface motion, not `MMAsolver`
- `xh` exists only as a physics reconstruction field, not as the primary design
  state
- shape evolution is regularized through an interface-velocity solve, not only
  by density filtering
- wall treatment is aware of the moving LSM interface
- reinitialization quality is monitored explicitly
- high-order advection, topological nucleation, and other advanced features are
  treated as later upgrades rather than bring-up prerequisites

### 3.2 What is deliberately not selected for first implementation

The following may remain future options, but they are not part of the baseline
working path:

- topological nucleation during early bring-up
- fully differentiated turbulence transport
- hard-interface-only coefficients from iteration 1
- WENO5 advection before the first-order path is stable
- robust multi-case optimization before the single-case branch is healthy

---

## 4. Architecture Bridge From The Current `turbulenceLSMOpt` Scaffold

Because the current branch is still density/MMA-based, the practical route is a
staged replacement rather than a full rewrite.

### 4.1 Transitional architecture

The first developable LSM architecture should be:

- keep the current primal and adjoint equation organization
- introduce `phiLS` as the sole design state
- reconstruct `xh = H_epsilon(phiLS)` every optimization iteration
- continue evaluating `alpha(xh)`, `DT(xh)`, and any DDHS terms through `xh`
- compute sensitivities with respect to `xh` first
- map those sensitivities onto the interface through `delta_epsilon(phiLS)`
- update `phiLS` through interface velocity regularization plus advection

This keeps the turbulence physics path close to the working MMA branch while
still making the optimizer genuinely level-set-native.

### 4.2 Production path versus debug path

The branch should carry two explicitly different update modes:

#### Production path

- `phiLS -> xh -> primal/adjoint -> dJ/dxh -> dJ/dphi -> Vn -> phiLS`

#### Debug fallback

- reconstruct auxiliary `x/xp/xh`-style quantities from `phiLS`
- use density-style filtering or chain-rule checks only to diagnose bugs
- never present this fallback as the branch's production formulation

### 4.3 Files that should be retired from the production path

Once the LSM path is active, the following current mechanisms should no longer
be part of the production update loop:

- direct use of `MMA` in `turbulenceLSMOpt`
- `filter_x.H` as the primary design update
- `filter_chainrule.H` as the primary sensitivity propagation route
- `x` as the optimizer-owned design field
- `xp` as a required state variable outside debug fallback mode

They may remain temporarily during refactoring, but the end state must clearly
separate:

- reusable turbulence physics infrastructure
- obsolete density-update infrastructure

---

## 5. Governing Formulation

### 5.1 Level-set representation

Use a signed-distance level-set field:

$$|\nabla \phiLS| = 1$$

with:

$$\phiLS > 0: \text{fluid}, \qquad \phiLS < 0: \text{solid}, \qquad \phiLS = 0: \Gamma$$

### 5.2 Narrow-band reconstruction for the physics

Use:

$$xh = H_\epsilon(\phiLS)$$

where `H_epsilon` is a smooth Heaviside over a narrow band of width
`epsilonLSM = O(h)` to `O(2h)`.

Recommended bring-up default:

- `epsilonLSM` about `1.5` local cell widths for quasi-2D cases

This field exists to feed the primal and adjoint equations and to preserve
continuity with the current `alpha(xh)` and `DT(xh)` infrastructure.

### 5.3 Turbulent primal momentum

Use a Brinkman-penalized turbulent RANS momentum equation:

$$({\bf U}\cdot\nabla){\bf U} = -\nabla p + \nabla\cdot[(\nu+\nu_t)(\nabla{\bf U}+(\nabla{\bf U})^T)] - \alpha(xh){\bf U}$$

with

$$\alpha(xh) = \alpha_{max}\frac{qAlpha(1-xh)}{qAlpha+xh}$$

The shared runtime key should remain `qAlpha`, not `qAlphaLSM`, unless the code
later requires both density and LSM formulations in one executable.

### 5.4 Turbulence suppression

The selected baseline remains equation-level damping in the turbulence
equations:

$$S_k = -\alpha(xh)\rho k, \qquad S_\epsilon = -\alpha(xh)\rho \epsilon$$

This should remain runtime-switchable exactly as in the MMA branch.

### 5.5 Thermal transport

Use separate thermal interpolation:

$$D_T(xh)=\frac{k_s+(k_f-k_s)\,xh\,(1+qKappa)/(qKappa+xh)}{\rho c_p}$$

with optional turbulent thermal contribution:

$$D_{T,eff}=D_T+\nu_t/Pr_t$$

If design-dependent heat generation is used, a separate `qHeat` control may be
retained exactly as in the MMA branch.

### 5.6 Adjoint baseline

Use the same dual-adjoint structure as the MMA branch:

- power adjoint `(Ua, pa)`
- thermal adjoint `(Ub, pb, Tb)`

The selected baseline remains Frozen Turbulence:

$$\delta \nu_t = 0$$

This is the only sensible developable baseline until the branch is otherwise
stable.

### 5.7 Interface sensitivity pipeline

The production sensitivity path should be:

1. compute raw sensitivities with respect to `xh`
2. map them to the interface using `delta_epsilon(phiLS)`
3. assemble a raw normal velocity `Vn_raw`
4. regularize that velocity
5. advect `phiLS`
6. reinitialize when required

Representative mapping:

$$\frac{dJ}{d\phiLS} = -\frac{\partial J}{\partial xh}\,\delta_\epsilon(\phiLS)$$

This should be implemented in a way that naturally supports both objective and
constraint sensitivities, since the branch must still satisfy the same power and
volume requirements as the MMA branch.

### 5.8 Velocity regularization

The selected baseline is a reaction-diffusion or Helmholtz-type regularization
of the interface velocity:

$$-r_V^2 \nabla^2 V_n + V_n = V_{n,\mathrm{raw}}$$

or an equivalent reaction-diffusion pseudo-time solve.

This is preferred over direct unsmoothed advection because:

- it is closer to common LSM practice
- it replaces the design-space smoothing role previously played by MMA plus
  density filtering
- it is easier to debug than immediately introducing high-order advection

### 5.9 Level-set advection and reinitialization

Use:

$$\frac{\partial \phiLS}{\partial \tau} + V_n |\nabla \phiLS| = 0$$

with:

- first-order bounded advection for initial bring-up
- CFL-capped pseudo-time stepping
- periodic or quality-triggered reinitialization using a Sussman-style signed
  distance restoration:

$$\frac{\partial \phi}{\partial \tau} + \mathrm{sgn}_\epsilon(\phi_0)(|\nabla \phi|-1)=0$$

Reinitialization should be triggered:

- every `reinitInterval` optimization iterations, or
- immediately if `|grad(phiLS)|` diagnostics show significant signed-distance
  drift

### 5.10 Wall treatment

#### Preferred option

Use an interface-derived wall distance from the reinitialized level-set field on
the fluid side:

$$d_{LS} = \min(\max(\phiLS,0), d_{fixed})$$

combined with fixed-wall distance information where appropriate.

#### Fallback

If the direct interface distance is too noisy, solve an auxiliary PDE distance
field from the reconstructed interface each iteration.

#### Bring-up-only fallback

Allow `meshWave` wall distance as a diagnostic fallback only.

---

## 6. Objectives And Optimization Strategy

The LSM branch should use the same objective hierarchy as the MMA branch so the
two optimizers remain meaningfully comparable.

### 6.1 Bring-up objective

$$J = MeanT$$

### 6.2 Preferred production objective

$$J_{log} = \ln\left(\frac{MeanT}{T_{ref}}\right)$$

### 6.3 Preferred hotspot-aware objective

$$J_{KS} = T_{max}^\star + \frac{1}{\rho_{KS}}\ln\left(\sum_e \exp[\rho_{KS}(T_e-T_{max}^\star)]\right)$$

### 6.4 Alternative uniformity objective

$$J_\sigma = \frac{1}{|\Omega_h|}\int_{\Omega_h}(T-\bar{T}_h)^2\,dV$$

### 6.5 Constraints

The LSM branch must retain the same two primary optimizer constraints as the
working branch:

- hydraulic/power dissipation constraint
- fluid-volume constraint

The same weighted-constraint logic already used in the working solver should be
retained so both branches are judged under comparable operating conditions.

---

## 7. Runtime And Tuning Architecture

Every unstable or experimental feature must be runtime-selectable through
`constant/tuneOptParameters`, and the dictionary structure should closely mirror
the working MMA branch.

### 7.1 `optProperties`

`optProperties` should carry:

- physical constants and constraint scales
- shared continuation magnitudes
- LSM numerical magnitudes that are scalar control parameters

Required additions beyond the current LSM app:

- `qAlpha0`, `qAlphaMin`
- `qKappa0`, `qKappaMin`
- optional `qHeat0`, `qHeatMin`
- `qContinuationStartIter`
- `qContinuationInterval`
- `qContinuationFactor`
- `powerConstraintRelaxationRate`
- `continuationFeasibilityTol`
- `epsilonLSM`
- `lsmVelocityRadius`
- `lsmCFL`
- `maxNormalVelocity`
- `reinitInterval`
- `reinitPseudoSteps`
- `reinitTolerance`

### 7.2 Startup defaults inferred from the working branch

The LSM branch should inherit the same startup caution learned from the MMA
debugging cycle.

Recommended bring-up defaults:

- `qAlpha0 = 0.005`
- `qKappa0 = 0.005`
- if used, `qHeat0 = 0.005`
- `qContinuationStartIter` delayed well past the initial parity checks
- continuation disabled entirely during the first primal/adjoint validation
  cases if needed

Explicitly avoid:

- starting from `q=0.1`
- early aggressive hardening before the LSM path has matched the expected
  low-`q` hydraulic regime

### 7.3 Required `tuneOptParameters` blocks

The LSM branch should adopt the same high-level dictionary layout as the MMA
branch:

- `frameworkSwitches`
- `lsmSwitches`
- `objectiveSwitches`
- `continuationSwitches`
- `experimentControl`
- `adjointControl`
- `convergenceControl`

### 7.4 Required shared framework switches

| Key | Purpose |
|---|---|
| `useFrozenTurbulenceAdjoint` | Toggle FT adjoint assumption |
| `useKEpsilonModel` | Baseline turbulence model |
| `useWrayAgarwalFallback` | Optional fallback turbulence closure |
| `useBrinkmanSinkInKEpsilon` | Enable `k` and `epsilon` sinks |
| `useSplitRAMPControls` | Enable separate `qAlpha/qKappa/qHeat` |
| `useSingleQFallback` | One-parameter interpolation fallback for debugging |
| `useTurbulentThermalDiffusivity` | Toggle `nut/Prt` contribution |
| `usePowerConstraintRelaxation` | Reuse the working relaxed power-limit schedule |
| `useFullAdjointSymmetricStress` | Retain adjoint fidelity toggle |

### 7.5 Required LSM-specific switches

| Key | Purpose |
|---|---|
| `useReactionDiffusionLSMUpdate` | Preferred velocity regularization path |
| `usePureHamiltonJacobiFallback` | Simpler fallback without separate velocity solve |
| `useDensityAssistedLSMDebug` | Reconstruct density-style fields for debugging only |
| `useInterfaceDerivedWallDistance` | Preferred wall treatment |
| `useAuxiliaryPDEWallDistance` | Wall-distance fallback |
| `useMeshWaveWallDistanceFallback` | Bring-up-only fallback |
| `useSussmanReinitialization` | Baseline reinitialization path |
| `useWENO5ForPhiAdvection` | Later higher-order advection upgrade |
| `useTopologicalNucleation` | Later experimental feature, disabled initially |

### 7.6 Continuation and experiment-control policy

The LSM branch should reuse the same continuation-control philosophy already
validated in `turbulenceMMAOpt`.

That means carrying over:

- power-feasibility gating
- force-hardening floors
- late-stage strict gating
- late-stage refinement throttles
- lagging-gray-collapse throttles
- overactive-topology throttles

The names can stay identical to the MMA branch even though the underlying
design update is no longer MMA. In the LSM branch, these controls govern:

- interpolation hardening
- interface-band sharpening
- wall-distance hardening if applicable
- any late-stage reduction in allowable interface motion

### 7.7 Consistency rules

- exactly one turbulence closure path is active
- exactly one primary wall-distance path is active
- exactly one primary LSM update path is active
- at most one primary thermal objective is active
- `useDensityAssistedLSMDebug` is never treated as the production path
- logging is not optional during current development

---

## 8. Debug And Verification Instrumentation

The LSM branch must match the working branch's logging discipline from the
start.

### 8.1 Required log files

Always write:

- `debugOptimizer.log`
- `debugOptimizer.jsonl`
- `gradientOpt.log`
- `solverConvergences.log`
- `optimization.hst`

The current LSM app does not yet provide the full MMA-grade logging surface, so
porting this infrastructure is an early implementation task, not a late
refinement.

### 8.2 Startup option dump

At optimizer startup, `debugOptimizer.log` must dump all active runtime options
from:

- `constant/optProperties`
- `constant/tuneOptParameters`
- any active transport/thermal-property inputs relevant to the formulation

This should follow the working `turbulenceMMAOpt` style closely so run
reconstruction is easy across branches.

### 8.3 Per-iteration diagnostics that must match the MMA branch

Each iteration should still report:

- objective and active objective label
- power and volume constraint values
- weighted constraint values
- active power limit and relaxation state
- continuation gate state
- late-stage throttle state
- solver health status
- adjoint runaway status
- step sizes
- non-finite counts

### 8.4 LSM-specific diagnostics that must be added

Each iteration must additionally report:

- `qAlpha`, `qKappa`, and optional `qHeat`
- `epsilonLSM`
- interface-band volume fraction `|phiLS| <= epsilonLSM`
- min/max/L2/non-finite count of raw `dJ/dxh`
- min/max/L2/non-finite count of mapped `dJ/dphiLS`
- min/max/L2/non-finite count of raw and regularized `Vn`
- max absolute and L2 change of `phiLS`
- min/max of `|grad(phiLS)|`
- reinitialization trigger and residual indicators
- active wall-distance mode
- active LSM update mode

### 8.5 Acceptance checks for a healthy LSM run

Before any advanced features are enabled, a healthy LSM run should show:

- no non-finite counts in `phiLS`, `xh`, sensitivities, or velocities
- bounded solver residuals and no recurring solver warnings
- stable signed-distance restoration
- shrinking interface motion late in the run
- meaningful gray-band collapse in `xh`
- power and volume constraints moving toward the same feasible regime as the
  working branch

---

## 9. Development Order

The implementation order below is intentionally shaped by the current
repository, not by abstract LSM literature alone.

### Phase 0 - Port the proven optimizer framework first

Before introducing `phiLS`, port into `turbulenceLSMOpt`:

- the full `tuneOptParameters` structure used by the MMA branch
- runtime-option dumping
- `gradientOpt.log`
- solver convergence aggregation
- power-relaxation controls
- continuation gating and throttle controls
- adjoint-control switches and runaway protection

Acceptance target:

- `turbulenceLSMOpt` can emit the same class of debug evidence as
  `turbulenceMMAOpt` even before the design update is replaced

### Phase 1 - Reach turbulent physics parity on the shared `xh` path

Refactor the LSM branch so its turbulence physics options match the working MMA
branch as closely as practical:

- `kEpsilon`
- frozen-turbulence adjoint
- optional turbulent thermal diffusivity
- optional Brinkman sinks in `k` and `epsilon`
- split `qAlpha/qKappa/qHeat`

Acceptance target:

- the branch can run with the same soft-start interpolation strategy that was
  validated in MMA debugging

### Phase 2 - Introduce `phiLS` without changing the physics contract

Implement:

- `phiLS`
- `xh = H_epsilon(phiLS)`
- interface-band bookkeeping
- signed-distance quality diagnostics

At this stage, the primal/adjoint equations should still consume `xh` in the
same style as the working branch.

Acceptance target:

- the branch can run the turbulent physics using `xh` reconstructed from
  `phiLS`, even if update motion is still temporarily simplified

### Phase 3 - Replace the update law with true LSM motion

Implement in order:

1. raw `dJ/dxh` and constraint sensitivities
2. interface mapping to `dJ/dphiLS`
3. raw normal-velocity assembly
4. reaction-diffusion or Helmholtz regularization
5. Hamilton-Jacobi advection
6. reinitialization

Acceptance target:

- `MMAsolver` is no longer on the production path
- `phiLS` is the optimizer-owned state

### Phase 4 - Implement interface-aware wall treatment

Implement:

- interface-derived wall distance from `phiLS`
- auxiliary PDE fallback
- debug-only `meshWave` fallback

Acceptance target:

- wall-distance diagnostics remain stable over optimization iterations and the
  turbulence model stays well behaved near evolving internal walls

### Phase 5 - Tune the branch using the MMA debug philosophy

Only after the LSM path is numerically healthy should the branch enable:

- continuation schedules beyond fixed low-`q`
- late-stage refinement profiles
- log-mean and KS/variance objectives
- higher-order advection
- topological nucleation experiments

Acceptance target:

- the branch shows the same broad health signals as the working optimizer:
  stable startup, controlled mid-run redesign, and late-stage sharpening

### Phase 6 - Remove obsolete production dependencies

Once the LSM production path is verified, remove or demote to debug-only:

- mandatory dependence on `MMA`
- mandatory dependence on `filter_x.H`
- mandatory dependence on `filter_chainrule.H`
- any code that still treats `x` as the production design variable

Acceptance target:

- the branch is cleanly identifiable as an LSM optimizer rather than a renamed
  density optimizer

---

## 10. Target File Map

| File | Status | Role |
|---|---|---|
| `turbulenceLSMOpt/src/turbulenceLSMOpt.C` | refactor | Main optimization loop; replace production MMA update path with `phiLS` evolution |
| `turbulenceLSMOpt/src/createFields.H` | refactor | Add full tuning-switch reads and LSM fields |
| `turbulenceLSMOpt/src/readTransportProperties.H` | refactor | Split interpolation setup and wall-distance mode setup |
| `turbulenceLSMOpt/src/readThermalProperties.H` | refactor | Split thermal interpolation and objective-source setup |
| `turbulenceLSMOpt/src/Primal_U.H` | reuse/refactor | Turbulent primal momentum with Brinkman term |
| `turbulenceLSMOpt/src/Primal_T.H` | reuse/refactor | Energy equation with optional turbulent diffusion |
| `turbulenceLSMOpt/src/AdjointFlow_Ua.H` | reuse/refactor | Power adjoint |
| `turbulenceLSMOpt/src/AdjointHeat_Ub.H` | reuse/refactor | Thermal adjoint momentum |
| `turbulenceLSMOpt/src/AdjointHeat_Tb.H` | reuse/refactor | Thermal adjoint scalar |
| `turbulenceLSMOpt/src/lsmInterfaceReconstruct.H` | new | `xh = H(phiLS)` and interface-band reconstruction |
| `turbulenceLSMOpt/src/lsmWallDistance.H` | new | Interface-derived wall distance and fallbacks |
| `turbulenceLSMOpt/src/lsmSensitivity.H` | new | Map `dJ/dxh` and constraint gradients to the interface |
| `turbulenceLSMOpt/src/lsmVelocityRegularization.H` | new | Reaction-diffusion or Helmholtz regularization of `Vn` |
| `turbulenceLSMOpt/src/lsmAdvection.H` | new | Hamilton-Jacobi update of `phiLS` |
| `turbulenceLSMOpt/src/lsmReinitialize.H` | new | Signed-distance restoration and diagnostics |
| `turbulenceLSMOpt/src/update.H` | rewrite | Drive continuation and LSM-control updates, not MMA motion |
| `turbulenceLSMOpt/src/debugOptimizer.H` | refactor | Mirror MMA-grade diagnostics plus LSM-specific health fields |
| `turbulenceLSMOpt/src/gradientOptWrite.H` | new | Dedicated gradient, interface, and velocity logging |
| `turbulenceLSMOpt/src/filter_x.H` | retire/debug-only | No longer production design update |
| `turbulenceLSMOpt/src/filter_chainrule.H` | retire/debug-only | No longer production sensitivity path |
| `turbulenceLSMOpt/src/MMA/` | retire/debug-only | Debug fallback only once LSM path is working |
| `turbulenceLSMOpt/app/constant/optProperties` | refactor | Add shared continuation controls and LSM scalar controls |
| `turbulenceLSMOpt/app/constant/tuneOptParameters` | refactor | Mirror MMA dictionary structure plus `lsmSwitches` |

---

## 11. Final Position

For the intended extruded-cold-plate workflow, `turbulenceLSMOpt` should be
developed as an independent geometry-focused alternative to the working MMA
optimizer, but **not** as an unrelated parallel framework.

The correct baseline is:

- quasi-2D only
- `phiLS` as the design state
- `xh = H_epsilon(phiLS)` as the shared physics field
- turbulent primal/adjoint and runtime-control infrastructure borrowed from the
  working MMA branch
- low-`q` soft startup inherited from the completed MMA debugging evidence
- interface-native motion regularization, advection, and reinitialization
- always-on debug instrumentation

This makes the branch both:

- SOTA-aligned as a real LSM optimizer
- realistically developable inside the current codebase because it stands on
  the same turbulence, tuning, and debugging foundation that already produced a
  working `turbulenceMMAOpt`
