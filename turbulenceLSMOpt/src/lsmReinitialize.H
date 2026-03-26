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
    phiGradDeviationMax =
      Foam::max(phiGradDeviationMax, Foam::mag(Foam::mag(gradPhiField[cellI]) - 1.0));
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
  const scalar reinitStepSize = 0.3*epsilonLSMActive;
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
      const scalar signValue =
        phiReference[cellI]
       /Foam::sqrt(phiReference[cellI]*phiReference[cellI] + epsilonLSMActive*epsilonLSMActive);
      const scalar gradMag = Foam::mag(gradPhiField[cellI]);
      phiLS[cellI] -= reinitStepSize*signValue*(gradMag - 1.0);
    }
    phiLS.correctBoundaryConditions();
  }
}
