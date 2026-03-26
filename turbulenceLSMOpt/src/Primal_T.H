Info<< "primal thermal solve" << endl;
T=T2;
fvScalarMatrix TEqn
(
    fvm::div(phi,T)
   -fvm::laplacian(DT, T)
);
TEqn -= thermalSourceField;
const auto TLinearSolvePerformance = TEqn.solve();
recordLinearSolve
(
  TLinearSolveStats,
  TLinearSolvePerformance.initialResidual(),
  TLinearSolvePerformance.finalResidual(),
  TLinearSolvePerformance.nIterations()
);
{
    scalar primalThermalTMin = GREAT;
    scalar primalThermalTMax = -GREAT;
    label primalThermalTNonFinite = 0;
    forAll(T, cellI)
    {
        const scalar value = T[cellI];
        if (!std::isfinite(static_cast<double>(value)))
        {
            primalThermalTNonFinite++;
            continue;
        }
        primalThermalTMin = Foam::min(primalThermalTMin, value);
        primalThermalTMax = Foam::max(primalThermalTMax, value);
    }
    reduce(primalThermalTMin, minOp<scalar>());
    reduce(primalThermalTMax, maxOp<scalar>());
    reduce(primalThermalTNonFinite, sumOp<label>());

    scalar primalThermalDTMin = GREAT;
    scalar primalThermalDTMax = -GREAT;
    label primalThermalDTNonFinite = 0;
    forAll(DT, cellI)
    {
        const scalar value = DT[cellI];
        if (!std::isfinite(static_cast<double>(value)))
        {
            primalThermalDTNonFinite++;
            continue;
        }
        primalThermalDTMin = Foam::min(primalThermalDTMin, value);
        primalThermalDTMax = Foam::max(primalThermalDTMax, value);
    }
    reduce(primalThermalDTMin, minOp<scalar>());
    reduce(primalThermalDTMax, maxOp<scalar>());
    reduce(primalThermalDTNonFinite, sumOp<label>());

    Info<< "primal thermal diagnostics: T[min,max]=(" << primalThermalTMin << ", " << primalThermalTMax
        << ") nonFinite=" << primalThermalTNonFinite
        << " DT[min,max]=(" << primalThermalDTMin << ", " << primalThermalDTMax
        << ") nonFinite=" << primalThermalDTNonFinite << endl;

    if (primalThermalTNonFinite > 0 || primalThermalDTNonFinite > 0 || primalThermalDTMin <= 0)
    {
        FatalErrorInFunction
            << "Invalid thermal state before adjoint stage. "
            << "TNonFinite=" << primalThermalTNonFinite
            << ", DTNonFinite=" << primalThermalDTNonFinite
            << ", DTMin=" << primalThermalDTMin
            << exit(FatalError);
    }
}
