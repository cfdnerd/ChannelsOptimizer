# PlanTurbulenceOpt-LSM: Hybrid Turbulent MMA-to-LSM Channel Optimizer

**Target folder:** `turbulenceLSMOpt/`  
**Method:** one continuous quasi-2D optimization run with a staged design update  
**Stage 1:** density-based `x -> xp -> xh` topology discovery using MMA  
**Stage 2 baseline:** signed-distance level-set refinement with reconstruction back to `xh` for controlled geometry updates  
**Later milestone:** explicit-wall sharp-interface turbulent LSM if the baseline Stage 2 proves stable and useful  
**Primary LSM role:** control channel width, rib thickness, and regional geometry after topology has already been discovered  
**Target product:** planar channel topology for later extrusion into a prescribed cold-plate thickness

See also:

- [PlanTurbulenceOpt-MMA.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/PlanTurbulenceOpt-MMA.md)
- [TurbulenceMMAOptDebugFindings.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/TurbulenceMMAOptDebugFindings.md)

---

## 1. Development Position

This document **completely supersedes** the earlier idea of building `turbulenceLSMOpt/` as an independent level-set topology optimizer that explores topology from scratch.

That is no longer the intended architecture.

The correct development position is:

1. `turbulenceLSMOpt/` starts from the current `turbulenceMMAOpt/` baseline, exactly as already done.
2. The density/MMA machinery remains the **primary topology-exploration engine**.
3. LSM is added as a **secondary in-run refinement stage**, not as a separate optimizer and not as a fresh topology generator.
4. The first LSM implementation is a **level-set-controlled diffuse-interface refinement stage** that reuses the current primal, adjoint, interpolation, and logging path.
5. A true explicit-wall sharp-interface turbulent LSM formulation is a **later milestone**, not the first delivery target.

This directly matches the archived 2026 debugging context summarized in [TurbulenceMMAOptDebugFindings.md](/home/tomathew/work/jobs/chaos/wDir/ChannelsOptimizer/docs/TurbulenceMMAOptDebugFindings.md):

- early topology evolution is highly sensitive to continuation and interpolation startup
- the density stage must remain soft and robust during discovery
- LSM must **not** be used as a workaround for a poorly conditioned density startup

The practical intent is therefore:

- **MMA discovers where channels should exist**
- **LSM tunes what those channels should look like**

---

## 2. Core Decision

### 2.1 Selected baseline

The selected baseline for `turbulenceLSMOpt/` is:

1. Run the same turbulent primal and adjoint framework already used in the MMA branch.
2. Use cell-wise density design variables and MMA until the topology becomes connected, feasible, and sufficiently low-gray.
3. Extract a signed-distance level-set field from the mature density topology.
4. Freeze large topological changes by default.
5. Continue the **same optimization run** with an LSM-controlled diffuse-interface refinement stage that keeps the same global objective family and constraints.
6. Treat true explicit-wall LSM physics as a later extension after the first Stage 2 has been validated.

### 2.2 What LSM is for in this branch

In this branch, LSM is **primarily a geometry controller**, not a topology explorer.

Its first responsibilities are:

- minimum channel width control
- maximum channel width control where needed
- minimum rib or wall thickness control
- maximum rib thickness control where useful
- global size tuning
- regionwise size tuning
- curvature and shape smoothing
- cleaner geometry reporting from an explicit signed-distance field

Branchwise graph-driven controls are useful later, but they are **not** part of the first implementation baseline.

### 2.3 What LSM is not for in this branch

By default, the LSM stage is **not** intended to:

- create brand-new disconnected channels
- seed holes from a uniform starting domain
- replace the density stage
- become a separate standalone solver workflow
- require XFEM, CutFEM, remeshing, or full body-fitted 3D geometry handling before the branch becomes useful
- treat branchwise skeleton graphs as a day-one dependency

### 2.4 Honest formulation label

The first Stage 2 in this branch should be described honestly as:

- a **level-set-parameterized diffuse-interface refinement stage**

It should **not** be presented as already equivalent to the strongest sharp-interface turbulent level-set frameworks in the literature, which generally rely on explicit wall imposition through IBM, XFEM, cut-cell, or equivalent interface treatments.

---

## 3. Why This Hybrid Direction Is the Right One

The literature is consistent on the broad tradeoff:

- density methods remain the most practical route for early topology discovery in fluid and thermal-fluid topology optimization, especially when porous Brinkman interpolation and robust continuation are needed [1, 2, 3]
- level-set methods become especially valuable once geometry control, wall definition, and feature-size management matter directly [4, 5, 6, 7]
- level-set methods remain sensitive to initialization and hole-seeding strategy if they are forced to discover topology from scratch [5, 8, 9]

That makes the hybrid staging natural for this project:

- **Stage 1** uses density because it tolerates unknown topology and gray transitions better.
- **Stage 2** uses level sets because channel size, spacing, and shape are easier to control once the topology is already meaningful.

The important nuance is:

- the strongest turbulent level-set references use explicit interface treatment and explicit wall handling [4, 5]
- this branch will **not** start there
- instead, the first implementation will reuse the current `xh`-based solver path and add level-set control on top of it

That is still literature-aligned as a hybrid density/level-set development direction [6, 7, 8], and it is a much better fit to the current code reality:

- `turbulenceLSMOpt/` is now copied from `turbulenceMMAOpt/`
- the current primal, adjoint, cost, logging, and continuation framework already exist
- the present code already computes sensitivities with respect to `xh`
- adding a conservative LSM stage on top of that baseline is lower risk than building a fresh sharp-interface stack from zero

---

## 4. Selected Solver Architecture

### 4.1 Stage A - Density/MMA topology discovery

The first stage is the existing density pipeline:

$$x \rightarrow xp \rightarrow xh$$

with:

- Helmholtz filtering
- Heaviside projection
- Brinkman-penalized turbulent RANS primal
- frozen-turbulence adjoint as the initial baseline
- MMA update on the cell-wise design field

This stage keeps the current strengths of the copied solver:

- robust continuation machinery
- established debug logs
- power and volume constraints already wired into the optimization loop
- the current `k-epsilon` infrastructure

### 4.2 Stage B - Controlled handover from density to level set

The handover is **not** iteration-only and **must not** happen too early.

At handover:

1. checkpoint the full optimizer state
2. extract the `xh = 0.5` contour
3. initialize a signed-distance field `phi`
4. derive compatibility fields from `phi`
5. blend density- and level-set-based physical indicators over a short transition window

The transition should be smooth enough that the primal and adjoint equations do not see a violent physics jump.

The plan assumes that restartable checkpoint and rollback support are a prerequisite for this handover, not a luxury feature.

### 4.3 Stage C - Level-set-controlled diffuse-interface refinement

After handover, the level-set field becomes the master geometry description:

$$
\phi > 0 \; \text{fluid}, \qquad
\phi < 0 \; \text{solid}, \qquad
\phi = 0 \; \Gamma
$$

In the selected first implementation:

- `phi` is the **master geometry field**
- `xh_phi = H_\epsilon(\phi)` is the **derived physical indicator**
- `x`, `xp`, and `xh` remain compatibility carriers for logging, restart simplicity, and minimum code disruption

The primal, adjoint, and logging path continue to use reconstructed `xh_phi` through the existing interpolation route.

This first Stage 2 is therefore:

- level-set-controlled
- geometry-explicit at the parameterization level
- still diffuse-interface in the primal and adjoint physics path

That compromise is deliberate because it keeps the code risk acceptable for the current OpenFOAM-6 baseline.

### 4.4 Later Stage D - explicit-wall sharp-interface refinement

Only after Stage C is working and validated should the branch consider a sharper physics treatment such as:

- immersed-boundary wall imposition
- XFEM / cut-cell / weak boundary enforcement
- a fully `phi`-consistent wall-distance treatment
- more consistent Stage-2 shape sensitivities near the moving wall

This later stage is worth keeping in view, but it is **not** the selected first delivery target.

### 4.5 Rollback and recovery

The handover must always support rollback.

If the LSM stage:

- breaks feasibility badly
- destroys connectivity
- creates unphysical width collapse
- shows adjoint sign failure or runaway behavior

then the run must revert to the density checkpoint and either:

- retry with gentler LSM controls, or
- continue with density-only optimization

This rollback is part of the selected baseline, but it depends on real checkpointable optimizer state being implemented first.

---

## 5. Stage-Switch Criteria

The switch from density to LSM should be governed by the existing optimizer diagnostics, not by a hardcoded iteration count alone.

### 5.1 Selected default switch gate

All of the following should be satisfied:

1. `grayVolumeFraction <= 0.15 - 0.25`
2. `powerFeasibilityRatio <= 1.02 - 1.05`
3. `previousXhStepMax <= 0.05 - 0.10`
4. `beta >= 10 - 15`
5. no adjoint runaway and no solver health warning
6. a connected inlet-to-outlet fluid path exists in the thresholded design
7. the density stage has already run for a minimum maturity window, typically at least `80 - 150` iterations depending on the case
8. checkpoint/restart support for rollback is available

### 5.2 Important interpretation

The switch criterion is intentionally conservative.

The LSM stage should begin only after:

- the optimizer has found the main branches
- the power constraint is close to feasible
- the topology is moving less like exploration and more like refinement

That is exactly the point where LSM becomes valuable.

### 5.3 Blended handover

The selected baseline includes `3 - 10` blending iterations:

$$
xh_{\text{blend}} = \omega \, xh_{\text{density}} + (1-\omega)\,H_\epsilon(\phi)
$$

with `omega` decaying from `1` to `0`.

This reduces the risk of a sudden step in:

- Brinkman resistance
- thermal interpolation
- wall distance
- adjoint sensitivities

In the first implementation, this blend is especially important because the solver remains diffuse-interface in the physical interpolation path even after `phi` becomes the geometry master.

---

## 6. Governing Formulation

### 6.1 Stage 1 physics

The density stage keeps the current turbulent MMA formulation:

$$
({\bf U}\cdot\nabla){\bf U}
= -\nabla p
+ \nabla \cdot \left[ (\nu+\nu_t)(\nabla {\bf U}+(\nabla {\bf U})^T) \right]
- \alpha(xh)\,{\bf U}
$$

with:

$$
\alpha(xh)=\alpha_{max}\frac{q_\alpha(1-xh)}{q_\alpha+xh}
$$

and thermal transport:

$$
D_T(xh)=\frac{k_s+(k_f-k_s)\,xh\,(1+q_\kappa)/(q_\kappa+xh)}{\rho c_p}
$$

The current split-`q` continuation logic remains the correct baseline for this branch as well.

### 6.2 Level-set master field after handover

After the switch:

$$
xh_\phi = H_\epsilon(\phi)
$$

where `H_\epsilon` is a smooth Heaviside over a narrow band.

Then the existing interpolation path becomes:

$$
\phi \rightarrow xh_\phi \rightarrow \alpha(xh_\phi), \; D_T(xh_\phi)
$$

This is the selected **first** Stage-2 formulation:

- level-set parameterization
- reconstructed diffuse-interface physics
- minimum disruption to the copied MMA solver

### 6.3 Compatibility-field rule

During the LSM stage:

- `phi` is the **master** geometry field
- `xh` is a **derived** physical indicator
- `x` and `xp` are compatibility fields kept for logging, restart simplicity, and minimum code disruption

The LSM stage must **not** keep competing masters for geometry.

### 6.4 Selected Stage-2 sensitivity bridge

This is the most important formulation choice for the first LSM implementation.

The current solver already computes volumetric sensitivities with respect to the physical indicator field. It does **not** yet compute a full sharp-interface shape derivative on `\phi = 0`.

Therefore the selected first baseline is:

- reuse the existing adjoint and sensitivity path to obtain cell-wise sensitivities with respect to `xh_\phi`
- localize those sensitivities to a narrow band around the zero contour
- convert the localized signal into a regularized interface normal velocity

Let:

$$
s_J = \frac{\partial J}{\partial xh_\phi}, \qquad
s_P = \frac{\partial g_P}{\partial xh_\phi}, \qquad
s_V = \frac{\partial g_V}{\partial xh_\phi}
$$

denote the objective and constraint sensitivities obtained from the existing Stage-1 adjoint machinery after reconstruction from `\phi`.

For the first implementation, form narrow-band interface-driving signals:

$$
\tilde{s}_\bullet = \mathcal{R}\!\left[\delta_\epsilon(\phi)\,s_\bullet\right]
$$

where:

- `\delta_\epsilon(\phi)` is a smooth narrow-band delta concentrated near `\phi = 0`
- `\mathcal{R}` denotes smoothing and normalization in the narrow band

Then the first-stage interface-driving velocities are:

$$
V_J^{(0)} = -\tilde{s}_J, \qquad
V_P^{(0)} = -\tilde{s}_P, \qquad
V_V^{(0)} = -\tilde{s}_V
$$

with the final sign convention to be confirmed by finite-difference tests.

This is intentionally a **pragmatic narrow-band sensitivity bridge**, not a claim of a fully consistent sharp-interface shape derivative.

### 6.5 Objective and global constraints

The LSM stage keeps the same global objective family as the MMA branch:

- `MeanT`
- `log(MeanT/Tref)`
- KS hotspot objective
- temperature variance objective

The primary global constraints remain:

$$
g_P = \frac{PowerDiss}{PowerLimit} - 1 \le 0
$$

$$
g_V = V(\phi) - V_{max} \le 0
$$

where `V(\phi)` is evaluated from `xh_\phi`.

---

## 7. LSM Geometry-Control Formulation

This section is the main reason the branch exists.

### 7.1 Primary size quantities

For the quasi-2D extruded channel design, the preferred primary quantity is the **local channel width**.

Let `S_f` denote a fluid centerline or equivalent fluid distance-support set. Then the local half-width is:

$$
r_f(s) = \operatorname{dist}(S_f, \Gamma)
$$

and the local channel width is:

$$
w(s)=2r_f(s)
$$

For the solid side, the local rib thickness is:

$$
t_{rib}(s)=2\,\operatorname{dist}(S_s,\Gamma)
$$

If the extrusion thickness `h_ext` is fixed, the local hydraulic diameter can also be reported as:

$$
D_h(s)=\frac{2\,w(s)\,h_{ext}}{w(s)+h_{ext}}
$$

but **width control should remain the primary implemented control**, because it is more direct geometrically.

### 7.2 Selected geometric constraints

The selected initial LSM constraints are:

1. minimum channel width
2. maximum channel width
3. minimum rib thickness
4. optional maximum rib thickness in protected interior regions
5. optional curvature cap

The preferred aggregated forms are KS-type inequalities:

$$
g_{w,\min} = KS_s\left(\frac{w_{\min}(s)-w(s)}{w_{ref}}\right) \le 0
$$

$$
g_{w,\max} = KS_s\left(\frac{w(s)-w_{\max}(s)}{w_{ref}}\right) \le 0
$$

$$
g_{r,\min} = KS_s\left(\frac{t_{rib,\min}(s)-t_{rib}(s)}{t_{ref}}\right) \le 0
$$

$$
g_{r,\max} = KS_s\left(\frac{t_{rib}(s)-t_{rib,\max}(s)}{t_{ref}}\right) \le 0
$$

$$
g_{\kappa} = KS_\Gamma\left(\frac{|\kappa|-\kappa_{\max}}{\kappa_{ref}}\right) \le 0
$$

This is preferred over simple perimeter-only control because the user requirement is **size control**, not merely smoothness.

### 7.3 How the size control acts

The size-control velocity must be local and directional:

- if a channel branch is too narrow, the interface should move outward locally
- if a channel branch is too wide, the interface should move inward locally
- if a rib is too thin, the neighboring fluid interface should retreat locally
- if a rib is too thick, the neighboring fluid interface should advance locally

The important implementation rule is:

- geometry constraints act on `phi = 0` through normal motion
- they do not modify the global objective definition
- they are enforced alongside the thermal and hydraulic constraints

### 7.4 Selected first baseline for extracting width and rib metrics

The selected **first** baseline is:

1. threshold `xh_\phi` to fluid and solid masks
2. compute fluid-side and solid-side distance fields
3. evaluate width and rib-thickness violations from those distance fields
4. start with global and regionwise aggregates

This approach is intentionally simpler than full skeleton-graph control and is a better fit for the first OpenFOAM implementation.

### 7.5 Specialized target modes

The geometry targets should support three levels of specificity:

1. global scalar targets
2. regionwise targets
3. branchwise targets

#### Global scalar targets

These are the simplest manufacturing controls:

- one `channelWidthMin`
- one `channelWidthMax`
- one `ribThicknessMin`
- one `ribThicknessMax`

They are the correct first implementation baseline.

#### Regionwise targets

Regionwise targets allow different size rules in different zones of the design domain, for example:

- inlet manifold region
- heated core region
- outlet collector region
- peripheral bypass region

In notation, this simply means the size limits become piecewise fields:

$$
w_{\min}(s) \rightarrow w_{\min}^{(m)} , \qquad
w_{\max}(s) \rightarrow w_{\max}^{(m)}
$$

$$
t_{rib,\min}(s) \rightarrow t_{rib,\min}^{(m)} , \qquad
t_{rib,\max}(s) \rightarrow t_{rib,\max}^{(m)}
$$

for region index `m`.

Regionwise targets are the preferred **second** implementation tier after global controls are stable.

#### Branchwise targets

Branchwise targets remain attractive, but they are a **later milestone**.

They become useful only once the channel network has a stable skeleton graph and reliable branch identification.

The intended later workflow is:

1. extract the fluid and solid skeletons
2. convert the skeleton into a graph
3. identify branch segments between junctions and boundaries
4. assign each branch a `branchId`
5. apply size targets per branch

Then the limits become:

$$
w_{\min}(s) \rightarrow w_{\min}^{(b)} , \qquad
w_{\max}(s) \rightarrow w_{\max}^{(b)}
$$

$$
t_{rib,\min}(s) \rightarrow t_{rib,\min}^{(b)} , \qquad
t_{rib,\max}(s) \rightarrow t_{rib,\max}^{(b)}
$$

for branch index `b`.

Branchwise targets should **not** block the first LSM implementation.

#### Protected-region logic

Maximum-rib-thickness control should normally support protected masks so it does not incorrectly erode:

- boundary-attached walls
- non-design solid anchors
- inlet and outlet support ligaments
- any region reserved for structural or sealing purposes

The plan therefore assumes selective application masks for advanced rib-thickness control, especially for `ribThicknessMax`.

### Alternative A: thickness PDE

If direct distance-field metrics are noisy on the mesh, use an auxiliary thickness field or heat-method-like distance solve.

### Alternative B: skeleton and branch graph later

If pointwise centerline logic is too heavy for the first implementation, keep global and regionwise aggregates first and promote skeleton/graph logic only after the simpler controls are validated.

---

## 8. Level-Set Update Strategy

### 8.1 Selected baseline

The selected baseline for the first OpenFOAM implementation is:

1. reconstruct `xh_\phi` from `\phi`
2. solve the same primal and adjoint framework already used in the MMA branch
3. compute narrow-band objective and global-constraint signals from the existing `xh` sensitivities
4. add explicit geometry-control velocities
5. regularize the total normal velocity
6. advect the level set with a Hamilton-Jacobi update
7. periodically reinitialize `phi` as a signed-distance field

The update equation is:

$$
\frac{\partial \phi}{\partial \tau} + \tilde{V}_n |\nabla \phi| = 0
$$

where `\tilde{V}_n` is the regularized normal velocity.

### 8.2 Velocity regularization

The selected initial regularization is a Helmholtz-type smoothing of the normal velocity:

$$
\left(I-\ell_v^2 \nabla^2\right)\tilde{V}_n = V_n
$$

This is preferred because it is:

- easy to integrate into the current finite-volume framework
- easy to debug
- compatible with the existing solver architecture

### 8.3 Curvature control

Curvature suppression should be present from the first LSM implementation:

$$
V_\kappa \propto -\kappa
$$

with:

$$
\kappa = \nabla \cdot \left(\frac{\nabla \phi}{|\nabla \phi|}\right)
$$

This is important because the goal is not only better objective value but also **manufacturable and size-controlled channels**.

### 8.4 Reinitialization

The selected baseline is periodic reinitialization using the standard pseudo-time signed-distance restoration:

$$
\frac{\partial \phi}{\partial \tau_r}
+ \operatorname{sign}(\phi_0)\left(|\nabla \phi|-1\right)=0
$$

Reinitialization should be triggered:

- every few LSM iterations, or
- when the signed-distance drift exceeds a threshold

### 8.5 Yamada-inspired complexity control

Yamada's fictitious-interface-energy and reaction-diffusion direction [10] is highly relevant for this branch.

For this codebase:

- the selected first baseline is still HJ update + velocity smoothing + curvature control because it is easier to integrate into the present solver
- reaction-diffusion should remain a **first-class alternative path**, not a decorative afterthought

That means:

- baseline: HJ update + velocity smoothing + curvature control
- alternative primary route: reaction-diffusion or fictitious-interface-energy regularization for stronger complexity management

This gives a better risk profile for the current codebase while staying honest about the literature.

---

## 9. Turbulence and Wall Treatment by Stage

### 9.1 Density stage

The density stage should stay aligned with the MMA branch:

- `k-epsilon` remains the production turbulence model
- frozen-turbulence adjoint remains the initial baseline
- porosity-aware wall distance remains the initial wall-distance method

### 9.2 Stage-2 first implementation baseline

Although `\phi` explicitly describes geometry, the selected first Stage-2 solver still uses reconstructed `xh_\phi` in the diffuse-interface primal and adjoint path.

Therefore the safest wall-distance baseline for the **first** Stage 2 is:

- keep the porosity-aware wall-distance treatment as the default
- optionally blend toward `\phi`-based distance only for controlled diagnostics and validation

This is lower risk because it keeps the wall treatment consistent with the physics path actually being solved.

### 9.3 Promotion path for `\phi`-based wall distance

`phi`-based wall distance should become the default only after:

1. Stage-2 motion is validated against finite differences
2. wall-distance behavior near moving internal walls is shown to be stable
3. the near-wall formulation is sufficiently consistent with the chosen Stage-2 physics

At that point the promoted wall-distance candidate can be:

$$
d_{LS} = \max(\phi,0)
$$

or a heat-method / Poisson auxiliary distance solve if that proves more robust numerically.

### 9.4 Fallback wall-distance options

If direct `\phi`-based wall distance is noisy:

1. use a Poisson or heat-method auxiliary distance solve from the zero contour
2. keep the old porosity-aware wall-distance solve as a debug fallback

### 9.5 Adjoint stance

The literature is clear that frozen-turbulence assumptions can be inaccurate for turbulent optimization [1, 11, 12, 13].

For this branch, the selected practical stance is:

- keep frozen turbulence as the first implementation baseline
- make finite-difference validation of Stage-2 interface motion **mandatory**
- only then decide whether Stage 2 needs partial or fuller turbulence-adjoint corrections

This is the correct reliability-first order for the current OpenFOAM-6 codebase.

---

## 10. Topology-Freezing Policy in the LSM Stage

The default policy for Stage 2 is:

- preserve topology
- preserve inlet-outlet connectivity
- do not intentionally nucleate new channels
- reject interface steps that trigger accidental branch pinch-off or merger

This is essential because the branch objective is **shape and size refinement**.

### Experimental exception

An experimental toggle may later allow limited topology change in the LSM stage, but this must remain off by default:

- `allowTopologyChangeInLSM = false`

---

## 11. Runtime Control Philosophy

All major LSM decisions must be exposed in dictionaries, not hardcoded.

### 11.1 New dictionary groups to add

### `hybridStageControl`

- `useHybridMMAtoLSM`
- `lsmSwitchGrayFraction`
- `lsmSwitchPowerFeasibility`
- `lsmSwitchMinBeta`
- `lsmSwitchMinIterations`
- `lsmSwitchRequireConnectivity`
- `lsmBlendIterations`
- `lsmRollbackEnabled`
- `requireRestartableLSMCheckpoint`
- `allowTopologyChangeInLSM`

### `lsmControl`

- `useLevelSetStage`
- `phiBandWidthCells`
- `narrowBandHalfWidthCells`
- `phiReinitInterval`
- `phiGradDriftTol`
- `lsmPseudoDt`
- `lsmCfl`
- `useVelocityHelmholtz`
- `velocityFilterRadius`
- `useCurvaturePenalty`
- `curvaturePenaltyWeight`
- `useReactionDiffusionComplexityControl`

### `lsmSensitivityControl`

- `useApproximateNarrowBandSensitivityBridge`
- `lsmSensitivitySmoothingRadius`
- `normalizeInterfaceVelocity`
- `finiteDifferenceCheckStage2`

### `geometryControl`

- `useChannelWidthControl`
- `channelWidthMin`
- `channelWidthMax`
- `useRegionwiseWidthTargets`
- `useBranchwiseWidthTargets`
- `useRibThicknessControl`
- `ribThicknessMin`
- `useRibThicknessMaxControl`
- `ribThicknessMax`
- `useRegionwiseRibTargets`
- `useBranchwiseRibTargets`
- `useProtectedGeometryMasks`
- `applyRibTargetsToInternalRibsOnly`
- `branchwiseChannelWidthMin`
- `branchwiseChannelWidthMax`
- `branchwiseRibThicknessMin`
- `branchwiseRibThicknessMax`
- `regionwiseChannelWidthMin`
- `regionwiseChannelWidthMax`
- `regionwiseRibThicknessMin`
- `regionwiseRibThicknessMax`
- `useHydraulicDiameterReporting`
- `ksRhoGeom`
- `sizeConstraintWeightMin`
- `sizeConstraintWeightMax`
- `widthConstraintWeight`
- `ribConstraintWeight`
- `protectedSolidRegionNames`
- `protectedFluidRegionNames`

Branchwise geometry keys may remain inactive until the later milestone that adds reliable skeleton/graph extraction.

### `wallDistanceControl`

- `lsmWallDistanceMode`
- `useLSMWallDistance`
- `useHeatMethodWallDistanceFallback`
- `lsmWallDistanceBlendIterations`

---

## 12. Repo-Level Implementation Plan

This plan is intentionally aligned with the copied MMA baseline now present in `turbulenceLSMOpt/`.

### Phase A - stabilize the copied baseline

Goal:

- make `turbulenceLSMOpt/` build and run as a clean copy of the current turbulent MMA optimizer before adding LSM logic

Actions:

- keep the current primal, adjoint, objective, continuation, and logging behavior intact
- restore naming consistency where needed so the branch is clearly `turbulenceLSMOpt`
- ensure the existing logs remain valid references before the stage split is introduced

### Phase B - add stage-switch and checkpoint prerequisites

Goal:

- support one continuous run with a controlled handover and real rollback capability

Actions:

- add stage-state variables such as `densityStage`, `handoverStage`, `lsmStage`
- add checkpoint, restart, and rollback support for optimizer state
- add switch gates using existing diagnostics such as gray fraction, `xhStepMax`, feasibility ratio, and solver health

### Phase C - add level-set fields and reconstruction

Goal:

- create the minimum `phi` infrastructure without breaking the existing solver

Actions:

- create `phi`
- initialize `phi` from the `xh = 0.5` contour
- reconstruct `xh_\phi`
- derive compatibility fields used by the existing interpolation and output logic
- implement the blended handover window

### Phase D - add first geometry metrics and controls

Goal:

- make channel width and rib thickness measurable and controllable with the lightest viable implementation

Actions:

- threshold the fluid and solid masks
- compute distance fields
- compute global width and rib metrics
- add regionwise metrics next
- write these metrics into `gradientOpt.log` and `debugOptimizer.log`
- verify mask-aware application of `ribThicknessMax`

### Phase E - add the Stage-2 sensitivity bridge and update kernel

Goal:

- move the interface in a constrained and reversible way using the current adjoint outputs

Actions:

- compute narrow-band objective and constraint signals from the existing `xh` sensitivities
- add geometry-control velocities
- regularize the velocity
- advect `phi`
- reinitialize
- keep porosity-aware wall distance as the default early Stage-2 wall treatment

### Phase F - validate the first LSM stage

Goal:

- make the branch trustworthy before extending scope

Actions:

- finite-difference check of Stage-2 interface sensitivities
- narrow-channel widening test
- over-wide branch contraction test
- rib-protection test
- switch-and-rollback test
- compare density-stage final design vs. LSM-refined design on the same base case

### Phase G - later sharp-interface and branchwise extensions

Goal:

- extend the branch only after the first Stage-2 formulation is demonstrably stable

Actions:

- add branchwise graph logic only after regionwise controls work
- promote `\phi`-based wall distance only after consistency checks pass
- explore sharper wall/interface treatments only after the diffuse-interface LSM stage is validated

---

## 13. Recommended File Map

The branch should evolve from the current MMA copy with the following additions.

| File | Responsibility |
|---|---|
| `turbulenceLSMOpt/src/createFields.H` | new stage-control, LSM-control, sensitivity-bridge, and geometry-control dictionary entries |
| `turbulenceLSMOpt/src/opt_initialization.H` | initialize stage state, checkpoint metadata, and new logs |
| `turbulenceLSMOpt/src/update.H` | stage gate, handover logic, and rollback decision |
| `turbulenceLSMOpt/src/levelSetCreateFields.H` | create `phi`, derived masks, and interface fields |
| `turbulenceLSMOpt/src/initializeLevelSetFromDensity.H` | build signed-distance `phi` from mature `xh` |
| `turbulenceLSMOpt/src/updateDerivedDensityFromLevelSet.H` | reconstruct `xh` and compatibility fields from `phi` |
| `turbulenceLSMOpt/src/lsmGeometryMetrics.H` | width, rib-thickness, distance-field, and optional hydraulic-diameter metrics |
| `turbulenceLSMOpt/src/lsmBranchGraph.H` | optional later milestone for branch extraction and branch IDs |
| `turbulenceLSMOpt/src/lsmSensitivity.H` | narrow-band mapping from volumetric sensitivities to Stage-2 interface velocity; later hooks for sharper shape derivatives |
| `turbulenceLSMOpt/src/lsmVelocityRegularization.H` | Helmholtz or reaction-diffusion smoothing of `V_n` |
| `turbulenceLSMOpt/src/updateLevelSet.H` | Hamilton-Jacobi update and reinitialization |
| `turbulenceLSMOpt/src/levelSetWallDistance.H` | blended wall-distance logic and later `phi`-based wall-distance modes |
| `turbulenceLSMOpt/src/gradientOptWrite.H` | write width, rib, curvature, stage, and sensitivity-bridge diagnostics |
| `turbulenceLSMOpt/src/debugOptimizer.H` | report stage state, switch gates, and geometry-constraint health |
| `turbulenceLSMOpt/src/turbulenceLSMOpt.C` | top-level loop with stage-specific includes |

---

## 14. Verification Ladder

The LSM stage should not be trusted until it passes the following ladder.

### 14.1 Geometric unit checks

1. straight channel initialized too narrow expands toward `channelWidthMin`
2. straight channel initialized too wide contracts toward `channelWidthMax`
3. thin separating rib grows away from `ribThicknessMin` violation
4. over-thick rib contracts toward `ribThicknessMax` when that control is enabled
5. width and rib metrics remain stable under mesh refinement

### 14.2 Physics consistency checks

1. `xh(phi)` reproduces the handover design with small error
2. power and temperature fields change smoothly through the blend window
3. porosity-aware wall distance remains stable through early Stage 2
4. `phi`-based wall distance behaves consistently near moving internal walls when explicitly enabled

### 14.3 Sensitivity checks

1. compare the narrow-band Stage-2 motion direction against finite differences on simple 2D branches
2. verify sign of width-control response for inward and outward motions
3. verify sign of rib-thickness response for both `ribThicknessMin` and `ribThicknessMax`
4. confirm objective and power sensitivities remain finite in the narrow band

### 14.4 Run-level checks

1. handover does not break the run
2. rollback restores the last density checkpoint
3. LSM stage improves geometry metrics without unacceptable objective loss
4. regionwise targets are honored when enabled
5. connectivity is preserved unless explicitly allowed otherwise
6. branchwise targets remain a later extension until their extraction pipeline is proven reliable

---

## 15. Selected Reference Map

These are the references that most directly justify the plan.

| Ref | Why it matters for this branch |
|---|---|
| [1] Alexandersen and Andreasen, *Fluids* 2020, DOI: `10.3390/fluids5010029` | Broad review. Useful for positioning density vs. level-set tradeoffs in fluid topology optimization and for the turbulence-related cautionary context. |
| [2] Dilgen et al., *Struct Multidisc Optim* 2018, DOI: `10.1007/s00158-018-1967-6` | Strong density-based turbulent conjugate heat transfer reference. Supports keeping density as the topology-discovery stage. |
| [3] Yoon, *Comput Methods Appl Mech Eng* 2020, DOI: `10.1016/j.cma.2019.112784` | Direct `k-epsilon` turbulent topology optimization reference. Relevant because this branch already uses `k-epsilon`. |
| [4] Yaji et al., *J Comput Phys* 2021, DOI: `10.1016/j.jcp.2021.110630` | Turbulent level-set topology optimization with explicit interfaces and wall treatment. Strong motivation for the long-term sharp-interface direction, but not the same as the first implementation baseline here. |
| [5] Noel and Maute, *Struct Multidisc Optim* 2022, DOI: `10.1007/s00158-022-03353-3` | Turbulent conjugate heat transfer with level sets and explicit interface treatment. Relevant for the later sharp-interface refinement direction. |
| [6] Andreasen et al., *Struct Multidisc Optim* 2020, DOI: `10.1007/s00158-020-02527-1` | Shows that many density-method ingredients can be reused to drive a crisp level-set method with length-scale control. Very relevant to this repo transition. |
| [7] Barrera et al., *Struct Multidisc Optim* 2022, DOI: `10.1007/s00158-021-03096-7` | Extremely relevant for combining density assistance with level sets and for minimum feature-size control. Strong conceptual support for the hybrid stage design. |
| [8] Hoghøj et al., *Struct Multidisc Optim* 2024, DOI: `10.1007/s00158-024-03956-y` | Modern fluid-specific density-assisted hole seeding in level-set optimization. Reinforces that hybrid density/level-set workflows are state-of-the-art. |
| [9] Guo et al., *Comput Methods Appl Mech Eng* 2014, DOI: `10.1016/j.cma.2014.01.010` | Explicit feature control with signed-distance fields. Important for channel-size control strategy. |
| [10] Yamada et al., *Comput Methods Appl Mech Eng* 2010, DOI: `10.1016/j.cma.2010.05.013` | Fictitious-interface-energy and reaction-diffusion regularization. Key reference for complexity control and stable LSM evolution. |
| [11] Othmer, *Int J Numer Methods Fluids* 2008, DOI: `10.1002/fld.1770` | Foundational continuous-adjoint fluid topology optimization reference. Relevant to the adjoint philosophy in this codebase. |
| [12] Dilgen et al., *Comput Methods Appl Mech Eng* 2018, DOI: `10.1016/j.cma.2017.11.029` | Exact-sensitivity turbulent topology optimization reference. Important caution against over-trusting frozen turbulence. |
| [13] Zhou and Li, *J Comput Phys* 2008, DOI: `10.1016/j.jcp.2008.08.022` | Foundational variational level-set Navier-Stokes optimization reference. Helpful for the Stage-2 update philosophy. |
| [14] Yaji et al., *Int J Heat Mass Transfer* 2015, DOI: `10.1016/j.ijheatmasstransfer.2014.11.005` | Thermal-fluid level-set optimization with complexity control. Good supporting reference for channel-shape refinement. |
| [15] Chen and Hasegawa, *Applied Thermal Engineering* 2025, DOI: `10.1016/j.applthermaleng.2025.127626` | Recent OpenFOAM-native level-set conjugate heat transfer reference. Not turbulent, but highly relevant for implementation style inside OpenFOAM. |

---

## 16. Final Branch Decision

The final planning decision is:

- `turbulenceLSMOpt/` should remain an **MMA-first turbulent optimizer**
- level set should be added as a **secondary geometric refinement stage in the same run**
- the **first deliverable** of the LSM stage is a **level-set-parameterized diffuse-interface refinement workflow** with global and then regionwise size control
- topology discovery remains the job of density/MMA
- a sharper explicit-wall turbulent LSM stage is a **later milestone**, not a prerequisite for initial usefulness

This is the most robust, code-aligned, and literature-supported path for the current project state.
