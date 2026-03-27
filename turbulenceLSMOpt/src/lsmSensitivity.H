forAll(interfaceObjectiveSensitivity, cellI)
{
  const scalar diracValue = diracPhiLS[cellI];
  interfaceObjectiveSensitivity[cellI] = -fsensMeanT[cellI]*diracValue;
  interfacePowerSensitivity[cellI] = -gsensPowerDiss[cellI]*diracValue;
  interfaceVolumeSensitivity[cellI] = -gsensVol[cellI]*diracValue;
}

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

const scalar powerProjectionMultiplier =
  (gx[0] > 0.0 && powerSensitivityNormLocal > SMALL)
  ? Foam::max(scalar(0.0), -powerObjectiveInnerLocal/powerSensitivityNormLocal)
  : scalar(0.0);

forAll(totalPhiSensitivity, cellI)
{
  totalPhiSensitivity[cellI] =
      interfaceObjectiveSensitivity[cellI]
    + powerProjectionMultiplier*interfacePowerSensitivity[cellI];
}

interfaceObjectiveSensitivity.correctBoundaryConditions();
interfacePowerSensitivity.correctBoundaryConditions();
interfaceVolumeSensitivity.correctBoundaryConditions();
totalPhiSensitivity.correctBoundaryConditions();
