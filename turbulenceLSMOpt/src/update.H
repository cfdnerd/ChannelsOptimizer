//***********************************// optimizer and stage update
opt++;

if (lsmRollbackRequested && lsmRollbackEnabled && lsmCheckpointAvailable)
{
    const word rollbackReason(lsmRollbackReason);
    Info<< "Restoring density checkpoint after LSM issue: "
        << rollbackReason << endl;
    restoreLSMCheckpoint();
    hybridStageState = HYBRID_STAGE_DENSITY;
    hybridStageName = "densityRollback";
    lsmSwitchTriggered = false;
    lsmBlendIteration = 0;
    lsmStageIteration = 0;
    lsmBlendWeight = 1.0;
    lsmRollbackCount++;
    lsmRollbackReason = rollbackReason;
    scheduleLSMRetry
    (
        rollbackReason
    );
}

scalar grayVolumeContinuationLocal(0.0);
forAll(xh, cellI)
{
    const scalar xhValue = xh[cellI];
    if (xhValue > 0.05 && xhValue < 0.95)
    {
        grayVolumeContinuationLocal += mesh.V()[cellI];
    }
}
reduce(grayVolumeContinuationLocal, sumOp<scalar>());
const scalar grayVolumeFractionForContinuation =
    area > SMALL ? grayVolumeContinuationLocal/area : 0.0;
const scalar continuationPowerLimit =
    usePowerConstraintRelaxation ? activePowerLimit : PowerDissMax;
const scalar powerFeasibilityRatio =
    continuationPowerLimit > SMALL ? PowerDiss/continuationPowerLimit : 0.0;
const bool connectivitySatisfied =
    !lsmSwitchRequireConnectivity || hasThresholdedFluidConnectivity(xh, 0.5);

lsmLastGrayVolumeFraction = grayVolumeFractionForContinuation;
lsmLastPowerFeasibilityRatio = powerFeasibilityRatio;
lsmLastConnectivitySatisfied = connectivitySatisfied;
lsmSwitchHealthSatisfied = continuationHealthOk && previousSolverSeverity < 2;
lsmSwitchCheckpointSatisfied =
    !requireRestartableLSMCheckpoint || lsmRollbackEnabled;
lsmSwitchLockedOut = opt < lsmNextSwitchAllowedIter;
lsmSwitchGateSatisfied =
    useHybridMMAtoLSM
 && useLevelSetStage
 && !lsmSwitchLockedOut
 && hybridStageState == HYBRID_STAGE_DENSITY
 && opt >= lsmSwitchMinIterations
 && grayVolumeFractionForContinuation <= lsmSwitchGrayFraction
 && powerFeasibilityRatio <= lsmSwitchPowerFeasibility
 && del >= lsmSwitchMinBeta
 && lsmSwitchHealthSatisfied
 && connectivitySatisfied
 && (!requireRestartableLSMCheckpoint || lsmRollbackEnabled);

bool switchedToLSMThisIteration(false);

if (lsmSwitchGateSatisfied)
{
    Info<< "Switching turbulenceLSMOpt from density stage to level-set refinement."
        << " gray=" << grayVolumeFractionForContinuation
        << " powerRatio=" << powerFeasibilityRatio
        << " beta=" << del
        << endl;

    saveLSMCheckpoint();
    lsmSwitchCheckpointSatisfied = lsmCheckpointAvailable;
    lsmSwitchTriggered = true;
    hybridStageState = lsmBlendIterations > 0 ? HYBRID_STAGE_HANDOVER : HYBRID_STAGE_LSM;
    hybridStageName = hybridStageState == HYBRID_STAGE_HANDOVER ? word("handover") : word("lsm");
    lsmBlendIteration = 0;
    lsmStageIteration = 0;
    lsmBlendWeight =
        hybridStageState == HYBRID_STAGE_HANDOVER
      ? Foam::max(0.0, 1.0 - 1.0/scalar(lsmBlendIterations + 1))
      : 0.0;
    switchedToLSMThisIteration = true;
    #include "initializeLevelSetFromDensity.H"

    {
        #include "lsmGeometryMetrics.H"
    }

    const bool initializedLevelSetConnectivity =
        !lsmSwitchRequireConnectivity
     || hasThresholdedFluidConnectivity(xhLevelSet, 0.5);
    const bool initializedWidthCollapseDetected =
        useChannelWidthControl
     && channelWidthMinPhysical > SMALL
     && lsmLastChannelWidthMin > SMALL
     && lsmLastChannelWidthMin < 0.5*channelWidthMinPhysical;
    const bool initializedRibCollapseDetected =
        useRibThicknessControl
     && ribThicknessMinPhysical > SMALL
     && lsmLastRibThicknessMin > SMALL
     && lsmLastRibThicknessMin < 0.5*ribThicknessMinPhysical;
    const bool initializedLevelSetAcceptable =
        allowTopologyChangeInLSM
     || (
            initializedLevelSetConnectivity
         && !initializedWidthCollapseDetected
         && !initializedRibCollapseDetected
        );

    if (!initializedLevelSetAcceptable)
    {
        const word initRejectReason
        (
            !initializedLevelSetConnectivity
          ? "handoverInitConnectivity"
          : initializedWidthCollapseDetected
          ? "handoverInitWidth"
          : "handoverInitRib"
        );

        Info<< "Rejecting LSM handover initialization before Stage 2 commit: "
            << initRejectReason
            << " gray=" << grayVolumeFractionForContinuation
            << " wMin=" << lsmLastChannelWidthMin
            << " wMinAccept>=" << 0.5*channelWidthMinPhysical
            << " ribMin=" << lsmLastRibThicknessMin
            << " ribMinAccept>=" << 0.5*ribThicknessMinPhysical
            << " len=" << len
            << endl;

        restoreLSMCheckpoint();
        hybridStageState = HYBRID_STAGE_DENSITY;
        hybridStageName = "density";
        lsmSwitchTriggered = false;
        lsmSwitchGateSatisfied = false;
        lsmBlendIteration = 0;
        lsmStageIteration = 0;
        lsmBlendWeight = 1.0;
        lsmLastStepAccepted = true;
        lsmRollbackRequested = false;
        lsmRollbackReason = initRejectReason;
        scheduleLSMRetry
        (
            initRejectReason
        );
        switchedToLSMThisIteration = false;
    }
    else if (hybridStageState == HYBRID_STAGE_HANDOVER && lsmBlendIterations > 0)
    {
        lsmBlendIteration = 1;
    }
}

if (hybridStageState == HYBRID_STAGE_DENSITY)
{
    if(solid_area)
    {
       forAll(cells_solid, i)
       {
          xh[cells_solid[i]]=0;
       }
    }
    if(fluid_area)
    {
       forAll(cells_fluid, i)
       {
          xh[cells_fluid[i]]=1.0;
       }
    }
    xh.correctBoundaryConditions();

    scalar continuationFeasibilityTolActive = continuationFeasibilityTol;
    const bool lateStageStrictGateActive =
      lateStageStrictGateBelowGrayFraction >= 0.0
      && lateStageContinuationFeasibilityTol >= 0.0
      && grayVolumeFractionForContinuation <= lateStageStrictGateBelowGrayFraction;
    if (lateStageStrictGateActive)
    {
      continuationFeasibilityTolActive =
        Foam::min(continuationFeasibilityTolActive, lateStageContinuationFeasibilityTol);
    }

    const bool continuationGateSatisfied =
      powerFeasibilityRatio <= continuationFeasibilityTolActive
      && continuationHealthOk;
    const bool continuationFloorActive =
      opt <= forceContinuationHardeningUntilIter
      || (forceContinuationHardeningUntilBeta >= 0.0 && del < forceContinuationHardeningUntilBeta);
    const bool continuationCeilingActive =
      stopContinuationHardeningBelowGrayFraction >= 0.0
      && grayVolumeFractionForContinuation <= stopContinuationHardeningBelowGrayFraction;
    const scalar grayVolumeFractionDrop =
      previousGrayVolumeFractionForContinuation - grayVolumeFractionForContinuation;
    const bool profileUsesAdaptiveHardeningThrottles =
      experimentProfile == "betaFloorGrayRamp"
      || experimentProfile == "branchRefinement400";
    const bool laggingGrayCollapseDetected =
      profileUsesAdaptiveHardeningThrottles
      && !continuationFloorActive
      && laggingGrayCollapseStartIter > 0
      && opt >= laggingGrayCollapseStartIter
      && laggingGrayCollapseAboveGrayFraction >= 0.0
      && laggingGrayCollapseBelowXhStepMax >= 0.0
      && laggingGrayCollapseMinGrayDrop >= 0.0
      && grayVolumeFractionForContinuation >= laggingGrayCollapseAboveGrayFraction
      && previousXhStepMax <= laggingGrayCollapseBelowXhStepMax
      && grayVolumeFractionDrop <= laggingGrayCollapseMinGrayDrop;
    const bool overactiveTopologyDetected =
      profileUsesAdaptiveHardeningThrottles
      && !continuationFloorActive
      && overactiveTopologyStartIter > 0
      && opt >= overactiveTopologyStartIter
      && overactiveTopologyBelowGrayFraction >= 0.0
      && overactiveTopologyAboveXhStepMax >= 0.0
      && grayVolumeFractionForContinuation <= overactiveTopologyBelowGrayFraction
      && previousXhStepMax >= overactiveTopologyAboveXhStepMax;
    const bool laggingGrayCollapsePulseIter =
      laggingGrayCollapsePauseInterval > 0
      && ((opt - laggingGrayCollapseStartIter) % laggingGrayCollapsePauseInterval == 0);
    const bool continuationThrottleAllowsHardening =
      !laggingGrayCollapseDetected || laggingGrayCollapsePulseIter;
    const bool allowContinuationHardening =
      !continuationCeilingActive
      && !overactiveTopologyDetected
      && continuationThrottleAllowsHardening
      && (forceContinuationHardening || continuationFloorActive || continuationGateSatisfied);
    const bool lateStageFilterRCapActive =
      useLateStageFilterRCap
      && lateStageFilterRStartIter > 0
      && lateStageFilterRCap >= 0.0
      && opt > lateStageFilterRStartIter;
    activeFilterR =
      lateStageFilterRCapActive
      ? Foam::min(filterR, lateStageFilterRCap)
      : filterR;
    b.value()=1.0/(activeFilterR*len/3.464)/(activeFilterR*len/3.464);
    const bool lateStageRefinementActive =
      (lateStageRefinementStartIter > 0 && opt >= lateStageRefinementStartIter)
      || (
        lateStageRefinementBelowGrayFraction >= 0.0
        && grayVolumeFractionForContinuation <= lateStageRefinementBelowGrayFraction
      );
    scalar betaIncrementActive = betaIncrement;
    scalar alphaRampLateFactorActive = alphaRampLateFactor;
    scalar betaMaxActive = betaMax;
    if (lateStageRefinementActive)
    {
      if (lateStageBetaRampFactor >= 0.0)
      {
        betaIncrementActive *= lateStageBetaRampFactor;
      }
      if (lateStageAlphaRampFactor >= 0.0)
      {
        alphaRampLateFactorActive = lateStageAlphaRampFactor;
      }
      if (lateStageMaxBeta >= 0.0)
      {
        betaMaxActive = Foam::min(betaMaxActive, lateStageMaxBeta);
      }
    }

    if(opt<=63 && allowContinuationHardening)
    {
      alphaMax=alphamax*(opt*alphaRampEarlySlope+1.0);
    }
    if(opt>100 && allowContinuationHardening)
    {
      alphaMax=alphaMax*alphaRampLateFactorActive;
    }
    alphaMax.value()=Foam::min(alphaMax.value(),alphaMAX.value());

    if (qContinuationInterval > 0 && opt >= qContinuationStartIter && ((opt - qContinuationStartIter) % qContinuationInterval == 0))
    {
      if (useSplitRAMPControls)
      {
        qAlpha = Foam::max(qAlpha*qContinuationFactor, qAlphaMin);
        qKappa = Foam::max(qKappa*qContinuationFactor, qKappaMin);
        qHeat = Foam::max(qHeat*qContinuationFactor, qHeatMin);
      }
      else
      {
        qSingle = Foam::max(qSingle*qContinuationFactor, qSingleMin);
      }
    }

    if (!useStagnationTriggeredBeta)
    {
      if (allowContinuationHardening)
      {
        del = Foam::min(del + betaIncrementActive, betaMaxActive);
      }
    }
    else if (optimizerConvergedStreak > 0 && allowContinuationHardening)
    {
      del = Foam::min(del + betaIncrementActive, betaMaxActive);
    }

    hybridStageName = "density";
}
else
{
    activeFilterR = filterR;
    b.value()=1.0/(activeFilterR*len/3.464)/(activeFilterR*len/3.464);

    if (hybridStageState == HYBRID_STAGE_HANDOVER)
    {
        lsmBlendWeight =
            lsmBlendIterations > 0
          ? Foam::max
            (
                0.0,
                1.0
              - scalar(lsmBlendIteration + 1)
               /scalar(lsmBlendIterations + 1)
            )
          : 0.0;
        hybridStageName = "handover";
    }
    else
    {
        lsmBlendWeight = 0.0;
        hybridStageName = "lsm";
    }

    if (!switchedToLSMThisIteration)
    {
        {
            #include "updateDerivedDensityFromLevelSet.H"
        }
    }

    if (hybridStageState == HYBRID_STAGE_HANDOVER)
    {
        lsmBlendIteration++;
        if (lsmBlendIteration >= lsmBlendIterations)
        {
            hybridStageState = HYBRID_STAGE_LSM;
            hybridStageName = "lsm";
            lsmBlendWeight = 0.0;
        }
    }
}

previousGrayVolumeFractionForContinuation = grayVolumeFractionForContinuation;

#include "updateInterpolationFields.H"
