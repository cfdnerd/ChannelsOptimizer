powerProjectionMultiplierAnalytical = 0.0;
powerSupportFloorValue = 0.0;
powerSupportFloorNegativeReach = 0.0;
powerSupportFloorVolumeFraction = 0.0;
powerSupportFloorCellCount = 0;
powerProjectionMultiplierFloor = 0.0;
const bool broadenPowerReopenSupport =
  experimentProfile == "adjointSensitivityProbe"
  && gx[0] > 0.0;
if (broadenPowerReopenSupport)
{
  // The trapped reruns show a two-stage loss of reopening support:
  // first the meaningful dirac band collapses, then the over-solid branch
  // marches through the negative shoulder into the outer band. Keep the debug
  // power signal alive across that trapped negative side long enough to test
  // whether a stronger reopening push can recover channels.
  powerSupportFloorValue = 0.1/Foam::max(epsilonLSMActive, scalar(SMALL));
  powerSupportFloorNegativeReach = 4.0*epsilonLSMActive;
}

scalar powerSupportVolumeLocal(0.0);
label powerSupportCellCountLocal(0);
forAll(interfaceObjectiveSensitivity, cellI)
{
  const scalar phiValue = phiLS[cellI];
  const scalar diracValue = diracPhiLS[cellI];
  scalar powerSupportValue = diracValue;
  if
  (
    powerSupportFloorValue > 0.0
    && phiValue <= epsilonLSMActive
    && phiValue >= -powerSupportFloorNegativeReach
  )
  {
    const scalar broadenedSupport =
      Foam::max(powerSupportValue, powerSupportFloorValue);
    if (broadenedSupport > powerSupportValue + SMALL)
    {
      powerSupportVolumeLocal += mesh.V()[cellI];
      powerSupportCellCountLocal++;
    }
    powerSupportValue = broadenedSupport;
  }
  interfaceObjectiveSensitivity[cellI] = -fsensMeanT[cellI]*diracValue;
  interfacePowerSensitivity[cellI] = -gsensPowerDiss[cellI]*powerSupportValue;
  interfaceVolumeSensitivity[cellI] = -gsensVol[cellI]*diracValue;
}
reduce(powerSupportVolumeLocal, sumOp<scalar>());
reduce(powerSupportCellCountLocal, sumOp<label>());
powerSupportFloorVolumeFraction =
  area > SMALL ? powerSupportVolumeLocal/area : 0.0;
powerSupportFloorCellCount = powerSupportCellCountLocal;

scalar powerObjectiveInnerLocal(0.0);
scalar powerSensitivityNormLocal(0.0);
forAll(interfaceObjectiveSensitivity, cellI)
{
  const scalar cellVolume = mesh.V()[cellI];
  powerObjectiveInnerLocal +=
    interfaceObjectiveSensitivity[cellI]*interfacePowerSensitivity[cellI]*cellVolume;
  powerSensitivityNormLocal +=
    interfacePowerSensitivity[cellI]*interfacePowerSensitivity[cellI]*cellVolume;
}
reduce(powerObjectiveInnerLocal, sumOp<scalar>());
reduce(powerSensitivityNormLocal, sumOp<scalar>());

powerProjectionMultiplierAnalytical =
  (gx[0] > 0.0 && powerSensitivityNormLocal > SMALL)
  ? Foam::max(scalar(0.0), -powerObjectiveInnerLocal/powerSensitivityNormLocal)
  : scalar(0.0);
powerProjectionMultiplier = powerProjectionMultiplierAnalytical;

const bool broadenedPowerSupportActive =
  powerSupportFloorCellCount > 0
  && powerSupportFloorVolumeFraction > 0.05;
if
(
  broadenPowerReopenSupport
  && broadenedPowerSupportActive
  && powerProjectionMultiplierAnalytical <= SMALL
)
{
  // The previous rerun showed that a small additive reopening push keeps the
  // trapped power channel alive but still recovers essentially no fluid.
  // Drive the fallback power signal up to the normal-velocity cap so the next
  // rerun cleanly answers whether the remaining blocker is magnitude or
  // direction/localization.
  powerProjectionMultiplierFloor =
    maxNormalVelocity/Foam::max(powerSupportFloorValue, scalar(SMALL));
  powerProjectionMultiplier =
    Foam::max(powerProjectionMultiplier, powerProjectionMultiplierFloor);
}

forAll(totalPhiSensitivity, cellI)
{
  totalPhiSensitivity[cellI] =
      interfaceObjectiveSensitivity[cellI]
    + powerProjectionMultiplier*interfacePowerSensitivity[cellI];
}

scalar volumeProjectionInnerLocal(0.0);
scalar volumeSensitivityNormLocal(0.0);
forAll(totalPhiSensitivity, cellI)
{
  const scalar cellVolume = mesh.V()[cellI];
  volumeProjectionInnerLocal +=
    totalPhiSensitivity[cellI]*interfaceVolumeSensitivity[cellI]*cellVolume;
  volumeSensitivityNormLocal +=
    interfaceVolumeSensitivity[cellI]*interfaceVolumeSensitivity[cellI]*cellVolume;
}
reduce(volumeProjectionInnerLocal, sumOp<scalar>());
reduce(volumeSensitivityNormLocal, sumOp<scalar>());

volumeProjectionMultiplier =
  volumeSensitivityNormLocal > SMALL
  ? -volumeProjectionInnerLocal/volumeSensitivityNormLocal
  : scalar(0.0);

forAll(totalPhiSensitivity, cellI)
{
  totalPhiSensitivity[cellI] +=
    volumeProjectionMultiplier*interfaceVolumeSensitivity[cellI];
}

interfaceObjectiveSensitivity.correctBoundaryConditions();
interfacePowerSensitivity.correctBoundaryConditions();
interfaceVolumeSensitivity.correctBoundaryConditions();
totalPhiSensitivity.correctBoundaryConditions();
