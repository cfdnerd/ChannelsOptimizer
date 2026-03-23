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
