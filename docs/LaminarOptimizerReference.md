# Laminar Optimizer Reference Baseline

Reference logs examined:

- `laminarOptimizer/optimizerlogs_reference/runLOG`
- `laminarOptimizer/optimizerlogs_reference/optimization.hst`
- `laminarOptimizer/optimizerlogs_reference/debugOptimizer.log`
- `laminarOptimizer/optimizerlogs_reference/debugOptimizer.jsonl`
- `laminarOptimizer/optimizerlogs_reference/solverConvergences.log`
- `laminarOptimizer/optimizerlogs_reference/channelsApp-796.Out`

This note captures how a successful `laminarOptimizer` run behaves so it can be
used later as a debugging baseline for `turbulenceMMAOpt`.

## Outcome

- The run completes cleanly through iteration `200` and finalises normally.
- It does **not** stop via the optimizer convergence criteria; it reaches the configured run end with `candidate=no`, `stop=no`.
- `channelsApp-796.Out` only contains OpenMPI/OpenIB startup warnings, not solver failures.
- Solver health is `OK` for all 200 iterations.
- Sensitivity health is `OK` for all 200 iterations.
- No non-finite design variables or sensitivities are reported.

## Main Evolution

Anchor iterations:

- `Iter 1`: `MeanT=17.01`, `PDCon=3.51`, `V=4.44e-2`, gray fraction `0.9269`, `beta=0.2`
- `Iter 12`: `MeanT=13.62`, `PDCon=10.11`, `V=-1.80e-2`, first near-feasible power point
- `Iter 28`: objective minimum `MeanT=13.19`, but `PDCon=19.70` and volume still negative
- `Iter 50`: `MeanT=13.64`, `PDCon=15.65`, gray fraction still `0.9164`
- `Iter 60`: start of strongest redesign burst, `xhStepL2/sqrtN=0.2278`
- `Iter 66`: large redesign still active, `MeanT=17.44`, `PDCon=10.80`
- `Iter 71`: largest single objective jump already happened, gray fraction down to `0.5697`
- `Iter 100`: `MeanT=16.84`, `PDCon=10.19`, gray fraction `0.2204`
- `Iter 150`: `MeanT=20.40`, `PDCon=10.06`, gray fraction `0.0451`
- `Iter 200`: `MeanT=21.57`, `PDCon=10.07`, `V=3.62e-5`, gray fraction `0.0195`, `beta=40`

High-level phases:

1. `Iter 1-25`: rapid objective drop and hotspot reduction.
   Power constraint rises from `3.51` to `17.38`.
   Volume quickly crosses from positive to negative by `Iter 3`.
   Design remains very gray, around `0.92`.

2. `Iter 26-50`: over-constrained but still smooth regime.
   Objective stays near its minimum band.
   Power remains very high, averaging about `17.97`.
   Gray fraction barely changes.

3. `Iter 51-75`: major topology reshaping regime.
   Largest `x`/`xh` updates occur here.
   Objective becomes volatile.
   Power moves back toward the active limit.
   Gray fraction collapses from about `0.92` toward `0.57`.

4. `Iter 76-150`: sharpening and settling.
   Power stays close to `10`.
   Objective climbs steadily.
   Gray fraction drops from `0.31` to `0.045`.

5. `Iter 151-200`: near-binary polishing.
   Design updates become tiny.
   Gray fraction drops below `0.03`.
   Constraint residual stays small but slightly positive.

## Design Variable Behavior

Consistent healthy signals:

- `xMean` and `xhMean` stay pinned near `0.40` almost the whole run.
- `xMin` decreases from about `0.361` to about `0.177`; `xMax` stays near `1`.
- `xp` stays bounded and smooth: `xpMin` roughly `0.173-0.361`, `xpMax` roughly `0.986-0.992`.
- Gray volume fraction monotonically drops from about `0.927` to `0.0195`.
- Final phase split is approximately:
  solid `0.5899`, fluid `0.3906`, gray `0.0195`.

Important implication:

- A healthy run does **not** need early binary behavior.
- Long gray persistence is acceptable, but it should eventually collapse.
- The strongest topology changes happen mid-run, not at the start.

## Filter / Projection Behavior

- `beta` (`del`) ramps monotonically from `0.2` to `40.0`.
- `eta5` drifts smoothly from `0.3622` to `0.3311`.
- `drhoMax` grows with `beta`, from `0.4` to `39.80`.
- Final `drhoMin` is essentially zero in solid regions, which is expected for a strongly projected design.

Healthy interpretation:

- Increasing projection strength is a major driver of late-stage sharpening.
- Small `drhoMin` values alone are not a failure signal.
- `eta5` remains stable and only shifts modestly; wild oscillation would be suspicious.

## Objective / Constraint Behavior

- Objective minimum occurs early at `Iter 28` with `MeanT=13.19`.
- Final objective is worse than the minimum but occurs with a much sharper, nearly feasible design.
- `MaxT` drops sharply from `58.94` at `Iter 2` to `26.71` at `Iter 34`, then rises again to `41.35` by `Iter 200`.
- Power constraint first exceeds `10` at `Iter 12`.
- Last iteration with `PDCon <= 10` is `Iter 157`.
- Final power margin is small but positive: about `7.1e-3`.
- Final volume margin is also small and positive: about `3.6e-5`.

Important implication:

- This baseline is a good example of a numerically healthy run that ends slightly constraint-positive.
- Later objective degradation is not automatically a bug if it coincides with feasibility recovery and sharpening.

## Gradient / Sensitivity Behavior

Norm trends:

- Objective sensitivity `L2` peaks at `Iter 60` (`551.996`) and decays to `0.609` by `Iter 200`.
- Power sensitivity `L2` peaks at `Iter 62` (`356.297`) and decays to `0.525` by `Iter 200`.
- Volume sensitivity `L2` peaks at `Iter 57` (`1769.083`) and remains the dominant scale throughout the run.

Late-run signatures:

- Filtered objective sensitivity range narrows to about `[-3.83e-2, 1.08e-1]`.
- Filtered power sensitivity is almost entirely non-positive near the end.
- Filtered volume sensitivity remains large and positive, with max about `18.58`.
- No non-finite counts appear anywhere.

Healthy interpretation:

- Large mid-run sensitivity spikes are normal during major topology changes.
- Sensitivity collapse in the final stage is expected.
- Volume sensitivities can stay much larger than objective/power sensitivities without indicating failure.

## Solver Baseline

Late-run solver profile at `Iter 200`:

- `U momentum`: `OK`, about `6.17` average iterations
- `p pressure`: `OK`, about `7.97` average iterations
- `T thermal`: `OK`, `937` iterations
- `Tb adjoint thermal`: `OK`, `459` iterations
- `Ub adjoint momentum`: `OK`, about `10.2` average iterations
- `Ua adjoint momentum`: `OK`, about `6.45` average iterations
- `xp filter`: `OK`, `6` iterations, final residual about `3.0e-9`
- `fsens/gsens filters`: `OK`, `8` iterations, final residuals about `5e-9` to `9e-9`

Healthy interpretation:

- Very long thermal solves are normal in this branch if residuals remain clean.
- Filter and projection solves should stay cheap and consistently convergent.
- `solver=OK` for every iteration is a strong stability marker.

## What Transfers Well To `turbulenceMMAOpt`

Useful baseline signals:

- No non-finite design or sensitivity counts.
- Filter solves remain consistently cheap and stable.
- Gray fraction should trend down over time, even if slowly at first.
- Biggest design changes can happen well after the first tens of iterations.
- Objective can worsen later while the design sharpens and constraints improve.

Branch-specific laminar features that should not be copied blindly:

- Exact objective trajectory.
- Exact power/volume feasibility history.
- Exact thermal and adjoint iteration counts.
- Exact sensitivity magnitudes.
- The scalar continuation schedule (`qu`) instead of the turbulent split controls (`qAlpha`, `qKappa`, `qHeat`).

## Practical Debugging Use

When debugging `turbulenceMMAOpt`, compare against this reference in the following order:

1. Numerical hygiene:
   no non-finite counts, no solver warnings, no exploding filter/projection residuals.
2. Design evolution:
   bounded `x/xp/xh`, shrinking gray fraction, decreasing step sizes in late iterations.
3. Constraint behavior:
   power and volume margins may oscillate, but should not diverge.
4. Sensitivity behavior:
   mid-run spikes can be normal; persistent growth or sign/pathology is not.
5. Late-stage behavior:
   tiny steps, stable filters, mostly binary design, and near-active constraints are all healthy signs.
