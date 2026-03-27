powerSupportFloorValue = 0.0;
powerSupportFloorVolumeFraction = 0.0;
powerSupportFloorCellCount = 0;
const bool broadenPowerReopenSupport =
  experimentProfile == "adjointSensitivityProbe"
  && gx[0] > 0.0;
if (broadenPowerReopenSupport)
{
  // The instrumented trap rerun showed that meaningful dirac support dies one
  // iteration before the broad interface band catches up. Keep the power
  // reopening signal alive across that remaining band for this debug profile.
  powerSupportFloorValue = 0.1/Foam::max(epsilonLSMActive, scalar(SMALL));
}

scalar powerSupportVolumeLocal(0.0);
label powerSupportCellCountLocal(0);
forAll(interfaceObjectiveSensitivity, cellI)
{
  const scalar diracValue = diracPhiLS[cellI];
  scalar powerSupportValue = diracValue;
  if
  (
    powerSupportFloorValue > 0.0
    && Foam::mag(phiLS[cellI]) <= epsilonLSMActive
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

powerProjectionMultiplier =
  (gx[0] > 0.0 && powerSensitivityNormLocal > SMALL)
  ? Foam::max(scalar(0.0), -powerObjectiveInnerLocal/powerSensitivityNormLocal)
  : scalar(0.0);

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
