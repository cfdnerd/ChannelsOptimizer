forAll(interfaceObjectiveSensitivity, cellI)
{
  const scalar diracValue = diracPhiLS[cellI];
  interfaceObjectiveSensitivity[cellI] = -fsensMeanT[cellI]*diracValue;
  interfacePowerSensitivity[cellI] = -gsensPowerDiss[cellI]*diracValue;
  interfaceVolumeSensitivity[cellI] = -gsensVol[cellI]*diracValue;
}

forAll(totalPhiSensitivity, cellI)
{
  totalPhiSensitivity[cellI] =
      interfaceObjectiveSensitivity[cellI]
    + effectivePowerConstraintWeight*interfacePowerSensitivity[cellI]
    + optVolumeConstraintWeight*interfaceVolumeSensitivity[cellI];
}

interfaceObjectiveSensitivity.correctBoundaryConditions();
interfacePowerSensitivity.correctBoundaryConditions();
interfaceVolumeSensitivity.correctBoundaryConditions();
totalPhiSensitivity.correctBoundaryConditions();
