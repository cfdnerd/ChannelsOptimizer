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

const scalar continuationPowerLimit =
  usePowerConstraintRelaxation ? activePowerLimit : PowerDissMax;
const scalar powerFeasibilityRatio =
  continuationPowerLimit > SMALL ? PowerDiss/continuationPowerLimit : 0.0;
const bool allowContinuationHardening =
  powerFeasibilityRatio <= continuationFeasibilityTol
  && continuationHealthOk;

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

#include "updateInterpolationFields.H"
