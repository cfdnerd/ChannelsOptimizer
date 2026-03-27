forAll(interfaceObjectiveSensitivity, cellI)
{
  const scalar diracValue = diracPhiLS[cellI];
  interfaceObjectiveSensitivity[cellI] = -fsensMeanT[cellI]*diracValue;
  interfacePowerSensitivity[cellI] = -gsensPowerDiss[cellI]*diracValue;
  interfaceVolumeSensitivity[cellI] = -gsensVol[cellI]*diracValue;
}

const scalar activePowerConstraintWeight =
  gx[0] > 0.0 ? effectivePowerConstraintWeight : scalar(0.0);
const scalar activeVolumeConstraintWeight =
  gx[1] > 0.0 ? optVolumeConstraintWeight : scalar(0.0);

forAll(totalPhiSensitivity, cellI)
{
  totalPhiSensitivity[cellI] =
      interfaceObjectiveSensitivity[cellI]
    + activePowerConstraintWeight*interfacePowerSensitivity[cellI]
    + activeVolumeConstraintWeight*interfaceVolumeSensitivity[cellI];
}

interfaceObjectiveSensitivity.correctBoundaryConditions();
interfacePowerSensitivity.correctBoundaryConditions();
interfaceVolumeSensitivity.correctBoundaryConditions();
totalPhiSensitivity.correctBoundaryConditions();
