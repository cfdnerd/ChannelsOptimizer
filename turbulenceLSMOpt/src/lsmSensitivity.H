Info<< "LSM stage: mapping volumetric sensitivities into interface velocities" << endl;

lsmObjectiveVelocity = dimensionedScalar("zero", lsmObjectiveVelocity.dimensions(), 0.0);
lsmPowerVelocity = dimensionedScalar("zero", lsmPowerVelocity.dimensions(), 0.0);
lsmVolumeVelocity = dimensionedScalar("zero", lsmVolumeVelocity.dimensions(), 0.0);
lsmGeometryVelocity = dimensionedScalar("zero", lsmGeometryVelocity.dimensions(), 0.0);
lsmNormalVelocityRaw = dimensionedScalar("zero", lsmNormalVelocityRaw.dimensions(), 0.0);

scalar objSensScale = gMax(mag(fsenshMeanT.primitiveField()));
scalar powerSensScale = gMax(mag(gsenshPowerDiss.primitiveField()));
scalar volSensScale = gMax(mag(gsenshVol.primitiveField()));

const scalar powerMarginPositive = Foam::max(PowerDiss/ Foam::max(activePowerLimit, SMALL) - 1.0, scalar(0.0));
const scalar volumeMarginPositive = Foam::max(V, scalar(0.0));
const scalar powerConstraintWeightActive =
    optPowerConstraintWeight
  * Foam::pow
    (
        Foam::max(PowerDiss/Foam::max(activePowerLimit, SMALL), scalar(1.0)),
        optPowerViolationScaleExponent
    );

forAll(phiLS, cellI)
{
    const scalar bandWeight = phiInterfaceDelta[cellI];
    if (bandWeight <= SMALL)
    {
        continue;
    }

    if (useApproximateNarrowBandSensitivityBridge)
    {
        const scalar objectiveSignal =
            objSensScale > SMALL ? fsenshMeanT[cellI]/objSensScale : 0.0;
        const scalar powerSignal =
            powerSensScale > SMALL ? gsenshPowerDiss[cellI]/powerSensScale : 0.0;
        const scalar volumeSignal =
            volSensScale > SMALL ? gsenshVol[cellI]/volSensScale : 0.0;

        lsmObjectiveVelocity[cellI] = -len*bandWeight*objectiveSignal;
        lsmPowerVelocity[cellI] = -len*bandWeight*powerSignal;
        lsmVolumeVelocity[cellI] = -len*bandWeight*volumeSignal;
    }

    scalar geometryVelocity = bandWeight*(lsmWidthVelocity[cellI] + lsmRibVelocity[cellI]);
    if (useCurvaturePenalty)
    {
        geometryVelocity += -curvaturePenaltyWeight*len*len*bandWeight*lsmCurvature[cellI];
    }
    lsmGeometryVelocity[cellI] = geometryVelocity;

    lsmNormalVelocityRaw[cellI] =
        lsmObjectiveVelocity[cellI]
      + powerConstraintWeightActive*powerMarginPositive*lsmPowerVelocity[cellI]
      + optVolumeConstraintWeight*volumeMarginPositive*lsmVolumeVelocity[cellI]
      + lsmGeometryVelocity[cellI];
}

if (normalizeInterfaceVelocity)
{
    auto normalizeVelocityField =
    [&](volScalarField& velocityField)
    {
        scalar velocityScale = gMax(mag(velocityField.primitiveField()));
        if (velocityScale > SMALL)
        {
            velocityField = velocityField/velocityScale*len;
        }
    };

    normalizeVelocityField(lsmObjectiveVelocity);
    normalizeVelocityField(lsmPowerVelocity);
    normalizeVelocityField(lsmVolumeVelocity);
    normalizeVelocityField(lsmGeometryVelocity);
    normalizeVelocityField(lsmNormalVelocityRaw);
}

lsmLastObjectiveVelocityMax = gMax(mag(lsmObjectiveVelocity.primitiveField()));
lsmLastPowerVelocityMax = gMax(mag(lsmPowerVelocity.primitiveField()));
lsmLastVolumeVelocityMax = gMax(mag(lsmVolumeVelocity.primitiveField()));
lsmLastGeometryVelocityMax = gMax(mag(lsmGeometryVelocity.primitiveField()));
lsmLastVelocityMax = gMax(mag(lsmNormalVelocityRaw.primitiveField()));

if (finiteDifferenceCheckStage2 && Pstream::master())
{
    Info<< "finiteDifferenceCheckStage2 is enabled: writing interface velocity fields "
        << "for external Stage-2 finite-difference validation." << endl;
}
