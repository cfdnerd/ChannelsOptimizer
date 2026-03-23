Info<< "adjoint thermal solve" << endl;
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
{
   scalar adjointThermalTMin = GREAT;
   scalar adjointThermalTMax = -GREAT;
   label adjointThermalTNonFinite = 0;
   forAll(T, cellI)
   {
      const scalar value = T[cellI];
      if (!std::isfinite(static_cast<double>(value)))
      {
         adjointThermalTNonFinite++;
         continue;
      }
      adjointThermalTMin = Foam::min(adjointThermalTMin, value);
      adjointThermalTMax = Foam::max(adjointThermalTMax, value);
   }
   reduce(adjointThermalTMin, minOp<scalar>());
   reduce(adjointThermalTMax, maxOp<scalar>());
   reduce(adjointThermalTNonFinite, sumOp<label>());

   scalar adjointThermalTbMin = GREAT;
   scalar adjointThermalTbMax = -GREAT;
   label adjointThermalTbNonFinite = 0;
   forAll(Tb, cellI)
   {
      const scalar value = Tb[cellI];
      if (!std::isfinite(static_cast<double>(value)))
      {
         adjointThermalTbNonFinite++;
         continue;
      }
      adjointThermalTbMin = Foam::min(adjointThermalTbMin, value);
      adjointThermalTbMax = Foam::max(adjointThermalTbMax, value);
   }
   reduce(adjointThermalTbMin, minOp<scalar>());
   reduce(adjointThermalTbMax, maxOp<scalar>());
   reduce(adjointThermalTbNonFinite, sumOp<label>());

   scalar adjointThermalDTMin = GREAT;
   scalar adjointThermalDTMax = -GREAT;
   label adjointThermalDTNonFinite = 0;
   forAll(DT, cellI)
   {
      const scalar value = DT[cellI];
      if (!std::isfinite(static_cast<double>(value)))
      {
         adjointThermalDTNonFinite++;
         continue;
      }
      adjointThermalDTMin = Foam::min(adjointThermalDTMin, value);
      adjointThermalDTMax = Foam::max(adjointThermalDTMax, value);
   }
   reduce(adjointThermalDTMin, minOp<scalar>());
   reduce(adjointThermalDTMax, maxOp<scalar>());
   reduce(adjointThermalDTNonFinite, sumOp<label>());

   Info<< "adjoint thermal diagnostics: "
       << "T[min,max]=(" << adjointThermalTMin << ", " << adjointThermalTMax << ") nonFinite=" << adjointThermalTNonFinite
       << " Tb[min,max]=(" << adjointThermalTbMin << ", " << adjointThermalTbMax << ") nonFinite=" << adjointThermalTbNonFinite
       << " DT[min,max]=(" << adjointThermalDTMin << ", " << adjointThermalDTMax << ") nonFinite=" << adjointThermalDTNonFinite
       << endl;

   if (adjointThermalTNonFinite > 0 || adjointThermalTbNonFinite > 0 || adjointThermalDTNonFinite > 0 || adjointThermalDTMin <= 0)
   {
      FatalErrorInFunction
         << "Invalid adjoint thermal inputs. "
         << "TNonFinite=" << adjointThermalTNonFinite
         << ", TbNonFinite=" << adjointThermalTbNonFinite
         << ", DTNonFinite=" << adjointThermalDTNonFinite
         << ", DTMin=" << adjointThermalDTMin
         << exit(FatalError);
   }
}

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
{
   scalar adjointThermalTbSolvedMin = GREAT;
   scalar adjointThermalTbSolvedMax = -GREAT;
   label adjointThermalTbSolvedNonFinite = 0;
   forAll(Tb, cellI)
   {
      const scalar value = Tb[cellI];
      if (!std::isfinite(static_cast<double>(value)))
      {
         adjointThermalTbSolvedNonFinite++;
         continue;
      }
      adjointThermalTbSolvedMin = Foam::min(adjointThermalTbSolvedMin, value);
      adjointThermalTbSolvedMax = Foam::max(adjointThermalTbSolvedMax, value);
   }
   reduce(adjointThermalTbSolvedMin, minOp<scalar>());
   reduce(adjointThermalTbSolvedMax, maxOp<scalar>());
   reduce(adjointThermalTbSolvedNonFinite, sumOp<label>());

   Info<< "adjoint thermal solve complete: Tb[min,max]=(" << adjointThermalTbSolvedMin
       << ", " << adjointThermalTbSolvedMax << ") nonFinite=" << adjointThermalTbSolvedNonFinite << endl;

   if (adjointThermalTbSolvedNonFinite > 0)
   {
      FatalErrorInFunction
         << "Adjoint thermal solve produced non-finite Tb values."
         << exit(FatalError);
   }
}
