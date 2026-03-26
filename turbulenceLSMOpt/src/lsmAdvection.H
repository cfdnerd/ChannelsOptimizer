scalar maxVelocityMagnitudeLocal(0.0);
forAll(normalVelocity, cellI)
{
  maxVelocityMagnitudeLocal = Foam::max(maxVelocityMagnitudeLocal, Foam::mag(normalVelocity[cellI]));
}
reduce(maxVelocityMagnitudeLocal, maxOp<scalar>());
lsmPseudoTimeStep =
  lsmCFL*epsilonLSMActive/Foam::max(maxVelocityMagnitudeLocal, scalar(1.0));
const scalar advectionPhiMagnitudeCap = 4.0*Foam::max(epsilonLSMActive, scalar(SMALL));

volVectorField gradPhiField
(
    IOobject
    (
        "gradPhiField",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    fvc::grad(phiLS)
);
forAll(phiLS, cellI)
{
  const scalar gradMag = Foam::mag(gradPhiField[cellI]);
  scalar phiValue =
    phiLS[cellI] - lsmPseudoTimeStep*normalVelocity[cellI]*Foam::max(gradMag, scalar(1.0));
  if (!std::isfinite(static_cast<double>(phiValue)))
  {
    phiValue = 0.0;
  }
  phiLS[cellI] =
    Foam::max(Foam::min(phiValue, advectionPhiMagnitudeCap), -advectionPhiMagnitudeCap);
}
phiLS.correctBoundaryConditions();
