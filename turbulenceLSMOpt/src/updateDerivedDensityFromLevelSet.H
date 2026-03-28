const scalar phiSmoothingWidth = Foam::max(phiBandWidthCells*len, scalar(0.5*len));

forAll(phiLS, cellI)
{
    const scalar phiValue = phiLS[cellI];
    scalar heavisideValue(0.0);
    scalar deltaValue(0.0);

    if (phiValue <= -phiSmoothingWidth)
    {
        heavisideValue = 0.0;
    }
    else if (phiValue >= phiSmoothingWidth)
    {
        heavisideValue = 1.0;
    }
    else
    {
        const scalar scaledPhi = phiValue/phiSmoothingWidth;
        heavisideValue =
            0.5
          * (
                1.0
              + scaledPhi
              + Foam::sin(constant::mathematical::pi*scaledPhi)
               /constant::mathematical::pi
            );
        deltaValue =
            0.5
          * (
                1.0
              + Foam::cos(constant::mathematical::pi*scaledPhi)
            );
    }

    xhLevelSet[cellI] = Foam::max(Foam::min(heavisideValue, scalar(1.0)), scalar(0.0));
    phiInterfaceDelta[cellI] = Foam::max(deltaValue, scalar(0.0));
    phiFluidDistance[cellI] = Foam::max(phiValue, scalar(0.0));
    phiSolidDistance[cellI] = Foam::max(-phiValue, scalar(0.0));
}

forAll(xh, cellI)
{
    const scalar blendedValue =
        hybridStageState == HYBRID_STAGE_DENSITY
      ? xh[cellI]
      : lsmBlendWeight*xhDensityReference[cellI] + (1.0 - lsmBlendWeight)*xhLevelSet[cellI];

    xh[cellI] = Foam::max(Foam::min(blendedValue, scalar(1.0)), scalar(0.0));
    xp[cellI] = xh[cellI];
    x[cellI] = xh[cellI];
}

if (solid_area)
{
    forAll(cells_solid, cellI)
    {
        x[cells_solid[cellI]] = 0.0;
        xp[cells_solid[cellI]] = 0.0;
        xh[cells_solid[cellI]] = 0.0;
        xhLevelSet[cells_solid[cellI]] = 0.0;
    }
}
if (fluid_area)
{
    forAll(cells_fluid, cellI)
    {
        x[cells_fluid[cellI]] = 1.0;
        xp[cells_fluid[cellI]] = 1.0;
        xh[cells_fluid[cellI]] = 1.0;
        xhLevelSet[cells_fluid[cellI]] = 1.0;
    }
}
if (test_area)
{
    forAll(cells_test, cellI)
    {
        x[cells_test[cellI]] = 1.0;
        xp[cells_test[cellI]] = 1.0;
        xh[cells_test[cellI]] = 1.0;
        xhLevelSet[cells_test[cellI]] = 1.0;
    }
}

x.correctBoundaryConditions();
xp.correctBoundaryConditions();
xh.correctBoundaryConditions();
xhLevelSet.correctBoundaryConditions();
phiInterfaceDelta.correctBoundaryConditions();
phiFluidDistance.correctBoundaryConditions();
phiSolidDistance.correctBoundaryConditions();
