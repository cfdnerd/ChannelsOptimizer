Info<< "Creating hybrid density-to-level-set fields\n" << endl;

volScalarField phiLS
(
    IOobject
    (
        "phiLevelSet",
        runTime.timeName(),
        mesh,
        IOobject::READ_IF_PRESENT,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField phiReinitReference
(
    IOobject
    (
        "phiReinitReference",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField xhLevelSet
(
    IOobject
    (
        "xhLevelSet",
        runTime.timeName(),
        mesh,
        IOobject::READ_IF_PRESENT,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, voluse),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField xhDensityReference
(
    IOobject
    (
        "xhDensityReference",
        runTime.timeName(),
        mesh,
        IOobject::READ_IF_PRESENT,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, voluse),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField phiInterfaceDelta
(
    IOobject
    (
        "phiInterfaceDelta",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField phiFluidDistance
(
    IOobject
    (
        "phiFluidDistance",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField phiSolidDistance
(
    IOobject
    (
        "phiSolidDistance",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField channelWidthMetric
(
    IOobject
    (
        "channelWidthMetric",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField ribThicknessMetric
(
    IOobject
    (
        "ribThicknessMetric",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField hydraulicDiameterMetric
(
    IOobject
    (
        "hydraulicDiameterMetric",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField channelWidthSupport
(
    IOobject
    (
        "channelWidthSupport",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField ribThicknessSupport
(
    IOobject
    (
        "ribThicknessSupport",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField channelWidthMinTargetField
(
    IOobject
    (
        "channelWidthMinTargetField",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, channelWidthMin),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField channelWidthMaxTargetField
(
    IOobject
    (
        "channelWidthMaxTargetField",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, channelWidthMax),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField ribThicknessMinTargetField
(
    IOobject
    (
        "ribThicknessMinTargetField",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, ribThicknessMin),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField ribThicknessMaxTargetField
(
    IOobject
    (
        "ribThicknessMaxTargetField",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, ribThicknessMax),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField protectedSolidMask
(
    IOobject
    (
        "protectedSolidMask",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField protectedFluidMask
(
    IOobject
    (
        "protectedFluidMask",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField boundaryAttachedSolidMask
(
    IOobject
    (
        "boundaryAttachedSolidMask",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField dAlphaLSM
(
    IOobject
    (
        "dAlphaLSM",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField lsmCurvature
(
    IOobject
    (
        "lsmCurvature",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimensionSet(0, -1, 0, 0, 0, 0, 0), 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField lsmWidthVelocitySource
(
    IOobject
    (
        "lsmWidthVelocitySource",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField lsmRibVelocitySource
(
    IOobject
    (
        "lsmRibVelocitySource",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField lsmWidthVelocity
(
    IOobject
    (
        "lsmWidthVelocity",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField lsmRibVelocity
(
    IOobject
    (
        "lsmRibVelocity",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField lsmObjectiveVelocity
(
    IOobject
    (
        "lsmObjectiveVelocity",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField lsmPowerVelocity
(
    IOobject
    (
        "lsmPowerVelocity",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField lsmVolumeVelocity
(
    IOobject
    (
        "lsmVolumeVelocity",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField lsmGeometryVelocity
(
    IOobject
    (
        "lsmGeometryVelocity",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField lsmNormalVelocityRaw
(
    IOobject
    (
        "lsmNormalVelocityRaw",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField lsmNormalVelocity
(
    IOobject
    (
        "lsmNormalVelocity",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimLength, 0.0),
    zeroGradientFvPatchScalarField::typeName
);

auto applyZoneValues =
[&](const dictionary& zoneValues, volScalarField& targetField)
{
    const wordList zoneNames(zoneValues.toc());
    forAll(zoneNames, zoneI)
    {
        const word& zoneName = zoneNames[zoneI];
        const label zoneId = mesh.cellZones().findZoneID(zoneName);
        if (zoneId < 0)
        {
            FatalErrorInFunction
                << "Cell zone '" << zoneName << "' requested in "
                << targetField.name() << " was not found."
                << exit(FatalError);
        }

        const scalar zoneValue = readScalar(zoneValues.lookup(zoneName));
        const labelList& zoneCells = mesh.cellZones()[zoneId];
        forAll(zoneCells, cellI)
        {
            targetField[zoneCells[cellI]] = zoneValue;
        }
    }
};

if (useRegionwiseWidthTargets)
{
    applyZoneValues(regionwiseChannelWidthMinDict, channelWidthMinTargetField);
    if (regionwiseChannelWidthMaxDict.toc().size())
    {
        applyZoneValues(regionwiseChannelWidthMaxDict, channelWidthMaxTargetField);
    }
}
if (useRegionwiseRibTargets)
{
    applyZoneValues(regionwiseRibThicknessMinDict, ribThicknessMinTargetField);
    if (regionwiseRibThicknessMaxDict.toc().size())
    {
        applyZoneValues(regionwiseRibThicknessMaxDict, ribThicknessMaxTargetField);
    }
}

auto applyProtectedMask =
[&](const wordList& regionNames, volScalarField& maskField)
{
    forAll(regionNames, regionI)
    {
        const label zoneId = mesh.cellZones().findZoneID(regionNames[regionI]);
        if (zoneId < 0)
        {
            FatalErrorInFunction
                << "Protected region cell zone '" << regionNames[regionI]
                << "' was not found."
                << exit(FatalError);
        }

        const labelList& zoneCells = mesh.cellZones()[zoneId];
        forAll(zoneCells, cellI)
        {
            maskField[zoneCells[cellI]] = 1.0;
        }
    }
};

if (useProtectedGeometryMasks)
{
    applyProtectedMask(protectedSolidRegionNames, protectedSolidMask);
    applyProtectedMask(protectedFluidRegionNames, protectedFluidMask);
}

forAll(mesh.boundary(), patchI)
{
    const fvPatch& patch = mesh.boundary()[patchI];
    const labelList& faceCells = patch.faceCells();
    forAll(faceCells, faceCellI)
    {
        boundaryAttachedSolidMask[faceCells[faceCellI]] = 1.0;
    }
}

channelWidthMinTargetField.correctBoundaryConditions();
channelWidthMaxTargetField.correctBoundaryConditions();
ribThicknessMinTargetField.correctBoundaryConditions();
ribThicknessMaxTargetField.correctBoundaryConditions();
protectedSolidMask.correctBoundaryConditions();
protectedFluidMask.correctBoundaryConditions();
boundaryAttachedSolidMask.correctBoundaryConditions();
xhDensityReference = xh;
xhLevelSet = xh;
