if
(
    hybridStageState != HYBRID_STAGE_DENSITY
    && useLevelSetStage
    && useLSMWallDistance
    && lsmWallDistanceMode != "porosity"
)
{
    forAll(dAlphaLSM, cellI)
    {
        dAlphaLSM[cellI] = Foam::max(phiLS[cellI], scalar(0.0));
    }
    dAlphaLSM.correctBoundaryConditions();

    if (lsmWallDistanceMode == "phi")
    {
        dAlpha = dAlphaLSM;
        activeWallDistanceMode = "lsmPhi";
    }
    else
    {
        scalar blendWeight(0.0);
        if (lsmWallDistanceBlendIterations > 0 && lsmStageIteration < lsmWallDistanceBlendIterations)
        {
            blendWeight =
                Foam::max
                (
                    0.0,
                    1.0
                  - scalar(lsmStageIteration + 1)
                   /scalar(lsmWallDistanceBlendIterations + 1)
                );
        }
        dAlpha = blendWeight*dAlpha + (1.0 - blendWeight)*dAlphaLSM;
        activeWallDistanceMode = blendWeight > SMALL ? "lsmBlend" : "lsmPhi";
    }

    dAlpha.correctBoundaryConditions();
}
