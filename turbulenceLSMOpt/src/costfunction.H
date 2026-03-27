Info<<"cost function"<<endl;
MeanT=fvc::domainIntegrate(T).value()/area;
maxT=gMax(T);
V=fvc::domainIntegrate(xh).value()/area-(voluse+0*Foam::max(0,0.5-opt*0.015));  
varianceObjectiveValue = 0.0;
ksObjectiveValue = MeanT;

if (useKSHotspotObjective)
{
  scalar ksExpSumLocal(0);
  forAll(T, i)
  {
    ksExpSumLocal += std::exp(ksRho*(T[i] - maxT))*mesh.V()[i];
  }
  reduce(ksExpSumLocal, sumOp<scalar>());
  ksObjectiveValue = maxT + std::log(Foam::max(ksExpSumLocal/area, SMALL))/ksRho;
  objectiveValueCurrent = ksObjectiveValue;
  activeObjectiveLabel = "KSHotspot";
}
else if (useVarianceObjective)
{
  scalar weightedMeanLocal(0);
  forAll(T, i)
  {
    weightedMeanLocal += T[i]*mesh.V()[i];
  }
  reduce(weightedMeanLocal, sumOp<scalar>());
  const scalar weightedMean = weightedMeanLocal/area;
  scalar varianceLocal(0);
  forAll(T, i)
  {
    const scalar deltaT = T[i] - weightedMean;
    varianceLocal += deltaT*deltaT*mesh.V()[i];
  }
  reduce(varianceLocal, sumOp<scalar>());
  varianceObjectiveValue = varianceLocal/area;
  objectiveValueCurrent = varianceObjectiveValue;
  activeObjectiveLabel = "VarianceT";
}
else if (useLogMeanTObjective)
{
  objectiveValueCurrent = std::log(Foam::max(MeanT, Tref)/Foam::max(Tref, SMALL));
  activeObjectiveLabel = "LogMeanT";
}
else if (objFunction == 2)
{
  objectiveValueCurrent = maxT;
  activeObjectiveLabel = "PnormT";
}
else
{
  objectiveValueCurrent = MeanT;
  activeObjectiveLabel = "MeanT";
}

PowerDiss=0;
for (i=0; i<nObjPatch; i++)
{
     PowerDiss =PowerDiss - sum(
     phi.boundaryField()[conPatchList[i]]*(p.boundaryField()[conPatchList[i]]
     + 0.5*magSqr(U.boundaryField()[conPatchList[i]]))); 
}
PowerDiss=PowerDiss/PowerDiss0;
const bool isQuasi2D = Foam::mag(GeoDim - scalar(2.0)) < SMALL;
const scalar meshFactor = std::pow(mesh.V()[0], 1.0/GeoDim);
if(isQuasi2D)
{
 PowerDiss=PowerDiss/meshFactor;
}
reduce(PowerDiss, sumOp<scalar>());

PowerDissActualOverBase = PowerDiss;
if(isQuasi2D)
{
  PowerDissActualOverBase = PowerDissActualOverBase*meshFactor;
}
PowerDissActual = PowerDissActualOverBase*PowerDiss0;
const scalar PDAct = PowerDiss*PowerDiss0*meshFactor;
PDActLog = PDAct/meshFactor;

if (Pstream::master())
{ 
   const char* optimizationHistoryStatus = optimizationHistorySolverStatus();
   std::ofstream historyFile(optimizationHistoryFile, std::ios::app);
   if (opt > 1 && ((opt - 1) % 25 == 0))
   {
      historyFile
        << std::left
        << std::setw(8) << "Iter"
        << std::setw(16) << "PDActLog"
        << std::setw(16) << "PDCon"
        << std::setw(16) << "V"
        << std::setw(16) << "MeanT"
        << std::setw(16) << "MaxT"
        << std::setw(12) << "SolverStat"
        << '\n';
   }
    historyFile
      << std::right
      << std::setw(8) << opt
      << std::scientific << std::setprecision(8)
      << std::setw(16) << PDActLog
      << std::setw(16) << PowerDiss
      << std::setw(16) << V
      << std::setw(16) << objectiveValueCurrent
      << std::setw(16) << maxT
      << std::setw(12) << optimizationHistoryStatus
      << '\n';
}
