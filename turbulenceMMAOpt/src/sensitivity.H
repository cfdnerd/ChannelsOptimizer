
Info<<"sensitivity analysis"<<endl;

std::vector<scalar> xOld(n);
std::vector<scalar> xhOld(n);
for(i=0;i<n;i++)
{
 xOld[i]=x[i];
 xhOld[i]=xh[i];
}

fsenshMeanT=alphaSensCoeff*(U&Ub)+b1*kappaSensCoeff*(fvc::grad(T)&fvc::grad(Tb));
if(DDHS)//design dependent heat source
{
 fsenshMeanT -= b1*Tb*thermalSource*heatSourceSensCoeff;
}
gsenshPowerDiss=alphaSensCoeff*(U&Ua);
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
