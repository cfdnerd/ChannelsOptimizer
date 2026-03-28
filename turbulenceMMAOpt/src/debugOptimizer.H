scalar xLocalMin = 1e300, xLocalMax = -1e300;
scalar xpLocalMin = 1e300, xpLocalMax = -1e300;
scalar xhLocalMin = 1e300, xhLocalMax = -1e300;
scalar xgLocalMin = 1e300, xgLocalMax = -1e300;
scalar alphaLocalMin = 1e300, alphaLocalMax = -1e300;
scalar DTLocalMin = 1e300, DTLocalMax = -1e300;
scalar TLocalMin = 1e300, TLocalMax = -1e300;
scalar pLocalMin = 1e300, pLocalMax = -1e300;
scalar paLocalMin = 1e300, paLocalMax = -1e300;
scalar pbLocalMin = 1e300, pbLocalMax = -1e300;
scalar xVolumeSumLocal = 0;
scalar xhVolumeSumLocal = 0;
scalar xgVolumeSumLocal = 0;
scalar grayVolumeLocal = 0;
scalar solidVolumeLocal = 0;
scalar fluidVolumeLocal = 0;
scalar maxAbsXChangeLocal = 0;
scalar l2XChangeLocal = 0;
scalar maxAbsXhChangeLocal = 0;
scalar l2XhChangeLocal = 0;
label xLowerBoundCountLocal = 0;
label xUpperBoundCountLocal = 0;
label xMoveLimitHitCountLocal = 0;
label xNonFiniteCountLocal = 0;
scalar drhoLocalMin = 1e300, drhoLocalMax = -1e300;
scalar drhoGeomLocalMin = 1e300, drhoGeomLocalMax = -1e300;
scalar fsensLocalMin = 1e300, fsensLocalMax = -1e300, fsensL2Local = 0;
scalar gsensPowerLocalMin = 1e300, gsensPowerLocalMax = -1e300, gsensPowerL2Local = 0;
scalar gsensVolLocalMin = 1e300, gsensVolLocalMax = -1e300, gsensVolL2Local = 0;
scalar gsensSolidMinLocalMin = 1e300, gsensSolidMinLocalMax = -1e300, gsensSolidMinL2Local = 0;
scalar gsensSolidMaxLocalMin = 1e300, gsensSolidMaxLocalMax = -1e300, gsensSolidMaxL2Local = 0;
label fsensNonFiniteCountLocal = 0;
label gsensPowerNonFiniteCountLocal = 0;
label gsensVolNonFiniteCountLocal = 0;
label gsensSolidMinNonFiniteCountLocal = 0;
label gsensSolidMaxNonFiniteCountLocal = 0;
scalar UMaxMagLocal = 0, UaMaxMagLocal = 0, UbMaxMagLocal = 0;
scalar UMeanMagLocal = 0, UaMeanMagLocal = 0, UbMeanMagLocal = 0;
scalar objectivePnormSumLocal = 0;

for(i=0;i<n;i++)
{
  const scalar cellVolume = mesh.V()[i];
  const scalar xValue = x[i];
  const scalar xpValue = xp[i];
  const scalar xhValue = xh[i];
  const scalar xgValue = xg[i];
  const scalar alphaValue = alpha[i];
  const scalar DTValue = DT[i];
  const scalar TValue = T[i];
  const scalar pValue = p[i];
  const scalar paValue = pa[i];
  const scalar pbValue = pb[i];
  const scalar xChange = xValue - xOld[i];
  const scalar xhChange = xhValue - xhOld[i];
  const scalar absXChange = Foam::mag(xChange);
  const scalar absXhChange = Foam::mag(xhChange);
  const scalar UMag = Foam::mag(U[i]);
  const scalar UaMag = Foam::mag(Ua[i]);
  const scalar UbMag = Foam::mag(Ub[i]);
  const scalar drhoValue = drho[i];
  const scalar drhoGeomValue = drhoGeom[i];
  const scalar fsensValue = fsensMeanT[i];
  const scalar gsensPowerValue = gsensPowerDiss[i];
  const scalar gsensVolValue = gsensVol[i];
  const scalar gsensSolidMinValue = useMinSolidGeometryControl ? gsensSolidMin[i] : 0.0;
  const scalar gsensSolidMaxValue = useMaxSolidGeometryControl ? gsensSolidMax[i] : 0.0;

  xLocalMin = Foam::min(xLocalMin, xValue);
  xLocalMax = Foam::max(xLocalMax, xValue);
  xpLocalMin = Foam::min(xpLocalMin, xpValue);
  xpLocalMax = Foam::max(xpLocalMax, xpValue);
  xhLocalMin = Foam::min(xhLocalMin, xhValue);
  xhLocalMax = Foam::max(xhLocalMax, xhValue);
  xgLocalMin = Foam::min(xgLocalMin, xgValue);
  xgLocalMax = Foam::max(xgLocalMax, xgValue);
  alphaLocalMin = Foam::min(alphaLocalMin, alphaValue);
  alphaLocalMax = Foam::max(alphaLocalMax, alphaValue);
  DTLocalMin = Foam::min(DTLocalMin, DTValue);
  DTLocalMax = Foam::max(DTLocalMax, DTValue);
  TLocalMin = Foam::min(TLocalMin, TValue);
  TLocalMax = Foam::max(TLocalMax, TValue);
  pLocalMin = Foam::min(pLocalMin, pValue);
  pLocalMax = Foam::max(pLocalMax, pValue);
  paLocalMin = Foam::min(paLocalMin, paValue);
  paLocalMax = Foam::max(paLocalMax, paValue);
  pbLocalMin = Foam::min(pbLocalMin, pbValue);
  pbLocalMax = Foam::max(pbLocalMax, pbValue);
  UMaxMagLocal = Foam::max(UMaxMagLocal, UMag);
  UaMaxMagLocal = Foam::max(UaMaxMagLocal, UaMag);
  UbMaxMagLocal = Foam::max(UbMaxMagLocal, UbMag);

  xVolumeSumLocal += xValue*cellVolume;
  xhVolumeSumLocal += xhValue*cellVolume;
  xgVolumeSumLocal += xgValue*cellVolume;
  UMeanMagLocal += UMag*cellVolume;
  UaMeanMagLocal += UaMag*cellVolume;
  UbMeanMagLocal += UbMag*cellVolume;

  if (xhValue > 0.05 && xhValue < 0.95)
  {
    grayVolumeLocal += cellVolume;
  }
  if (xhValue <= 0.05)
  {
    solidVolumeLocal += cellVolume;
  }
  if (xhValue >= 0.95)
  {
    fluidVolumeLocal += cellVolume;
  }

  maxAbsXChangeLocal = Foam::max(maxAbsXChangeLocal, absXChange);
  l2XChangeLocal += xChange*xChange;
  maxAbsXhChangeLocal = Foam::max(maxAbsXhChangeLocal, absXhChange);
  l2XhChangeLocal += xhChange*xhChange;

  if (xValue <= 1e-8)
  {
    xLowerBoundCountLocal++;
  }
  if (xValue >= 1.0 - 1e-8)
  {
    xUpperBoundCountLocal++;
  }
  if (absXChange >= 0.999*movlim)
  {
    xMoveLimitHitCountLocal++;
  }
  if (!std::isfinite(static_cast<double>(xValue)))
  {
    xNonFiniteCountLocal++;
  }

  drhoLocalMin = Foam::min(drhoLocalMin, drhoValue);
  drhoLocalMax = Foam::max(drhoLocalMax, drhoValue);
  drhoGeomLocalMin = Foam::min(drhoGeomLocalMin, drhoGeomValue);
  drhoGeomLocalMax = Foam::max(drhoGeomLocalMax, drhoGeomValue);

  fsensLocalMin = Foam::min(fsensLocalMin, fsensValue);
  fsensLocalMax = Foam::max(fsensLocalMax, fsensValue);
  fsensL2Local += fsensValue*fsensValue;
  if (!std::isfinite(static_cast<double>(fsensValue)))
  {
    fsensNonFiniteCountLocal++;
  }

  gsensPowerLocalMin = Foam::min(gsensPowerLocalMin, gsensPowerValue);
  gsensPowerLocalMax = Foam::max(gsensPowerLocalMax, gsensPowerValue);
  gsensPowerL2Local += gsensPowerValue*gsensPowerValue;
  if (!std::isfinite(static_cast<double>(gsensPowerValue)))
  {
    gsensPowerNonFiniteCountLocal++;
  }

  gsensVolLocalMin = Foam::min(gsensVolLocalMin, gsensVolValue);
  gsensVolLocalMax = Foam::max(gsensVolLocalMax, gsensVolValue);
  gsensVolL2Local += gsensVolValue*gsensVolValue;
  if (!std::isfinite(static_cast<double>(gsensVolValue)))
  {
    gsensVolNonFiniteCountLocal++;
  }
  if (useMinSolidGeometryControl)
  {
    gsensSolidMinLocalMin = Foam::min(gsensSolidMinLocalMin, gsensSolidMinValue);
    gsensSolidMinLocalMax = Foam::max(gsensSolidMinLocalMax, gsensSolidMinValue);
    gsensSolidMinL2Local += gsensSolidMinValue*gsensSolidMinValue;
    if (!std::isfinite(static_cast<double>(gsensSolidMinValue)))
    {
      gsensSolidMinNonFiniteCountLocal++;
    }
  }
  if (useMaxSolidGeometryControl)
  {
    gsensSolidMaxLocalMin = Foam::min(gsensSolidMaxLocalMin, gsensSolidMaxValue);
    gsensSolidMaxLocalMax = Foam::max(gsensSolidMaxLocalMax, gsensSolidMaxValue);
    gsensSolidMaxL2Local += gsensSolidMaxValue*gsensSolidMaxValue;
    if (!std::isfinite(static_cast<double>(gsensSolidMaxValue)))
    {
      gsensSolidMaxNonFiniteCountLocal++;
    }
  }

  if (objFunction == 2 && maxT > SMALL)
  {
    objectivePnormSumLocal += std::pow(TValue/maxT, Pnorm);
  }
}

reduce(xLocalMin, minOp<scalar>());
reduce(xLocalMax, maxOp<scalar>());
reduce(xpLocalMin, minOp<scalar>());
reduce(xpLocalMax, maxOp<scalar>());
reduce(xhLocalMin, minOp<scalar>());
reduce(xhLocalMax, maxOp<scalar>());
reduce(xgLocalMin, minOp<scalar>());
reduce(xgLocalMax, maxOp<scalar>());
reduce(alphaLocalMin, minOp<scalar>());
reduce(alphaLocalMax, maxOp<scalar>());
reduce(DTLocalMin, minOp<scalar>());
reduce(DTLocalMax, maxOp<scalar>());
reduce(TLocalMin, minOp<scalar>());
reduce(TLocalMax, maxOp<scalar>());
reduce(pLocalMin, minOp<scalar>());
reduce(pLocalMax, maxOp<scalar>());
reduce(paLocalMin, minOp<scalar>());
reduce(paLocalMax, maxOp<scalar>());
reduce(pbLocalMin, minOp<scalar>());
reduce(pbLocalMax, maxOp<scalar>());
reduce(UMaxMagLocal, maxOp<scalar>());
reduce(UaMaxMagLocal, maxOp<scalar>());
reduce(UbMaxMagLocal, maxOp<scalar>());
reduce(xVolumeSumLocal, sumOp<scalar>());
reduce(xhVolumeSumLocal, sumOp<scalar>());
reduce(xgVolumeSumLocal, sumOp<scalar>());
reduce(grayVolumeLocal, sumOp<scalar>());
reduce(solidVolumeLocal, sumOp<scalar>());
reduce(fluidVolumeLocal, sumOp<scalar>());
reduce(UMeanMagLocal, sumOp<scalar>());
reduce(UaMeanMagLocal, sumOp<scalar>());
reduce(UbMeanMagLocal, sumOp<scalar>());
reduce(maxAbsXChangeLocal, maxOp<scalar>());
reduce(l2XChangeLocal, sumOp<scalar>());
reduce(maxAbsXhChangeLocal, maxOp<scalar>());
reduce(l2XhChangeLocal, sumOp<scalar>());
reduce(xLowerBoundCountLocal, sumOp<label>());
reduce(xUpperBoundCountLocal, sumOp<label>());
reduce(xMoveLimitHitCountLocal, sumOp<label>());
reduce(xNonFiniteCountLocal, sumOp<label>());
reduce(drhoLocalMin, minOp<scalar>());
reduce(drhoLocalMax, maxOp<scalar>());
reduce(drhoGeomLocalMin, minOp<scalar>());
reduce(drhoGeomLocalMax, maxOp<scalar>());
reduce(fsensLocalMin, minOp<scalar>());
reduce(fsensLocalMax, maxOp<scalar>());
reduce(fsensL2Local, sumOp<scalar>());
reduce(fsensNonFiniteCountLocal, sumOp<label>());
reduce(gsensPowerLocalMin, minOp<scalar>());
reduce(gsensPowerLocalMax, maxOp<scalar>());
reduce(gsensPowerL2Local, sumOp<scalar>());
reduce(gsensPowerNonFiniteCountLocal, sumOp<label>());
reduce(gsensVolLocalMin, minOp<scalar>());
reduce(gsensVolLocalMax, maxOp<scalar>());
reduce(gsensVolL2Local, sumOp<scalar>());
reduce(gsensVolNonFiniteCountLocal, sumOp<label>());
if (useMinSolidGeometryControl)
{
  reduce(gsensSolidMinLocalMin, minOp<scalar>());
  reduce(gsensSolidMinLocalMax, maxOp<scalar>());
  reduce(gsensSolidMinL2Local, sumOp<scalar>());
  reduce(gsensSolidMinNonFiniteCountLocal, sumOp<label>());
}
if (useMaxSolidGeometryControl)
{
  reduce(gsensSolidMaxLocalMin, minOp<scalar>());
  reduce(gsensSolidMaxLocalMax, maxOp<scalar>());
  reduce(gsensSolidMaxL2Local, sumOp<scalar>());
  reduce(gsensSolidMaxNonFiniteCountLocal, sumOp<label>());
}
reduce(objectivePnormSumLocal, sumOp<scalar>());

const scalar xMean = xVolumeSumLocal/area;
const scalar xhMean = xhVolumeSumLocal/area;
const scalar xgMean = xgVolumeSumLocal/area;
const scalar grayVolumeFraction = grayVolumeLocal/area;
const scalar solidVolumeFraction = solidVolumeLocal/area;
const scalar fluidVolumeFraction = fluidVolumeLocal/area;
const scalar l2XChange = std::sqrt(l2XChangeLocal);
const scalar l2XhChange = std::sqrt(l2XhChangeLocal);
const scalar normalizedL2XChange = l2XChange/std::sqrt(Foam::max(N, scalar(1)));
const scalar normalizedL2XhChange = l2XhChange/std::sqrt(Foam::max(N, scalar(1)));
const scalar UMeanMag = UMeanMagLocal/area;
const scalar UaMeanMag = UaMeanMagLocal/area;
const scalar UbMeanMag = UbMeanMagLocal/area;
const scalar fsensL2 = std::sqrt(fsensL2Local);
const scalar gsensPowerL2 = std::sqrt(gsensPowerL2Local);
const scalar gsensVolL2 = std::sqrt(gsensVolL2Local);
const scalar gsensSolidMinMin =
  useMinSolidGeometryControl ? gsensSolidMinLocalMin : 0.0;
const scalar gsensSolidMinMax =
  useMinSolidGeometryControl ? gsensSolidMinLocalMax : 0.0;
const scalar gsensSolidMinL2 =
  useMinSolidGeometryControl ? std::sqrt(gsensSolidMinL2Local) : 0.0;
const scalar gsensSolidMaxMin =
  useMaxSolidGeometryControl ? gsensSolidMaxLocalMin : 0.0;
const scalar gsensSolidMaxMax =
  useMaxSolidGeometryControl ? gsensSolidMaxLocalMax : 0.0;
const scalar gsensSolidMaxL2 =
  useMaxSolidGeometryControl ? std::sqrt(gsensSolidMaxL2Local) : 0.0;
const scalar primalVelocityScale = Foam::max(UMaxMagLocal, SMALL);
const scalar adjointPowerToPrimalRatio = UaMaxMagLocal/primalVelocityScale;
const scalar adjointHeatToPrimalRatio = UbMaxMagLocal/primalVelocityScale;
const bool adjointRunaway =
  adjointPowerToPrimalRatio > adjointRunawayRatioLimit
  || adjointHeatToPrimalRatio > adjointRunawayRatioLimit;

scalar objectiveValue = objectiveValueCurrent;
word objectiveLabel(activeObjectiveLabel);
const scalar objectiveDelta = hasPreviousObjectiveValue ? objectiveValue - previousObjectiveValue : 0;
previousObjectiveValue = objectiveValue;
hasPreviousObjectiveValue = true;

label solverSeverity(0);
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(ULinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(pLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(TLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(TbLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(UbLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(pbLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(UaLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(paLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(dAlphaLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(xpFilterLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(fsensFilterLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(gsensPowerFilterLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(gsensVolFilterLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(gsensSolidMinFilterLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(gsensSolidMaxFilterLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(solidMinGeomFilterLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(solidMaxGeomFilterLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(solidMinGeomAdjointLinearSolveStats));
solverSeverity = Foam::max(solverSeverity, linearSolveSeverityFromStats(solidMaxGeomAdjointLinearSolveStats));
if (adjointRunaway)
{
  solverSeverity = Foam::max(solverSeverity, label(2));
}
reduce(solverSeverity, maxOp<label>());
continuationHealthOk = !adjointRunaway;

const scalar objectiveRelativeDelta = Foam::mag(objectiveDelta)/Foam::max(Foam::mag(objectiveValue), SMALL);
const scalar powerConstraintWeightSafe = Foam::max(optPowerConstraintWeight, SMALL);
const scalar powerConstraintMargin = gx[powerConstraintIndex]/powerConstraintWeightSafe;
const scalar volumeConstraintWeightSafe = Foam::max(optVolumeConstraintWeight, SMALL);
const scalar volumeConstraintMargin = gx[volumeConstraintIndex]/volumeConstraintWeightSafe;
const bool debugMinSolidConstraintApplied =
  useMinSolidGeometryControl && minSolidGeometryConstraintWeightApplied > SMALL;
const bool debugMaxSolidConstraintApplied =
  useMaxSolidGeometryControl && maxSolidGeometryConstraintWeightApplied > SMALL;
const scalar minSolidConstraintMargin =
  debugMinSolidConstraintApplied ? solidMinConstraintValue : 0.0;
const scalar maxSolidConstraintMargin =
  debugMaxSolidConstraintApplied ? solidMaxConstraintValue : 0.0;
const scalar minSolidConstraintMarginNormalized =
  debugMinSolidConstraintApplied
  ? solidMinConstraintValue/Foam::max(minSolidGeometryConstraintScaleApplied, SMALL)
  : 0.0;
const scalar maxSolidConstraintMarginNormalized =
  debugMaxSolidConstraintApplied
  ? solidMaxConstraintValue/Foam::max(maxSolidGeometryConstraintScaleApplied, SMALL)
  : 0.0;
const scalar debugContinuationPowerLimit =
  usePowerConstraintRelaxation ? activePowerLimit : PowerDissMax;
const scalar debugPowerFeasibilityRatio =
  debugContinuationPowerLimit > SMALL ? PowerDiss/debugContinuationPowerLimit : 0.0;
scalar debugContinuationFeasibilityTolActive = continuationFeasibilityTol;
const bool debugLateStageStrictGateActive =
  lateStageStrictGateBelowGrayFraction >= 0.0
  && lateStageContinuationFeasibilityTol >= 0.0
  && grayVolumeFraction <= lateStageStrictGateBelowGrayFraction;
if (debugLateStageStrictGateActive)
{
  debugContinuationFeasibilityTolActive =
    Foam::min(debugContinuationFeasibilityTolActive, lateStageContinuationFeasibilityTol);
}
const bool debugContinuationGateSatisfied =
  debugPowerFeasibilityRatio <= debugContinuationFeasibilityTolActive
  && continuationHealthOk;
const bool debugContinuationFloorActive =
  opt <= forceContinuationHardeningUntilIter
  || (forceContinuationHardeningUntilBeta >= 0.0 && del < forceContinuationHardeningUntilBeta);
const bool debugContinuationCeilingActive =
  stopContinuationHardeningBelowGrayFraction >= 0.0
  && grayVolumeFraction <= stopContinuationHardeningBelowGrayFraction;
const scalar debugGrayVolumeFractionDrop =
  previousGrayVolumeFractionForContinuation - grayVolumeFraction;
const bool debugProfileUsesAdaptiveHardeningThrottles =
  experimentProfile == "betaFloorGrayRamp"
  || experimentProfile == "branchRefinement400";
const bool debugLaggingGrayCollapseDetected =
  debugProfileUsesAdaptiveHardeningThrottles
  && !debugContinuationFloorActive
  && laggingGrayCollapseStartIter > 0
  && opt >= laggingGrayCollapseStartIter
  && laggingGrayCollapseAboveGrayFraction >= 0.0
  && laggingGrayCollapseBelowXhStepMax >= 0.0
  && laggingGrayCollapseMinGrayDrop >= 0.0
  && grayVolumeFraction >= laggingGrayCollapseAboveGrayFraction
  && previousXhStepMax <= laggingGrayCollapseBelowXhStepMax
  && debugGrayVolumeFractionDrop <= laggingGrayCollapseMinGrayDrop;
const bool debugOveractiveTopologyDetected =
  debugProfileUsesAdaptiveHardeningThrottles
  && !debugContinuationFloorActive
  && overactiveTopologyStartIter > 0
  && opt >= overactiveTopologyStartIter
  && overactiveTopologyBelowGrayFraction >= 0.0
  && overactiveTopologyAboveXhStepMax >= 0.0
  && grayVolumeFraction <= overactiveTopologyBelowGrayFraction
  && previousXhStepMax >= overactiveTopologyAboveXhStepMax;
const bool debugLaggingGrayCollapsePulseIter =
  laggingGrayCollapsePauseInterval > 0
  && ((opt - laggingGrayCollapseStartIter) % laggingGrayCollapsePauseInterval == 0);
const bool debugContinuationThrottleAllowsHardening =
  !debugLaggingGrayCollapseDetected || debugLaggingGrayCollapsePulseIter;
const bool debugContinuationHardeningEnabled =
  !debugContinuationCeilingActive
  && !debugOveractiveTopologyDetected
  && !geometryControlHardeningFrozen
  && debugContinuationThrottleAllowsHardening
  && (forceContinuationHardening || debugContinuationFloorActive || debugContinuationGateSatisfied);
const bool debugLateStageFilterRCapActive =
  useLateStageFilterRCap
  && lateStageFilterRStartIter > 0
  && lateStageFilterRCap >= 0.0
  && opt > lateStageFilterRStartIter;
const bool debugLateStageRefinementActive =
  (lateStageRefinementStartIter > 0 && opt >= lateStageRefinementStartIter)
  || (
    lateStageRefinementBelowGrayFraction >= 0.0
    && grayVolumeFraction <= lateStageRefinementBelowGrayFraction
  );
scalar debugBetaIncrementActive = betaIncrement;
scalar debugAlphaRampLateFactorActive = alphaRampLateFactor;
scalar debugBetaMaxActive = betaMax;
if (debugLateStageRefinementActive)
{
  if (lateStageBetaRampFactor >= 0.0)
  {
    debugBetaIncrementActive *= lateStageBetaRampFactor;
  }
  if (lateStageAlphaRampFactor >= 0.0)
  {
    debugAlphaRampLateFactorActive = lateStageAlphaRampFactor;
  }
  if (lateStageMaxBeta >= 0.0)
  {
    debugBetaMaxActive = Foam::min(debugBetaMaxActive, lateStageMaxBeta);
  }
}
scalar maxConstraintViolation =
  Foam::max(Foam::max(powerConstraintMargin, volumeConstraintMargin), scalar(0));
if (useMinSolidGeometryControl)
{
  maxConstraintViolation = Foam::max(maxConstraintViolation, minSolidConstraintMargin);
}
if (useMaxSolidGeometryControl)
{
  maxConstraintViolation = Foam::max(maxConstraintViolation, maxSolidConstraintMargin);
}
const scalar objectiveToVolumeSensitivityRatio = fsensL2/Foam::max(gsensVolL2, SMALL);
const scalar powerToVolumeSensitivityRatio = gsensPowerL2/Foam::max(gsensVolL2, SMALL);
const bool convergenceCandidate =
  opt >= optMinIterations
  &&
  solverSeverity < 2
  &&
  maxConstraintViolation <= optConstraintTol
  &&
  objectiveRelativeDelta <= optObjectiveTol
  &&
  maxAbsXChangeLocal <= optStepTol
  &&
  maxAbsXhChangeLocal <= optStepTol;
optimizerConvergedStreak = convergenceCandidate ? optimizerConvergedStreak + 1 : 0;
stopOptimization = optimizerConvergedStreak >= optStallWindow;
optimizerStopReason = stopOptimization ? word("converged") : word("running");
if (adjointRunaway && stopOnAdjointRunaway)
{
  stopOptimization = true;
  optimizerConvergedStreak = 0;
  optimizerStopReason = word("adjointRunaway");
}

if (Pstream::master())
{
  const scalar clockTime = runTime.elapsedClockTime();
  const char* designHealth =
    (xNonFiniteCountLocal > 0 || xLocalMin < -1e-8 || xLocalMax > 1.0 + 1e-8 || xhLocalMin < -1e-8 || xhLocalMax > 1.0 + 1e-8)
    ? "WARN"
    : (grayVolumeFraction > 0.50 ? "OBS" : "OK");
  const bool geometryConstraintViolated =
    (useMinSolidGeometryControl && minSolidConstraintMargin > 0)
    || (useMaxSolidGeometryControl && maxSolidConstraintMargin > 0);
  const char* constraintHealth =
    (powerConstraintMargin > 0 || volumeConstraintMargin > 0 || geometryConstraintViolated) ? "WARN" : "OK";
  const char* sensitivityHealth =
    (
      fsensNonFiniteCountLocal > 0
      || gsensPowerNonFiniteCountLocal > 0
      || gsensVolNonFiniteCountLocal > 0
      || (useMinSolidGeometryControl && gsensSolidMinNonFiniteCountLocal > 0)
      || (useMaxSolidGeometryControl && gsensSolidMaxNonFiniteCountLocal > 0)
    )
    ? "WARN"
    : "OK";
  const char* solverHealth =
    solverSeverity >= 2 ? "WARN" : (solverSeverity == 1 ? "OBS" : "OK");

  std::ofstream debugLogFile(debugOptimizerLogFile, std::ios::app);
  const int itemWidth = 18;
  const int valueWidth = 24;
  const int tableWidth = itemWidth + 4*valueWidth;
  auto fmtScalar =
  [&](const scalar value)
  {
    std::ostringstream os;
    os << std::scientific << std::setprecision(6) << value;
    return os.str();
  };
  auto fmtCount =
  [&](const label value)
  {
    std::ostringstream os;
    os << value;
    return os.str();
  };
  auto writeSectionHeader =
  [&](const char* title)
  {
    debugLogFile
      << "\n[" << title << "]\n"
      << "  "
      << std::left
      << std::setw(itemWidth) << "Item"
      << std::setw(valueWidth) << "Value 1"
      << std::setw(valueWidth) << "Value 2"
      << std::setw(valueWidth) << "Value 3"
      << std::setw(valueWidth) << "Value 4"
      << "\n"
      << "  "
      << std::string(tableWidth, '-')
      << "\n";
  };
  auto writeRow =
  [&](const char* item, const std::string& value1, const std::string& value2 = "", const std::string& value3 = "", const std::string& value4 = "")
  {
    debugLogFile
      << "  "
      << std::left
      << std::setw(itemWidth) << item
      << std::setw(valueWidth) << value1
      << std::setw(valueWidth) << value2
      << std::setw(valueWidth) << value3
      << std::setw(valueWidth) << value4
      << "\n";
  };

  debugLogFile
    << "\n"
    << "====================================================================================================\n"
    << " Optimizer Debug Dashboard\n"
    << "----------------------------------------------------------------------------------------------------\n";

  writeSectionHeader("Overview");
  writeRow("Run", "Iter="+fmtCount(opt), "Clock[s]="+fmtScalar(clockTime), "ObjMode="+std::string(objectiveLabel.c_str()), "ObjVal="+fmtScalar(objectiveValue));
  writeRow("Health", std::string("Design=")+designHealth, std::string("Constr=")+constraintHealth, std::string("Sens=")+sensitivityHealth, std::string("Solver=")+solverHealth);
  writeRow("Convergence", "objRel="+fmtScalar(objectiveRelativeDelta), "maxViol="+fmtScalar(maxConstraintViolation), "xhStep="+fmtScalar(maxAbsXhChangeLocal), "streak="+fmtCount(optimizerConvergedStreak)+"/"+fmtCount(optStallWindow));
  writeRow("Stop", "candidate="+std::string(convergenceCandidate ? "yes" : "no"), "stop="+std::string(stopOptimization ? "yes" : "no"), "reason="+std::string(optimizerStopReason.c_str()));

  writeSectionHeader("Design Diagnostics");
  writeRow("x bounds", "min="+fmtScalar(xLocalMin), "max="+fmtScalar(xLocalMax), "mean="+fmtScalar(xMean));
  writeRow("xp bounds", "min="+fmtScalar(xpLocalMin), "max="+fmtScalar(xpLocalMax));
  writeRow("xh bounds", "min="+fmtScalar(xhLocalMin), "max="+fmtScalar(xhLocalMax), "mean="+fmtScalar(xhMean));
  writeRow("xg bounds", "min="+fmtScalar(xgLocalMin), "max="+fmtScalar(xgLocalMax), "mean="+fmtScalar(xgMean));
  writeRow("xh phases", "gray="+fmtScalar(grayVolumeFraction), "solid="+fmtScalar(solidVolumeFraction), "fluid="+fmtScalar(fluidVolumeFraction));
  writeRow("x step", "max="+fmtScalar(maxAbsXChangeLocal), "l2="+fmtScalar(l2XChange), "l2/sqrtN="+fmtScalar(normalizedL2XChange));
  writeRow("xh step", "max="+fmtScalar(maxAbsXhChangeLocal), "l2="+fmtScalar(l2XhChange), "l2/sqrtN="+fmtScalar(normalizedL2XhChange));

  writeSectionHeader("Filter Diagnostics");
  writeRow("projection", "beta="+fmtScalar(del), "eta5="+fmtScalar(eta5));
  writeRow("drho bounds", "min="+fmtScalar(drhoLocalMin), "max="+fmtScalar(drhoLocalMax));
  writeRow("geom proj", "beta="+fmtScalar(geometricSizeProjectionBeta), "eta="+fmtScalar(geometryProjectionEtaActive), "drhoMin="+fmtScalar(drhoGeomLocalMin), "drhoMax="+fmtScalar(drhoGeomLocalMax));
  writeRow("obj sens", "min="+fmtScalar(fsensLocalMin), "max="+fmtScalar(fsensLocalMax));
  writeRow("power sens", "min="+fmtScalar(gsensPowerLocalMin), "max="+fmtScalar(gsensPowerLocalMax));
  writeRow("vol sens", "min="+fmtScalar(gsensVolLocalMin), "max="+fmtScalar(gsensVolLocalMax));

  writeSectionHeader("Objective And Constraints");
  writeRow("objective", "value="+fmtScalar(objectiveValue), "delta="+fmtScalar(objectiveDelta));
  writeRow("power diss", "actual="+fmtScalar(PowerDissActual), "act/base="+fmtScalar(PowerDissActualOverBase), "constraint="+fmtScalar(PowerDiss), "limit="+fmtScalar(activePowerLimit));
  writeRow("volume", "value="+fmtScalar(V), "margin="+fmtScalar(volumeConstraintMargin));
  if (useMinSolidGeometryControl)
  {
    writeRow
    (
      "solid min",
      "value="+fmtScalar(solidMinConstraintValue),
      "margin="+fmtScalar(minSolidConstraintMargin),
      "mmaNorm="+fmtScalar(minSolidConstraintMarginNormalized),
      "w="+fmtScalar(minSolidGeometryConstraintWeightApplied)
    );
  }
  if (useMaxSolidGeometryControl)
  {
    writeRow
    (
      "solid max",
      "value="+fmtScalar(solidMaxConstraintValue),
      "margin="+fmtScalar(maxSolidConstraintMargin),
      "mmaNorm="+fmtScalar(maxSolidConstraintMarginNormalized),
      "w="+fmtScalar(maxSolidGeometryConstraintWeightApplied)
    );
  }
  if (geometryControlConfigured)
  {
    writeRow
    (
      "geom size ctl",
      "stage="+std::string(geometryControlStageActivated ? "on" : "off"),
      "grayGate="+std::string(geometryControlGrayGateSatisfied ? "yes" : "no"),
      "stepGate="+std::string(geometryControlStepGateSatisfied ? "yes" : "no"),
      "ramp="+fmtScalar(geometryControlStageWeight)
    );
  }
  writeRow("thermal", "meanT="+fmtScalar(MeanT), "maxT="+fmtScalar(maxT));

  writeSectionHeader("MMA Diagnostics");
  writeRow("controls", "moveLimit="+fmtScalar(movlim), "moveHits="+fmtCount(xMoveLimitHitCountLocal));
  writeRow("filter", "baseR="+fmtScalar(filterR), "activeR="+fmtScalar(activeFilterR), "capIter="+fmtCount(lateStageFilterRStartIter), "cap="+fmtScalar(lateStageFilterRCap));
  writeRow("bound clamps", "lower="+fmtCount(xLowerBoundCountLocal), "upper="+fmtCount(xUpperBoundCountLocal), "nonFiniteX="+fmtCount(xNonFiniteCountLocal));

  writeSectionHeader("Sensitivity Diagnostics");
  writeRow("objective", "min="+fmtScalar(fsensLocalMin), "max="+fmtScalar(fsensLocalMax), "l2="+fmtScalar(fsensL2), "nonFinite="+fmtCount(fsensNonFiniteCountLocal));
  writeRow("power", "min="+fmtScalar(gsensPowerLocalMin), "max="+fmtScalar(gsensPowerLocalMax), "l2="+fmtScalar(gsensPowerL2), "nonFinite="+fmtCount(gsensPowerNonFiniteCountLocal));
  writeRow("volume", "min="+fmtScalar(gsensVolLocalMin), "max="+fmtScalar(gsensVolLocalMax), "l2="+fmtScalar(gsensVolL2), "nonFinite="+fmtCount(gsensVolNonFiniteCountLocal));
  if (useMinSolidGeometryControl)
  {
    writeRow("solid min", "min="+fmtScalar(gsensSolidMinMin), "max="+fmtScalar(gsensSolidMinMax), "l2="+fmtScalar(gsensSolidMinL2), "nonFinite="+fmtCount(gsensSolidMinNonFiniteCountLocal));
  }
  if (useMaxSolidGeometryControl)
  {
    writeRow("solid max", "min="+fmtScalar(gsensSolidMaxMin), "max="+fmtScalar(gsensSolidMaxMax), "l2="+fmtScalar(gsensSolidMaxL2), "nonFinite="+fmtCount(gsensSolidMaxNonFiniteCountLocal));
  }
  writeRow("ratios", "obj/vol="+fmtScalar(objectiveToVolumeSensitivityRatio), "power/vol="+fmtScalar(powerToVolumeSensitivityRatio));

  writeSectionHeader("Solver Diagnostics");
  writeRow("config", "primal=60", "adjPower="+fmtCount(adjointMomentumSweeps), "adjHeat="+fmtCount(adjointMomentumSweeps));
  writeRow("primal flow", "|U|max="+fmtScalar(UMaxMagLocal), "|U|mean="+fmtScalar(UMeanMag), "pMin="+fmtScalar(pLocalMin), "pMax="+fmtScalar(pLocalMax));
  writeRow("thermal field", "TMin="+fmtScalar(TLocalMin), "TMax="+fmtScalar(TLocalMax));
  writeRow("adjoint power", "|Ua|max="+fmtScalar(UaMaxMagLocal), "|Ua|mean="+fmtScalar(UaMeanMag), "paMin="+fmtScalar(paLocalMin), "paMax="+fmtScalar(paLocalMax));
  writeRow("adjoint heat", "|Ub|max="+fmtScalar(UbMaxMagLocal), "|Ub|mean="+fmtScalar(UbMeanMag), "pbMin="+fmtScalar(pbLocalMin), "pbMax="+fmtScalar(pbLocalMax));
  writeRow("adjoint ratios", "Ua/U="+fmtScalar(adjointPowerToPrimalRatio), "Ub/U="+fmtScalar(adjointHeatToPrimalRatio), "limit="+fmtScalar(adjointRunawayRatioLimit), "hardening="+std::string(continuationHealthOk ? "on" : "frozen"));

  writeSectionHeader("Interpolation Diagnostics");
  writeRow("interp bounds", "alphaMin="+fmtScalar(alphaLocalMin), "alphaMax="+fmtScalar(alphaLocalMax), "DTMin="+fmtScalar(DTLocalMin), "DTMax="+fmtScalar(DTLocalMax));
  writeRow("continuation", "alphaMax="+fmtScalar(alphaMax.value()), "qAlpha="+fmtScalar(qAlpha), "qKappa="+fmtScalar(qKappa), "qHeat="+fmtScalar(qHeat));
  writeRow("hardening", "profile="+std::string(experimentProfile.c_str()), "gate="+std::string(debugContinuationGateSatisfied ? "open" : "closed"), "force="+std::string(forceContinuationHardening ? "yes" : "no"), "floor="+std::string(debugContinuationFloorActive ? "yes" : "no"));
  writeRow("hardening ctl", "PD/limit="+fmtScalar(debugPowerFeasibilityRatio), "tol<="+fmtScalar(debugContinuationFeasibilityTolActive), "iter<="+fmtCount(forceContinuationHardeningUntilIter), "beta<="+fmtScalar(forceContinuationHardeningUntilBeta));
  if (geometryControlConfigured)
  {
    writeRow
    (
      "geom stage",
      "active="+std::string(geometryControlStageActivated ? "yes" : "no"),
      "gate="+std::string(geometryControlGateSatisfied ? "open" : "closed"),
      "frozen="+std::string(geometryControlHardeningFrozen ? "yes" : "no"),
      "actIter="+fmtCount(geometryControlActivationIter)
    );
  }
  if (stopContinuationHardeningBelowGrayFraction >= 0.0)
  {
    writeRow("hardening stop", "gray<="+fmtScalar(stopContinuationHardeningBelowGrayFraction), "gray="+fmtScalar(grayVolumeFraction), "ceiling="+std::string(debugContinuationCeilingActive ? "yes" : "no"), "strict="+std::string(debugLateStageStrictGateActive ? "yes" : "no"));
  }
  if
  (
    lateStageRefinementStartIter > 0
    || lateStageRefinementBelowGrayFraction >= 0.0
    || lateStageMaxBeta >= 0.0
  )
  {
    writeRow
    (
      "late refine",
      "active="+std::string(debugLateStageRefinementActive ? "yes" : "no"),
      "betaInc="+fmtScalar(debugBetaIncrementActive),
      "alphaFac="+fmtScalar(debugAlphaRampLateFactorActive),
      "betaMax="+fmtScalar(debugBetaMaxActive)
    );
  }
  if (debugProfileUsesAdaptiveHardeningThrottles && laggingGrayCollapseStartIter > 0)
  {
    writeRow
    (
      "lag throttle",
      "active="+std::string(debugLaggingGrayCollapseDetected ? "yes" : "no"),
      "grayDrop="+fmtScalar(debugGrayVolumeFractionDrop),
      "prevXhStep="+fmtScalar(previousXhStepMax),
      "pulse="+std::string(debugLaggingGrayCollapsePulseIter ? "yes" : "no")
    );
  }
  if (debugProfileUsesAdaptiveHardeningThrottles && overactiveTopologyStartIter > 0)
  {
    writeRow
    (
      "step throttle",
      "active="+std::string(debugOveractiveTopologyDetected ? "yes" : "no"),
      "prevXhStep="+fmtScalar(previousXhStepMax),
      "gray<="+fmtScalar(overactiveTopologyBelowGrayFraction),
      "xhStep>="+fmtScalar(overactiveTopologyAboveXhStepMax)
    );
  }
  writeRow("modes", "objective="+std::string(activeObjectiveLabel.c_str()), "wallDist="+std::string(activeWallDistanceMode.c_str()), "turbulence="+std::string(activeTurbulenceModelMode.c_str()), "beta="+fmtScalar(del));
  writeRow("thermal props", "heatSource="+fmtScalar(heatSource.value()), "thermalSrc="+fmtScalar(thermalSource.value()), "rhoCp="+fmtScalar(rhoCp.value()));

  std::ofstream debugJsonFile(debugOptimizerJsonFile, std::ios::app);
  debugJsonFile
    << std::scientific << std::setprecision(8)
    << "{"
    << "\"iteration\":" << opt
    << ",\"clockTime\":" << clockTime
    << ",\"health\":{"
      << "\"design\":\"" << designHealth << "\""
      << ",\"constraints\":\"" << constraintHealth << "\""
      << ",\"sensitivity\":\"" << sensitivityHealth << "\""
      << ",\"solver\":\"" << solverHealth << "\""
    << "}"
    << ",\"convergence\":{"
      << "\"objectiveRelativeDelta\":" << objectiveRelativeDelta
      << ",\"maxConstraintViolation\":" << maxConstraintViolation
      << ",\"xStepMax\":" << maxAbsXChangeLocal
      << ",\"xhStepMax\":" << maxAbsXhChangeLocal
      << ",\"xStepL2OverSqrtN\":" << normalizedL2XChange
      << ",\"xhStepL2OverSqrtN\":" << normalizedL2XhChange
      << ",\"streak\":" << optimizerConvergedStreak
      << ",\"stallWindow\":" << optStallWindow
      << ",\"candidate\":" << (convergenceCandidate ? 1 : 0)
      << ",\"stop\":" << (stopOptimization ? 1 : 0)
      << ",\"reason\":\"" << optimizerStopReason << "\""
    << "}"
    << ",\"design\":{"
      << "\"xMin\":" << xLocalMin
      << ",\"xMax\":" << xLocalMax
      << ",\"xMean\":" << xMean
      << ",\"xpMin\":" << xpLocalMin
      << ",\"xpMax\":" << xpLocalMax
      << ",\"xhMin\":" << xhLocalMin
      << ",\"xhMax\":" << xhLocalMax
      << ",\"xhMean\":" << xhMean
      << ",\"xgMin\":" << xgLocalMin
      << ",\"xgMax\":" << xgLocalMax
      << ",\"xgMean\":" << xgMean
      << ",\"xhGrayVolumeFraction\":" << grayVolumeFraction
      << ",\"xhSolidVolumeFraction\":" << solidVolumeFraction
      << ",\"xhFluidVolumeFraction\":" << fluidVolumeFraction
      << ",\"xStepMax\":" << maxAbsXChangeLocal
      << ",\"xStepL2\":" << l2XChange
      << ",\"xhStepMax\":" << maxAbsXhChangeLocal
      << ",\"xhStepL2\":" << l2XhChange
    << "}"
      << ",\"filter\":{"
      << "\"beta\":" << del
      << ",\"eta5\":" << eta5
      << ",\"drhoMin\":" << drhoLocalMin
      << ",\"drhoMax\":" << drhoLocalMax
      << ",\"geometryProjectionBeta\":" << geometricSizeProjectionBeta
      << ",\"geometryProjectionEta\":" << geometryProjectionEtaActive
      << ",\"drhoGeomMin\":" << drhoGeomLocalMin
      << ",\"drhoGeomMax\":" << drhoGeomLocalMax
      << ",\"objectiveSensitivityFilteredMin\":" << fsensLocalMin
      << ",\"objectiveSensitivityFilteredMax\":" << fsensLocalMax
      << ",\"powerConstraintSensitivityFilteredMin\":" << gsensPowerLocalMin
      << ",\"powerConstraintSensitivityFilteredMax\":" << gsensPowerLocalMax
      << ",\"volumeConstraintSensitivityFilteredMin\":" << gsensVolLocalMin
      << ",\"volumeConstraintSensitivityFilteredMax\":" << gsensVolLocalMax
    << "}"
      << ",\"objective\":{"
      << "\"name\":\"" << objectiveLabel << "\""
      << ",\"value\":" << objectiveValue
      << ",\"delta\":" << objectiveDelta
      << ",\"powerDissipationActual\":" << PowerDissActual
      << ",\"powerDissipationActualOverPowerDiss0\":" << PowerDissActualOverBase
      << ",\"powerDissipationConstraintValue\":" << PowerDiss
      << ",\"powerDissipationConstraintMargin\":" << powerConstraintMargin
      << ",\"volumeConstraintValue\":" << V
      << ",\"volumeConstraintMargin\":" << volumeConstraintMargin
      << ",\"powerDissipationLimit\":" << debugContinuationPowerLimit
      << ",\"meanT\":" << MeanT
      << ",\"maxT\":" << maxT
    << "}"
    << ",\"geometryControl\":{"
      << "\"useMinSolid\":" << (useMinSolidGeometryControl ? 1 : 0)
      << ",\"useMaxSolid\":" << (useMaxSolidGeometryControl ? 1 : 0)
      << ",\"minSolidFeatureSize\":" << minSolidGeometryFeatureSize
      << ",\"maxSolidFeatureSize\":" << maxSolidGeometryFeatureSize
      << ",\"minSolidFillFraction\":" << minSolidGeometryFillFraction
      << ",\"maxSolidFillFraction\":" << maxSolidGeometryFillFraction
      << ",\"minSolidConstraintValue\":" << solidMinConstraintValue
      << ",\"maxSolidConstraintValue\":" << solidMaxConstraintValue
      << ",\"minSolidConstraintMargin\":" << minSolidConstraintMargin
      << ",\"maxSolidConstraintMargin\":" << maxSolidConstraintMargin
      << ",\"minSolidConstraintMarginNormalized\":" << minSolidConstraintMarginNormalized
      << ",\"maxSolidConstraintMarginNormalized\":" << maxSolidConstraintMarginNormalized
      << ",\"stageConfigured\":" << (geometryControlConfigured ? 1 : 0)
      << ",\"stageActivated\":" << (geometryControlStageActivated ? 1 : 0)
      << ",\"stageGateSatisfied\":" << (geometryControlGateSatisfied ? 1 : 0)
      << ",\"stageGrayGateSatisfied\":" << (geometryControlGrayGateSatisfied ? 1 : 0)
      << ",\"stageStepGateSatisfied\":" << (geometryControlStepGateSatisfied ? 1 : 0)
      << ",\"stageActivationIter\":" << geometryControlActivationIter
      << ",\"stageWeight\":" << geometryControlStageWeight
      << ",\"hardeningFrozen\":" << (geometryControlHardeningFrozen ? 1 : 0)
      << ",\"minSolidConstraintWeightApplied\":" << minSolidGeometryConstraintWeightApplied
      << ",\"maxSolidConstraintWeightApplied\":" << maxSolidGeometryConstraintWeightApplied
      << ",\"minSolidConstraintScaleApplied\":" << minSolidGeometryConstraintScaleApplied
      << ",\"maxSolidConstraintScaleApplied\":" << maxSolidGeometryConstraintScaleApplied
    << "}"
    << ",\"mma\":{"
      << "\"iteration\":" << opt
      << ",\"moveLimit\":" << movlim
      << ",\"moveLimitHitCount\":" << xMoveLimitHitCountLocal
      << ",\"lowerBoundCount\":" << xLowerBoundCountLocal
      << ",\"upperBoundCount\":" << xUpperBoundCountLocal
      << ",\"designNonFiniteCount\":" << xNonFiniteCountLocal
    << "}"
    << ",\"sensitivity\":{"
      << "\"objectiveMin\":" << fsensLocalMin
      << ",\"objectiveMax\":" << fsensLocalMax
      << ",\"objectiveL2\":" << fsensL2
      << ",\"objectiveNonFiniteCount\":" << fsensNonFiniteCountLocal
      << ",\"powerConstraintMin\":" << gsensPowerLocalMin
      << ",\"powerConstraintMax\":" << gsensPowerLocalMax
      << ",\"powerConstraintL2\":" << gsensPowerL2
      << ",\"powerConstraintNonFiniteCount\":" << gsensPowerNonFiniteCountLocal
      << ",\"volumeConstraintMin\":" << gsensVolLocalMin
      << ",\"volumeConstraintMax\":" << gsensVolLocalMax
      << ",\"volumeConstraintL2\":" << gsensVolL2
      << ",\"volumeConstraintNonFiniteCount\":" << gsensVolNonFiniteCountLocal
      << ",\"solidMinConstraintMin\":" << gsensSolidMinMin
      << ",\"solidMinConstraintMax\":" << gsensSolidMinMax
      << ",\"solidMinConstraintL2\":" << gsensSolidMinL2
      << ",\"solidMinConstraintNonFiniteCount\":" << gsensSolidMinNonFiniteCountLocal
      << ",\"solidMaxConstraintMin\":" << gsensSolidMaxMin
      << ",\"solidMaxConstraintMax\":" << gsensSolidMaxMax
      << ",\"solidMaxConstraintL2\":" << gsensSolidMaxL2
      << ",\"solidMaxConstraintNonFiniteCount\":" << gsensSolidMaxNonFiniteCountLocal
      << ",\"objectiveToVolumeL2Ratio\":" << objectiveToVolumeSensitivityRatio
      << ",\"powerToVolumeL2Ratio\":" << powerToVolumeSensitivityRatio
    << "}"
    << ",\"solver\":{"
      << "\"primalFlowSweeps\":" << 60
      << ",\"adjointFlowPowerSweeps\":" << adjointMomentumSweeps
      << ",\"adjointFlowHeatSweeps\":" << adjointMomentumSweeps
      << ",\"uMaxMag\":" << UMaxMagLocal
      << ",\"uMeanMag\":" << UMeanMag
      << ",\"pMin\":" << pLocalMin
      << ",\"pMax\":" << pLocalMax
      << ",\"tMin\":" << TLocalMin
      << ",\"tMax\":" << TLocalMax
      << ",\"uaMaxMag\":" << UaMaxMagLocal
      << ",\"uaMeanMag\":" << UaMeanMag
      << ",\"paMin\":" << paLocalMin
      << ",\"paMax\":" << paLocalMax
      << ",\"ubMaxMag\":" << UbMaxMagLocal
      << ",\"ubMeanMag\":" << UbMeanMag
      << ",\"pbMin\":" << pbLocalMin
      << ",\"pbMax\":" << pbLocalMax
      << ",\"uaToUMaxRatio\":" << adjointPowerToPrimalRatio
      << ",\"ubToUMaxRatio\":" << adjointHeatToPrimalRatio
      << ",\"adjointRunaway\":" << (adjointRunaway ? 1 : 0)
    << "}"
      << ",\"interpolation\":{"
      << "\"alphaMin\":" << alphaLocalMin
      << ",\"alphaMax\":" << alphaLocalMax
      << ",\"dtMin\":" << DTLocalMin
      << ",\"dtMax\":" << DTLocalMax
      << ",\"alphaMaxSchedule\":" << alphaMax.value()
      << ",\"qAlpha\":" << qAlpha
      << ",\"qKappa\":" << qKappa
      << ",\"qHeat\":" << qHeat
      << ",\"beta\":" << del
      << ",\"experimentProfile\":\"" << experimentProfile << "\""
      << ",\"powerFeasibilityRatio\":" << debugPowerFeasibilityRatio
      << ",\"continuationFeasibilityTolActive\":" << debugContinuationFeasibilityTolActive
      << ",\"continuationGateSatisfied\":" << (debugContinuationGateSatisfied ? 1 : 0)
      << ",\"continuationFloorActive\":" << (debugContinuationFloorActive ? 1 : 0)
      << ",\"continuationCeilingActive\":" << (debugContinuationCeilingActive ? 1 : 0)
      << ",\"lateStageStrictGateActive\":" << (debugLateStageStrictGateActive ? 1 : 0)
      << ",\"lateStageFilterRCapActive\":" << (debugLateStageFilterRCapActive ? 1 : 0)
      << ",\"filterRActive\":" << activeFilterR
      << ",\"lateStageRefinementActive\":" << (debugLateStageRefinementActive ? 1 : 0)
      << ",\"betaIncrementActive\":" << debugBetaIncrementActive
      << ",\"alphaRampLateFactorActive\":" << debugAlphaRampLateFactorActive
      << ",\"betaMaxActive\":" << debugBetaMaxActive
      << ",\"laggingGrayCollapseDetected\":" << (debugLaggingGrayCollapseDetected ? 1 : 0)
      << ",\"laggingGrayCollapsePulseIter\":" << (debugLaggingGrayCollapsePulseIter ? 1 : 0)
      << ",\"overactiveTopologyDetected\":" << (debugOveractiveTopologyDetected ? 1 : 0)
      << ",\"grayVolumeFractionDrop\":" << debugGrayVolumeFractionDrop
      << ",\"previousXhStepMax\":" << previousXhStepMax
      << ",\"geometryControlGateSatisfied\":" << (geometryControlGateSatisfied ? 1 : 0)
      << ",\"geometryControlGrayGateSatisfied\":" << (geometryControlGrayGateSatisfied ? 1 : 0)
      << ",\"geometryControlStepGateSatisfied\":" << (geometryControlStepGateSatisfied ? 1 : 0)
      << ",\"geometryControlActivationStreak\":" << geometryControlActivationStreak
      << ",\"geometryControlActivationIter\":" << geometryControlActivationIter
      << ",\"geometryControlStageWeight\":" << geometryControlStageWeight
      << ",\"geometryControlHardeningFrozen\":" << (geometryControlHardeningFrozen ? 1 : 0)
      << ",\"forceContinuationHardening\":" << (forceContinuationHardening ? 1 : 0)
      << ",\"forceContinuationHardeningUntilIter\":" << forceContinuationHardeningUntilIter
      << ",\"forceContinuationHardeningUntilBeta\":" << forceContinuationHardeningUntilBeta
      << ",\"stopContinuationHardeningBelowGrayFraction\":" << stopContinuationHardeningBelowGrayFraction
      << ",\"lateStageStrictGateBelowGrayFraction\":" << lateStageStrictGateBelowGrayFraction
      << ",\"lateStageContinuationFeasibilityTol\":" << lateStageContinuationFeasibilityTol
      << ",\"lateStageRefinementStartIter\":" << lateStageRefinementStartIter
      << ",\"lateStageRefinementBelowGrayFraction\":" << lateStageRefinementBelowGrayFraction
      << ",\"lateStageBetaRampFactor\":" << lateStageBetaRampFactor
      << ",\"lateStageAlphaRampFactor\":" << lateStageAlphaRampFactor
      << ",\"lateStageMaxBeta\":" << lateStageMaxBeta
      << ",\"lateStageFilterRStartIter\":" << lateStageFilterRStartIter
      << ",\"lateStageFilterRCap\":" << lateStageFilterRCap
      << ",\"overactiveTopologyStartIter\":" << overactiveTopologyStartIter
      << ",\"overactiveTopologyBelowGrayFraction\":" << overactiveTopologyBelowGrayFraction
      << ",\"overactiveTopologyAboveXhStepMax\":" << overactiveTopologyAboveXhStepMax
      << ",\"hardeningEnabled\":" << (debugContinuationHardeningEnabled ? 1 : 0)
      << ",\"wallDistanceMode\":\"" << activeWallDistanceMode << "\""
      << ",\"turbulenceModelMode\":\"" << activeTurbulenceModelMode << "\""
      << ",\"heatSource\":" << heatSource.value()
      << ",\"thermalSource\":" << thermalSource.value()
      << ",\"rhoCp\":" << rhoCp.value()
    << "}"
    << "}\n";
}

previousXhStepMax = maxAbsXhChangeLocal;
