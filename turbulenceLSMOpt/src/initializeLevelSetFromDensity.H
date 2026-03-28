Info<< "Initializing level-set field from mature density topology" << endl;

xhDensityReference = xh;
volVectorField gradXhForLevelSet
(
    IOobject
    (
        "gradXhForLevelSet",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    fvc::grad(xhDensityReference)
);

const scalar initPhiScale = Foam::max(phiBandWidthCells*len, scalar(len));
const scalar clipRadius = 6.0*initPhiScale;
forAll(phiLS, cellI)
{
    const scalar signedIndicator = xhDensityReference[cellI] - 0.5;
    const scalar signValue = signedIndicator >= 0.0 ? 1.0 : -1.0;
    const scalar gradMag = Foam::mag(gradXhForLevelSet[cellI]);
    scalar phiValue = signValue*clipRadius;

    if (gradMag > SMALL)
    {
        phiValue = signedIndicator/gradMag;
    }

    phiLS[cellI] = Foam::max(Foam::min(phiValue, clipRadius), -clipRadius);
}
phiLS.correctBoundaryConditions();

phiReinitReference = phiLS;
const scalar reinitDt = 0.25*len;
const label handoverReinitIterations =
    Foam::max
    (
        label(8),
        label(std::ceil(clipRadius/Foam::max(reinitDt, SMALL)))
    );
for (label reinitIter = 0; reinitIter < handoverReinitIterations; ++reinitIter)
{
    volVectorField gradPhiInit
    (
        IOobject
        (
            "gradPhiInit",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        fvc::grad(phiLS)
    );

    forAll(phiLS, cellI)
    {
        const scalar gradMag = Foam::mag(gradPhiInit[cellI]);
        const scalar smoothSign =
            phiReinitReference[cellI]
           /Foam::sqrt
            (
                phiReinitReference[cellI]*phiReinitReference[cellI]
              + Foam::sqr(0.5*len)
            );
        phiLS[cellI] -= reinitDt*smoothSign*(gradMag - 1.0);
    }
    phiLS.correctBoundaryConditions();
}

#include "updateDerivedDensityFromLevelSet.H"
