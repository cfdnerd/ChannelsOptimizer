lsmNormalVelocity = lsmNormalVelocityRaw;

if (useVelocityHelmholtz)
{
    const scalar velocityRadius = Foam::max(velocityFilterRadius*len, scalar(len));
    dimensionedScalar bVelocity
    (
        "bVelocity",
        dimensionSet(0, -2, 0, 0, 0, 0, 0),
        1.0/Foam::sqr(velocityRadius)
    );

    solve
    (
        fvm::laplacian(lsmNormalVelocity)
      - fvm::Sp(bVelocity, lsmNormalVelocity)
      + lsmNormalVelocityRaw*bVelocity
    );
    lsmNormalVelocity.correctBoundaryConditions();
}

lsmLastVelocityMax = gMax(mag(lsmNormalVelocity.primitiveField()));
