const scalar qAlphaActive = useSplitRAMPControls ? qAlpha : qSingle;
const scalar qKappaActive = useSplitRAMPControls ? qKappa : qSingle;
const scalar qHeatActive = useSplitRAMPControls ? qHeat : qSingle;

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
if (useTurbulentThermalDiffusivity)
{
  turbulentDT = turbulence->nut()/Foam::max(Prt, SMALL);
}
DT = (ks + (kf - ks)*xh*(1.0 + qKappaActive)/(qKappaActive + xh))/rhoCp + turbulentDT;

nuEffAdjoint = nu + turbulence->nut();
alpha.correctBoundaryConditions();
turbulenceSinkCoeff.correctBoundaryConditions();
thermalSourceField.correctBoundaryConditions();
turbulentDT.correctBoundaryConditions();
DT.correctBoundaryConditions();
nuEffAdjoint.correctBoundaryConditions();
