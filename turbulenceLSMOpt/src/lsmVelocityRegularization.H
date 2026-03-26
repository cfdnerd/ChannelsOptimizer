forAll(normalVelocityRaw, cellI)
{
  scalar value = -totalPhiSensitivity[cellI];
  if (!std::isfinite(static_cast<double>(value)))
  {
    value = 0.0;
  }
  normalVelocityRaw[cellI] =
    Foam::max(Foam::min(value, maxNormalVelocity), -maxNormalVelocity);
}
normalVelocityRaw.correctBoundaryConditions();

if (useReactionDiffusionLSMUpdate)
{
  const dimensionedScalar velocityMassCoeff("velocityMassCoeff", b.dimensions(), b.value());
  const auto velocityRegularizationSolvePerformance =
    solve
    (
      -fvm::laplacian(dimensionedScalar("one", dimless, 1.0), normalVelocity)
      + fvm::Sp(velocityMassCoeff, normalVelocity)
     ==
      velocityMassCoeff*normalVelocityRaw
    );
  recordLinearSolve
  (
    velocityRegularizationLinearSolveStats,
    velocityRegularizationSolvePerformance.initialResidual(),
    velocityRegularizationSolvePerformance.finalResidual(),
    velocityRegularizationSolvePerformance.nIterations()
  );
  activeLSMUpdateMode = "reactionDiffusion";
}
else
{
  normalVelocity = normalVelocityRaw;
  activeLSMUpdateMode = "hamiltonJacobi";
}

forAll(normalVelocity, cellI)
{
  scalar value = normalVelocity[cellI];
  if (!std::isfinite(static_cast<double>(value)))
  {
    value = 0.0;
  }
  normalVelocity[cellI] =
    Foam::max(Foam::min(value, maxNormalVelocity), -maxNormalVelocity);
}
normalVelocity.correctBoundaryConditions();
