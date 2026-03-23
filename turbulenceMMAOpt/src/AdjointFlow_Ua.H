Info<< "adjoint flow momentum solve" << endl;
for(i=0;i<adjointMomentumSweeps;i++)
{ 
  volVectorField adjointTransposeConvectiona((fvc::grad(U) & Ua));
  volVectorField explicitAdjointStressCorrectionA
  (
      IOobject
      (
          "explicitAdjointStressCorrectionA",
          runTime.timeName(),
          mesh,
          IOobject::NO_READ,
          IOobject::NO_WRITE
      ),
      mesh,
      dimensionedVector("zero", dimensionSet(0,1,-2,0,0,0,0), vector::zero)
  );
  if (useFullAdjointSymmetricStress)
  {
      explicitAdjointStressCorrectionA =
        -fvc::div(nuEffAdjoint*Foam::T(fvc::grad(Ua)));
  }

  tmp<fvVectorMatrix> tUaEqn
  (
      fvm::div(-phi, Ua)
    + adjointTransposeConvectiona
    - fvm::laplacian(nuEffAdjoint,Ua)
    + explicitAdjointStressCorrectionA
    + fvm::Sp(alpha, Ua)
   ==
      fvOptions(Ua)
  );
  fvVectorMatrix& UaEqn = tUaEqn.ref();

  UaEqn.relax();

  fvOptions.constrain(UaEqn);

  const auto UaLinearSolvePerformance = solve(UaEqn == -fvc::grad(pa));
  recordLinearSolve
  (
      UaLinearSolveStats,
      cmptMax(UaLinearSolvePerformance.initialResidual()),
      cmptMax(UaLinearSolvePerformance.finalResidual()),
      cmptMax(UaLinearSolvePerformance.nIterations())
  );

  fvOptions.correct(Ua);
//****************************************
  volScalarField rAUa(1.0/UaEqn.A());
  volVectorField HbyAa("HbyAa", Ua);
  HbyAa = rAUa*UaEqn.H();
  tUaEqn.clear();          
  surfaceScalarField phiHbyAa("phiHbyAa", fvc::flux(HbyAa));
  adjustPhi(phiHbyAa, Ua, pa);
  
  tmp<volScalarField> rAtUa(rAUa);
  
   if (simple.consistent())
  {
      rAtUa = 1.0/(1.0/rAUa - UaEqn.H1());
      phiHbyAa +=
          fvc::interpolate(rAtUa() - rAUa)*fvc::snGrad(pa)*mesh.magSf();
      HbyAa -= (rAUa - rAtUa())*fvc::grad(pa);
  }

  tUaEqn.clear();

  // Update the pressure BCs to ensure flux consistency
  constrainPressure(pa, Ua, phiHbyAa, rAtUa(), MRF);           
  // Non-orthogonal pressure corrector loop
  while (simple.correctNonOrthogonal())
  {
      fvScalarMatrix paEqn
      (
          fvm::laplacian(rAUa, pa) == fvc::div(phiHbyAa)
      );

      paEqn.setReference(paRefCell, paRefValue);
      const auto paLinearSolvePerformance = paEqn.solve();
      recordLinearSolve
      (
          paLinearSolveStats,
          paLinearSolvePerformance.initialResidual(),
          paLinearSolvePerformance.finalResidual(),
          paLinearSolvePerformance.nIterations()
      );

      if (simple.finalNonOrthogonalIter())
      {
          phia = phiHbyAa - paEqn.flux();
      }
  }

  //#include "adjointContinuityErrs.H"

  // Explicitly relax pressure for adjoint momentum corrector
  pa.relax();

  // Adjoint momentum corrector
  Ua = HbyAa - rAUa*fvc::grad(pa);
  Ua.correctBoundaryConditions();
  fvOptions.correct(Ua);
  
  Ua.storePrevIter();
  pa.storePrevIter();
  phia.storePrevIter();
}
{
    scalar adjointFlowUaMaxMag = 0;
    label adjointFlowUaNonFinite = 0;
    forAll(Ua, cellI)
    {
        const scalar magValue = mag(Ua[cellI]);
        if (!std::isfinite(static_cast<double>(magValue)))
        {
            adjointFlowUaNonFinite++;
            continue;
        }
        adjointFlowUaMaxMag = Foam::max(adjointFlowUaMaxMag, magValue);
    }
    reduce(adjointFlowUaMaxMag, maxOp<scalar>());
    reduce(adjointFlowUaNonFinite, sumOp<label>());

    scalar adjointFlowPaMin = GREAT;
    scalar adjointFlowPaMax = -GREAT;
    label adjointFlowPaNonFinite = 0;
    forAll(pa, cellI)
    {
        const scalar value = pa[cellI];
        if (!std::isfinite(static_cast<double>(value)))
        {
            adjointFlowPaNonFinite++;
            continue;
        }
        adjointFlowPaMin = Foam::min(adjointFlowPaMin, value);
        adjointFlowPaMax = Foam::max(adjointFlowPaMax, value);
    }
    reduce(adjointFlowPaMin, minOp<scalar>());
    reduce(adjointFlowPaMax, maxOp<scalar>());
    reduce(adjointFlowPaNonFinite, sumOp<label>());

    Info<< "adjoint flow momentum solve complete: |Ua|max=" << adjointFlowUaMaxMag
        << " UaNonFinite=" << adjointFlowUaNonFinite
        << " pa[min,max]=(" << adjointFlowPaMin << ", " << adjointFlowPaMax
        << ") paNonFinite=" << adjointFlowPaNonFinite << endl;
}
