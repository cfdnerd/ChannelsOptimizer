
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

forAll(phiLS, cellI)
{
  scalar phiValue = phiLS[cellI] + phiVolumeShift;
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
