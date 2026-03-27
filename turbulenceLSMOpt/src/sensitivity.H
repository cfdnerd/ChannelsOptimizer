
adjointSoftFreezeAppliedPower = false;
adjointSoftFreezeAppliedHeat = false;
adjointSoftFreezeScalePower = 1.0;
adjointSoftFreezeScaleHeat = 1.0;
adjointPowerToPrimalRatioPreGuard = 0.0;
adjointHeatToPrimalRatioPreGuard = 0.0;

scalar primalVelocityScaleGuardLocal(0.0);
scalar adjointPowerMaxGuardLocal(0.0);
scalar adjointHeatMaxGuardLocal(0.0);

forAll(U, cellI)
{
  primalVelocityScaleGuardLocal =
    Foam::max(primalVelocityScaleGuardLocal, Foam::mag(U[cellI]));
  adjointPowerMaxGuardLocal =
    Foam::max(adjointPowerMaxGuardLocal, Foam::mag(Ua[cellI]));
  adjointHeatMaxGuardLocal =
    Foam::max(adjointHeatMaxGuardLocal, Foam::mag(Ub[cellI]));
}

reduce(primalVelocityScaleGuardLocal, maxOp<scalar>());
reduce(adjointPowerMaxGuardLocal, maxOp<scalar>());
reduce(adjointHeatMaxGuardLocal, maxOp<scalar>());

const scalar primalVelocityScaleGuard =
  Foam::max(primalVelocityScaleGuardLocal, scalar(SMALL));
adjointPowerToPrimalRatioPreGuard =
  adjointPowerMaxGuardLocal/primalVelocityScaleGuard;
adjointHeatToPrimalRatioPreGuard =
  adjointHeatMaxGuardLocal/primalVelocityScaleGuard;

const scalar adjointPowerGrowthFactorPreGuard =
  previousAdjointPowerToPrimalRatio > SMALL
  ? adjointPowerToPrimalRatioPreGuard
   /Foam::max(previousAdjointPowerToPrimalRatio, scalar(SMALL))
  : scalar(1.0);
const scalar adjointHeatGrowthFactorPreGuard =
  previousAdjointHeatToPrimalRatio > SMALL
  ? adjointHeatToPrimalRatioPreGuard
   /Foam::max(previousAdjointHeatToPrimalRatio, scalar(SMALL))
  : scalar(1.0);

const bool adjointPowerNeedsSoftFreeze =
  previousAdjointPowerToPrimalRatio > SMALL
  && adjointSoftFreezeRatio > 0.0
  && adjointSoftFreezeGrowthFactor > 0.0
  && adjointPowerToPrimalRatioPreGuard >= adjointSoftFreezeRatio
  && adjointPowerGrowthFactorPreGuard >= adjointSoftFreezeGrowthFactor;
const bool adjointHeatNeedsSoftFreeze =
  previousAdjointHeatToPrimalRatio > SMALL
  && adjointSoftFreezeRatio > 0.0
  && adjointSoftFreezeGrowthFactor > 0.0
  && adjointHeatToPrimalRatioPreGuard >= adjointSoftFreezeRatio
  && adjointHeatGrowthFactorPreGuard >= adjointSoftFreezeGrowthFactor;

const bool adjointPowerNeedsRunawayClamp =
  adjointPowerToPrimalRatioPreGuard > adjointRunawayRatioLimit
  && previousAdjointPowerToPrimalRatio > SMALL;
const bool adjointHeatNeedsRunawayClamp =
  adjointHeatToPrimalRatioPreGuard > adjointRunawayRatioLimit
  && previousAdjointHeatToPrimalRatio > SMALL;

if (adjointPowerNeedsSoftFreeze || adjointPowerNeedsRunawayClamp)
{
  const scalar targetRatio =
    Foam::max(previousAdjointPowerToPrimalRatio, scalar(SMALL));
  adjointSoftFreezeScalePower =
    Foam::min
    (
      scalar(1.0),
      targetRatio/Foam::max(adjointPowerToPrimalRatioPreGuard, scalar(SMALL))
    );

  forAll(Ua, cellI)
  {
    Ua[cellI] *= adjointSoftFreezeScalePower;
  }
  forAll(pa, cellI)
  {
    pa[cellI] *= adjointSoftFreezeScalePower;
  }
  forAll(phia, faceI)
  {
    phia[faceI] *= adjointSoftFreezeScalePower;
  }
  Ua.correctBoundaryConditions();
  pa.correctBoundaryConditions();
  adjointSoftFreezeAppliedPower = adjointSoftFreezeScalePower < 0.999999;
}

if (adjointHeatNeedsSoftFreeze || adjointHeatNeedsRunawayClamp)
{
  const scalar targetRatio =
    Foam::max(previousAdjointHeatToPrimalRatio, scalar(SMALL));
  adjointSoftFreezeScaleHeat =
    Foam::min
    (
      scalar(1.0),
      targetRatio/Foam::max(adjointHeatToPrimalRatioPreGuard, scalar(SMALL))
    );

  forAll(Ub, cellI)
  {
    Ub[cellI] *= adjointSoftFreezeScaleHeat;
  }
  forAll(pb, cellI)
  {
    pb[cellI] *= adjointSoftFreezeScaleHeat;
  }
  forAll(phib, faceI)
  {
    phib[faceI] *= adjointSoftFreezeScaleHeat;
  }
  Ub.correctBoundaryConditions();
  pb.correctBoundaryConditions();
  adjointSoftFreezeAppliedHeat = adjointSoftFreezeScaleHeat < 0.999999;
}

Info<< "sensitivity analysis" << endl;

std::vector<scalar> xOld(n);
std::vector<scalar> xhOld(n);
std::vector<scalar> phiOld(n);
for(i=0;i<n;i++)
{
  xOld[i] = x[i];
  xhOld[i] = xh[i];
  phiOld[i] = phiLS[i];
}

Info<< "sensitivity analysis: assemble raw objective sensitivity" << endl;
volVectorField gradTField
(
    IOobject
    (
        "gradTField",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    fvc::grad(T)
);
volVectorField gradTbField
(
    IOobject
    (
        "gradTbField",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    fvc::grad(Tb)
);

forAll(fsenshMeanT, cellI)
{
  const scalar brinkmanTerm = alphaSensCoeff[cellI]*(U[cellI] & Ub[cellI]);
  const scalar thermalGradTerm =
    b1.value()*kappaSensCoeff[cellI]*(gradTField[cellI] & gradTbField[cellI]);
  scalar objectiveSensitivity = brinkmanTerm + thermalGradTerm;

  if (DDHS)
  {
    objectiveSensitivity -=
      b1.value()*Tb[cellI]*thermalSource.value()*heatSourceSensCoeff[cellI];
  }

  fsenshMeanT[cellI] =
    std::isfinite(static_cast<double>(objectiveSensitivity)) ? objectiveSensitivity : 0.0;
}

forAll(gsenshPowerDiss, cellI)
{
  const scalar powerSensitivity = alphaSensCoeff[cellI]*(U[cellI] & Ua[cellI]);
  gsenshPowerDiss[cellI] =
    std::isfinite(static_cast<double>(powerSensitivity)) ? powerSensitivity : 0.0;
  gsensVolRaw[cellI] = 1.0;
}

fsensMeanTRaw = fsenshMeanT;
gsensPowerDissRaw = gsenshPowerDiss;

// Keep the production LSM update aligned with the MMA optimizer semantics:
// sanitize and normalize the objective/power sensitivities before combining
// them into a single descent field. This prevents one raw adjoint channel from
// overwhelming the constrained update purely by scale.
const scalar fsensScale = gMax(mag(fsenshMeanT.primitiveField()));
if (fsensScale > SMALL)
{
  fsensMeanT = fsenshMeanT/fsensScale;
}
else
{
  fsensMeanT = dimensionedScalar("zero", fsenshMeanT.dimensions(), 0.0);
}

const scalar gsensPowerScale = gMax(mag(gsenshPowerDiss.primitiveField()));
if (gsensPowerScale > SMALL)
{
  gsensPowerDiss = gsenshPowerDiss/gsensPowerScale;
}
else
{
  gsensPowerDiss = dimensionedScalar("zero", gsenshPowerDiss.dimensions(), 0.0);
}

gsensVol = gsensVolRaw;
gsenshVol = gsensVolRaw;

const scalar powerLimit =
  usePowerConstraintRelaxation
  ? Foam::max(PowerDissRelax - opt*powerConstraintRelaxationRate, PowerDissMax)
  : PowerDissMax;
activePowerLimit = powerLimit;
const scalar powerViolationScale =
  Foam::pow
  (
    Foam::max(PowerDiss/Foam::max(powerLimit, SMALL), scalar(1.0)),
    optPowerViolationScaleExponent
  );
const scalar effectivePowerConstraintWeight =
  optPowerConstraintWeight*powerViolationScale;
const scalar weightedPowerConstraint =
  effectivePowerConstraintWeight*(PowerDiss/powerLimit - 1.0);
const scalar weightedVolumeConstraint = optVolumeConstraintWeight*V;
gx[0] = weightedPowerConstraint;
gx[1] = weightedVolumeConstraint;

#include "lsmSensitivity.H"
#include "lsmVelocityRegularization.H"
#include "lsmAdvection.H"
#include "lsmReinitialize.H"
const scalar volumePhiMagnitudeCap = 6.0*Foam::max(epsilonLSMActive, scalar(SMALL));

auto smoothedHeavisideAtPhi = [&](const scalar phiValue) -> scalar
{
  if (phiValue <= -epsilonLSMActive)
  {
    return 0.0;
  }
  if (phiValue >= epsilonLSMActive)
  {
    return 1.0;
  }

  const scalar ratio = phiValue/Foam::max(epsilonLSMActive, SMALL);
  return
    0.5
   *(
      1.0
    + ratio
    + Foam::sin(constant::mathematical::pi*ratio)/constant::mathematical::pi
    );
};

auto fluidFractionForPhiShift = [&](const scalar phiShift) -> scalar
{
  scalar fluidVolumeLocal(0.0);
  forAll(phiLS, cellI)
  {
    scalar xhValue = smoothedHeavisideAtPhi(phiLS[cellI] + phiShift);
    fluidVolumeLocal += xhValue*mesh.V()[cellI];
  }

  if (solid_area)
  {
    forAll(cells_solid, zoneI)
    {
      fluidVolumeLocal -=
        smoothedHeavisideAtPhi(phiLS[cells_solid[zoneI]] + phiShift)*mesh.V()[cells_solid[zoneI]];
    }
  }
  if (fluid_area)
  {
    forAll(cells_fluid, zoneI)
    {
      const label cellI = cells_fluid[zoneI];
      fluidVolumeLocal +=
        (1.0 - smoothedHeavisideAtPhi(phiLS[cellI] + phiShift))*mesh.V()[cellI];
    }
  }
  if (test_area)
  {
    forAll(cells_test, zoneI)
    {
      const label cellI = cells_test[zoneI];
      fluidVolumeLocal +=
        (1.0 - smoothedHeavisideAtPhi(phiLS[cellI] + phiShift))*mesh.V()[cellI];
    }
  }

  reduce(fluidVolumeLocal, sumOp<scalar>());
  return area > SMALL ? fluidVolumeLocal/area : 0.0;
};

scalar lowerShift = -volumePhiMagnitudeCap;
scalar upperShift = volumePhiMagnitudeCap;
scalar lowerFraction = fluidFractionForPhiShift(lowerShift);
scalar upperFraction = fluidFractionForPhiShift(upperShift);

for (label expandIter = 0; expandIter < 8 && (lowerFraction > voluse || upperFraction < voluse); ++expandIter)
{
  lowerShift *= 2.0;
  upperShift *= 2.0;
  lowerFraction = fluidFractionForPhiShift(lowerShift);
  upperFraction = fluidFractionForPhiShift(upperShift);
}

scalar phiVolumeShift(0.0);
if (lowerFraction <= voluse && upperFraction >= voluse)
{
  for (label bisectIter = 0; bisectIter < 40; ++bisectIter)
  {
    const scalar midShift = 0.5*(lowerShift + upperShift);
    const scalar midFraction = fluidFractionForPhiShift(midShift);
    phiVolumeShift = midShift;

    if (midFraction < voluse)
    {
      lowerShift = midShift;
    }
    else
    {
      upperShift = midShift;
    }
  }
}

volumePhiShiftRaw = phiVolumeShift;
const scalar baseVolumePhiShiftCap =
  maxVolumePhiShiftFactor*Foam::max(epsilonLSMActive, scalar(SMALL));
const scalar aggressiveVolumePhiShiftCap =
  0.25*Foam::max(epsilonLSMActive, scalar(SMALL));
volumePhiShiftCap = baseVolumePhiShiftCap;

// Large one-shot global phi shifts create the two-state flip-flop seen in the
// optimizer logs, so keep the correction bounded even when the volume error is
// large enough to tempt an aggressive reset.
if (Foam::mag(V) > 0.10)
{
  volumePhiShiftCap = Foam::max(volumePhiShiftCap, aggressiveVolumePhiShiftCap);
}
volumePhiShiftCap = Foam::min(volumePhiShiftCap, aggressiveVolumePhiShiftCap);

volumePhiShiftApplied =
  Foam::max(Foam::min(volumePhiShiftRaw, volumePhiShiftCap), -volumePhiShiftCap);

forAll(phiLS, cellI)
{
  scalar phiValue = phiLS[cellI] + volumePhiShiftApplied;
  if (!std::isfinite(static_cast<double>(phiValue)))
  {
    phiValue = 0.0;
  }
  phiLS[cellI] = Foam::max(Foam::min(phiValue, volumePhiMagnitudeCap), -volumePhiMagnitudeCap);
}
phiLS.correctBoundaryConditions();
#include "lsmInterfaceReconstruct.H"
V = fvc::domainIntegrate(xh).value()/area - voluse;
gx[1] = optVolumeConstraintWeight*V;
#include "updateInterpolationFields.H"
#include "gradientOptWrite.H"
#include "debugOptimizer.H"
#include "solverConvergenceWrite.H"
