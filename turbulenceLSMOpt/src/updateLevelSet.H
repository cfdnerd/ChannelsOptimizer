Info<< "LSM stage: advecting level-set field" << endl;

Field<scalar> phiLSBeforeUpdate(phiLS.primitiveField());
volVectorField gradPhiForUpdate
(
    IOobject
    (
        "gradPhiForUpdate",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    fvc::grad(phiLS)
);

const scalar narrowBandHalfWidth = Foam::max(narrowBandHalfWidthCells*len, scalar(len));
const scalar maxInterfaceSpeed = gMax(mag(lsmNormalVelocity.primitiveField()));
const scalar cflDt =
    maxInterfaceSpeed > SMALL
  ? lsmCfl*len/maxInterfaceSpeed
  : Foam::max(lsmPseudoDt, scalar(0.25*len));
const scalar pseudoDt =
    lsmPseudoDt > SMALL
  ? Foam::min(lsmPseudoDt, cflDt)
  : cflDt;

lsmLastTimeStep = pseudoDt;

forAll(phiLS, cellI)
{
    if (Foam::mag(phiLS[cellI]) <= narrowBandHalfWidth)
    {
        const scalar gradMag = Foam::max(Foam::mag(gradPhiForUpdate[cellI]), scalar(1e-8));
        phiLS[cellI] -= pseudoDt*lsmNormalVelocity[cellI]*gradMag;
    }
}
phiLS.correctBoundaryConditions();

if (useReactionDiffusionComplexityControl)
{
    volScalarField phiReactionDiffused(phiLS);
    dimensionedScalar bReaction
    (
        "bReaction",
        dimensionSet(0, -2, 0, 0, 0, 0, 0),
        1.0/Foam::sqr(Foam::max(velocityFilterRadius*len, scalar(len)))
    );

    solve
    (
        fvm::laplacian(phiReactionDiffused)
      - fvm::Sp(bReaction, phiReactionDiffused)
      + phiLS*bReaction
    );
    phiLS = phiReactionDiffused;
    phiLS.correctBoundaryConditions();
}

volVectorField gradPhiPostUpdate
(
    IOobject
    (
        "gradPhiPostUpdate",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    fvc::grad(phiLS)
);

scalar signedDistanceDriftLocal(0.0);
forAll(phiLS, cellI)
{
    if (Foam::mag(phiLS[cellI]) <= narrowBandHalfWidth)
    {
        signedDistanceDriftLocal =
            Foam::max
            (
                signedDistanceDriftLocal,
                Foam::mag(Foam::mag(gradPhiPostUpdate[cellI]) - 1.0)
            );
    }
}
reduce(signedDistanceDriftLocal, maxOp<scalar>());
lsmLastSignedDistanceDrift = signedDistanceDriftLocal;

const bool reinitByInterval =
    phiReinitInterval > 0
 && ((lsmStageIteration + 1) % phiReinitInterval == 0);
const bool reinitByDrift = phiGradDriftTol > 0.0 && signedDistanceDriftLocal > phiGradDriftTol;
if (reinitByInterval || reinitByDrift)
{
    phiReinitReference = phiLS;
    const scalar reinitDt = 0.25*len;
    for (label reinitIter = 0; reinitIter < 5; ++reinitIter)
    {
        volVectorField gradPhiReinit
        (
            IOobject
            (
                "gradPhiReinit",
                runTime.timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            fvc::grad(phiLS)
        );

        forAll(phiLS, cellI)
        {
            const scalar smoothSign =
                phiReinitReference[cellI]
               /Foam::sqrt
                (
                    phiReinitReference[cellI]*phiReinitReference[cellI]
                  + Foam::sqr(0.5*len)
                );
            const scalar gradMag = Foam::mag(gradPhiReinit[cellI]);
            phiLS[cellI] -= reinitDt*smoothSign*(gradMag - 1.0);
        }
        phiLS.correctBoundaryConditions();
    }
}

{
    #include "updateDerivedDensityFromLevelSet.H"
}
{
    #include "lsmGeometryMetrics.H"
}

lsmLastConnectivitySatisfied =
    allowTopologyChangeInLSM
 || !lsmSwitchRequireConnectivity
 || hasThresholdedFluidConnectivity(xh, 0.5);

const bool widthCollapseDetected =
    useChannelWidthControl
 && channelWidthMin > SMALL
 && lsmLastChannelWidthMin > SMALL
 && lsmLastChannelWidthMin < 0.5*channelWidthMin;
const bool ribCollapseDetected =
    useRibThicknessControl
 && ribThicknessMin > SMALL
 && lsmLastRibThicknessMin > SMALL
 && lsmLastRibThicknessMin < 0.5*ribThicknessMin;

lsmLastStepAccepted =
    allowTopologyChangeInLSM
 || (lsmLastConnectivitySatisfied && !widthCollapseDetected && !ribCollapseDetected);

if (!lsmLastStepAccepted)
{
    phiLS.primitiveFieldRef() = phiLSBeforeUpdate;
    phiLS.correctBoundaryConditions();
    {
        #include "updateDerivedDensityFromLevelSet.H"
    }
    {
        #include "lsmGeometryMetrics.H"
    }

    lsmRejectedStepCount++;
    if (lsmRollbackEnabled && lsmRejectedStepCount >= 2 && lsmCheckpointAvailable)
    {
        lsmRollbackRequested = true;
        lsmRollbackReason = word("lsmStepRejected");
    }
}
else
{
    lsmRejectedStepCount = 0;
}

scalar phiStepMaxLocal(0.0);
forAll(phiLS, cellI)
{
    phiStepMaxLocal =
        Foam::max(phiStepMaxLocal, Foam::mag(phiLS[cellI] - phiLSBeforeUpdate[cellI]));
}
reduce(phiStepMaxLocal, maxOp<scalar>());
lsmLastPhiStepMax = phiStepMaxLocal;

lsmStageIteration++;
