
Info<<"sensitivity analysis"<<endl;

std::vector<scalar> xOld(n);
std::vector<scalar> xhOld(n);
for(i=0;i<n;i++)
{
 xOld[i]=x[i];
 xhOld[i]=xh[i];
}

Info<< "sensitivity analysis: assemble raw objective sensitivity" << endl;
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
fsenshMeanT =
  alphaSensCoeff*(U&Ub)
  + b1*kappaSensCoeff*(gradTField&gradTbField);
if(DDHS)//design dependent heat source
{
 fsenshMeanT -= b1*Tb*thermalSource*heatSourceSensCoeff;
}
Info<< "sensitivity analysis: assemble raw power sensitivity" << endl;
gsenshPowerDiss=alphaSensCoeff*(U&Ua);
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
