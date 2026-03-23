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

scalar TLocalMin = GREAT;
scalar TLocalMax = -GREAT;
label TNonFiniteLocal = 0;
forAll(T, cellI)
{
    const scalar value = T[cellI];
    if (!std::isfinite(static_cast<double>(value)))
    {
        TNonFiniteLocal++;
        continue;
    }
    TLocalMin = Foam::min(TLocalMin, value);
    TLocalMax = Foam::max(TLocalMax, value);
}
reduce(TLocalMin, minOp<scalar>());
reduce(TLocalMax, maxOp<scalar>());
reduce(TNonFiniteLocal, sumOp<label>());

scalar DTLocalMin = GREAT;
scalar DTLocalMax = -GREAT;
label DTNonFiniteLocal = 0;
forAll(DT, cellI)
{
    const scalar value = DT[cellI];
    if (!std::isfinite(static_cast<double>(value)))
    {
        DTNonFiniteLocal++;
        continue;
    }
    DTLocalMin = Foam::min(DTLocalMin, value);
    DTLocalMax = Foam::max(DTLocalMax, value);
}
reduce(DTLocalMin, minOp<scalar>());
reduce(DTLocalMax, maxOp<scalar>());
reduce(DTNonFiniteLocal, sumOp<label>());

Info<< "primal thermal diagnostics: T[min,max]=(" << TLocalMin << ", " << TLocalMax
    << ") nonFinite=" << TNonFiniteLocal
    << " DT[min,max]=(" << DTLocalMin << ", " << DTLocalMax
    << ") nonFinite=" << DTNonFiniteLocal << endl;

if (TNonFiniteLocal > 0 || DTNonFiniteLocal > 0 || DTLocalMin <= 0)
{
    FatalErrorInFunction
        << "Invalid thermal state before adjoint stage. "
        << "TNonFinite=" << TNonFiniteLocal
        << ", DTNonFinite=" << DTNonFiniteLocal
        << ", DTMin=" << DTLocalMin
        << exit(FatalError);
}
