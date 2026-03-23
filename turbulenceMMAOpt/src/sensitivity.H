
Info<<"sensitivity analysis"<<endl;

std::vector<scalar> xOld(n);
std::vector<scalar> xhOld(n);
for(i=0;i<n;i++)
{
 xOld[i]=x[i];
 xhOld[i]=xh[i];
}

Info<< "sensitivity analysis: assemble raw objective sensitivity" << endl;
Info<< "sensitivity analysis: compute grad(T)" << endl;
volVectorField gradTField
(
    IOobject
    (
        "gradTField",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    fvc::grad(T)
);
Info<< "sensitivity analysis: compute grad(Tb)" << endl;
volVectorField gradTbField
(
    IOobject
    (
        "gradTbField",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    fvc::grad(Tb)
);
Info<< "sensitivity analysis: assemble objective cells" << endl;
forAll(fsenshMeanT, cellI)
{
  const scalar brinkmanTerm = alphaSensCoeff[cellI]*(U[cellI] & Ub[cellI]);
  const scalar thermalGradTerm =
    b1.value()*kappaSensCoeff[cellI]*(gradTField[cellI] & gradTbField[cellI]);
  scalar objectiveSensitivity = brinkmanTerm + thermalGradTerm;

  if (DDHS)
  {
    objectiveSensitivity -=
      b1.value()*Tb[cellI]*thermalSource.value()*heatSourceSensCoeff[cellI];
  }

  if (std::isfinite(static_cast<double>(objectiveSensitivity)))
  {
    fsenshMeanT[cellI] = objectiveSensitivity;
  }
  else
  {
    fsenshMeanT[cellI] = 0.0;
  }
}
Info<< "sensitivity analysis: assemble raw power sensitivity" << endl;
forAll(gsenshPowerDiss, cellI)
{
  const scalar powerSensitivity = alphaSensCoeff[cellI]*(U[cellI] & Ua[cellI]);
  gsenshPowerDiss[cellI] =
    std::isfinite(static_cast<double>(powerSensitivity)) ? powerSensitivity : 0.0;
}
Info<< "sensitivity analysis: sanitize raw sensitivities" << endl;
label fsensRawNonFiniteLocal(0);
label gsensRawNonFiniteLocal(0);
forAll(fsenshMeanT, i)
{
  if (!std::isfinite(static_cast<double>(fsenshMeanT[i])))
  {
    fsenshMeanT[i] = 0.0;
    fsensRawNonFiniteLocal++;
  }

  if (!std::isfinite(static_cast<double>(gsenshPowerDiss[i])))
  {
    gsenshPowerDiss[i] = 0.0;
    gsensRawNonFiniteLocal++;
  }
}
reduce(fsensRawNonFiniteLocal, sumOp<label>());
reduce(gsensRawNonFiniteLocal, sumOp<label>());
if (Pstream::master() && (fsensRawNonFiniteLocal > 0 || gsensRawNonFiniteLocal > 0))
{
  Info<< "sensitivity analysis: clamped non-finite raw sensitivities"
      << " objective=" << fsensRawNonFiniteLocal
      << " power=" << gsensRawNonFiniteLocal << endl;
}
gsensVolRaw = dimensionedScalar("one", dimless, 1.0);
fsensMeanTRaw = fsenshMeanT;
gsensPowerDissRaw = gsenshPowerDiss;


#include "filter_chainrule.H"

const scalar powerLimit =
  usePowerConstraintRelaxation
  ? Foam::max(PowerDissRelax - opt*powerConstraintRelaxationRate, PowerDissMax)
  : PowerDissMax;
activePowerLimit = powerLimit;
const scalar weightedPowerConstraint =
  optPowerConstraintWeight*(PowerDiss/powerLimit-1.0);
const scalar weightedVolumeConstraint = optVolumeConstraintWeight*V;
gx[0]=weightedPowerConstraint;
gx[1]=weightedVolumeConstraint;

for(i=0;i<n;i++)
{
 xmma[i] = x[i];
 dfdx[i]=fsensMeanT[i]/N;//sensitivities of objective function
 dgdx[0][i]=optPowerConstraintWeight*(gsensPowerDiss[i]/N);//sensitivities of constraint function
 dgdx[1][i]=optVolumeConstraintWeight*(gsensVol[i]/N);//sensitivities of constraint function
}


Info<< "\n run MMA \n" << endl;
mma.MMAsolver(xmma, dfdx, gx, dgdx);
for(i=0;i<n;i++)
{
   x[i]=xmma[i];
}

#include "filter_x.H"
#include "updateInterpolationFields.H"
#include "gradientOptWrite.H"
#include "debugOptimizer.H"
#include "solverConvergenceWrite.H"
