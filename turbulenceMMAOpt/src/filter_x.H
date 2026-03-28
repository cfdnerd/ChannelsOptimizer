
//***********************************//PDE filter
const auto xpFilterLinearSolvePerformance =
solve(fvm::laplacian(xp)-fvm::Sp(b,xp)+x*b);
recordLinearSolve
(
  xpFilterLinearSolveStats,
  xpFilterLinearSolvePerformance.initialResidual(),
  xpFilterLinearSolvePerformance.finalResidual(),
  xpFilterLinearSolvePerformance.nIterations()
);
//***********************************//Heaviside_filter
eta0=0.0001;
eta1=1.0;
y0=diff(xp,mesh.V(),del,eta0,n);
reduce(y0, sumOp<scalar>());
do
{
  eta5=(eta0+eta1)/2.0;
  y5=diff(xp,mesh.V(),del,eta5,n);
  reduce(y5, sumOp<scalar>());
  if(y0*y5<0)
  { 
     eta1=eta5;
  }
  else
  { 
     eta0=eta5;
     y0=y5;
  }
} while ((eta1-eta0)>0.0001);

geometryProjectionEtaActive =
  geometricSizeProjectionEta >= 0.0 ? geometricSizeProjectionEta : eta5;
geometryProjectionEtaActive =
  Foam::min(Foam::max(geometryProjectionEtaActive, scalar(1e-4)), scalar(1.0 - 1e-4));

for(i=0;i<n;i++)
{
   if(xp[i]<=eta5)
   {
     xh[i]=eta5*(Foam::exp(-del*(1-xp[i]/eta5))-(1-xp[i]/eta5)*Foam::exp(-del)); 
     drho[i]=del*Foam::exp(-del*(1-xp[i]/eta5))+Foam::exp(-del);
   }
   else
   {
     xh[i]=eta5+(1-eta5)*(1-Foam::exp(-del*(xp[i]-eta5)/(1-eta5))+(xp[i]-eta5)*Foam::exp(-del)/(1-eta5));
     drho[i]=del*Foam::exp(-del*(xp[i]-eta5)/(1-eta5))+Foam::exp(-del);
   }

   if(xp[i]<=geometryProjectionEtaActive)
   {
     xg[i]=
       geometryProjectionEtaActive
       *(
         Foam::exp(-geometricSizeProjectionBeta*(1-xp[i]/geometryProjectionEtaActive))
         -(1-xp[i]/geometryProjectionEtaActive)*Foam::exp(-geometricSizeProjectionBeta)
       );
     drhoGeom[i]=
       geometricSizeProjectionBeta
       *Foam::exp(-geometricSizeProjectionBeta*(1-xp[i]/geometryProjectionEtaActive))
       + Foam::exp(-geometricSizeProjectionBeta);
   }
   else
   {
     xg[i]=
       geometryProjectionEtaActive
       +(1-geometryProjectionEtaActive)
       *(
         1-Foam::exp(-geometricSizeProjectionBeta*(xp[i]-geometryProjectionEtaActive)/(1-geometryProjectionEtaActive))
         +(xp[i]-geometryProjectionEtaActive)*Foam::exp(-geometricSizeProjectionBeta)/(1-geometryProjectionEtaActive)
       );
     drhoGeom[i]=
       geometricSizeProjectionBeta
       *Foam::exp(-geometricSizeProjectionBeta*(xp[i]-geometryProjectionEtaActive)/(1-geometryProjectionEtaActive))
       + Foam::exp(-geometricSizeProjectionBeta);
   }   
}
xh.correctBoundaryConditions();
xg.correctBoundaryConditions();
drho.correctBoundaryConditions();
drhoGeom.correctBoundaryConditions();
