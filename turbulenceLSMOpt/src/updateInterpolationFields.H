qAlphaActive = useSplitRAMPControls ? qAlpha : qSingle;
qKappaActive = useSplitRAMPControls ? qKappa : qSingle;
qHeatActive = useSplitRAMPControls ? qHeat : qSingle;

alpha = alphaMax*qAlphaActive*(1.0 - xh)/(qAlphaActive + xh);
alphaSensCoeff = -alphaMax*(1.0 + qAlphaActive)*qAlphaActive/((qAlphaActive + xh)*(qAlphaActive + xh));
kappaSensCoeff = (kf - ks)*(1.0 + qKappaActive)*qKappaActive/((qKappaActive + xh)*(qKappaActive + xh))/rhoCp;
heatSourceSensCoeff = -(1.0 + qHeatActive)*qHeatActive/((qHeatActive + xh)*(qHeatActive + xh));

turbulenceSinkCoeff = dimensionedScalar("zero", alpha.dimensions(), 0.0);
if (useBrinkmanSinkInKEpsilon)
{
  turbulenceSinkCoeff = alpha;
}
thermalSourceField = thermalSource;
if (DDHS)
{
  thermalSourceField = thermalSource*(qHeatActive*(1.0 - xh)/(qHeatActive + xh));
}

turbulentDT = dimensionedScalar("zero", DT.dimensions(), 0.0);
{
  volScalarField limitedNut(turbulence->nut());
  forAll(limitedNut, cellI)
  {
    scalar nutValue = limitedNut[cellI];
    if (!std::isfinite(static_cast<double>(nutValue)))
    {
      nutValue = 0.0;
    }
    limitedNut[cellI] = Foam::min(Foam::max(nutValue, scalar(0.0)), maxTurbulentViscosity);
  }
  limitedNut.correctBoundaryConditions();

  if (useTurbulentThermalDiffusivity)
  {
    turbulentDT = limitedNut/Foam::max(Prt, SMALL);
  }
  DT = (ks + (kf - ks)*xh*(1.0 + qKappaActive)/(qKappaActive + xh))/rhoCp + turbulentDT;

  nuEffAdjoint = nu + limitedNut;
}
alpha.correctBoundaryConditions();
turbulenceSinkCoeff.correctBoundaryConditions();
thermalSourceField.correctBoundaryConditions();
turbulentDT.correctBoundaryConditions();
DT.correctBoundaryConditions();
nuEffAdjoint.correctBoundaryConditions();
