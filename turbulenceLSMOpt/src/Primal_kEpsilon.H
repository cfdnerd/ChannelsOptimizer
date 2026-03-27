if (useKEpsilonModel)
{
    const scalar nutCap = maxTurbulentViscosity;
    turbulence->correct();

    if
    (
      useBrinkmanSinkInKEpsilon
      && mesh.foundObject<volScalarField>("k")
      && mesh.foundObject<volScalarField>("epsilon")
      && mesh.foundObject<volScalarField>("nut")
    )
    {
        volScalarField& kField = const_cast<volScalarField&>(mesh.lookupObject<volScalarField>("k"));
        volScalarField& epsilonField = const_cast<volScalarField&>(mesh.lookupObject<volScalarField>("epsilon"));
        volScalarField& nutField = const_cast<volScalarField&>(mesh.lookupObject<volScalarField>("nut"));

        forAll(kField, cellI)
        {
            const scalar damping = Foam::max(xh[cellI], scalar(1e-6));
            kField[cellI] = Foam::max(kField[cellI]*damping, scalar(1e-12));
            epsilonField[cellI] = Foam::max(epsilonField[cellI]*damping, scalar(1e-12));
            nutField[cellI] =
              Foam::min(Foam::max(nutField[cellI]*damping, scalar(0.0)), nutCap);
        }

        kField.correctBoundaryConditions();
        epsilonField.correctBoundaryConditions();
        nutField.correctBoundaryConditions();
    }

    turbulence->correct();

    if (mesh.foundObject<volScalarField>("nut"))
    {
        volScalarField& nutField = const_cast<volScalarField&>(mesh.lookupObject<volScalarField>("nut"));
        forAll(nutField, cellI)
        {
            scalar nutValue = nutField[cellI];
            if (!std::isfinite(static_cast<double>(nutValue)))
            {
                nutValue = 0.0;
            }
            nutField[cellI] = Foam::min(Foam::max(nutValue, scalar(0.0)), nutCap);
        }
        nutField.correctBoundaryConditions();
    }

    activeTurbulenceModelMode = "kEpsilon";
}
