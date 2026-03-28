solidMinConstraintValue = 0.0;
solidMaxConstraintValue = 0.0;

solidIndicator = dimensionedScalar("one", dimless, 1.0) - xh;
solidIndicator.correctBoundaryConditions();

solidMinFiltered = dimensionedScalar("zero", dimless, 0.0);
solidMaxFiltered = dimensionedScalar("zero", dimless, 0.0);
solidMinViolation = dimensionedScalar("zero", dimless, 0.0);
solidMaxViolation = dimensionedScalar("zero", dimless, 0.0);
gsensSolidMinGeom = dimensionedScalar("zero", dimless, 0.0);
gsensSolidMaxGeom = dimensionedScalar("zero", dimless, 0.0);
gsenshSolidMin = dimensionedScalar("zero", dimless, 0.0);
gsenshSolidMax = dimensionedScalar("zero", dimless, 0.0);
gsensSolidMin = dimensionedScalar("zero", dimless, 0.0);
gsensSolidMax = dimensionedScalar("zero", dimless, 0.0);

const scalar geometricSizeSmoothPositiveEpsSqr =
  geometricSizeSmoothPositiveEps*geometricSizeSmoothPositiveEps;

auto smoothPositivePart =
[&](const scalar z) -> scalar
{
  return 0.5*(z + std::sqrt(z*z + geometricSizeSmoothPositiveEpsSqr));
};

auto smoothPositivePartDerivative =
[&](const scalar z) -> scalar
{
  const scalar denom = std::sqrt(z*z + geometricSizeSmoothPositiveEpsSqr);
  return 0.5*(1.0 + z/Foam::max(denom, SMALL));
};

if (useMinSolidGeometryControl)
{
  const auto solidMinGeomFilterLinearSolvePerformance =
    solve(fvm::laplacian(solidMinFiltered) - fvm::Sp(bSolidMinGeom, solidMinFiltered) + solidIndicator*bSolidMinGeom);
  recordLinearSolve
  (
    solidMinGeomFilterLinearSolveStats,
    solidMinGeomFilterLinearSolvePerformance.initialResidual(),
    solidMinGeomFilterLinearSolvePerformance.finalResidual(),
    solidMinGeomFilterLinearSolvePerformance.nIterations()
  );

  scalar solidMinLocalViolationMax = -GREAT;
  forAll(solidMinFiltered, cellI)
  {
    const scalar z = minSolidGeometryFillFraction - solidMinFiltered[cellI];
    solidMinViolation[cellI] = solidIndicator[cellI]*smoothPositivePart(z);
    solidMinLocalViolationMax = Foam::max(solidMinLocalViolationMax, solidMinViolation[cellI]);
  }
  reduce(solidMinLocalViolationMax, maxOp<scalar>());

  scalar solidMinViolationKSSumLocal = 0.0;
  forAll(solidMinViolation, cellI)
  {
    solidMinViolationKSSumLocal +=
      mesh.V()[cellI]*std::exp
      (
        geometricSizeAggregationKSFactor
        *(solidMinViolation[cellI] - solidMinLocalViolationMax)
      );
  }
  reduce(solidMinViolationKSSumLocal, sumOp<scalar>());

  const scalar solidMinViolationKSSum =
    Foam::max(solidMinViolationKSSumLocal, SMALL);
  const scalar solidMinViolationKSAverage =
    area > SMALL ? solidMinViolationKSSum/area : solidMinViolationKSSum;
  solidMinConstraintValue =
    solidMinLocalViolationMax
    + std::log(Foam::max(solidMinViolationKSAverage, SMALL))/geometricSizeAggregationKSFactor
    - minSolidGeometryConstraintTolerance;

  volScalarField solidMinAdjointSource
  (
      IOobject
      (
          "solidMinAdjointSource",
          runTime.timeName(),
          mesh,
          IOobject::NO_READ,
          IOobject::NO_WRITE
      ),
      mesh,
      dimensionedScalar("zero", dimless, 0.0),
      zeroGradientFvPatchScalarField::typeName
  );

  forAll(solidMinFiltered, cellI)
  {
    const scalar z = minSolidGeometryFillFraction - solidMinFiltered[cellI];
    const scalar positivePart = smoothPositivePart(z);
    const scalar dPositivePartDz = smoothPositivePartDerivative(z);
    const scalar ksWeight =
      mesh.V()[cellI]
      *std::exp(geometricSizeAggregationKSFactor*(solidMinViolation[cellI] - solidMinLocalViolationMax))
      /solidMinViolationKSSum;
    const scalar dConstraintDLocalViolation = ksWeight;
    const scalar dLocalViolationDSolid = positivePart;
    const scalar dLocalViolationDSolidMinFiltered =
      -solidIndicator[cellI]*dPositivePartDz;

    gsenshSolidMin[cellI] =
      -dConstraintDLocalViolation*dLocalViolationDSolid;
    solidMinAdjointSource[cellI] =
      dConstraintDLocalViolation*dLocalViolationDSolidMinFiltered;
  }

  const auto solidMinGeomAdjointLinearSolvePerformance =
    solve(fvm::laplacian(gsensSolidMinGeom) - fvm::Sp(bSolidMinGeom, gsensSolidMinGeom) + solidMinAdjointSource*bSolidMinGeom);
  recordLinearSolve
  (
    solidMinGeomAdjointLinearSolveStats,
    solidMinGeomAdjointLinearSolvePerformance.initialResidual(),
    solidMinGeomAdjointLinearSolvePerformance.finalResidual(),
    solidMinGeomAdjointLinearSolvePerformance.nIterations()
  );

  gsenshSolidMin -= gsensSolidMinGeom;
}

if (useMaxSolidGeometryControl)
{
  const auto solidMaxGeomFilterLinearSolvePerformance =
    solve(fvm::laplacian(solidMaxFiltered) - fvm::Sp(bSolidMaxGeom, solidMaxFiltered) + solidIndicator*bSolidMaxGeom);
  recordLinearSolve
  (
    solidMaxGeomFilterLinearSolveStats,
    solidMaxGeomFilterLinearSolvePerformance.initialResidual(),
    solidMaxGeomFilterLinearSolvePerformance.finalResidual(),
    solidMaxGeomFilterLinearSolvePerformance.nIterations()
  );

  scalar solidMaxLocalViolationMax = -GREAT;
  forAll(solidMaxFiltered, cellI)
  {
    const scalar z = solidMaxFiltered[cellI] - maxSolidGeometryFillFraction;
    solidMaxViolation[cellI] = smoothPositivePart(z);
    solidMaxLocalViolationMax = Foam::max(solidMaxLocalViolationMax, solidMaxViolation[cellI]);
  }
  reduce(solidMaxLocalViolationMax, maxOp<scalar>());

  scalar solidMaxViolationKSSumLocal = 0.0;
  forAll(solidMaxViolation, cellI)
  {
    solidMaxViolationKSSumLocal +=
      mesh.V()[cellI]*std::exp
      (
        geometricSizeAggregationKSFactor
        *(solidMaxViolation[cellI] - solidMaxLocalViolationMax)
      );
  }
  reduce(solidMaxViolationKSSumLocal, sumOp<scalar>());

  const scalar solidMaxViolationKSSum =
    Foam::max(solidMaxViolationKSSumLocal, SMALL);
  const scalar solidMaxViolationKSAverage =
    area > SMALL ? solidMaxViolationKSSum/area : solidMaxViolationKSSum;
  solidMaxConstraintValue =
    solidMaxLocalViolationMax
    + std::log(Foam::max(solidMaxViolationKSAverage, SMALL))/geometricSizeAggregationKSFactor
    - maxSolidGeometryConstraintTolerance;

  volScalarField solidMaxAdjointSource
  (
      IOobject
      (
          "solidMaxAdjointSource",
          runTime.timeName(),
          mesh,
          IOobject::NO_READ,
          IOobject::NO_WRITE
      ),
      mesh,
      dimensionedScalar("zero", dimless, 0.0),
      zeroGradientFvPatchScalarField::typeName
  );

  forAll(solidMaxFiltered, cellI)
  {
    const scalar z = solidMaxFiltered[cellI] - maxSolidGeometryFillFraction;
    const scalar dPositivePartDz = smoothPositivePartDerivative(z);
    const scalar ksWeight =
      mesh.V()[cellI]
      *std::exp(geometricSizeAggregationKSFactor*(solidMaxViolation[cellI] - solidMaxLocalViolationMax))
      /solidMaxViolationKSSum;

    solidMaxAdjointSource[cellI] = ksWeight*dPositivePartDz;
  }

  const auto solidMaxGeomAdjointLinearSolvePerformance =
    solve(fvm::laplacian(gsensSolidMaxGeom) - fvm::Sp(bSolidMaxGeom, gsensSolidMaxGeom) + solidMaxAdjointSource*bSolidMaxGeom);
  recordLinearSolve
  (
    solidMaxGeomAdjointLinearSolveStats,
    solidMaxGeomAdjointLinearSolvePerformance.initialResidual(),
    solidMaxGeomAdjointLinearSolvePerformance.finalResidual(),
    solidMaxGeomAdjointLinearSolvePerformance.nIterations()
  );

  gsenshSolidMax = -gsensSolidMaxGeom;
}
