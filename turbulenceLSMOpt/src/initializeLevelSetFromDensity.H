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
forAll(phiLS, cellI)
{
    const scalar signedIndicator = xhDensityReference[cellI] - 0.5;
    const scalar signValue = signedIndicator >= 0.0 ? 1.0 : -1.0;
    const scalar gradMag = Foam::mag(gradXhForLevelSet[cellI]);
    scalar phiValue = signValue*initPhiScale;

    if (gradMag > SMALL)
    {
        phiValue = signedIndicator/gradMag;
    }

    const scalar clipRadius = 6.0*initPhiScale;
    phiLS[cellI] = Foam::max(Foam::min(phiValue, clipRadius), -clipRadius);
}
phiLS.correctBoundaryConditions();

phiReinitReference = phiLS;
const scalar reinitDt = 0.25*len;
for (label reinitIter = 0; reinitIter < 8; ++reinitIter)
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
