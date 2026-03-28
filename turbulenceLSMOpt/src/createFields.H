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
dictionary hybridStageControl;
if (tuneOptParameters.found("hybridStageControl"))
{
  hybridStageControl = tuneOptParameters.subDict("hybridStageControl");
}
dictionary lsmControl;
if (tuneOptParameters.found("lsmControl"))
{
  lsmControl = tuneOptParameters.subDict("lsmControl");
}
dictionary lsmSensitivityControl;
if (tuneOptParameters.found("lsmSensitivityControl"))
{
  lsmSensitivityControl = tuneOptParameters.subDict("lsmSensitivityControl");
}
dictionary geometryControl;
if (tuneOptParameters.found("geometryControl"))
{
  geometryControl = tuneOptParameters.subDict("geometryControl");
}
dictionary wallDistanceControl;
if (tuneOptParameters.found("wallDistanceControl"))
{
  wallDistanceControl = tuneOptParameters.subDict("wallDistanceControl");
}

dictionary regionwiseChannelWidthMinDict;
dictionary regionwiseChannelWidthMaxDict;
dictionary regionwiseRibThicknessMinDict;
dictionary regionwiseRibThicknessMaxDict;

if (geometryControl.found("regionwiseChannelWidthMin"))
{
  regionwiseChannelWidthMinDict = geometryControl.subDict("regionwiseChannelWidthMin");
}
if (geometryControl.found("regionwiseChannelWidthMax"))
{
  regionwiseChannelWidthMaxDict = geometryControl.subDict("regionwiseChannelWidthMax");
}
if (geometryControl.found("regionwiseRibThicknessMin"))
{
  regionwiseRibThicknessMinDict = geometryControl.subDict("regionwiseRibThicknessMin");
}
if (geometryControl.found("regionwiseRibThicknessMax"))
{
  regionwiseRibThicknessMaxDict = geometryControl.subDict("regionwiseRibThicknessMax");
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
Switch useHybridMMAtoLSM
(
  hybridStageControl.lookupOrDefault<Switch>("useHybridMMAtoLSM", true)
);
scalar lsmSwitchGrayFraction
(
  hybridStageControl.lookupOrDefault<scalar>("lsmSwitchGrayFraction", 0.20)
);
scalar lsmSwitchPowerFeasibility
(
  hybridStageControl.lookupOrDefault<scalar>("lsmSwitchPowerFeasibility", 1.05)
);
scalar lsmSwitchMaxXhStep
(
  hybridStageControl.lookupOrDefault<scalar>("lsmSwitchMaxXhStep", 0.08)
);
scalar lsmSwitchMinBeta
(
  hybridStageControl.lookupOrDefault<scalar>("lsmSwitchMinBeta", 12.0)
);
label lsmSwitchMinIterations
(
  hybridStageControl.lookupOrDefault<label>("lsmSwitchMinIterations", 120)
);
Switch lsmSwitchRequireConnectivity
(
  hybridStageControl.lookupOrDefault<Switch>("lsmSwitchRequireConnectivity", true)
);
label lsmBlendIterations
(
  hybridStageControl.lookupOrDefault<label>("lsmBlendIterations", 5)
);
Switch lsmRollbackEnabled
(
  hybridStageControl.lookupOrDefault<Switch>("lsmRollbackEnabled", true)
);
Switch requireRestartableLSMCheckpoint
(
  hybridStageControl.lookupOrDefault<Switch>("requireRestartableLSMCheckpoint", true)
);
Switch allowTopologyChangeInLSM
(
  hybridStageControl.lookupOrDefault<Switch>("allowTopologyChangeInLSM", false)
);

Switch useLevelSetStage
(
  lsmControl.lookupOrDefault<Switch>("useLevelSetStage", true)
);
scalar phiBandWidthCells
(
  lsmControl.lookupOrDefault<scalar>("phiBandWidthCells", 1.5)
);
scalar narrowBandHalfWidthCells
(
  lsmControl.lookupOrDefault<scalar>("narrowBandHalfWidthCells", 4.0)
);
label phiReinitInterval
(
  lsmControl.lookupOrDefault<label>("phiReinitInterval", 5)
);
scalar phiGradDriftTol
(
  lsmControl.lookupOrDefault<scalar>("phiGradDriftTol", 0.25)
);
scalar lsmPseudoDt
(
  lsmControl.lookupOrDefault<scalar>("lsmPseudoDt", 0.0)
);
scalar lsmCfl
(
  lsmControl.lookupOrDefault<scalar>("lsmCfl", 0.50)
);
Switch useVelocityHelmholtz
(
  lsmControl.lookupOrDefault<Switch>("useVelocityHelmholtz", true)
);
scalar velocityFilterRadius
(
  lsmControl.lookupOrDefault<scalar>("velocityFilterRadius", 2.0)
);
Switch useCurvaturePenalty
(
  lsmControl.lookupOrDefault<Switch>("useCurvaturePenalty", true)
);
scalar curvaturePenaltyWeight
(
  lsmControl.lookupOrDefault<scalar>("curvaturePenaltyWeight", 0.05)
);
Switch useReactionDiffusionComplexityControl
(
  lsmControl.lookupOrDefault<Switch>("useReactionDiffusionComplexityControl", false)
);

Switch useApproximateNarrowBandSensitivityBridge
(
  lsmSensitivityControl.lookupOrDefault<Switch>("useApproximateNarrowBandSensitivityBridge", true)
);
scalar lsmSensitivitySmoothingRadius
(
  lsmSensitivityControl.lookupOrDefault<scalar>("lsmSensitivitySmoothingRadius", 2.0)
);
Switch normalizeInterfaceVelocity
(
  lsmSensitivityControl.lookupOrDefault<Switch>("normalizeInterfaceVelocity", true)
);
Switch finiteDifferenceCheckStage2
(
  lsmSensitivityControl.lookupOrDefault<Switch>("finiteDifferenceCheckStage2", false)
);

Switch useChannelWidthControl
(
  geometryControl.lookupOrDefault<Switch>("useChannelWidthControl", true)
);
scalar channelWidthMin
(
  geometryControl.lookupOrDefault<scalar>("channelWidthMin", 4.0)
);
scalar channelWidthMax
(
  geometryControl.lookupOrDefault<scalar>("channelWidthMax", 14.0)
);
Switch useRegionwiseWidthTargets
(
  geometryControl.lookupOrDefault<Switch>("useRegionwiseWidthTargets", false)
);
Switch useBranchwiseWidthTargets
(
  geometryControl.lookupOrDefault<Switch>("useBranchwiseWidthTargets", false)
);
Switch useRibThicknessControl
(
  geometryControl.lookupOrDefault<Switch>("useRibThicknessControl", true)
);
scalar ribThicknessMin
(
  geometryControl.lookupOrDefault<scalar>("ribThicknessMin", 3.0)
);
Switch useRibThicknessMaxControl
(
  geometryControl.lookupOrDefault<Switch>("useRibThicknessMaxControl", false)
);
scalar ribThicknessMax
(
  geometryControl.lookupOrDefault<scalar>("ribThicknessMax", 12.0)
);
Switch useRegionwiseRibTargets
(
  geometryControl.lookupOrDefault<Switch>("useRegionwiseRibTargets", false)
);
Switch useBranchwiseRibTargets
(
  geometryControl.lookupOrDefault<Switch>("useBranchwiseRibTargets", false)
);
Switch useProtectedGeometryMasks
(
  geometryControl.lookupOrDefault<Switch>("useProtectedGeometryMasks", false)
);
Switch applyRibTargetsToInternalRibsOnly
(
  geometryControl.lookupOrDefault<Switch>("applyRibTargetsToInternalRibsOnly", true)
);
Switch useHydraulicDiameterReporting
(
  geometryControl.lookupOrDefault<Switch>("useHydraulicDiameterReporting", false)
);
scalar hydraulicDiameterExtrusionThickness
(
  geometryControl.lookupOrDefault<scalar>("hydraulicDiameterExtrusionThickness", 1.0)
);
scalar ksRhoGeom
(
  geometryControl.lookupOrDefault<scalar>("ksRhoGeom", 20.0)
);
scalar sizeConstraintWeightMin
(
  geometryControl.lookupOrDefault<scalar>("sizeConstraintWeightMin", 1.0)
);
scalar sizeConstraintWeightMax
(
  geometryControl.lookupOrDefault<scalar>("sizeConstraintWeightMax", 1.0)
);
scalar widthConstraintWeight
(
  geometryControl.lookupOrDefault<scalar>("widthConstraintWeight", 1.0)
);
scalar ribConstraintWeight
(
  geometryControl.lookupOrDefault<scalar>("ribConstraintWeight", 1.0)
);
wordList protectedSolidRegionNames
(
  geometryControl.lookupOrDefault<wordList>("protectedSolidRegionNames", wordList())
);
wordList protectedFluidRegionNames
(
  geometryControl.lookupOrDefault<wordList>("protectedFluidRegionNames", wordList())
);

word lsmWallDistanceMode
(
  wallDistanceControl.lookupOrDefault<word>("lsmWallDistanceMode", "porosity")
);
Switch useLSMWallDistance
(
  wallDistanceControl.lookupOrDefault<Switch>("useLSMWallDistance", false)
);
Switch useHeatMethodWallDistanceFallback
(
  wallDistanceControl.lookupOrDefault<Switch>("useHeatMethodWallDistanceFallback", false)
);
label lsmWallDistanceBlendIterations
(
  wallDistanceControl.lookupOrDefault<label>("lsmWallDistanceBlendIterations", 5)
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
    << "useGCMMA=true is not implemented in turbulenceLSMOpt yet."
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
if (lsmSwitchGrayFraction < 0.0 || lsmSwitchGrayFraction > 1.0)
{
  FatalErrorInFunction
    << "hybridStageControl.lsmSwitchGrayFraction must lie in [0,1]. Found "
    << lsmSwitchGrayFraction
    << exit(FatalError);
}
if (lsmSwitchPowerFeasibility <= 0.0)
{
  FatalErrorInFunction
    << "hybridStageControl.lsmSwitchPowerFeasibility must be > 0. Found "
    << lsmSwitchPowerFeasibility
    << exit(FatalError);
}
if (lsmSwitchMaxXhStep < 0.0)
{
  FatalErrorInFunction
    << "hybridStageControl.lsmSwitchMaxXhStep must be >= 0. Found "
    << lsmSwitchMaxXhStep
    << exit(FatalError);
}
if (lsmSwitchMinBeta < 0.0)
{
  FatalErrorInFunction
    << "hybridStageControl.lsmSwitchMinBeta must be >= 0. Found "
    << lsmSwitchMinBeta
    << exit(FatalError);
}
if (lsmSwitchMinIterations < 0)
{
  FatalErrorInFunction
    << "hybridStageControl.lsmSwitchMinIterations must be >= 0. Found "
    << lsmSwitchMinIterations
    << exit(FatalError);
}
if (lsmBlendIterations < 0)
{
  FatalErrorInFunction
    << "hybridStageControl.lsmBlendIterations must be >= 0. Found "
    << lsmBlendIterations
    << exit(FatalError);
}
if (requireRestartableLSMCheckpoint && !lsmRollbackEnabled && useHybridMMAtoLSM)
{
  FatalErrorInFunction
    << "requireRestartableLSMCheckpoint=true requires lsmRollbackEnabled=true."
    << exit(FatalError);
}
if (phiBandWidthCells <= 0.0)
{
  FatalErrorInFunction
    << "lsmControl.phiBandWidthCells must be > 0. Found "
    << phiBandWidthCells
    << exit(FatalError);
}
if (narrowBandHalfWidthCells <= 0.0)
{
  FatalErrorInFunction
    << "lsmControl.narrowBandHalfWidthCells must be > 0. Found "
    << narrowBandHalfWidthCells
    << exit(FatalError);
}
if (phiReinitInterval < 0)
{
  FatalErrorInFunction
    << "lsmControl.phiReinitInterval must be >= 0. Found "
    << phiReinitInterval
    << exit(FatalError);
}
if (phiGradDriftTol < 0.0)
{
  FatalErrorInFunction
    << "lsmControl.phiGradDriftTol must be >= 0. Found "
    << phiGradDriftTol
    << exit(FatalError);
}
if (lsmPseudoDt < 0.0)
{
  FatalErrorInFunction
    << "lsmControl.lsmPseudoDt must be >= 0. Found "
    << lsmPseudoDt
    << exit(FatalError);
}
if (lsmCfl <= 0.0)
{
  FatalErrorInFunction
    << "lsmControl.lsmCfl must be > 0. Found "
    << lsmCfl
    << exit(FatalError);
}
if (velocityFilterRadius <= 0.0)
{
  FatalErrorInFunction
    << "lsmControl.velocityFilterRadius must be > 0. Found "
    << velocityFilterRadius
    << exit(FatalError);
}
if (curvaturePenaltyWeight < 0.0)
{
  FatalErrorInFunction
    << "lsmControl.curvaturePenaltyWeight must be >= 0. Found "
    << curvaturePenaltyWeight
    << exit(FatalError);
}
if (lsmSensitivitySmoothingRadius <= 0.0)
{
  FatalErrorInFunction
    << "lsmSensitivityControl.lsmSensitivitySmoothingRadius must be > 0. Found "
    << lsmSensitivitySmoothingRadius
    << exit(FatalError);
}
if (channelWidthMin < 0.0)
{
  FatalErrorInFunction
    << "geometryControl.channelWidthMin must be >= 0. Found "
    << channelWidthMin
    << exit(FatalError);
}
if (channelWidthMax > 0.0 && channelWidthMax < channelWidthMin)
{
  FatalErrorInFunction
    << "geometryControl.channelWidthMax must be >= channelWidthMin when enabled."
    << exit(FatalError);
}
if (ribThicknessMin < 0.0)
{
  FatalErrorInFunction
    << "geometryControl.ribThicknessMin must be >= 0. Found "
    << ribThicknessMin
    << exit(FatalError);
}
if (useRibThicknessMaxControl && ribThicknessMax < ribThicknessMin)
{
  FatalErrorInFunction
    << "geometryControl.ribThicknessMax must be >= ribThicknessMin when enabled."
    << exit(FatalError);
}
if (hydraulicDiameterExtrusionThickness <= 0.0)
{
  FatalErrorInFunction
    << "geometryControl.hydraulicDiameterExtrusionThickness must be > 0. Found "
    << hydraulicDiameterExtrusionThickness
    << exit(FatalError);
}
if (ksRhoGeom <= 0.0)
{
  FatalErrorInFunction
    << "geometryControl.ksRhoGeom must be > 0. Found "
    << ksRhoGeom
    << exit(FatalError);
}
if (sizeConstraintWeightMin < 0.0 || sizeConstraintWeightMax < 0.0)
{
  FatalErrorInFunction
    << "geometryControl.sizeConstraintWeightMin/Max must be >= 0."
    << exit(FatalError);
}
if (widthConstraintWeight < 0.0 || ribConstraintWeight < 0.0)
{
  FatalErrorInFunction
    << "geometryControl.widthConstraintWeight and ribConstraintWeight must be >= 0."
    << exit(FatalError);
}
if (useBranchwiseWidthTargets || useBranchwiseRibTargets)
{
  FatalErrorInFunction
    << "Branchwise geometry targets remain a later milestone and are not implemented yet."
    << exit(FatalError);
}
if
(
  lsmWallDistanceMode != "porosity"
  && lsmWallDistanceMode != "blend"
  && lsmWallDistanceMode != "phi"
)
{
  FatalErrorInFunction
    << "wallDistanceControl.lsmWallDistanceMode must be one of porosity, blend, or phi. Found "
    << lsmWallDistanceMode
    << exit(FatalError);
}
if (lsmWallDistanceBlendIterations < 0)
{
  FatalErrorInFunction
    << "wallDistanceControl.lsmWallDistanceBlendIterations must be >= 0. Found "
    << lsmWallDistanceBlendIterations
    << exit(FatalError);
}
if (useHeatMethodWallDistanceFallback)
{
  Info<< "wallDistanceControl.useHeatMethodWallDistanceFallback=true requested, "
      << "but the first baseline keeps porosity or phi-based wall distance only."
      << endl;
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
    << "useRobustMultiCaseObjective=true is not implemented yet for turbulenceLSMOpt."
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
