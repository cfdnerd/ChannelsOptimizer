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
IOdictionary geometricSizeControlDict
(
   IOobject
   (
       "geometricSizeControlDict",
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
dictionary geometricSizeGeneral;
if (geometricSizeControlDict.found("general"))
{
  geometricSizeGeneral = geometricSizeControlDict.subDict("general");
}
dictionary minSolidGeometryControl;
if (geometricSizeControlDict.found("minSolidGeometryControl"))
{
  minSolidGeometryControl = geometricSizeControlDict.subDict("minSolidGeometryControl");
}
else if (geometricSizeControlDict.found("path2SolidMin"))
{
  Info<< "geometricSizeControlDict.path2SolidMin is deprecated; use "
      << "geometricSizeControlDict.minSolidGeometryControl" << endl;
  minSolidGeometryControl = geometricSizeControlDict.subDict("path2SolidMin");
}
dictionary maxSolidGeometryControl;
if (geometricSizeControlDict.found("maxSolidGeometryControl"))
{
  maxSolidGeometryControl = geometricSizeControlDict.subDict("maxSolidGeometryControl");
}
else if (geometricSizeControlDict.found("path3SolidMax"))
{
  Info<< "geometricSizeControlDict.path3SolidMax is deprecated; use "
      << "geometricSizeControlDict.maxSolidGeometryControl" << endl;
  maxSolidGeometryControl = geometricSizeControlDict.subDict("path3SolidMax");
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
Switch useLateStageFilterRCap
(
  continuationSwitches.lookupOrDefault<Switch>("useLateStageFilterRCap", true)
);
Switch useMinSolidGeometryControl
(
  minSolidGeometryControl.lookupOrDefault<Switch>("active", false)
);
Switch useMaxSolidGeometryControl
(
  maxSolidGeometryControl.lookupOrDefault<Switch>("active", false)
);
scalar minSolidGeometryFeatureSize
(
  minSolidGeometryControl.lookupOrDefault<scalar>("featureSize", -1.0)
);
scalar minSolidGeometryFillFraction
(
  minSolidGeometryControl.lookupOrDefault<scalar>("fillFraction", 0.55)
);
scalar minSolidGeometryConstraintTolerance
(
  minSolidGeometryControl.lookupOrDefault<scalar>("tolerance", 1e-4)
);
scalar minSolidGeometryConstraintWeight
(
  minSolidGeometryControl.lookupOrDefault<scalar>("constraintWeight", 1.0)
);
scalar maxSolidGeometryFeatureSize
(
  maxSolidGeometryControl.lookupOrDefault<scalar>("featureSize", -1.0)
);
scalar maxSolidGeometryFillFraction
(
  maxSolidGeometryControl.lookupOrDefault<scalar>("fillFraction", 0.85)
);
scalar maxSolidGeometryConstraintTolerance
(
  maxSolidGeometryControl.lookupOrDefault<scalar>("tolerance", 1e-4)
);
scalar maxSolidGeometryConstraintWeight
(
  maxSolidGeometryControl.lookupOrDefault<scalar>("constraintWeight", 1.0)
);
scalar geometricSizeAggregationKSFactor
(
  geometricSizeGeneral.lookupOrDefault<scalar>("aggregationKSFactor", 40.0)
);
scalar geometricSizeSmoothPositiveEps
(
  geometricSizeGeneral.lookupOrDefault<scalar>("smoothPositiveEps", 1e-4)
);
Switch writeGeometricSizeDiagnosticFields
(
  geometricSizeGeneral.lookupOrDefault<Switch>("writeDiagnosticFields", false)
);
label adjointMomentumSweeps
(
  adjointControl.lookupOrDefault<label>("adjointMomentumSweeps", 20)
);
scalar adjointExplicitStressScale
(
  adjointControl.lookupOrDefault<scalar>("adjointExplicitStressScale", -1.0)
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
scalar lateStageStrictGateBelowGrayFraction
(
  experimentControl.lookupOrDefault<scalar>("lateStageStrictGateBelowGrayFraction", -1.0)
);
scalar lateStageContinuationFeasibilityTol
(
  experimentControl.lookupOrDefault<scalar>("lateStageContinuationFeasibilityTol", -1.0)
);
label lateStageRefinementStartIter
(
  experimentControl.lookupOrDefault<label>("lateStageRefinementStartIter", 0)
);
scalar lateStageRefinementBelowGrayFraction
(
  experimentControl.lookupOrDefault<scalar>("lateStageRefinementBelowGrayFraction", -1.0)
);
scalar lateStageBetaRampFactor
(
  experimentControl.lookupOrDefault<scalar>("lateStageBetaRampFactor", -1.0)
);
scalar lateStageAlphaRampFactor
(
  experimentControl.lookupOrDefault<scalar>("lateStageAlphaRampFactor", -1.0)
);
scalar lateStageMaxBeta
(
  experimentControl.lookupOrDefault<scalar>("lateStageMaxBeta", -1.0)
);
label lateStageFilterRStartIter
(
  experimentControl.lookupOrDefault<label>("lateStageFilterRStartIter", 0)
);
scalar lateStageFilterRCap
(
  experimentControl.lookupOrDefault<scalar>("lateStageFilterRCap", -1.0)
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
label overactiveTopologyStartIter
(
  experimentControl.lookupOrDefault<label>("overactiveTopologyStartIter", 0)
);
scalar overactiveTopologyBelowGrayFraction
(
  experimentControl.lookupOrDefault<scalar>("overactiveTopologyBelowGrayFraction", -1.0)
);
scalar overactiveTopologyAboveXhStepMax
(
  experimentControl.lookupOrDefault<scalar>("overactiveTopologyAboveXhStepMax", -1.0)
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
    stopContinuationHardeningBelowGrayFraction = 0.07;
  }
  if (lateStageStrictGateBelowGrayFraction < 0.0)
  {
    lateStageStrictGateBelowGrayFraction = 0.08;
  }
  if (lateStageContinuationFeasibilityTol < 0.0)
  {
    lateStageContinuationFeasibilityTol = 1.0;
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
  overactiveTopologyStartIter =
    Foam::max(overactiveTopologyStartIter, label(220));
  if (overactiveTopologyBelowGrayFraction < 0.0)
  {
    overactiveTopologyBelowGrayFraction = 0.07;
  }
  if (overactiveTopologyAboveXhStepMax < 0.0)
  {
    overactiveTopologyAboveXhStepMax = 0.35;
  }
}
else if (experimentProfile == "branchRefinement400")
{
  betaIncrement = 0.2;
  alphaRampEarlySlope = 1.0/7.0;
  alphaRampLateFactor = 1.05;
  continuationFeasibilityTol = Foam::max(continuationFeasibilityTol, scalar(2.0));
  forceContinuationHardeningUntilIter =
    Foam::max(forceContinuationHardeningUntilIter, label(80));
  forceContinuationHardeningUntilBeta =
    Foam::max(forceContinuationHardeningUntilBeta, scalar(16.0));
  if (stopContinuationHardeningBelowGrayFraction < 0.0)
  {
    stopContinuationHardeningBelowGrayFraction = 0.10;
  }
  if (lateStageStrictGateBelowGrayFraction < 0.0)
  {
    lateStageStrictGateBelowGrayFraction = 0.14;
  }
  if (lateStageContinuationFeasibilityTol < 0.0)
  {
    lateStageContinuationFeasibilityTol = 0.98;
  }
  if (lateStageRefinementStartIter <= 0)
  {
    lateStageRefinementStartIter = 220;
  }
  if (lateStageRefinementBelowGrayFraction < 0.0)
  {
    lateStageRefinementBelowGrayFraction = 0.18;
  }
  if (lateStageBetaRampFactor < 0.0)
  {
    lateStageBetaRampFactor = 0.25;
  }
  if (lateStageAlphaRampFactor < 0.0)
  {
    lateStageAlphaRampFactor = 1.0;
  }
  if (lateStageMaxBeta < 0.0)
  {
    lateStageMaxBeta = 48.0;
  }
  if (lateStageFilterRStartIter <= 0)
  {
    lateStageFilterRStartIter = 240;
  }
  if (lateStageFilterRCap < 0.0)
  {
    lateStageFilterRCap = Foam::min(filterR, scalar(3.0));
  }
  laggingGrayCollapseStartIter =
    Foam::max(laggingGrayCollapseStartIter, label(140));
  if (laggingGrayCollapseAboveGrayFraction < 0.0)
  {
    laggingGrayCollapseAboveGrayFraction = 0.30;
  }
  if (laggingGrayCollapseBelowXhStepMax < 0.0)
  {
    laggingGrayCollapseBelowXhStepMax = 0.06;
  }
  if (laggingGrayCollapseMinGrayDrop < 0.0)
  {
    laggingGrayCollapseMinGrayDrop = 0.005;
  }
  laggingGrayCollapsePauseInterval =
    Foam::max(laggingGrayCollapsePauseInterval, label(4));
  overactiveTopologyStartIter =
    Foam::max(overactiveTopologyStartIter, label(180));
  if (overactiveTopologyBelowGrayFraction < 0.0)
  {
    overactiveTopologyBelowGrayFraction = 0.16;
  }
  if (overactiveTopologyAboveXhStepMax < 0.0)
  {
    overactiveTopologyAboveXhStepMax = 0.20;
  }
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
  if (adjointExplicitStressScale < 0.0)
  {
    adjointExplicitStressScale = 0.35;
  }
}
else
{
  FatalErrorInFunction
    << "Unknown experimentControl.profile '" << experimentProfile << "'. "
    << "Supported values are baseline, laminarGrayRamp, "
    << "relaxedGateGrayRamp, betaFloorGrayRamp, "
    << "branchRefinement400, "
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
if (adjointExplicitStressScale < 0.0)
{
  adjointExplicitStressScale = useFullAdjointSymmetricStress ? 1.0 : 0.0;
}
if (adjointExplicitStressScale > 1.0)
{
  FatalErrorInFunction
    << "adjointControl.adjointExplicitStressScale must be <= 1. Found "
    << adjointExplicitStressScale
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
if
(
  lateStageStrictGateBelowGrayFraction < 0.0
  && lateStageStrictGateBelowGrayFraction != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.lateStageStrictGateBelowGrayFraction must be -1 or lie in [0,1]. Found "
    << lateStageStrictGateBelowGrayFraction
    << exit(FatalError);
}
if (lateStageStrictGateBelowGrayFraction > 1.0)
{
  FatalErrorInFunction
    << "experimentControl.lateStageStrictGateBelowGrayFraction must be <= 1. Found "
    << lateStageStrictGateBelowGrayFraction
    << exit(FatalError);
}
if
(
  lateStageContinuationFeasibilityTol < 0.0
  && lateStageContinuationFeasibilityTol != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.lateStageContinuationFeasibilityTol must be -1 or > 0. Found "
    << lateStageContinuationFeasibilityTol
    << exit(FatalError);
}
if (lateStageRefinementStartIter < 0)
{
  FatalErrorInFunction
    << "experimentControl.lateStageRefinementStartIter must be >= 0. Found "
    << lateStageRefinementStartIter
    << exit(FatalError);
}
if
(
  lateStageRefinementBelowGrayFraction < 0.0
  && lateStageRefinementBelowGrayFraction != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.lateStageRefinementBelowGrayFraction must be -1 or lie in [0,1]. Found "
    << lateStageRefinementBelowGrayFraction
    << exit(FatalError);
}
if (lateStageRefinementBelowGrayFraction > 1.0)
{
  FatalErrorInFunction
    << "experimentControl.lateStageRefinementBelowGrayFraction must be <= 1. Found "
    << lateStageRefinementBelowGrayFraction
    << exit(FatalError);
}
if
(
  lateStageBetaRampFactor < 0.0
  && lateStageBetaRampFactor != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.lateStageBetaRampFactor must be -1 or >= 0. Found "
    << lateStageBetaRampFactor
    << exit(FatalError);
}
if
(
  lateStageAlphaRampFactor < 0.0
  && lateStageAlphaRampFactor != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.lateStageAlphaRampFactor must be -1 or >= 0. Found "
    << lateStageAlphaRampFactor
    << exit(FatalError);
}
if
(
  lateStageMaxBeta < 0.0
  && lateStageMaxBeta != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.lateStageMaxBeta must be -1 or >= 0. Found "
    << lateStageMaxBeta
    << exit(FatalError);
}
if (lateStageFilterRStartIter < 0)
{
  FatalErrorInFunction
    << "experimentControl.lateStageFilterRStartIter must be >= 0. Found "
    << lateStageFilterRStartIter
    << exit(FatalError);
}
if
(
  lateStageFilterRCap < 0.0
  && lateStageFilterRCap != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.lateStageFilterRCap must be -1 or > 0. Found "
    << lateStageFilterRCap
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
if (overactiveTopologyStartIter < 0)
{
  FatalErrorInFunction
    << "experimentControl.overactiveTopologyStartIter must be >= 0. Found "
    << overactiveTopologyStartIter
    << exit(FatalError);
}
if
(
  overactiveTopologyBelowGrayFraction < 0.0
  && overactiveTopologyBelowGrayFraction != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.overactiveTopologyBelowGrayFraction must be -1 or lie in [0,1]. Found "
    << overactiveTopologyBelowGrayFraction
    << exit(FatalError);
}
if (overactiveTopologyBelowGrayFraction > 1.0)
{
  FatalErrorInFunction
    << "experimentControl.overactiveTopologyBelowGrayFraction must be <= 1. Found "
    << overactiveTopologyBelowGrayFraction
    << exit(FatalError);
}
if
(
  overactiveTopologyAboveXhStepMax < 0.0
  && overactiveTopologyAboveXhStepMax != -1.0
)
{
  FatalErrorInFunction
    << "experimentControl.overactiveTopologyAboveXhStepMax must be -1 or >= 0. Found "
    << overactiveTopologyAboveXhStepMax
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
if (geometricSizeAggregationKSFactor <= 0.0)
{
  FatalErrorInFunction
    << "geometricSizeControlDict.general.aggregationKSFactor must be > 0. Found "
    << geometricSizeAggregationKSFactor
    << exit(FatalError);
}
if (geometricSizeSmoothPositiveEps <= 0.0)
{
  FatalErrorInFunction
    << "geometricSizeControlDict.general.smoothPositiveEps must be > 0. Found "
    << geometricSizeSmoothPositiveEps
    << exit(FatalError);
}
if (useMinSolidGeometryControl && minSolidGeometryFeatureSize <= 0.0)
{
  FatalErrorInFunction
    << "geometricSizeControlDict.minSolidGeometryControl.featureSize must be > 0 when active=true. Found "
    << minSolidGeometryFeatureSize
    << exit(FatalError);
}
if (useMaxSolidGeometryControl && maxSolidGeometryFeatureSize <= 0.0)
{
  FatalErrorInFunction
    << "geometricSizeControlDict.maxSolidGeometryControl.featureSize must be > 0 when active=true. Found "
    << maxSolidGeometryFeatureSize
    << exit(FatalError);
}
if
(
  minSolidGeometryFillFraction < 0.0
  || minSolidGeometryFillFraction > 1.0
)
{
  FatalErrorInFunction
    << "geometricSizeControlDict.minSolidGeometryControl.fillFraction must lie in [0,1]. Found "
    << minSolidGeometryFillFraction
    << exit(FatalError);
}
if
(
  maxSolidGeometryFillFraction < 0.0
  || maxSolidGeometryFillFraction > 1.0
)
{
  FatalErrorInFunction
    << "geometricSizeControlDict.maxSolidGeometryControl.fillFraction must lie in [0,1]. Found "
    << maxSolidGeometryFillFraction
    << exit(FatalError);
}
if (minSolidGeometryConstraintTolerance < 0.0)
{
  FatalErrorInFunction
    << "geometricSizeControlDict.minSolidGeometryControl.tolerance must be >= 0. Found "
    << minSolidGeometryConstraintTolerance
    << exit(FatalError);
}
if (maxSolidGeometryConstraintTolerance < 0.0)
{
  FatalErrorInFunction
    << "geometricSizeControlDict.maxSolidGeometryControl.tolerance must be >= 0. Found "
    << maxSolidGeometryConstraintTolerance
    << exit(FatalError);
}
if (minSolidGeometryConstraintWeight <= 0.0)
{
  FatalErrorInFunction
    << "geometricSizeControlDict.minSolidGeometryControl.constraintWeight must be > 0. Found "
    << minSolidGeometryConstraintWeight
    << exit(FatalError);
}
if (maxSolidGeometryConstraintWeight <= 0.0)
{
  FatalErrorInFunction
    << "geometricSizeControlDict.maxSolidGeometryControl.constraintWeight must be > 0. Found "
    << maxSolidGeometryConstraintWeight
    << exit(FatalError);
}
if
(
  useMinSolidGeometryControl
  && useMaxSolidGeometryControl
  && maxSolidGeometryFeatureSize <= minSolidGeometryFeatureSize
)
{
  FatalErrorInFunction
    << "The solid max-size featureSize must exceed the solid min-size featureSize. Found max="
    << maxSolidGeometryFeatureSize
    << " min=" << minSolidGeometryFeatureSize
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
const IOobject::writeOption geometricDiagnosticWriteOption =
  writeGeometricSizeDiagnosticFields ? IOobject::AUTO_WRITE : IOobject::NO_WRITE;
volScalarField solidIndicator
(
    IOobject
    (
        "solidIndicator",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        geometricDiagnosticWriteOption
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField solidMinFiltered
(
    IOobject
    (
        "solidMinFiltered",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        geometricDiagnosticWriteOption
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField solidMaxFiltered
(
    IOobject
    (
        "solidMaxFiltered",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        geometricDiagnosticWriteOption
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField solidMinViolation
(
    IOobject
    (
        "solidMinViolation",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        geometricDiagnosticWriteOption
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField solidMaxViolation
(
    IOobject
    (
        "solidMaxViolation",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        geometricDiagnosticWriteOption
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField gsensSolidMinGeom
(
    IOobject
    (
        "gsensSolidMinGeom",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField gsensSolidMaxGeom
(
    IOobject
    (
        "gsensSolidMaxGeom",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField gsensSolidMin
(
    IOobject
    (
        "gsensSolidMin",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField gsenshSolidMin(gsensSolidMin);
volScalarField gsensSolidMax
(
    IOobject
    (
        "gsensSolidMax",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("zero", dimless, 0.0),
    zeroGradientFvPatchScalarField::typeName
);
volScalarField gsenshSolidMax(gsensSolidMax);

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
