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

scalar TLocalMinTb = GREAT;
scalar TLocalMaxTb = -GREAT;
label TNonFiniteTb = 0;
forAll(T, cellI)
{
   const scalar value = T[cellI];
   if (!std::isfinite(static_cast<double>(value)))
   {
      TNonFiniteTb++;
      continue;
   }
   TLocalMinTb = Foam::min(TLocalMinTb, value);
   TLocalMaxTb = Foam::max(TLocalMaxTb, value);
}
reduce(TLocalMinTb, minOp<scalar>());
reduce(TLocalMaxTb, maxOp<scalar>());
reduce(TNonFiniteTb, sumOp<label>());

scalar TbLocalMin = GREAT;
scalar TbLocalMax = -GREAT;
label TbNonFinite = 0;
forAll(Tb, cellI)
{
   const scalar value = Tb[cellI];
   if (!std::isfinite(static_cast<double>(value)))
   {
      TbNonFinite++;
      continue;
   }
   TbLocalMin = Foam::min(TbLocalMin, value);
   TbLocalMax = Foam::max(TbLocalMax, value);
}
reduce(TbLocalMin, minOp<scalar>());
reduce(TbLocalMax, maxOp<scalar>());
reduce(TbNonFinite, sumOp<label>());

scalar DTLocalMinTb = GREAT;
scalar DTLocalMaxTb = -GREAT;
label DTNonFiniteTb = 0;
forAll(DT, cellI)
{
   const scalar value = DT[cellI];
   if (!std::isfinite(static_cast<double>(value)))
   {
      DTNonFiniteTb++;
      continue;
   }
   DTLocalMinTb = Foam::min(DTLocalMinTb, value);
   DTLocalMaxTb = Foam::max(DTLocalMaxTb, value);
}
reduce(DTLocalMinTb, minOp<scalar>());
reduce(DTLocalMaxTb, maxOp<scalar>());
reduce(DTNonFiniteTb, sumOp<label>());

Info<< "adjoint thermal diagnostics: "
    << "T[min,max]=(" << TLocalMinTb << ", " << TLocalMaxTb << ") nonFinite=" << TNonFiniteTb
    << " Tb[min,max]=(" << TbLocalMin << ", " << TbLocalMax << ") nonFinite=" << TbNonFinite
    << " DT[min,max]=(" << DTLocalMinTb << ", " << DTLocalMaxTb << ") nonFinite=" << DTNonFiniteTb
    << endl;

if (TNonFiniteTb > 0 || TbNonFinite > 0 || DTNonFiniteTb > 0 || DTLocalMinTb <= 0)
{
   FatalErrorInFunction
      << "Invalid adjoint thermal inputs. "
      << "TNonFinite=" << TNonFiniteTb
      << ", TbNonFinite=" << TbNonFinite
      << ", DTNonFinite=" << DTNonFiniteTb
      << ", DTMin=" << DTLocalMinTb
      << exit(FatalError);
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

TbLocalMin = GREAT;
TbLocalMax = -GREAT;
TbNonFinite = 0;
forAll(Tb, cellI)
{
   const scalar value = Tb[cellI];
   if (!std::isfinite(static_cast<double>(value)))
   {
      TbNonFinite++;
      continue;
   }
   TbLocalMin = Foam::min(TbLocalMin, value);
   TbLocalMax = Foam::max(TbLocalMax, value);
}
reduce(TbLocalMin, minOp<scalar>());
reduce(TbLocalMax, maxOp<scalar>());
reduce(TbNonFinite, sumOp<label>());

Info<< "adjoint thermal solve complete: Tb[min,max]=(" << TbLocalMin
    << ", " << TbLocalMax << ") nonFinite=" << TbNonFinite << endl;

if (TbNonFinite > 0)
{
   FatalErrorInFunction
      << "Adjoint thermal solve produced non-finite Tb values."
      << exit(FatalError);
}
