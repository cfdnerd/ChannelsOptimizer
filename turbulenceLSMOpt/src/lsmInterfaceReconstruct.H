forAll(phiLS, cellI)
{
  const scalar phiValue = phiLS[cellI];
  scalar xhValue = 0.0;
  scalar diracValue = 0.0;

  if (phiValue <= -epsilonLSMActive)
  {
    xhValue = 0.0;
  }
  else if (phiValue >= epsilonLSMActive)
  {
    xhValue = 1.0;
  }
  else
  {
    const scalar ratio = phiValue/Foam::max(epsilonLSMActive, SMALL);
    xhValue = 0.5*(1.0 + ratio + Foam::sin(constant::mathematical::pi*ratio)/constant::mathematical::pi);
    diracValue =
      0.5
     *(1.0 + Foam::cos(constant::mathematical::pi*ratio))
     /Foam::max(epsilonLSMActive, SMALL);
  }

  xh[cellI] = Foam::min(Foam::max(xhValue, scalar(0.0)), scalar(1.0));
  diracPhiLS[cellI] = Foam::max(diracValue, scalar(0.0));
}

if(solid_area)
{
  forAll(cells_solid, zoneI)
  {
    const label cellI = cells_solid[zoneI];
    phiLS[cellI] = -2.0*epsilonLSMActive;
    xh[cellI] = 0.0;
    diracPhiLS[cellI] = 0.0;
  }
}
if(fluid_area)
{
  forAll(cells_fluid, zoneI)
  {
    const label cellI = cells_fluid[zoneI];
    phiLS[cellI] = 2.0*epsilonLSMActive;
    xh[cellI] = 1.0;
    diracPhiLS[cellI] = 0.0;
  }
}
if(test_area)
{
  forAll(cells_test, zoneI)
  {
    const label cellI = cells_test[zoneI];
    phiLS[cellI] = 2.0*epsilonLSMActive;
    xh[cellI] = 1.0;
    diracPhiLS[cellI] = 0.0;
  }
}

x = xh;
xp = xh;
drho = diracPhiLS;

phiLS.correctBoundaryConditions();
x.correctBoundaryConditions();
xp.correctBoundaryConditions();
xh.correctBoundaryConditions();
drho.correctBoundaryConditions();
diracPhiLS.correctBoundaryConditions();
