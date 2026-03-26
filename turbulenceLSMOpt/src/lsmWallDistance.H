if (useInterfaceDerivedWallDistance)
{
    dAlpha2 = dimensionedScalar("zero", dAlpha2.dimensions(), 0.0);
    forAll(dAlpha, cellI)
    {
      dAlpha[cellI] = Foam::max(phiLS[cellI], scalar(0.0))*len;
    }
    dAlpha.correctBoundaryConditions();
    activeWallDistanceMode = "interfacePhi";
}
else if (useAuxiliaryPDEWallDistance && !useMeshWaveWallDistanceFallback)
{
    porosityWallDiffusivity = xh + dimensionedScalar("eps", dimless, dAlphaPorosityEps);
    const auto dAlphaLinearSolvePerformance =
    solve
    (
        -fvm::laplacian(porosityWallDiffusivity, dAlpha2)
      ==
        dimensionedScalar("one", dimless, 1.0)
    );
    recordLinearSolve
    (
      dAlphaLinearSolveStats,
      dAlphaLinearSolvePerformance.initialResidual(),
      dAlphaLinearSolvePerformance.finalResidual(),
      dAlphaLinearSolvePerformance.nIterations()
    );
    forAll(dAlpha, i)
    {
      dAlpha[i] = Foam::sqrt(Foam::max(dAlpha2[i], scalar(0.0)));
    }
    dAlpha.correctBoundaryConditions();
    activeWallDistanceMode = "interfacePDE";
}
else
{
    dAlpha2 = dimensionedScalar("zero", dAlpha2.dimensions(), 0.0);
    dAlpha = dimensionedScalar("zero", dAlpha.dimensions(), 0.0);
    activeWallDistanceMode = "meshWave";
}
