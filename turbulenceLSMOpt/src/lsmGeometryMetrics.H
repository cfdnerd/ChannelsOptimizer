channelWidthSupport = dimensionedScalar("zero", dimless, 0.0);
ribThicknessSupport = dimensionedScalar("zero", dimless, 0.0);
channelWidthMetric = dimensionedScalar("zero", dimLength, 0.0);
ribThicknessMetric = dimensionedScalar("zero", dimLength, 0.0);
hydraulicDiameterMetric = dimensionedScalar("zero", dimLength, 0.0);
lsmWidthVelocitySource = dimensionedScalar("zero", lsmWidthVelocitySource.dimensions(), 0.0);
lsmRibVelocitySource = dimensionedScalar("zero", lsmRibVelocitySource.dimensions(), 0.0);
lsmWidthVelocity = dimensionedScalar("zero", lsmWidthVelocity.dimensions(), 0.0);
lsmRibVelocity = dimensionedScalar("zero", lsmRibVelocity.dimensions(), 0.0);

volVectorField gradPhiForCurvature
(
    IOobject
    (
        "gradPhiForCurvature",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    fvc::grad(phiLS)
);
volVectorField phiUnitNormal
(
    IOobject
    (
        "phiUnitNormal",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    gradPhiForCurvature
);

forAll(phiLS, cellI)
{
    const scalar phiValue = phiLS[cellI];
    phiFluidDistance[cellI] = Foam::max(phiValue, scalar(0.0));
    phiSolidDistance[cellI] = Foam::max(-phiValue, scalar(0.0));
    channelWidthMetric[cellI] = 2.0*phiFluidDistance[cellI];
    ribThicknessMetric[cellI] = 2.0*phiSolidDistance[cellI];

    const scalar normalMag = Foam::mag(gradPhiForCurvature[cellI]);
    phiUnitNormal[cellI] =
        normalMag > SMALL
      ? gradPhiForCurvature[cellI]/normalMag
      : vector::zero;
}

lsmCurvature = fvc::div(phiUnitNormal);

const scalar supportTol = Foam::max(0.10*len, scalar(1e-9));
const scalar supportPeakTol = Foam::max(1e-4*len, scalar(1e-12));
label widthSupportCountLocal(0);
label ribSupportCountLocal(0);
scalar widthMinObservedLocal(GREAT);
scalar widthMaxObservedLocal(0.0);
scalar ribMinObservedLocal(GREAT);
scalar ribMaxObservedLocal(0.0);
scalar hydraulicDiameterSumLocal(0.0);
scalar hydraulicDiameterWeightLocal(0.0);
scalar widthMinMaxViolationLocal(-GREAT), widthMinExpSumLocal(0.0);
scalar widthMaxMaxViolationLocal(-GREAT), widthMaxExpSumLocal(0.0);
scalar ribMinMaxViolationLocal(-GREAT), ribMinExpSumLocal(0.0);
scalar ribMaxMaxViolationLocal(-GREAT), ribMaxExpSumLocal(0.0);
scalar curvatureMaxViolationLocal(-GREAT), curvatureExpSumLocal(0.0);

forAll(phiLS, cellI)
{
    const labelList& neighbours = cellCells[cellI];
    bool fluidSupport = phiFluidDistance[cellI] > supportTol;
    bool solidSupport = phiSolidDistance[cellI] > supportTol;

    forAll(neighbours, neighbourI)
    {
        const label neighbourCell = neighbours[neighbourI];
        if
        (
            fluidSupport
         && phiFluidDistance[neighbourCell] > phiFluidDistance[cellI] + supportPeakTol
        )
        {
            fluidSupport = false;
        }
        if
        (
            solidSupport
         && phiSolidDistance[neighbourCell] > phiSolidDistance[cellI] + supportPeakTol
        )
        {
            solidSupport = false;
        }
        if (!fluidSupport && !solidSupport)
        {
            break;
        }
    }

    channelWidthSupport[cellI] = fluidSupport ? 1.0 : 0.0;
    ribThicknessSupport[cellI] = solidSupport ? 1.0 : 0.0;

    if (fluidSupport)
    {
        widthSupportCountLocal++;
        const scalar widthValue = channelWidthMetric[cellI];
        const scalar widthMinTarget = Foam::max(channelWidthMinTargetField[cellI], scalar(len));
        const scalar widthMaxTarget = Foam::max(channelWidthMaxTargetField[cellI], scalar(widthMinTarget));
        widthMinObservedLocal = Foam::min(widthMinObservedLocal, widthValue);
        widthMaxObservedLocal = Foam::max(widthMaxObservedLocal, widthValue);

        if (useHydraulicDiameterReporting)
        {
            const scalar hydraulicDiameter =
                2.0*widthValue*hydraulicDiameterExtrusionThickness
               /Foam::max(widthValue + hydraulicDiameterExtrusionThickness, SMALL);
            hydraulicDiameterMetric[cellI] = hydraulicDiameter;
            hydraulicDiameterSumLocal += hydraulicDiameter;
            hydraulicDiameterWeightLocal += 1.0;
        }

        if (useChannelWidthControl && (!useProtectedGeometryMasks || protectedFluidMask[cellI] < 0.5))
        {
            const scalar widthMinViolation =
                Foam::max((widthMinTarget - widthValue)/widthMinTarget, scalar(0.0));
            const scalar widthMaxViolation =
                channelWidthMax > 0.0
              ? Foam::max((widthValue - widthMaxTarget)/widthMaxTarget, scalar(0.0))
              : 0.0;

            if (widthMinViolation > SMALL)
            {
                widthMinMaxViolationLocal = Foam::max(widthMinMaxViolationLocal, widthMinViolation);
                widthMinExpSumLocal += Foam::exp(ksRhoGeom*widthMinViolation);
            }
            if (widthMaxViolation > SMALL)
            {
                widthMaxMaxViolationLocal = Foam::max(widthMaxMaxViolationLocal, widthMaxViolation);
                widthMaxExpSumLocal += Foam::exp(ksRhoGeom*widthMaxViolation);
            }

            lsmWidthVelocitySource[cellI] =
                len
              * (
                    -widthConstraintWeight*sizeConstraintWeightMin*widthMinViolation
                    + widthConstraintWeight*sizeConstraintWeightMax*widthMaxViolation
                );
        }
    }

    if (solidSupport)
    {
        ribSupportCountLocal++;
        const scalar ribValue = ribThicknessMetric[cellI];
        const scalar ribMinTarget = Foam::max(ribThicknessMinTargetField[cellI], scalar(len));
        const scalar ribMaxTarget = Foam::max(ribThicknessMaxTargetField[cellI], scalar(ribMinTarget));
        const bool protectedSolid =
            useProtectedGeometryMasks && protectedSolidMask[cellI] > 0.5;
        const bool boundaryAttached =
            applyRibTargetsToInternalRibsOnly && boundaryAttachedSolidMask[cellI] > 0.5;

        ribMinObservedLocal = Foam::min(ribMinObservedLocal, ribValue);
        ribMaxObservedLocal = Foam::max(ribMaxObservedLocal, ribValue);

        if (useRibThicknessControl && !protectedSolid)
        {
            const scalar ribMinViolation =
                Foam::max((ribMinTarget - ribValue)/ribMinTarget, scalar(0.0));
            const scalar ribMaxViolation =
                useRibThicknessMaxControl && !boundaryAttached
              ? Foam::max((ribValue - ribMaxTarget)/ribMaxTarget, scalar(0.0))
              : 0.0;

            if (ribMinViolation > SMALL)
            {
                ribMinMaxViolationLocal = Foam::max(ribMinMaxViolationLocal, ribMinViolation);
                ribMinExpSumLocal += Foam::exp(ksRhoGeom*ribMinViolation);
            }
            if (ribMaxViolation > SMALL)
            {
                ribMaxMaxViolationLocal = Foam::max(ribMaxMaxViolationLocal, ribMaxViolation);
                ribMaxExpSumLocal += Foam::exp(ksRhoGeom*ribMaxViolation);
            }

            lsmRibVelocitySource[cellI] =
                len
              * (
                    ribConstraintWeight*sizeConstraintWeightMin*ribMinViolation
                    - ribConstraintWeight*sizeConstraintWeightMax*ribMaxViolation
                );
        }
    }

    if (phiInterfaceDelta[cellI] > SMALL && useCurvaturePenalty)
    {
        const scalar curvatureViolation = Foam::mag(lsmCurvature[cellI])*len;
        curvatureMaxViolationLocal = Foam::max(curvatureMaxViolationLocal, curvatureViolation);
        curvatureExpSumLocal += Foam::exp(ksRhoGeom*curvatureViolation);
    }
}

reduce(widthSupportCountLocal, sumOp<label>());
reduce(ribSupportCountLocal, sumOp<label>());
reduce(widthMinObservedLocal, minOp<scalar>());
reduce(widthMaxObservedLocal, maxOp<scalar>());
reduce(ribMinObservedLocal, minOp<scalar>());
reduce(ribMaxObservedLocal, maxOp<scalar>());
reduce(hydraulicDiameterSumLocal, sumOp<scalar>());
reduce(hydraulicDiameterWeightLocal, sumOp<scalar>());
reduce(widthMinMaxViolationLocal, maxOp<scalar>());
reduce(widthMaxMaxViolationLocal, maxOp<scalar>());
reduce(ribMinMaxViolationLocal, maxOp<scalar>());
reduce(ribMaxMaxViolationLocal, maxOp<scalar>());
reduce(curvatureMaxViolationLocal, maxOp<scalar>());
reduce(widthMinExpSumLocal, sumOp<scalar>());
reduce(widthMaxExpSumLocal, sumOp<scalar>());
reduce(ribMinExpSumLocal, sumOp<scalar>());
reduce(ribMaxExpSumLocal, sumOp<scalar>());
reduce(curvatureExpSumLocal, sumOp<scalar>());

lsmLastChannelWidthMin =
    widthSupportCountLocal > 0 && widthMinObservedLocal < GREAT
  ? widthMinObservedLocal
  : 0.0;
lsmLastChannelWidthMax = widthSupportCountLocal > 0 ? widthMaxObservedLocal : 0.0;
lsmLastRibThicknessMin =
    ribSupportCountLocal > 0 && ribMinObservedLocal < GREAT
  ? ribMinObservedLocal
  : 0.0;
lsmLastRibThicknessMax = ribSupportCountLocal > 0 ? ribMaxObservedLocal : 0.0;
lsmLastHydraulicDiameterMean =
    hydraulicDiameterWeightLocal > SMALL
  ? hydraulicDiameterSumLocal/hydraulicDiameterWeightLocal
  : 0.0;

lsmLastChannelWidthMinKS =
    widthMinExpSumLocal > SMALL
  ? widthMinMaxViolationLocal + Foam::log(widthMinExpSumLocal)/ksRhoGeom
  : 0.0;
lsmLastChannelWidthMaxKS =
    widthMaxExpSumLocal > SMALL
  ? widthMaxMaxViolationLocal + Foam::log(widthMaxExpSumLocal)/ksRhoGeom
  : 0.0;
lsmLastRibThicknessMinKS =
    ribMinExpSumLocal > SMALL
  ? ribMinMaxViolationLocal + Foam::log(ribMinExpSumLocal)/ksRhoGeom
  : 0.0;
lsmLastRibThicknessMaxKS =
    ribMaxExpSumLocal > SMALL
  ? ribMaxMaxViolationLocal + Foam::log(ribMaxExpSumLocal)/ksRhoGeom
  : 0.0;
lsmLastCurvatureKS =
    curvatureExpSumLocal > SMALL
  ? curvatureMaxViolationLocal + Foam::log(curvatureExpSumLocal)/ksRhoGeom
  : 0.0;

const scalar geometryFilterRadius = Foam::max(lsmSensitivitySmoothingRadius*len, scalar(len));
dimensionedScalar bGeometry
(
    "bGeometry",
    dimensionSet(0, -2, 0, 0, 0, 0, 0),
    1.0/Foam::sqr(geometryFilterRadius)
);

solve
(
    fvm::laplacian(lsmWidthVelocity)
  - fvm::Sp(bGeometry, lsmWidthVelocity)
  + lsmWidthVelocitySource*bGeometry
);
solve
(
    fvm::laplacian(lsmRibVelocity)
  - fvm::Sp(bGeometry, lsmRibVelocity)
  + lsmRibVelocitySource*bGeometry
);

lsmWidthVelocity.correctBoundaryConditions();
lsmRibVelocity.correctBoundaryConditions();
channelWidthSupport.correctBoundaryConditions();
ribThicknessSupport.correctBoundaryConditions();
channelWidthMetric.correctBoundaryConditions();
ribThicknessMetric.correctBoundaryConditions();
hydraulicDiameterMetric.correctBoundaryConditions();
lsmCurvature.correctBoundaryConditions();
