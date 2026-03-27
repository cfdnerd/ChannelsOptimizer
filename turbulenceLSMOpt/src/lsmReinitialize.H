const scalar reinitPhiMagnitudeCap = 4.0*Foam::max(epsilonLSMActive, scalar(SMALL));
forAll(phiLS, cellI)
{
  scalar phiValue = phiLS[cellI];
  if (!std::isfinite(static_cast<double>(phiValue)))
  {
    phiValue = 0.0;
  }
  phiLS[cellI] = Foam::max(Foam::min(phiValue, reinitPhiMagnitudeCap), -reinitPhiMagnitudeCap);
}
phiLS.correctBoundaryConditions();

phiGradDeviationMax = 0.0;
{
  volVectorField gradPhiField
  (
      IOobject
      (
          "gradPhiReinitField",
          runTime.timeName(),
          mesh,
          IOobject::NO_READ,
          IOobject::NO_WRITE
      ),
      fvc::grad(phiLS)
  );
  forAll(phiLS, cellI)
  {
    const scalar gradMag = len*Foam::mag(gradPhiField[cellI]);
    phiGradDeviationMax =
      Foam::max(phiGradDeviationMax, Foam::mag(gradMag - 1.0));
  }
  reduce(phiGradDeviationMax, maxOp<scalar>());
}

phiReinitResidual = phiGradDeviationMax;
const bool triggerReinitialization =
  useSussmanReinitialization
  && ((reinitInterval > 0 && (opt % reinitInterval == 0)) || phiGradDeviationMax > reinitTolerance);
if (triggerReinitialization)
{
  volScalarField phiReference(phiLS);
  const scalar reinitStepSize = 0.2*epsilonLSMActive;
  const scalar reinitPhiStepCap = 0.5*Foam::max(epsilonLSMActive, scalar(SMALL));
  for (label reinitStep = 0; reinitStep < reinitPseudoSteps; ++reinitStep)
  {
    volVectorField gradPhiField
    (
        IOobject
        (
            "gradPhiReinitStepField",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        fvc::grad(phiLS)
    );
    forAll(phiLS, cellI)
    {
      const scalar phiRef = phiReference[cellI];
      const scalar epsAbs = Foam::max(Foam::mag(epsilonLSMActive), scalar(SMALL));
      const scalar absPhiRef = Foam::mag(phiRef);
      scalar safeDenom(0.0);
      if (absPhiRef > epsAbs)
      {
        const scalar ratio = epsAbs/absPhiRef;
        safeDenom = absPhiRef*Foam::sqrt(1.0 + ratio*ratio);
      }
      else
      {
        const scalar ratio = absPhiRef/epsAbs;
        safeDenom = epsAbs*Foam::sqrt(1.0 + ratio*ratio);
      }
      const scalar signValue = phiRef/Foam::max(safeDenom, scalar(SMALL));
      const scalar gradMag = len*Foam::mag(gradPhiField[cellI]);
      scalar phiStep = reinitStepSize*signValue*(gradMag - 1.0);
      phiStep = Foam::max(Foam::min(phiStep, reinitPhiStepCap), -reinitPhiStepCap);
      scalar phiValue = phiLS[cellI] - phiStep;
      if (!std::isfinite(static_cast<double>(phiValue)))
      {
        phiValue = signValue*reinitPhiMagnitudeCap;
      }
      phiLS[cellI] =
        Foam::max(Foam::min(phiValue, reinitPhiMagnitudeCap), -reinitPhiMagnitudeCap);
    }
    phiLS.correctBoundaryConditions();
  }
}
