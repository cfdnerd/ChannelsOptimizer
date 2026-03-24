//***********************************//update mu and lambda
opt++;
if(solid_area)
{
   forAll(cells_solid, i)
   {
      xh[cells_solid[i]]=0;
   }
}
if(fluid_area)
{
   forAll(cells_fluid, i)
   {
      xh[cells_fluid[i]]=1.0;
   }
}
xh.correctBoundaryConditions();

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

const scalar continuationPowerLimit =
  usePowerConstraintRelaxation ? activePowerLimit : PowerDissMax;
const scalar powerFeasibilityRatio =
  continuationPowerLimit > SMALL ? PowerDiss/continuationPowerLimit : 0.0;
const bool continuationGateSatisfied =
  powerFeasibilityRatio <= continuationFeasibilityTol
  && continuationHealthOk;
const bool continuationFloorActive =
  opt <= forceContinuationHardeningUntilIter
  || (forceContinuationHardeningUntilBeta >= 0.0 && del < forceContinuationHardeningUntilBeta);
const bool continuationCeilingActive =
  stopContinuationHardeningBelowGrayFraction >= 0.0
  && grayVolumeFractionForContinuation <= stopContinuationHardeningBelowGrayFraction;
const scalar grayVolumeFractionDrop =
  previousGrayVolumeFractionForContinuation - grayVolumeFractionForContinuation;
const bool laggingGrayCollapseDetected =
  experimentProfile == "betaFloorGrayRamp"
  && !continuationFloorActive
  && laggingGrayCollapseStartIter > 0
  && opt >= laggingGrayCollapseStartIter
  && laggingGrayCollapseAboveGrayFraction >= 0.0
  && laggingGrayCollapseBelowXhStepMax >= 0.0
  && laggingGrayCollapseMinGrayDrop >= 0.0
  && grayVolumeFractionForContinuation >= laggingGrayCollapseAboveGrayFraction
  && previousXhStepMax <= laggingGrayCollapseBelowXhStepMax
  && grayVolumeFractionDrop <= laggingGrayCollapseMinGrayDrop;
const bool laggingGrayCollapsePulseIter =
  laggingGrayCollapsePauseInterval > 0
  && ((opt - laggingGrayCollapseStartIter) % laggingGrayCollapsePauseInterval == 0);
const bool continuationThrottleAllowsHardening =
  !laggingGrayCollapseDetected || laggingGrayCollapsePulseIter;
const bool allowContinuationHardening =
  !continuationCeilingActive
  && continuationThrottleAllowsHardening
  && (forceContinuationHardening || continuationFloorActive || continuationGateSatisfied);

if(opt<=63 && allowContinuationHardening)
{
  alphaMax=alphamax*(opt*alphaRampEarlySlope+1.0);
}
if(opt>100 && allowContinuationHardening)
{
  alphaMax=alphaMax*alphaRampLateFactor;
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
    del = Foam::min(del + betaIncrement, betaMax);
  }
}
else if (optimizerConvergedStreak > 0 && allowContinuationHardening)
{
  del = Foam::min(del + betaIncrement, betaMax);
}

previousGrayVolumeFractionForContinuation = grayVolumeFractionForContinuation;

#include "updateInterpolationFields.H"
