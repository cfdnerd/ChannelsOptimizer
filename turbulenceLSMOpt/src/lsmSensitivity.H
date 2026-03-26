forAll(interfaceObjectiveSensitivity, cellI)
{
  const scalar diracValue = diracPhiLS[cellI];
  interfaceObjectiveSensitivity[cellI] = -fsenshMeanT[cellI]*diracValue;
  interfacePowerSensitivity[cellI] = -gsenshPowerDiss[cellI]*diracValue;
  interfaceVolumeSensitivity[cellI] = -gsensVolRaw[cellI]*diracValue;
}

forAll(totalPhiSensitivity, cellI)
{
  totalPhiSensitivity[cellI] =
      interfaceObjectiveSensitivity[cellI]
    + gx[0]*interfacePowerSensitivity[cellI]
    + gx[1]*interfaceVolumeSensitivity[cellI];
}

interfaceObjectiveSensitivity.correctBoundaryConditions();
interfacePowerSensitivity.correctBoundaryConditions();
interfaceVolumeSensitivity.correctBoundaryConditions();
totalPhiSensitivity.correctBoundaryConditions();
