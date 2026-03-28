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
auto detectSupportPeaks =
[&](const label cellI, bool& fluidSupport, bool& solidSupport)
{
    const labelList& neighbours = cellCells[cellI];
    fluidSupport = phiFluidDistance[cellI] > supportTol;
    solidSupport = phiSolidDistance[cellI] > supportTol;

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
};
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
    bool fluidSupport(false);
    bool solidSupport(false);
    detectSupportPeaks(cellI, fluidSupport, solidSupport);

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
            }
            if (widthMaxViolation > SMALL)
            {
                widthMaxMaxViolationLocal = Foam::max(widthMaxMaxViolationLocal, widthMaxViolation);
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
            }
            if (ribMaxViolation > SMALL)
            {
                ribMaxMaxViolationLocal = Foam::max(ribMaxMaxViolationLocal, ribMaxViolation);
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

const bool accumulateWidthMinKS = widthMinMaxViolationLocal > SMALL;
const bool accumulateWidthMaxKS = widthMaxMaxViolationLocal > SMALL;
const bool accumulateRibMinKS = ribMinMaxViolationLocal > SMALL;
const bool accumulateRibMaxKS = ribMaxMaxViolationLocal > SMALL;
const bool accumulateCurvatureKS = curvatureMaxViolationLocal > SMALL;

scalar geometryOverflowRiskArg(0.0);
if (accumulateWidthMinKS)
{
    geometryOverflowRiskArg = Foam::max(geometryOverflowRiskArg, ksRhoGeom*widthMinMaxViolationLocal);
}
if (accumulateWidthMaxKS)
{
    geometryOverflowRiskArg = Foam::max(geometryOverflowRiskArg, ksRhoGeom*widthMaxMaxViolationLocal);
}
if (accumulateRibMinKS)
{
    geometryOverflowRiskArg = Foam::max(geometryOverflowRiskArg, ksRhoGeom*ribMinMaxViolationLocal);
}
if (accumulateRibMaxKS)
{
    geometryOverflowRiskArg = Foam::max(geometryOverflowRiskArg, ksRhoGeom*ribMaxMaxViolationLocal);
}
if (accumulateCurvatureKS)
{
    geometryOverflowRiskArg = Foam::max(geometryOverflowRiskArg, ksRhoGeom*curvatureMaxViolationLocal);
}
if (Pstream::master() && lsmStageIteration == 0 && geometryOverflowRiskArg > 700.0)
{
    Info<< "LSM geometry KS accumulation stabilized a handover overflow risk."
        << " stage=" << hybridStageName
        << " maxExpArg=" << geometryOverflowRiskArg
        << " widthMinViol=" << widthMinMaxViolationLocal
        << " widthMaxViol=" << widthMaxMaxViolationLocal
        << " ribMinViol=" << ribMinMaxViolationLocal
        << " ribMaxViol=" << ribMaxMaxViolationLocal
        << " curvatureViol=" << curvatureMaxViolationLocal
        << endl;
}

// Use a shifted log-sum-exp so the first LSM handover can report large
// geometry penalties without overflowing exp().
if
(
    accumulateWidthMinKS
 || accumulateWidthMaxKS
 || accumulateRibMinKS
 || accumulateRibMaxKS
 || accumulateCurvatureKS
)
{
    forAll(phiLS, cellI)
    {
        bool fluidSupport(false);
        bool solidSupport(false);
        detectSupportPeaks(cellI, fluidSupport, solidSupport);

        if (fluidSupport)
        {
            const scalar widthValue = channelWidthMetric[cellI];
            const scalar widthMinTarget = Foam::max(channelWidthMinTargetField[cellI], scalar(len));
            const scalar widthMaxTarget = Foam::max(channelWidthMaxTargetField[cellI], scalar(widthMinTarget));
            if (useChannelWidthControl && (!useProtectedGeometryMasks || protectedFluidMask[cellI] < 0.5))
            {
                const scalar widthMinViolation =
                    Foam::max((widthMinTarget - widthValue)/widthMinTarget, scalar(0.0));
                const scalar widthMaxViolation =
                    channelWidthMax > 0.0
                  ? Foam::max((widthValue - widthMaxTarget)/widthMaxTarget, scalar(0.0))
                  : 0.0;

                if (accumulateWidthMinKS && widthMinViolation > SMALL)
                {
                    widthMinExpSumLocal +=
                        Foam::exp(ksRhoGeom*(widthMinViolation - widthMinMaxViolationLocal));
                }
                if (accumulateWidthMaxKS && widthMaxViolation > SMALL)
                {
                    widthMaxExpSumLocal +=
                        Foam::exp(ksRhoGeom*(widthMaxViolation - widthMaxMaxViolationLocal));
                }
            }
        }

        if (solidSupport)
        {
            const scalar ribValue = ribThicknessMetric[cellI];
            const scalar ribMinTarget = Foam::max(ribThicknessMinTargetField[cellI], scalar(len));
            const scalar ribMaxTarget = Foam::max(ribThicknessMaxTargetField[cellI], scalar(ribMinTarget));
            const bool protectedSolid =
                useProtectedGeometryMasks && protectedSolidMask[cellI] > 0.5;
            const bool boundaryAttached =
                applyRibTargetsToInternalRibsOnly && boundaryAttachedSolidMask[cellI] > 0.5;
            if (useRibThicknessControl && !protectedSolid)
            {
                const scalar ribMinViolation =
                    Foam::max((ribMinTarget - ribValue)/ribMinTarget, scalar(0.0));
                const scalar ribMaxViolation =
                    useRibThicknessMaxControl && !boundaryAttached
                  ? Foam::max((ribValue - ribMaxTarget)/ribMaxTarget, scalar(0.0))
                  : 0.0;

                if (accumulateRibMinKS && ribMinViolation > SMALL)
                {
                    ribMinExpSumLocal +=
                        Foam::exp(ksRhoGeom*(ribMinViolation - ribMinMaxViolationLocal));
                }
                if (accumulateRibMaxKS && ribMaxViolation > SMALL)
                {
                    ribMaxExpSumLocal +=
                        Foam::exp(ksRhoGeom*(ribMaxViolation - ribMaxMaxViolationLocal));
                }
            }
        }

        if (accumulateCurvatureKS && phiInterfaceDelta[cellI] > SMALL)
        {
            const scalar curvatureViolation = Foam::mag(lsmCurvature[cellI])*len;
            if (curvatureViolation > SMALL)
            {
                curvatureExpSumLocal +=
                    Foam::exp(ksRhoGeom*(curvatureViolation - curvatureMaxViolationLocal));
            }
        }
    }
}

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
