//***********************************//update mu and lambda
opt++;
if(solid_area)
{
   forAll(cells_solid, i)
   {
      xh[cells_solid[i]]=0;
      xg[cells_solid[i]]=0;
   }
}
if(fluid_area)
{
   forAll(cells_fluid, i)
   {
      xh[cells_fluid[i]]=1.0;
      xg[cells_fluid[i]]=1.0;
   }
}
if(test_area)
{
   forAll(cells_test, i)
   {
      xh[cells_test[i]]=1.0;
      xg[cells_test[i]]=1.0;
   }
}
xh.correctBoundaryConditions();
xg.correctBoundaryConditions();

scalar grayVolumeContinuationLocal = 0.0;
forAll(xh, i)
{
  const scalar xhValue = xh[i];
  if (xhValue > 0.05 && xhValue < 0.95)
  {
    grayVolumeContinuationLocal += mesh.V()[i];
  }
}
reduce(grayVolumeContinuationLocal, sumOp<scalar>());
const scalar grayVolumeFractionForContinuation =
  area > SMALL ? grayVolumeContinuationLocal/area : 0.0;
if (!geometryControlConfigured || geometricSizeActivationGrayEnter < 0.0)
{
  geometryControlGrayGateSatisfied = true;
}
else if (geometryControlGrayGateSatisfied)
{
  geometryControlGrayGateSatisfied =
    grayVolumeFractionForContinuation <= geometricSizeActivationGrayExit;
}
else
{
  geometryControlGrayGateSatisfied =
    grayVolumeFractionForContinuation <= geometricSizeActivationGrayEnter;
}
geometryControlStepGateSatisfied =
  !geometryControlConfigured
  || geometricSizeActivationXhStepMax < 0.0
  || previousXhStepMax <= geometricSizeActivationXhStepMax;
geometryControlGateSatisfied =
  geometryControlConfigured
  && opt >= geometricSizeActivationMinIter
  && geometryControlGrayGateSatisfied
  && geometryControlStepGateSatisfied;
if (!geometryControlStageActivated)
{
  if (geometryControlGateSatisfied)
  {
    geometryControlActivationStreak++;
  }
  else
  {
    geometryControlActivationStreak = 0;
  }

  if (geometryControlActivationStreak >= geometricSizeActivationHoldIters)
  {
    geometryControlStageActivated = true;
    geometryControlActivationIter = opt;
  }
}
geometryControlStageWeight = 0.0;
if (geometryControlStageActivated)
{
  if (geometricSizeRampIters <= 1)
  {
    geometryControlStageWeight = 1.0;
  }
  else
  {
    geometryControlStageWeight =
      Foam::min
      (
        scalar(1.0),
        scalar(opt - geometryControlActivationIter + 1)
        /scalar(geometricSizeRampIters)
      );
  }
}
scalar continuationFeasibilityTolActive = continuationFeasibilityTol;
const bool lateStageStrictGateActive =
  lateStageStrictGateBelowGrayFraction >= 0.0
  && lateStageContinuationFeasibilityTol >= 0.0
  && grayVolumeFractionForContinuation <= lateStageStrictGateBelowGrayFraction;
if (lateStageStrictGateActive)
{
  continuationFeasibilityTolActive =
    Foam::min(continuationFeasibilityTolActive, lateStageContinuationFeasibilityTol);
}

const scalar continuationPowerLimit =
  usePowerConstraintRelaxation ? activePowerLimit : PowerDissMax;
const scalar powerFeasibilityRatio =
  continuationPowerLimit > SMALL ? PowerDiss/continuationPowerLimit : 0.0;
const bool continuationGateSatisfied =
  powerFeasibilityRatio <= continuationFeasibilityTolActive
  && continuationHealthOk;
const bool continuationFloorActive =
  opt <= forceContinuationHardeningUntilIter
  || (forceContinuationHardeningUntilBeta >= 0.0 && del < forceContinuationHardeningUntilBeta);
const bool continuationCeilingActive =
  stopContinuationHardeningBelowGrayFraction >= 0.0
  && grayVolumeFractionForContinuation <= stopContinuationHardeningBelowGrayFraction;
const scalar grayVolumeFractionDrop =
  previousGrayVolumeFractionForContinuation - grayVolumeFractionForContinuation;
const bool profileUsesAdaptiveHardeningThrottles =
  experimentProfile == "betaFloorGrayRamp"
  || experimentProfile == "branchRefinement400";
const bool laggingGrayCollapseDetected =
  profileUsesAdaptiveHardeningThrottles
  && !continuationFloorActive
  && laggingGrayCollapseStartIter > 0
  && opt >= laggingGrayCollapseStartIter
  && laggingGrayCollapseAboveGrayFraction >= 0.0
  && laggingGrayCollapseBelowXhStepMax >= 0.0
  && laggingGrayCollapseMinGrayDrop >= 0.0
  && grayVolumeFractionForContinuation >= laggingGrayCollapseAboveGrayFraction
  && previousXhStepMax <= laggingGrayCollapseBelowXhStepMax
  && grayVolumeFractionDrop <= laggingGrayCollapseMinGrayDrop;
const bool overactiveTopologyDetected =
  profileUsesAdaptiveHardeningThrottles
  && !continuationFloorActive
  && overactiveTopologyStartIter > 0
  && opt >= overactiveTopologyStartIter
  && overactiveTopologyBelowGrayFraction >= 0.0
  && overactiveTopologyAboveXhStepMax >= 0.0
  && grayVolumeFractionForContinuation <= overactiveTopologyBelowGrayFraction
  && previousXhStepMax >= overactiveTopologyAboveXhStepMax;
const bool laggingGrayCollapsePulseIter =
  laggingGrayCollapsePauseInterval > 0
  && ((opt - laggingGrayCollapseStartIter) % laggingGrayCollapsePauseInterval == 0);
const bool continuationThrottleAllowsHardening =
  !laggingGrayCollapseDetected || laggingGrayCollapsePulseIter;
geometryControlHardeningFrozen =
  freezeGeometricSizeHardeningWhileInfeasible
  && geometryControlStageActivated
  && (
    (useMinSolidGeometryControl && solidMinConstraintValue > 0.0)
    || (useMaxSolidGeometryControl && solidMaxConstraintValue > 0.0)
  );
const bool allowContinuationHardening =
  !continuationCeilingActive
  && !overactiveTopologyDetected
  && !geometryControlHardeningFrozen
  && continuationThrottleAllowsHardening
  && (forceContinuationHardening || continuationFloorActive || continuationGateSatisfied);
const bool lateStageFilterRCapActive =
  useLateStageFilterRCap
  && lateStageFilterRStartIter > 0
  && lateStageFilterRCap >= 0.0
  && opt > lateStageFilterRStartIter;
activeFilterR =
  lateStageFilterRCapActive
  ? Foam::min(filterR, lateStageFilterRCap)
  : filterR;
b.value()=1.0/(activeFilterR*len/3.464)/(activeFilterR*len/3.464);
const bool lateStageRefinementActive =
  (lateStageRefinementStartIter > 0 && opt >= lateStageRefinementStartIter)
  || (
    lateStageRefinementBelowGrayFraction >= 0.0
    && grayVolumeFractionForContinuation <= lateStageRefinementBelowGrayFraction
  );
scalar betaIncrementActive = betaIncrement;
scalar alphaRampLateFactorActive = alphaRampLateFactor;
scalar betaMaxActive = betaMax;
if (lateStageRefinementActive)
{
  if (lateStageBetaRampFactor >= 0.0)
  {
    betaIncrementActive *= lateStageBetaRampFactor;
  }
  if (lateStageAlphaRampFactor >= 0.0)
  {
    alphaRampLateFactorActive = lateStageAlphaRampFactor;
  }
  if (lateStageMaxBeta >= 0.0)
  {
    betaMaxActive = Foam::min(betaMaxActive, lateStageMaxBeta);
  }
}

if(opt<=63 && allowContinuationHardening)
{
  alphaMax=alphamax*(opt*alphaRampEarlySlope+1.0);
}
if(opt>100 && allowContinuationHardening)
{
  alphaMax=alphaMax*alphaRampLateFactorActive;
}
alphaMax.value()=Foam::min(alphaMax.value(),alphaMAX.value());

if (qContinuationInterval > 0 && opt >= qContinuationStartIter && ((opt - qContinuationStartIter) % qContinuationInterval == 0))
{
  if (useSplitRAMPControls)
  {
    qAlpha = Foam::max(qAlpha*qContinuationFactor, qAlphaMin);
    qKappa = Foam::max(qKappa*qContinuationFactor, qKappaMin);
    qHeat = Foam::max(qHeat*qContinuationFactor, qHeatMin);
  }
  else
  {
    qSingle = Foam::max(qSingle*qContinuationFactor, qSingleMin);
  }
}

if (!useStagnationTriggeredBeta)
{
  if (allowContinuationHardening)
  {
    del = Foam::min(del + betaIncrementActive, betaMaxActive);
  }
}
else if (optimizerConvergedStreak > 0 && allowContinuationHardening)
{
  del = Foam::min(del + betaIncrementActive, betaMaxActive);
}

previousGrayVolumeFractionForContinuation = grayVolumeFractionForContinuation;

#include "updateInterpolationFields.H"
