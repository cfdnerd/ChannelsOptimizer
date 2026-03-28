Info<< "Initialization\n" << endl;
const char* optimizationHistoryFile = "optimization.hst";
const char* debugOptimizerJsonFile = "debugOptimizer.jsonl";
const char* debugOptimizerLogFile = "debugOptimizer.log";
const char* gradientOptFile = "gradientOpt.log";
const char* solverConvergenceFile = "solverConvergences.log";
if (Pstream::master())
{
  std::remove(optimizationHistoryFile);
  std::remove(debugOptimizerJsonFile);
  std::remove(debugOptimizerLogFile);
  std::remove(gradientOptFile);
  std::remove(solverConvergenceFile);
  std::ofstream historyFile(optimizationHistoryFile, std::ios::out);
  historyFile
    << "# turbulenceLSMOpt optimization history\n"
    << "# Cols: Iter Stage PDActLog PDCon V MeanT MaxT SolverStat\n"
    << "# PDActLog=PDCon*PowerDiss0, PDCon=PowerDiss used by MMA\n"
    << std::left
    << std::setw(8) << "Iter"
    << std::setw(16) << "Stage"
    << std::setw(16) << "PDActLog"
    << std::setw(16) << "PDCon"
    << std::setw(16) << "V"
    << std::setw(16) << "MeanT"
    << std::setw(16) << "MaxT"
    << std::setw(12) << "SolverStat"
    << '\n';

  std::ofstream debugLogFile(debugOptimizerLogFile, std::ios::out);
  auto writeRuntimeOption =
  [&](const char* key, const scalar value)
  {
    debugLogFile << "  " << std::left << std::setw(36) << key
                 << std::scientific << std::setprecision(6) << value << "\n";
  };
  auto writeRuntimeSwitch =
  [&](const char* key, const bool value)
  {
    debugLogFile << "  " << std::left << std::setw(36) << key
                 << (value ? "true" : "false") << "\n";
  };
  auto writeRuntimeWord =
  [&](const char* key, const word& value)
  {
    debugLogFile << "  " << std::left << std::setw(36) << key
                 << value << "\n";
  };

  debugLogFile
    << "# turbulenceLSMOpt optimizer debug log\n"
    << "# Structured per-iteration diagnostics for optimizer development\n"
    << "# Format: sectioned human-readable report, one block per optimization iteration\n"
    << "\n[Runtime Option Dump]\n"
    << "\n--- optProperties ---\n";
  writeRuntimeOption("raa0", raa0);
  writeRuntimeOption("mma_init", mma_init);
  writeRuntimeOption("mma_dec", mma_dec);
  writeRuntimeOption("mma_inc", mma_inc);
  writeRuntimeOption("movlim", movlim);
  writeRuntimeOption("voluse", voluse);
  writeRuntimeOption("PowerDiss0", PowerDiss0);
  writeRuntimeOption("PowerDissMax", PowerDissMax);
  writeRuntimeOption("PowerDissRelax", PowerDissRelax);
  writeRuntimeOption("powerConstraintRelaxationRate", powerConstraintRelaxationRate);
  writeRuntimeOption("continuationFeasibilityTol", continuationFeasibilityTol);
  writeRuntimeOption("filterR", filterR);
  writeRuntimeOption("GeoDim", GeoDim);
  writeRuntimeOption("Pnorm", Pnorm);
  writeRuntimeOption("objFunction", objFunction);
  writeRuntimeOption("qAlpha0", qAlpha0);
  writeRuntimeOption("qAlphaMin", qAlphaMin);
  writeRuntimeOption("qKappa0", qKappa0);
  writeRuntimeOption("qKappaMin", qKappaMin);
  writeRuntimeOption("qHeat0", qHeat0);
  writeRuntimeOption("qHeatMin", qHeatMin);
  writeRuntimeOption("qSingle0", qSingle0);
  writeRuntimeOption("qSingleMin", qSingleMin);
  writeRuntimeOption("qContinuationStartIter", qContinuationStartIter);
  writeRuntimeOption("qContinuationInterval", qContinuationInterval);
  writeRuntimeOption("qContinuationFactor", qContinuationFactor);
  writeRuntimeOption("beta0", beta0);
  writeRuntimeOption("betaMax", betaMax);
  writeRuntimeOption("betaIncrement", betaIncrement);
  writeRuntimeOption("alphaRampEarlySlope", alphaRampEarlySlope);
  writeRuntimeOption("alphaRampLateFactor", alphaRampLateFactor);
  writeRuntimeOption("dAlphaPorosityEps", dAlphaPorosityEps);
  writeRuntimeSwitch("solid_area", solid_area);
  writeRuntimeSwitch("fluid_area", fluid_area);
  writeRuntimeSwitch("test_area", test_area);

  debugLogFile << "\n--- frameworkSwitches ---\n";
  writeRuntimeSwitch("useFrozenTurbulenceAdjoint", useFrozenTurbulenceAdjoint);
  writeRuntimeSwitch("useKEpsilonModel", useKEpsilonModel);
  writeRuntimeSwitch("useWrayAgarwalFallback", useWrayAgarwalFallback);
  writeRuntimeSwitch("usePorosityWallDistance", usePorosityWallDistance);
  writeRuntimeSwitch("useMeshWaveWallDistanceFallback", useMeshWaveWallDistanceFallback);
  writeRuntimeSwitch("useBrinkmanSinkInKEpsilon", useBrinkmanSinkInKEpsilon);
  writeRuntimeSwitch("useSplitRAMPControls", useSplitRAMPControls);
  writeRuntimeSwitch("useSingleQFallback", useSingleQFallback);
  writeRuntimeSwitch("useTurbulentThermalDiffusivity", useTurbulentThermalDiffusivity);
  writeRuntimeSwitch("usePowerConstraintRelaxation", usePowerConstraintRelaxation);
  writeRuntimeSwitch("useGCMMA", useGCMMA);
  writeRuntimeSwitch("useFullAdjointSymmetricStress", useFullAdjointSymmetricStress);
  writeRuntimeOption("optPowerConstraintWeight", optPowerConstraintWeight);
  writeRuntimeOption("optPowerViolationScaleExponent", optPowerViolationScaleExponent);
  writeRuntimeOption("optVolumeConstraintWeight", optVolumeConstraintWeight);

  debugLogFile << "\n--- objectiveSwitches ---\n";
  writeRuntimeSwitch("useLogMeanTObjective", useLogMeanTObjective);
  writeRuntimeSwitch("useKSHotspotObjective", useKSHotspotObjective);
  writeRuntimeSwitch("useVarianceObjective", useVarianceObjective);
  writeRuntimeSwitch("useRobustMultiCaseObjective", useRobustMultiCaseObjective);

  debugLogFile << "\n--- continuationSwitches ---\n";
  writeRuntimeSwitch("useStagnationTriggeredBeta", useStagnationTriggeredBeta);
  writeRuntimeSwitch("useLateStageFilterRCap", useLateStageFilterRCap);

  debugLogFile << "\n--- experimentControl ---\n";
  writeRuntimeWord("experimentProfile", experimentProfile);
  writeRuntimeSwitch("forceContinuationHardening", forceContinuationHardening);
  writeRuntimeOption("forceContinuationHardeningUntilIter", forceContinuationHardeningUntilIter);
  writeRuntimeOption("forceContinuationHardeningUntilBeta", forceContinuationHardeningUntilBeta);
  writeRuntimeOption("stopContinuationHardeningBelowGrayFraction", stopContinuationHardeningBelowGrayFraction);
  writeRuntimeOption("lateStageStrictGateBelowGrayFraction", lateStageStrictGateBelowGrayFraction);
  writeRuntimeOption("lateStageContinuationFeasibilityTol", lateStageContinuationFeasibilityTol);
  writeRuntimeOption("lateStageRefinementStartIter", lateStageRefinementStartIter);
  writeRuntimeOption("lateStageRefinementBelowGrayFraction", lateStageRefinementBelowGrayFraction);
  writeRuntimeOption("lateStageBetaRampFactor", lateStageBetaRampFactor);
  writeRuntimeOption("lateStageAlphaRampFactor", lateStageAlphaRampFactor);
  writeRuntimeOption("lateStageMaxBeta", lateStageMaxBeta);
  writeRuntimeOption("lateStageFilterRStartIter", lateStageFilterRStartIter);
  writeRuntimeOption("lateStageFilterRCap", lateStageFilterRCap);
  writeRuntimeOption("laggingGrayCollapseStartIter", laggingGrayCollapseStartIter);
  writeRuntimeOption("laggingGrayCollapseAboveGrayFraction", laggingGrayCollapseAboveGrayFraction);
  writeRuntimeOption("laggingGrayCollapseBelowXhStepMax", laggingGrayCollapseBelowXhStepMax);
  writeRuntimeOption("laggingGrayCollapseMinGrayDrop", laggingGrayCollapseMinGrayDrop);
  writeRuntimeOption("laggingGrayCollapsePauseInterval", laggingGrayCollapsePauseInterval);
  writeRuntimeOption("overactiveTopologyStartIter", overactiveTopologyStartIter);
  writeRuntimeOption("overactiveTopologyBelowGrayFraction", overactiveTopologyBelowGrayFraction);
  writeRuntimeOption("overactiveTopologyAboveXhStepMax", overactiveTopologyAboveXhStepMax);

  debugLogFile << "\n--- hybridStageControl ---\n";
  writeRuntimeSwitch("useHybridMMAtoLSM", useHybridMMAtoLSM);
  writeRuntimeOption("lsmSwitchGrayFraction", lsmSwitchGrayFraction);
  writeRuntimeOption("lsmSwitchPowerFeasibility", lsmSwitchPowerFeasibility);
  writeRuntimeOption("lsmSwitchMaxXhStep", lsmSwitchMaxXhStep);
  writeRuntimeOption("lsmSwitchMinBeta", lsmSwitchMinBeta);
  writeRuntimeOption("lsmSwitchMinIterations", lsmSwitchMinIterations);
  writeRuntimeSwitch("lsmSwitchRequireConnectivity", lsmSwitchRequireConnectivity);
  writeRuntimeOption("lsmBlendIterations", lsmBlendIterations);
  writeRuntimeSwitch("lsmRollbackEnabled", lsmRollbackEnabled);
  writeRuntimeSwitch("requireRestartableLSMCheckpoint", requireRestartableLSMCheckpoint);
  writeRuntimeSwitch("allowTopologyChangeInLSM", allowTopologyChangeInLSM);

  debugLogFile << "\n--- lsmControl ---\n";
  writeRuntimeSwitch("useLevelSetStage", useLevelSetStage);
  writeRuntimeOption("phiBandWidthCells", phiBandWidthCells);
  writeRuntimeOption("narrowBandHalfWidthCells", narrowBandHalfWidthCells);
  writeRuntimeOption("phiReinitInterval", phiReinitInterval);
  writeRuntimeOption("phiGradDriftTol", phiGradDriftTol);
  writeRuntimeOption("lsmPseudoDt", lsmPseudoDt);
  writeRuntimeOption("lsmCfl", lsmCfl);
  writeRuntimeSwitch("useVelocityHelmholtz", useVelocityHelmholtz);
  writeRuntimeOption("velocityFilterRadius", velocityFilterRadius);
  writeRuntimeSwitch("useCurvaturePenalty", useCurvaturePenalty);
  writeRuntimeOption("curvaturePenaltyWeight", curvaturePenaltyWeight);
  writeRuntimeSwitch("useReactionDiffusionComplexityControl", useReactionDiffusionComplexityControl);

  debugLogFile << "\n--- lsmSensitivityControl ---\n";
  writeRuntimeSwitch("useApproximateNarrowBandSensitivityBridge", useApproximateNarrowBandSensitivityBridge);
  writeRuntimeOption("lsmSensitivitySmoothingRadius", lsmSensitivitySmoothingRadius);
  writeRuntimeSwitch("normalizeInterfaceVelocity", normalizeInterfaceVelocity);
  writeRuntimeSwitch("finiteDifferenceCheckStage2", finiteDifferenceCheckStage2);

  debugLogFile << "\n--- geometryControl ---\n";
  writeRuntimeSwitch("useChannelWidthControl", useChannelWidthControl);
  writeRuntimeOption("channelWidthMin", channelWidthMin);
  writeRuntimeOption("channelWidthMax", channelWidthMax);
  writeRuntimeSwitch("useRegionwiseWidthTargets", useRegionwiseWidthTargets);
  writeRuntimeSwitch("useBranchwiseWidthTargets", useBranchwiseWidthTargets);
  writeRuntimeSwitch("useRibThicknessControl", useRibThicknessControl);
  writeRuntimeOption("ribThicknessMin", ribThicknessMin);
  writeRuntimeSwitch("useRibThicknessMaxControl", useRibThicknessMaxControl);
  writeRuntimeOption("ribThicknessMax", ribThicknessMax);
  writeRuntimeSwitch("useRegionwiseRibTargets", useRegionwiseRibTargets);
  writeRuntimeSwitch("useBranchwiseRibTargets", useBranchwiseRibTargets);
  writeRuntimeSwitch("useProtectedGeometryMasks", useProtectedGeometryMasks);
  writeRuntimeSwitch("applyRibTargetsToInternalRibsOnly", applyRibTargetsToInternalRibsOnly);
  writeRuntimeSwitch("useHydraulicDiameterReporting", useHydraulicDiameterReporting);
  writeRuntimeOption("hydraulicDiameterExtrusionThickness", hydraulicDiameterExtrusionThickness);
  writeRuntimeOption("ksRhoGeom", ksRhoGeom);
  writeRuntimeOption("sizeConstraintWeightMin", sizeConstraintWeightMin);
  writeRuntimeOption("sizeConstraintWeightMax", sizeConstraintWeightMax);
  writeRuntimeOption("widthConstraintWeight", widthConstraintWeight);
  writeRuntimeOption("ribConstraintWeight", ribConstraintWeight);

  debugLogFile << "\n--- wallDistanceControl ---\n";
  writeRuntimeWord("lsmWallDistanceMode", lsmWallDistanceMode);
  writeRuntimeSwitch("useLSMWallDistance", useLSMWallDistance);
  writeRuntimeSwitch("useHeatMethodWallDistanceFallback", useHeatMethodWallDistanceFallback);
  writeRuntimeOption("lsmWallDistanceBlendIterations", lsmWallDistanceBlendIterations);

  debugLogFile << "\n--- adjointControl ---\n";
  writeRuntimeOption("adjointMomentumSweeps", adjointMomentumSweeps);
  writeRuntimeOption("adjointExplicitStressScale", adjointExplicitStressScale);
  writeRuntimeSwitch("resetAdjointsEachIteration", resetAdjointsEachIteration);
  writeRuntimeSwitch("stopOnAdjointRunaway", stopOnAdjointRunaway);

  debugLogFile << "\n--- convergenceControl ---\n";
  writeRuntimeOption("optMinIterations", optMinIterations);
  writeRuntimeOption("optStallWindow", optStallWindow);
  writeRuntimeOption("optObjectiveTol", optObjectiveTol);
  writeRuntimeOption("optConstraintTol", optConstraintTol);
  writeRuntimeOption("optStepTol", optStepTol);
  writeRuntimeOption("optPowerConstraintWeight", optPowerConstraintWeight);
  writeRuntimeOption("optVolumeConstraintWeight", optVolumeConstraintWeight);

  debugLogFile << "\n--- transportProperties ---\n";
  writeRuntimeOption("nu", nu.value());
  writeRuntimeOption("alphaMax", alphaMax.value());
  writeRuntimeOption("alphaMAX", alphaMAX.value());

  debugLogFile << "\n--- thermalProperties ---\n";
  writeRuntimeOption("rhoCp", rhoCp.value());
  writeRuntimeOption("kf", kf.value());
  writeRuntimeOption("ks", ks.value());
  writeRuntimeOption("heatSource", heatSource.value());
  writeRuntimeOption("thermalSource", thermalSource.value());
  writeRuntimeOption("Prt", Prt);
  writeRuntimeOption("Tref", Tref);
  writeRuntimeOption("ksRho", ksRho);
  writeRuntimeSwitch("DDHS", DDHS);
  debugLogFile << "\n";

  std::ofstream gradientLogFile(gradientOptFile, std::ios::out);
  gradientLogFile
    << "# turbulenceLSMOpt gradient and sensitivity diagnostics\n"
    << "# Structured one-block-per-iteration report\n";

  std::ofstream solverConvergenceLogFile(solverConvergenceFile, std::ios::out);
  solverConvergenceLogFile
    << "# turbulenceLSMOpt solver convergence log\n"
    << "# Linear-solver statistics grouped by optimization iteration\n";
}

struct LinearSolveStats
{
  label callCount;
  scalar initialResidualSum;
  scalar initialResidualMax;
  scalar finalResidualSum;
  scalar finalResidualMax;
  label iterationSum;
  label iterationMax;
  scalar lastInitialResidual;
  scalar lastFinalResidual;
  label lastIterations;

  LinearSolveStats()
  :
    callCount(0),
    initialResidualSum(0),
    initialResidualMax(0),
    finalResidualSum(0),
    finalResidualMax(0),
    iterationSum(0),
    iterationMax(0),
    lastInitialResidual(0),
    lastFinalResidual(0),
    lastIterations(0)
  {}
};

auto resetLinearSolveStats =
[](LinearSolveStats& stats)
{
  stats.callCount = 0;
  stats.initialResidualSum = 0;
  stats.initialResidualMax = 0;
  stats.finalResidualSum = 0;
  stats.finalResidualMax = 0;
  stats.iterationSum = 0;
  stats.iterationMax = 0;
  stats.lastInitialResidual = 0;
  stats.lastFinalResidual = 0;
  stats.lastIterations = 0;
};

auto recordLinearSolve =
[](LinearSolveStats& stats, const scalar initialResidual, const scalar finalResidual, const label iterations)
{
  stats.callCount++;
  stats.initialResidualSum += initialResidual;
  stats.initialResidualMax = Foam::max(stats.initialResidualMax, initialResidual);
  stats.finalResidualSum += finalResidual;
  stats.finalResidualMax = Foam::max(stats.finalResidualMax, finalResidual);
  stats.iterationSum += iterations;
  stats.iterationMax = Foam::max(stats.iterationMax, iterations);
  stats.lastInitialResidual = initialResidual;
  stats.lastFinalResidual = finalResidual;
  stats.lastIterations = iterations;
};

const scalar linearSolveObsResidual = 1e-6;
const scalar linearSolveWarnResidual = 1e-4;
const label linearSolveWarnIterations = 20000;

auto classifyLinearSolveStatus =
[&](const LinearSolveStats& stats) -> const char*
{
  if (stats.callCount == 0)
  {
    return "SKIP";
  }
  if (stats.iterationMax >= linearSolveWarnIterations)
  {
    return "WARN";
  }
  if (stats.finalResidualMax > linearSolveWarnResidual)
  {
    return "WARN";
  }
  if (stats.finalResidualMax > linearSolveObsResidual)
  {
    return "OBS";
  }
  return "OK";
};

auto linearSolveSeverity =
[](const char* status) -> label
{
  if (std::strcmp(status, "WARN") == 0)
  {
    return 2;
  }
  if (std::strcmp(status, "OBS") == 0)
  {
    return 1;
  }
  return 0;
};

auto linearSolveSeverityFromStats =
[&](const LinearSolveStats& stats) -> label
{
  return linearSolveSeverity(classifyLinearSolveStatus(stats));
};

auto optimizationHistoryStatusIsAcceptable =
[](const char* status) -> bool
{
  return std::strcmp(status, "OK") == 0 || std::strcmp(status, "SKIP") == 0;
};

LinearSolveStats ULinearSolveStats;
LinearSolveStats pLinearSolveStats;
LinearSolveStats TLinearSolveStats;
LinearSolveStats TbLinearSolveStats;
LinearSolveStats UbLinearSolveStats;
LinearSolveStats pbLinearSolveStats;
LinearSolveStats UaLinearSolveStats;
LinearSolveStats paLinearSolveStats;
LinearSolveStats dAlphaLinearSolveStats;
LinearSolveStats xpFilterLinearSolveStats;
LinearSolveStats fsensFilterLinearSolveStats;
LinearSolveStats gsensPowerFilterLinearSolveStats;
LinearSolveStats gsensVolFilterLinearSolveStats;

auto optimizationHistorySolverStatus =
[&]() -> const char*
{
  return
    optimizationHistoryStatusIsAcceptable(classifyLinearSolveStatus(ULinearSolveStats))
    && optimizationHistoryStatusIsAcceptable(classifyLinearSolveStatus(pLinearSolveStats))
    && optimizationHistoryStatusIsAcceptable(classifyLinearSolveStatus(TLinearSolveStats))
    && optimizationHistoryStatusIsAcceptable(classifyLinearSolveStatus(TbLinearSolveStats))
    && optimizationHistoryStatusIsAcceptable(classifyLinearSolveStatus(UbLinearSolveStats))
    && optimizationHistoryStatusIsAcceptable(classifyLinearSolveStatus(pbLinearSolveStats))
    && optimizationHistoryStatusIsAcceptable(classifyLinearSolveStatus(UaLinearSolveStats))
    && optimizationHistoryStatusIsAcceptable(classifyLinearSolveStatus(paLinearSolveStats))
    ? "OK"
    : "WARN";
};

int i=0,opt=0;
double del=0;
scalar n(0),N(0),eta0(0),eta5(0.5),eta1(1),y0(0),y5(0);
scalar qAlphaActive(0), qKappaActive(0), qHeatActive(0);
n=x.size();//n:length of x in the i-th core
N=x.size();//N:design freedom
reduce(N, sumOp<scalar>()); 

scalar V(0),area=gSum(mesh.V()),MeanT(0),PowerDiss(0),PowerDissActual(0),PDActLog(0),PowerDissActualOverBase(0),maxT(0),sumT(0);
scalar objectiveValueCurrent(0);
scalar ksObjectiveValue(0);
scalar varianceObjectiveValue(0);
scalar previousObjectiveValue(0);
bool hasPreviousObjectiveValue(false);
bool stopOptimization(false);
label optimizerConvergedStreak(0);
word optimizerStopReason("running");
bool continuationHealthOk(true);
scalar previousGrayVolumeFractionForContinuation(1.0);
scalar previousXhStepMax(0.0);
const scalar adjointRunawayRatioLimit(1e6);
word activeObjectiveLabel("MeanT");
word activeWallDistanceMode("meshWave");
word activeTurbulenceModelMode("kEpsilon");
scalar activeFilterR(filterR);
label previousSolverSeverity(0);
const label HYBRID_STAGE_DENSITY(0);
const label HYBRID_STAGE_HANDOVER(1);
const label HYBRID_STAGE_LSM(2);
label hybridStageState(HYBRID_STAGE_DENSITY);
word hybridStageName("density");
label lsmBlendIteration(0);
label lsmStageIteration(0);
scalar lsmBlendWeight(1.0);
bool lsmCheckpointAvailable(false);
bool lsmRollbackRequested(false);
bool lsmSwitchLockedOut(false);
bool lsmSwitchTriggered(false);
label lsmNextSwitchAllowedIter(0);
word lsmRollbackReason("none");
label lsmRollbackCount(0);
bool lsmLastStepAccepted(true);
label lsmRejectedStepCount(0);
bool lsmLastConnectivitySatisfied(true);
bool lsmSwitchGateSatisfied(false);
bool lsmSwitchHealthSatisfied(false);
bool lsmSwitchCheckpointSatisfied(false);
scalar lsmLastGrayVolumeFraction(1.0);
scalar lsmLastPowerFeasibilityRatio(0.0);
scalar lsmLastSignedDistanceDrift(0.0);
scalar lsmLastTimeStep(0.0);
scalar lsmLastChannelWidthMin(0.0);
scalar lsmLastChannelWidthMax(0.0);
scalar lsmLastRibThicknessMin(0.0);
scalar lsmLastRibThicknessMax(0.0);
scalar lsmLastHydraulicDiameterMean(0.0);
scalar lsmLastChannelWidthMinKS(0.0);
scalar lsmLastChannelWidthMaxKS(0.0);
scalar lsmLastRibThicknessMinKS(0.0);
scalar lsmLastRibThicknessMaxKS(0.0);
scalar lsmLastCurvatureKS(0.0);
scalar lsmLastObjectiveVelocityMax(0.0);
scalar lsmLastPowerVelocityMax(0.0);
scalar lsmLastVolumeVelocityMax(0.0);
scalar lsmLastGeometryVelocityMax(0.0);
scalar lsmLastVelocityMax(0.0);
scalar lsmLastPhiStepMax(0.0);

scalar cellLengthSumLocal(0.0);
label cellLengthCountLocal(0);
const labelListList& cellNeighbours = mesh.cellCells();
forAll(cellNeighbours, cellI)
{
  const labelList& neighbours = cellNeighbours[cellI];
  forAll(neighbours, neighbourI)
  {
    cellLengthSumLocal += Foam::mag(mesh.C()[cellI] - mesh.C()[neighbours[neighbourI]]);
    cellLengthCountLocal++;
  }
}
reduce(cellLengthSumLocal, sumOp<scalar>());
reduce(cellLengthCountLocal, sumOp<label>());

double len =
  cellLengthCountLocal > 0
  ? cellLengthSumLocal/cellLengthCountLocal
  : std::pow(area/Foam::max(scalar(N), scalar(1)), 1.0/3);
const scalar channelWidthMinPhysical = channelWidthMin*len;
const scalar ribThicknessMinPhysical = ribThicknessMin*len;

// Geometry-control inputs are configured in mesh-length units, so convert the
// runtime target fields to physical lengths once the representative cell size
// is known.
channelWidthMinTargetField *= len;
channelWidthMaxTargetField *= len;
ribThicknessMinTargetField *= len;
ribThicknessMaxTargetField *= len;
channelWidthMinTargetField.correctBoundaryConditions();
channelWidthMaxTargetField.correctBoundaryConditions();
ribThicknessMinTargetField.correctBoundaryConditions();
ribThicknessMaxTargetField.correctBoundaryConditions();

b.value()=1.0/(activeFilterR*len/3.464)/(activeFilterR*len/3.464);
del=beta0;

Field<scalar> xCheckpoint;
Field<scalar> xpCheckpoint;
Field<scalar> xhCheckpoint;
Field<scalar> xhDensityReferenceCheckpoint;
Field<scalar> xhLevelSetCheckpoint;
Field<scalar> phiCheckpoint;
Field<scalar> pCheckpoint;
Field<scalar> paCheckpoint;
Field<scalar> pbCheckpoint;
Field<scalar> TCheckpoint;
Field<scalar> TbCheckpoint;
Field<vector> UCheckpoint;
Field<vector> UaCheckpoint;
Field<vector> UbCheckpoint;
Field<scalar> kCheckpoint;
Field<scalar> epsilonCheckpoint;
Field<scalar> nutCheckpoint;

const bool hasKEpsilonCheckpointFields =
  mesh.foundObject<volScalarField>("k")
  && mesh.foundObject<volScalarField>("epsilon")
  && mesh.foundObject<volScalarField>("nut");
volScalarField* kCheckpointField =
  hasKEpsilonCheckpointFields
  ? &const_cast<volScalarField&>(mesh.lookupObject<volScalarField>("k"))
  : nullptr;
volScalarField* epsilonCheckpointField =
  hasKEpsilonCheckpointFields
  ? &const_cast<volScalarField&>(mesh.lookupObject<volScalarField>("epsilon"))
  : nullptr;
volScalarField* nutCheckpointField =
  hasKEpsilonCheckpointFields
  ? &const_cast<volScalarField&>(mesh.lookupObject<volScalarField>("nut"))
  : nullptr;

label optCheckpoint(0);
double delCheckpoint(0.0);
scalar alphaMaxCheckpoint(alphaMax.value());
scalar activePowerLimitCheckpoint(activePowerLimit);
scalar activeFilterRCheckpoint(activeFilterR);
scalar qAlphaCheckpoint(qAlpha);
scalar qKappaCheckpoint(qKappa);
scalar qHeatCheckpoint(qHeat);
scalar qSingleCheckpoint(qSingle);
scalar previousObjectiveValueCheckpoint(0.0);
bool hasPreviousObjectiveValueCheckpoint(false);
label optimizerConvergedStreakCheckpoint(0);
scalar previousGrayVolumeFractionCheckpoint(1.0);
scalar previousXhStepMaxCheckpoint(0.0);
label previousSolverSeverityCheckpoint(0);
word activeObjectiveLabelCheckpoint("MeanT");
word activeWallDistanceModeCheckpoint("meshWave");
word activeTurbulenceModelModeCheckpoint("kEpsilon");

auto saveScalarFieldState =
[&](Field<scalar>& storage, const volScalarField& field)
{
  storage = field.primitiveField();
};
auto saveVectorFieldState =
[&](Field<vector>& storage, const volVectorField& field)
{
  storage = field.primitiveField();
};
auto restoreScalarFieldState =
[&](volScalarField& field, const Field<scalar>& storage)
{
  if (storage.size())
  {
    field.primitiveFieldRef() = storage;
    field.correctBoundaryConditions();
  }
};
auto restoreVectorFieldState =
[&](volVectorField& field, const Field<vector>& storage)
{
  if (storage.size())
  {
    field.primitiveFieldRef() = storage;
    field.correctBoundaryConditions();
  }
};

const labelListList& cellCells = mesh.cellCells();
const labelList inletCells = mesh.boundary()[conPatchList[0]].faceCells();
const labelList outletCells = mesh.boundary()[conPatchList[nObjPatch-1]].faceCells();

auto hasThresholdedFluidConnectivity =
[&](const volScalarField& indicatorField, const scalar threshold) -> bool
{
  if (inletCells.size() == 0 || outletCells.size() == 0)
  {
    return true;
  }

  std::vector<unsigned char> visited(indicatorField.size(), 0);
  std::queue<label> floodQueue;
  forAll(inletCells, inletI)
  {
    const label cellI = inletCells[inletI];
    if (cellI >= 0 && indicatorField[cellI] >= threshold && !visited[cellI])
    {
      visited[cellI] = 1;
      floodQueue.push(cellI);
    }
  }

  if (floodQueue.empty())
  {
    return false;
  }

  while (!floodQueue.empty())
  {
    const label cellI = floodQueue.front();
    floodQueue.pop();

    forAll(outletCells, outletI)
    {
      if (cellI == outletCells[outletI])
      {
        return true;
      }
    }

    const labelList& neighbours = cellCells[cellI];
    forAll(neighbours, neighbourI)
    {
      const label neighbourCell = neighbours[neighbourI];
      if
      (
        neighbourCell >= 0
        && !visited[neighbourCell]
        && indicatorField[neighbourCell] >= threshold
      )
      {
        visited[neighbourCell] = 1;
        floodQueue.push(neighbourCell);
      }
    }
  }

  return false;
};

auto saveLSMCheckpoint =
[&]()
{
  saveScalarFieldState(xCheckpoint, x);
  saveScalarFieldState(xpCheckpoint, xp);
  saveScalarFieldState(xhCheckpoint, xh);
  saveScalarFieldState(xhDensityReferenceCheckpoint, xhDensityReference);
  saveScalarFieldState(xhLevelSetCheckpoint, xhLevelSet);
  saveScalarFieldState(phiCheckpoint, phiLS);
  saveScalarFieldState(pCheckpoint, p);
  saveScalarFieldState(paCheckpoint, pa);
  saveScalarFieldState(pbCheckpoint, pb);
  saveScalarFieldState(TCheckpoint, T);
  saveScalarFieldState(TbCheckpoint, Tb);
  saveVectorFieldState(UCheckpoint, U);
  saveVectorFieldState(UaCheckpoint, Ua);
  saveVectorFieldState(UbCheckpoint, Ub);

  if (hasKEpsilonCheckpointFields)
  {
    saveScalarFieldState(kCheckpoint, *kCheckpointField);
    saveScalarFieldState(epsilonCheckpoint, *epsilonCheckpointField);
    saveScalarFieldState(nutCheckpoint, *nutCheckpointField);
  }

  optCheckpoint = opt;
  delCheckpoint = del;
  alphaMaxCheckpoint = alphaMax.value();
  activePowerLimitCheckpoint = activePowerLimit;
  activeFilterRCheckpoint = activeFilterR;
  qAlphaCheckpoint = qAlpha;
  qKappaCheckpoint = qKappa;
  qHeatCheckpoint = qHeat;
  qSingleCheckpoint = qSingle;
  previousObjectiveValueCheckpoint = previousObjectiveValue;
  hasPreviousObjectiveValueCheckpoint = hasPreviousObjectiveValue;
  optimizerConvergedStreakCheckpoint = optimizerConvergedStreak;
  previousGrayVolumeFractionCheckpoint = previousGrayVolumeFractionForContinuation;
  previousXhStepMaxCheckpoint = previousXhStepMax;
  previousSolverSeverityCheckpoint = previousSolverSeverity;
  activeObjectiveLabelCheckpoint = activeObjectiveLabel;
  activeWallDistanceModeCheckpoint = activeWallDistanceMode;
  activeTurbulenceModelModeCheckpoint = activeTurbulenceModelMode;
  lsmCheckpointAvailable = true;
};

auto restoreLSMCheckpoint =
[&]()
{
  if (!lsmCheckpointAvailable)
  {
    return;
  }

  restoreScalarFieldState(x, xCheckpoint);
  restoreScalarFieldState(xp, xpCheckpoint);
  restoreScalarFieldState(xh, xhCheckpoint);
  restoreScalarFieldState(xhDensityReference, xhDensityReferenceCheckpoint);
  restoreScalarFieldState(xhLevelSet, xhLevelSetCheckpoint);
  restoreScalarFieldState(phiLS, phiCheckpoint);
  restoreScalarFieldState(p, pCheckpoint);
  restoreScalarFieldState(pa, paCheckpoint);
  restoreScalarFieldState(pb, pbCheckpoint);
  restoreScalarFieldState(T, TCheckpoint);
  restoreScalarFieldState(Tb, TbCheckpoint);
  restoreVectorFieldState(U, UCheckpoint);
  restoreVectorFieldState(Ua, UaCheckpoint);
  restoreVectorFieldState(Ub, UbCheckpoint);

  if (hasKEpsilonCheckpointFields)
  {
    restoreScalarFieldState(*kCheckpointField, kCheckpoint);
    restoreScalarFieldState(*epsilonCheckpointField, epsilonCheckpoint);
    restoreScalarFieldState(*nutCheckpointField, nutCheckpoint);
  }

  phi = linearInterpolate(U) & mesh.Sf();
  phia = linearInterpolate(Ua) & mesh.Sf();
  phib = linearInterpolate(Ub) & mesh.Sf();

  opt = optCheckpoint;
  del = delCheckpoint;
  alphaMax.value() = alphaMaxCheckpoint;
  activePowerLimit = activePowerLimitCheckpoint;
  activeFilterR = activeFilterRCheckpoint;
  qAlpha = qAlphaCheckpoint;
  qKappa = qKappaCheckpoint;
  qHeat = qHeatCheckpoint;
  qSingle = qSingleCheckpoint;
  previousObjectiveValue = previousObjectiveValueCheckpoint;
  hasPreviousObjectiveValue = hasPreviousObjectiveValueCheckpoint;
  optimizerConvergedStreak = optimizerConvergedStreakCheckpoint;
  previousGrayVolumeFractionForContinuation = previousGrayVolumeFractionCheckpoint;
  previousXhStepMax = previousXhStepMaxCheckpoint;
  previousSolverSeverity = previousSolverSeverityCheckpoint;
  activeObjectiveLabel = activeObjectiveLabelCheckpoint;
  activeWallDistanceMode = activeWallDistanceModeCheckpoint;
  activeTurbulenceModelMode = activeTurbulenceModelModeCheckpoint;
  lsmRollbackRequested = false;
  lsmRejectedStepCount = 0;
  lsmLastStepAccepted = true;
  lsmLastPhiStepMax = 0.0;
  stopOptimization = false;
  optimizerStopReason = word("rollbackRestored");
};

auto scheduleLSMRetry =
[&](const word& reason, const scalar grayTarget, const scalar xhStepTarget)
{
  const label retryCooldown = lsmBlendIterations > 5 ? lsmBlendIterations : 5;
  lsmNextSwitchAllowedIter =
    lsmNextSwitchAllowedIter > opt + retryCooldown
    ? lsmNextSwitchAllowedIter
    : opt + retryCooldown;
  lsmSwitchLockedOut = opt < lsmNextSwitchAllowedIter;

  if (grayTarget > SMALL)
  {
    const scalar tightenedGrayGate =
      Foam::max(scalar(0.75)*grayTarget, scalar(0.02));
    lsmSwitchGrayFraction =
      Foam::min(lsmSwitchGrayFraction, tightenedGrayGate);
  }

  if (xhStepTarget > SMALL)
  {
    const scalar tightenedStepGate =
      Foam::max(scalar(0.85)*xhStepTarget, scalar(0.02));
    lsmSwitchMaxXhStep =
      Foam::min(lsmSwitchMaxXhStep, tightenedStepGate);
  }

  Info<< "Delaying the next LSM handover attempt after " << reason
      << " until iteration " << lsmNextSwitchAllowedIter
      << " with gray<=" << lsmSwitchGrayFraction
      << " and prevXhStep<=" << lsmSwitchMaxXhStep
      << endl;
};
//***********************************//MMA
int m=2;//number of constraint
MMA mma(n,m);
mma.asyminit = mma_init;
mma.asymdec = mma_dec;
mma.asyminc = mma_inc;
mma.raa0 = raa0;
mma.movelimit = movlim;
std::vector<double> xmma(n);
std::vector<double> dfdx(n);
std::vector<double> gx(m);
std::vector<std::vector<double>> dgdx(m);
for (int i = 0; i < m; i++)
{
  dgdx[i].resize(n);
}
