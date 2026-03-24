Info<< "Reading optimization parameters\n" << endl;
IOdictionary optProperties
(
   IOobject
   (
       "optProperties",
       runTime.constant(),
       mesh,
       IOobject::MUST_READ_IF_MODIFIED,
       IOobject::NO_WRITE
   )
);
IOdictionary tuneOptParameters
(
   IOobject
   (
       "tuneOptParameters",
       runTime.constant(),
       mesh,
       IOobject::MUST_READ_IF_MODIFIED,
       IOobject::NO_WRITE
   )
);
//optimization parameters
scalar raa0(readScalar(optProperties.lookup("raa0")));
scalar mma_init(readScalar(optProperties.lookup("mma_init")));
scalar mma_dec(readScalar(optProperties.lookup("mma_dec")));
scalar mma_inc(readScalar(optProperties.lookup("mma_inc")));
scalar movlim(readScalar(optProperties.lookup("movlim")));
scalar voluse(readScalar(optProperties.lookup("voluse")));
scalar GeoDim(readScalar(optProperties.lookup("GeoDim")));
scalar PowerDiss0(readScalar(optProperties.lookup("PowerDiss0")));
scalar PowerDissMax(readScalar(optProperties.lookup("PowerDissMax")));
scalar PowerDissRelax(readScalar(optProperties.lookup("PowerDissRelax")));
scalar filterR(readScalar(optProperties.lookup("filterR")));
dimensionedScalar b("b", dimensionSet(0,-2,0,0,0,0,0),1.0);//filter radius of the PDE filter
dimensionedScalar b1("b1", dimensionSet(0,2,-2,-2,0,0,0),1.0);
Switch solid_area(optProperties.lookup("solid_area"));
Switch fluid_area(optProperties.lookup("fluid_area"));
Switch test_area(optProperties.lookup("test_area"));
scalar Pnorm(readScalar(optProperties.lookup("Pnorm")));
scalar objFunction(readScalar(optProperties.lookup("objFunction")));
scalar qAlpha0(optProperties.lookupOrDefault<scalar>("qAlpha0", 0.1));
scalar qAlphaMin(optProperties.lookupOrDefault<scalar>("qAlphaMin", 1e-3));
scalar qKappa0(optProperties.lookupOrDefault<scalar>("qKappa0", 0.1));
scalar qKappaMin(optProperties.lookupOrDefault<scalar>("qKappaMin", 1e-3));
scalar qHeat0(optProperties.lookupOrDefault<scalar>("qHeat0", 0.1));
scalar qHeatMin(optProperties.lookupOrDefault<scalar>("qHeatMin", 1e-3));
scalar qSingle0(optProperties.lookupOrDefault<scalar>("qSingle0", 0.1));
scalar qSingleMin(optProperties.lookupOrDefault<scalar>("qSingleMin", 1e-3));
label qContinuationStartIter(optProperties.lookupOrDefault<label>("qContinuationStartIter", 20));
label qContinuationInterval(optProperties.lookupOrDefault<label>("qContinuationInterval", 10));
scalar qContinuationFactor(optProperties.lookupOrDefault<scalar>("qContinuationFactor", 0.9));
scalar beta0(optProperties.lookupOrDefault<scalar>("beta0", 0.2));
scalar betaMax(optProperties.lookupOrDefault<scalar>("betaMax", 100.0));
scalar betaIncrement(optProperties.lookupOrDefault<scalar>("betaIncrement", 0.2));
scalar alphaRampEarlySlope(optProperties.lookupOrDefault<scalar>("alphaRampEarlySlope", 1.0/7.0));
scalar alphaRampLateFactor(optProperties.lookupOrDefault<scalar>("alphaRampLateFactor", 1.05));
scalar powerConstraintRelaxationRate
(
  optProperties.lookupOrDefault<scalar>("powerConstraintRelaxationRate", 0.2)
);
scalar continuationFeasibilityTol
(
  optProperties.lookupOrDefault<scalar>("continuationFeasibilityTol", 1.05)
);
scalar dAlphaPorosityEps
(
  optProperties.lookupOrDefault<scalar>("dAlphaPorosityEps", 1e-3)
);

dictionary frameworkSwitches;
if (tuneOptParameters.found("frameworkSwitches"))
{
  frameworkSwitches = tuneOptParameters.subDict("frameworkSwitches");
}
dictionary objectiveSwitches;
if (tuneOptParameters.found("objectiveSwitches"))
{
  objectiveSwitches = tuneOptParameters.subDict("objectiveSwitches");
}
dictionary continuationSwitches;
if (tuneOptParameters.found("continuationSwitches"))
{
  continuationSwitches = tuneOptParameters.subDict("continuationSwitches");
}
dictionary adjointControl;
if (tuneOptParameters.found("adjointControl"))
{
  adjointControl = tuneOptParameters.subDict("adjointControl");
}
dictionary experimentControl;
if (tuneOptParameters.found("experimentControl"))
{
  experimentControl = tuneOptParameters.subDict("experimentControl");
}

Switch useFrozenTurbulenceAdjoint
(
  frameworkSwitches.lookupOrDefault<Switch>("useFrozenTurbulenceAdjoint", true)
);
Switch useKEpsilonModel
(
  frameworkSwitches.lookupOrDefault<Switch>("useKEpsilonModel", true)
);
Switch useWrayAgarwalFallback
(
  frameworkSwitches.lookupOrDefault<Switch>("useWrayAgarwalFallback", false)
);
Switch usePorosityWallDistance
(
  frameworkSwitches.lookupOrDefault<Switch>("usePorosityWallDistance", true)
);
Switch useMeshWaveWallDistanceFallback
(
  frameworkSwitches.lookupOrDefault<Switch>("useMeshWaveWallDistanceFallback", false)
);
Switch useBrinkmanSinkInKEpsilon
(
  frameworkSwitches.lookupOrDefault<Switch>("useBrinkmanSinkInKEpsilon", true)
);
Switch useSplitRAMPControls
(
  frameworkSwitches.lookupOrDefault<Switch>("useSplitRAMPControls", true)
);
Switch useSingleQFallback
(
  frameworkSwitches.lookupOrDefault<Switch>("useSingleQFallback", false)
);
Switch useTurbulentThermalDiffusivity
(
  frameworkSwitches.lookupOrDefault<Switch>("useTurbulentThermalDiffusivity", true)
);
Switch usePowerConstraintRelaxation
(
  frameworkSwitches.lookupOrDefault<Switch>("usePowerConstraintRelaxation", true)
);
Switch useGCMMA
(
  frameworkSwitches.lookupOrDefault<Switch>("useGCMMA", false)
);
Switch useFullAdjointSymmetricStress
(
  frameworkSwitches.lookupOrDefault<Switch>("useFullAdjointSymmetricStress", true)
);
Switch useLogMeanTObjective
(
  objectiveSwitches.lookupOrDefault<Switch>("useLogMeanTObjective", false)
);
Switch useKSHotspotObjective
(
  objectiveSwitches.lookupOrDefault<Switch>("useKSHotspotObjective", false)
);
Switch useVarianceObjective
(
  objectiveSwitches.lookupOrDefault<Switch>("useVarianceObjective", false)
);
Switch useRobustMultiCaseObjective
(
  objectiveSwitches.lookupOrDefault<Switch>("useRobustMultiCaseObjective", false)
);
Switch useStagnationTriggeredBeta
(
  continuationSwitches.lookupOrDefault<Switch>("useStagnationTriggeredBeta", false)
);
label adjointMomentumSweeps
(
  adjointControl.lookupOrDefault<label>("adjointMomentumSweeps", 20)
);
Switch resetAdjointsEachIteration
(
  adjointControl.lookupOrDefault<Switch>("resetAdjointsEachIteration", true)
);
Switch stopOnAdjointRunaway
(
  adjointControl.lookupOrDefault<Switch>("stopOnAdjointRunaway", true)
);
word experimentProfile
(
  experimentControl.lookupOrDefault<word>("profile", "baseline")
);
Switch forceContinuationHardening
(
  experimentControl.lookupOrDefault<Switch>("forceContinuationHardening", false)
);
label forceContinuationHardeningUntilIter
(
  experimentControl.lookupOrDefault<label>("forceContinuationHardeningUntilIter", 0)
);
scalar forceContinuationHardeningUntilBeta
(
  experimentControl.lookupOrDefault<scalar>("forceContinuationHardeningUntilBeta", -1.0)
);
scalar stopContinuationHardeningBelowGrayFraction
(
  experimentControl.lookupOrDefault<scalar>("stopContinuationHardeningBelowGrayFraction", -1.0)
);
label laggingGrayCollapseStartIter
(
  experimentControl.lookupOrDefault<label>("laggingGrayCollapseStartIter", 0)
);
scalar laggingGrayCollapseAboveGrayFraction
(
  experimentControl.lookupOrDefault<scalar>("laggingGrayCollapseAboveGrayFraction", -1.0)
);
scalar laggingGrayCollapseBelowXhStepMax
(
  experimentControl.lookupOrDefault<scalar>("laggingGrayCollapseBelowXhStepMax", -1.0)
);
scalar laggingGrayCollapseMinGrayDrop
(
  experimentControl.lookupOrDefault<scalar>("laggingGrayCollapseMinGrayDrop", -1.0)
);
label laggingGrayCollapsePauseInterval
(
  experimentControl.lookupOrDefault<label>("laggingGrayCollapsePauseInterval", 1)
);

if (experimentProfile == "baseline")
{
  // Keep the runtime dictionaries as-is.
}
else if (experimentProfile == "laminarGrayRamp")
{
  betaIncrement = 0.2;
  alphaRampEarlySlope = 1.0/7.0;
  alphaRampLateFactor = 1.05;
}
else if (experimentProfile == "relaxedGateGrayRamp")
{
  betaIncrement = 0.2;
  alphaRampEarlySlope = 1.0/7.0;
  alphaRampLateFactor = 1.05;
  continuationFeasibilityTol = Foam::max(continuationFeasibilityTol, scalar(2.5));
}
else if (experimentProfile == "betaFloorGrayRamp")
{
  betaIncrement = 0.2;
  alphaRampEarlySlope = 1.0/7.0;
  alphaRampLateFactor = 1.05;
  continuationFeasibilityTol = Foam::max(continuationFeasibilityTol, scalar(2.5));
  forceContinuationHardeningUntilIter =
    Foam::max(forceContinuationHardeningUntilIter, label(80));
  forceContinuationHardeningUntilBeta =
    Foam::max(forceContinuationHardeningUntilBeta, scalar(16.0));
  if (stopContinuationHardeningBelowGrayFraction < 0.0)
  {
    stopContinuationHardeningBelowGrayFraction = 0.05;
  }
  laggingGrayCollapseStartIter =
    Foam::max(laggingGrayCollapseStartIter, label(120));
  if (laggingGrayCollapseAboveGrayFraction < 0.0)
  {
    laggingGrayCollapseAboveGrayFraction = 0.20;
  }
  if (laggingGrayCollapseBelowXhStepMax < 0.0)
  {
    laggingGrayCollapseBelowXhStepMax = 0.05;
  }
  if (laggingGrayCollapseMinGrayDrop < 0.0)
  {
    laggingGrayCollapseMinGrayDrop = 0.01;
  }
  laggingGrayCollapsePauseInterval =
    Foam::max(laggingGrayCollapsePauseInterval, label(5));
}
else if (experimentProfile == "ungatedGrayRamp")
{
  betaIncrement = 0.2;
  alphaRampEarlySlope = 1.0/7.0;
  alphaRampLateFactor = 1.05;
  forceContinuationHardening = true;
}
else if (experimentProfile == "adjointSensitivityProbe")
{
  adjointMomentumSweeps = Foam::max(adjointMomentumSweeps, label(40));
  useFullAdjointSymmetricStress = true;
}
else
{
  FatalErrorInFunction
    << "Unknown experimentControl.profile '" << experimentProfile << "'. "
    << "Supported values are baseline, laminarGrayRamp, "
    << "relaxedGateGrayRamp, betaFloorGrayRamp, "
    << "ungatedGrayRamp, and "
    << "adjointSensitivityProbe."
    << exit(FatalError);
}

if (useGCMMA)
{
  FatalErrorInFunction
    << "useGCMMA=true is not implemented in turbulenceMMAOpt yet."
    << exit(FatalError);
}
if (adjointMomentumSweeps <= 0)
{
  FatalErrorInFunction
    << "adjointControl.adjointMomentumSweeps must be > 0. Found "
    << adjointMomentumSweeps
    << exit(FatalError);
}
if (useWrayAgarwalFallback || !useKEpsilonModel)
{
  FatalErrorInFunction
    << "The Wray-Agarwal fallback path is not implemented yet. "
    << "Use the baseline kEpsilon branch with useKEpsilonModel=true and useWrayAgarwalFallback=false."
    << exit(FatalError);
}
if (!useFrozenTurbulenceAdjoint)
{
  FatalErrorInFunction
    << "Non-frozen turbulence adjoint mode is not implemented yet. "
    << "Keep useFrozenTurbulenceAdjoint=true for the baseline turbulent MMA branch."
    << exit(FatalError);
}
if (useSplitRAMPControls && useSingleQFallback)
{
  FatalErrorInFunction
    << "useSplitRAMPControls and useSingleQFallback cannot both be true."
    << exit(FatalError);
}
if (!useSplitRAMPControls && !useSingleQFallback)
{
  FatalErrorInFunction
    << "Either useSplitRAMPControls or useSingleQFallback must be true."
    << exit(FatalError);
}
if (!usePorosityWallDistance && !useMeshWaveWallDistanceFallback)
{
  FatalErrorInFunction
    << "At least one wall-distance mode must be enabled."
    << exit(FatalError);
}

label objectiveModeCount = 0;
objectiveModeCount += useLogMeanTObjective ? 1 : 0;
objectiveModeCount += useKSHotspotObjective ? 1 : 0;
objectiveModeCount += useVarianceObjective ? 1 : 0;
if (objectiveModeCount > 1)
{
  FatalErrorInFunction
    << "Only one primary thermal objective may be active at a time."
    << exit(FatalError);
}
if (useRobustMultiCaseObjective)
{
  FatalErrorInFunction
    << "useRobustMultiCaseObjective=true is not implemented yet for turbulenceMMAOpt."
    << exit(FatalError);
}
if (forceContinuationHardeningUntilIter < 0)
{
  FatalErrorInFunction
    << "experimentControl.forceContinuationHardeningUntilIter must be >= 0. Found "
    << forceContinuationHardeningUntilIter
    << exit(FatalError);
}
if
(
  forceContinuationHardeningUntilBeta < 0.0
  && forceContinuationHardeningUntilBeta != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.forceContinuationHardeningUntilBeta must be -1 or >= 0. Found "
    << forceContinuationHardeningUntilBeta
    << exit(FatalError);
}
if
(
  stopContinuationHardeningBelowGrayFraction < 0.0
  && stopContinuationHardeningBelowGrayFraction != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.stopContinuationHardeningBelowGrayFraction must be -1 or lie in [0,1]. Found "
    << stopContinuationHardeningBelowGrayFraction
    << exit(FatalError);
}
if (stopContinuationHardeningBelowGrayFraction > 1.0)
{
  FatalErrorInFunction
    << "experimentControl.stopContinuationHardeningBelowGrayFraction must be <= 1. Found "
    << stopContinuationHardeningBelowGrayFraction
    << exit(FatalError);
}
if (laggingGrayCollapseStartIter < 0)
{
  FatalErrorInFunction
    << "experimentControl.laggingGrayCollapseStartIter must be >= 0. Found "
    << laggingGrayCollapseStartIter
    << exit(FatalError);
}
if
(
  laggingGrayCollapseAboveGrayFraction < 0.0
  && laggingGrayCollapseAboveGrayFraction != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.laggingGrayCollapseAboveGrayFraction must be -1 or lie in [0,1]. Found "
    << laggingGrayCollapseAboveGrayFraction
    << exit(FatalError);
}
if (laggingGrayCollapseAboveGrayFraction > 1.0)
{
  FatalErrorInFunction
    << "experimentControl.laggingGrayCollapseAboveGrayFraction must be <= 1. Found "
    << laggingGrayCollapseAboveGrayFraction
    << exit(FatalError);
}
if
(
  laggingGrayCollapseBelowXhStepMax < 0.0
  && laggingGrayCollapseBelowXhStepMax != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.laggingGrayCollapseBelowXhStepMax must be -1 or >= 0. Found "
    << laggingGrayCollapseBelowXhStepMax
    << exit(FatalError);
}
if
(
  laggingGrayCollapseMinGrayDrop < 0.0
  && laggingGrayCollapseMinGrayDrop != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.laggingGrayCollapseMinGrayDrop must be -1 or >= 0. Found "
    << laggingGrayCollapseMinGrayDrop
    << exit(FatalError);
}
if (laggingGrayCollapsePauseInterval <= 0)
{
  FatalErrorInFunction
    << "experimentControl.laggingGrayCollapsePauseInterval must be > 0. Found "
    << laggingGrayCollapsePauseInterval
    << exit(FatalError);
}

const dictionary& convergenceControl = tuneOptParameters.subDict("convergenceControl");
label optMinIterations(convergenceControl.lookupOrDefault<label>("optMinIterations", 50));
label optStallWindow(convergenceControl.lookupOrDefault<label>("optStallWindow", 5));
scalar optObjectiveTol(convergenceControl.lookupOrDefault<scalar>("optObjectiveTol", 1e-3));
scalar optConstraintTol(convergenceControl.lookupOrDefault<scalar>("optConstraintTol", 1e-3));
scalar optStepTol(convergenceControl.lookupOrDefault<scalar>("optStepTol", 5e-3));
scalar optPowerConstraintWeight
(
  convergenceControl.lookupOrDefault<scalar>("optPowerConstraintWeight", 1.0)
);
scalar optPowerViolationScaleExponent
(
  convergenceControl.lookupOrDefault<scalar>("optPowerViolationScaleExponent", 1.0)
);
scalar optVolumeConstraintWeight
(
  convergenceControl.lookupOrDefault<scalar>("optVolumeConstraintWeight", 1.0)
);
if (optPowerConstraintWeight <= 0.0)
{
  FatalErrorInFunction
    << "tuneOptParameters.convergenceControl.optPowerConstraintWeight must be > 0. Found "
    << optPowerConstraintWeight
    << exit(FatalError);
}
if (optPowerViolationScaleExponent < 0.0)
{
  FatalErrorInFunction
    << "tuneOptParameters.convergenceControl.optPowerViolationScaleExponent must be >= 0. Found "
    << optPowerViolationScaleExponent
    << exit(FatalError);
}
if (optVolumeConstraintWeight <= 0.0)
{
  FatalErrorInFunction
    << "tuneOptParameters.convergenceControl.optVolumeConstraintWeight must be > 0. Found "
    << optVolumeConstraintWeight
    << exit(FatalError);
}
Info<< "Reading pseudo-density field\n" << endl;
volScalarField xp
(
    IOobject
    (
        "xp",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    scalar(voluse),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField xh
(
    IOobject
    (
        "xh",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    scalar(voluse),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField x(xp);
volScalarField drho(x);

// find the cells in the non-design area
labelList cells_solid,cells_fluid,cells_test;
if(solid_area)
{
  word zoneName="solid_area";
  label zoneID=mesh.cellZones().findZoneID(zoneName);
  cells_solid=mesh.cellZones()[zoneID];
  forAll(cells_solid, i)
  {
     x[cells_solid[i]]=0;
  }
}
if(fluid_area)
{
  word zoneName="fluid_area";
  label zoneID=mesh.cellZones().findZoneID(zoneName);
  cells_fluid=mesh.cellZones()[zoneID];
  forAll(cells_fluid, i)
  {
     x[cells_fluid[i]]=1.0;
  }
}
if(test_area)
{
  word zoneName="test_area";
  label zoneID=mesh.cellZones().findZoneID(zoneName);
  cells_test=mesh.cellZones()[zoneID];
  forAll(cells_test, i)
  {
     x[cells_test[i]]=1.0;
     xh[cells_test[i]]=1.0;
  }
}
volScalarField gsensVol
(
    IOobject
    (
        "gsensVol",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    xh,
    zeroGradientFvPatchScalarField::typeName
);
volScalarField gsenshVol(xh);
volScalarField gsensVolRaw
(
    IOobject
    (
        "gsensVolRaw",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);

// find the cells on the boundary
dictionary conFunc = mesh.solutionDict().subDict("constraintFunctionDict_flow");
int nObjPatch =conFunc.lookupOrDefault<scalar>("numberConstraintPatches",2);
wordList conPatchNames = conFunc.lookup("constraintPatchesNames");
label conPatchList[nObjPatch];//patch list
int iLoop;
for(iLoop=0; iLoop<nObjPatch; iLoop++)
{
    conPatchList[iLoop]=mesh.boundaryMesh().findPatchID(conPatchNames[iLoop]);
}   
