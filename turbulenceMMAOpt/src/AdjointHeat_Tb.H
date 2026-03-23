maxT=gMax(T);
sumT=0;
scalar ksExpSumLocal(0);
scalar varianceMeanLocal(0);
for(i=0;i<n;i++)
{
   if (maxT > SMALL)
   {
      sumT += std::pow(T[i]/maxT, Pnorm);
      ksExpSumLocal += std::exp(ksRho*(T[i] - maxT));
   }
   varianceMeanLocal += T[i]*mesh.V()[i];
}
reduce(sumT, sumOp<scalar>());
reduce(ksExpSumLocal, sumOp<scalar>());
reduce(varianceMeanLocal, sumOp<scalar>());
sumT = maxT > SMALL ? std::pow(sumT, 1.0/Pnorm - 1.0) : 0.0;
const scalar varianceMean = varianceMeanLocal/area;

fvScalarMatrix TEqnb
(
   -fvm::div(-phi,Tb) 
   +fvm::laplacian(DT, Tb)
);
if (useLogMeanTObjective)
{
   activeObjectiveLabel = "LogMeanT";
   TEqnb -= unitThermalSource/Foam::max(MeanT, Tref);
}
else if (useKSHotspotObjective)
{
   activeObjectiveLabel = "KSHotspot";
   const scalar safeKsExpSum = Foam::max(ksExpSumLocal, SMALL);
   TEqnb -= Foam::exp(ksRho*(T - maxT))/safeKsExpSum;
}
else if (useVarianceObjective)
{
   activeObjectiveLabel = "VarianceT";
   TEqnb -= 2.0*(T - varianceMean)/area;
}
else if (objFunction==2)
{
   activeObjectiveLabel = "PnormT";
   TEqnb -= b2*sumT*Foam::pow(T/maxT,Pnorm-1);
}
else
{
   activeObjectiveLabel = "MeanT";
   TEqnb -= unitThermalSource;
}
const auto TbLinearSolvePerformance = TEqnb.solve();
recordLinearSolve
(
  TbLinearSolveStats,
  TbLinearSolvePerformance.initialResidual(),
  TbLinearSolvePerformance.finalResidual(),
  TbLinearSolvePerformance.nIterations()
);
