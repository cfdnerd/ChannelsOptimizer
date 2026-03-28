Info<< "adjoint heat momentum solve" << endl;
for(i=0;i<adjointMomentumSweeps;i++)
{    
volVectorField adjointTransposeConvectionb((fvc::grad(U) & Ub));
volVectorField explicitAdjointStressCorrectionB
(
    IOobject
    (
        "explicitAdjointStressCorrectionB",
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
    explicitAdjointStressCorrectionB =
      -adjointExplicitStressScale*fvc::div(nuEffAdjoint*Foam::T(fvc::grad(Ub)));
}

tmp<fvVectorMatrix> tUbEqn
(
    fvm::div(-phi, Ub)
  + adjointTransposeConvectionb
  - fvm::laplacian(nuEffAdjoint,Ub)
  + explicitAdjointStressCorrectionB
  + fvm::Sp(alpha, Ub)
  + b1*Tb*fvc::grad(T)
 ==
    fvOptions(Ub)
);
fvVectorMatrix& UbEqn = tUbEqn.ref();

UbEqn.relax();

fvOptions.constrain(UbEqn);

const auto UbLinearSolvePerformance = solve(UbEqn == -fvc::grad(pb));
recordLinearSolve
(
    UbLinearSolveStats,
    cmptMax(UbLinearSolvePerformance.initialResidual()),
    cmptMax(UbLinearSolvePerformance.finalResidual()),
    cmptMax(UbLinearSolvePerformance.nIterations())
);

fvOptions.correct(Ub);
//****************************************
volScalarField rAUb(1.0/UbEqn.A());
volVectorField HbyAb("HbyAb", Ub);
HbyAb = rAUb*UbEqn.H();
tUbEqn.clear();
surfaceScalarField phiHbyAb("phiHbyAb", fvc::flux(HbyAb));
adjustPhi(phiHbyAb, Ub, pb);

tmp<volScalarField> rAtUb(rAUb);

 if (simple.consistent())
{
    rAtUb = 1.0/(1.0/rAUb - UbEqn.H1());
    phiHbyAb +=
        fvc::interpolate(rAtUb() - rAUb)*fvc::snGrad(pb)*mesh.magSf();
    HbyAb -= (rAUb - rAtUb())*fvc::grad(pb);
}
tUbEqn.clear();
// Update the pressure BCs to ensure flux consistency
constrainPressure(pb, Ub, phiHbyAb, rAtUb(), MRF);  
// Non-orthogonal pressure corrector loop
while (simple.correctNonOrthogonal())
{
    fvScalarMatrix pbEqn
    (
        fvm::laplacian(rAUb, pb) == fvc::div(phiHbyAb)
    );

    pbEqn.setReference(pbRefCell, pbRefValue);
    const auto pbLinearSolvePerformance = pbEqn.solve();
    recordLinearSolve
    (
        pbLinearSolveStats,
        pbLinearSolvePerformance.initialResidual(),
        pbLinearSolvePerformance.finalResidual(),
        pbLinearSolvePerformance.nIterations()
    );

    if (simple.finalNonOrthogonalIter())
    {
        phib = phiHbyAb - pbEqn.flux();
    }
}

//#include "adjointContinuityErrs.H"

// Explicitly relax pressure for adjoint momentum corrector
pb.relax();

// Adjoint momentum corrector
Ub = HbyAb - rAUb*fvc::grad(pb);
Ub.correctBoundaryConditions();
fvOptions.correct(Ub);

Ub.storePrevIter();
pb.storePrevIter();
phib.storePrevIter();
}
{
    scalar adjointHeatUbMaxMag = 0;
    label adjointHeatUbNonFinite = 0;
    forAll(Ub, cellI)
    {
        const scalar magValue = mag(Ub[cellI]);
        if (!std::isfinite(static_cast<double>(magValue)))
        {
            adjointHeatUbNonFinite++;
            continue;
        }
        adjointHeatUbMaxMag = Foam::max(adjointHeatUbMaxMag, magValue);
    }
    reduce(adjointHeatUbMaxMag, maxOp<scalar>());
    reduce(adjointHeatUbNonFinite, sumOp<label>());

    scalar adjointHeatPbMin = GREAT;
    scalar adjointHeatPbMax = -GREAT;
    label adjointHeatPbNonFinite = 0;
    forAll(pb, cellI)
    {
        const scalar value = pb[cellI];
        if (!std::isfinite(static_cast<double>(value)))
        {
            adjointHeatPbNonFinite++;
            continue;
        }
        adjointHeatPbMin = Foam::min(adjointHeatPbMin, value);
        adjointHeatPbMax = Foam::max(adjointHeatPbMax, value);
    }
    reduce(adjointHeatPbMin, minOp<scalar>());
    reduce(adjointHeatPbMax, maxOp<scalar>());
    reduce(adjointHeatPbNonFinite, sumOp<label>());

    Info<< "adjoint heat momentum solve complete: |Ub|max=" << adjointHeatUbMaxMag
        << " UbNonFinite=" << adjointHeatUbNonFinite
        << " pb[min,max]=(" << adjointHeatPbMin << ", " << adjointHeatPbMax
        << ") pbNonFinite=" << adjointHeatPbNonFinite << endl;
}
