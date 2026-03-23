Info<< "filter chainrule: sanitize sensitivities" << endl;
label fsenshNonFiniteLocal(0);
label gsenshPowerNonFiniteLocal(0);
forAll(fsenshMeanT, i)
{
  if (!std::isfinite(static_cast<double>(fsenshMeanT[i])))
  {
    fsenshMeanT[i] = 0.0;
    fsenshNonFiniteLocal++;
  }

  if (!std::isfinite(static_cast<double>(gsenshPowerDiss[i])))
  {
    gsenshPowerDiss[i] = 0.0;
    gsenshPowerNonFiniteLocal++;
  }
}
reduce(fsenshNonFiniteLocal, sumOp<label>());
reduce(gsenshPowerNonFiniteLocal, sumOp<label>());

if (Pstream::master() && (fsenshNonFiniteLocal > 0 || gsenshPowerNonFiniteLocal > 0))
{
  Info<< "filter chainrule: clamped non-finite sensitivities"
      << " objective=" << fsenshNonFiniteLocal
      << " power=" << gsenshPowerNonFiniteLocal << endl;
}

Info<< "filter chainrule: normalize sensitivities" << endl;
const scalar fsenshMeanTScale = gMax(mag(fsenshMeanT.primitiveField()));
if (fsenshMeanTScale > SMALL)
{
  fsenshMeanT = fsenshMeanT/fsenshMeanTScale;
}
else
{
  fsenshMeanT = dimensionedScalar("zero", fsenshMeanT.dimensions(), 0.0);
}

const scalar gsenshPowerDissScale = gMax(mag(gsenshPowerDiss.primitiveField()));
if (gsenshPowerDissScale > SMALL)
{
  gsenshPowerDiss = gsenshPowerDiss/gsenshPowerDissScale;
}
else
{
  gsenshPowerDiss = dimensionedScalar("zero", gsenshPowerDiss.dimensions(), 0.0);
}
gsenshVol.primitiveFieldRef() = 1.0;
Info<< "filter chainrule: objective filter solve" << endl;
//***********************************//Heaviside_filter
if(opt>1)
{
  for(i=0;i<n;i++)
  {
     if(xp[i]<=eta5)
     {
       drho[i]=del*Foam::exp(-del*(1-xp[i]/eta5))+Foam::exp(-del); 
     }
     else
     {
       drho[i]=del*Foam::exp(-del*(xp[i]-eta5)/(1-eta5))+Foam::exp(-del); 
     } 
  }
  fsenshMeanT=fsenshMeanT*drho;
  gsenshPowerDiss=gsenshPowerDiss*drho;
  gsenshVol=gsenshVol*drho;
}
//***********************************//PDE filter
const auto fsensFilterLinearSolvePerformance =
solve(fvm::laplacian(fsensMeanT)-fvm::Sp(b,fsensMeanT)+fsenshMeanT*b);
recordLinearSolve
(
  fsensFilterLinearSolveStats,
  fsensFilterLinearSolvePerformance.initialResidual(),
  fsensFilterLinearSolvePerformance.finalResidual(),
  fsensFilterLinearSolvePerformance.nIterations()
);

Info<< "filter chainrule: power filter solve" << endl;
const auto gsensPowerFilterLinearSolvePerformance =
solve(fvm::laplacian(gsensPowerDiss)-fvm::Sp(b,gsensPowerDiss)+gsenshPowerDiss*b);
recordLinearSolve
(
  gsensPowerFilterLinearSolveStats,
  gsensPowerFilterLinearSolvePerformance.initialResidual(),
  gsensPowerFilterLinearSolvePerformance.finalResidual(),
  gsensPowerFilterLinearSolvePerformance.nIterations()
);

Info<< "filter chainrule: volume filter solve" << endl;
const auto gsensVolFilterLinearSolvePerformance =
solve(fvm::laplacian(gsensVol)-fvm::Sp(b,gsensVol)+gsenshVol*b);
recordLinearSolve
(
  gsensVolFilterLinearSolveStats,
  gsensVolFilterLinearSolvePerformance.initialResidual(),
  gsensVolFilterLinearSolvePerformance.finalResidual(),
  gsensVolFilterLinearSolvePerformance.nIterations()
);

Info<< "filter chainrule: scale filtered sensitivities" << endl;
fsensMeanT.primitiveFieldRef()=fsensMeanT.primitiveFieldRef()*mesh.V()/gMax(mesh.V());
gsensPowerDiss.primitiveFieldRef()=gsensPowerDiss.primitiveFieldRef()*mesh.V()/gMax(mesh.V());
gsensVol.primitiveFieldRef()=gsensVol.primitiveFieldRef()*mesh.V()/gMax(mesh.V());
