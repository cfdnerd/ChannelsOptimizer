Info<< "filter chainrule: sanitize sensitivities" << endl;
label fsenshNonFiniteLocal(0);
label gsenshPowerNonFiniteLocal(0);
label gsenshSolidMinNonFiniteLocal(0);
label gsenshSolidMaxNonFiniteLocal(0);
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

  if (useMinSolidGeometryControl && !std::isfinite(static_cast<double>(gsenshSolidMin[i])))
  {
    gsenshSolidMin[i] = 0.0;
    gsenshSolidMinNonFiniteLocal++;
  }

  if (useMaxSolidGeometryControl && !std::isfinite(static_cast<double>(gsenshSolidMax[i])))
  {
    gsenshSolidMax[i] = 0.0;
    gsenshSolidMaxNonFiniteLocal++;
  }
}
reduce(fsenshNonFiniteLocal, sumOp<label>());
reduce(gsenshPowerNonFiniteLocal, sumOp<label>());
reduce(gsenshSolidMinNonFiniteLocal, sumOp<label>());
reduce(gsenshSolidMaxNonFiniteLocal, sumOp<label>());

if
(
  Pstream::master()
  && (
    fsenshNonFiniteLocal > 0
    || gsenshPowerNonFiniteLocal > 0
    || gsenshSolidMinNonFiniteLocal > 0
    || gsenshSolidMaxNonFiniteLocal > 0
  )
)
{
  Info<< "filter chainrule: clamped non-finite sensitivities"
  << " objective=" << fsenshNonFiniteLocal
      << " power=" << gsenshPowerNonFiniteLocal
      << " solidMin=" << gsenshSolidMinNonFiniteLocal
      << " solidMax=" << gsenshSolidMaxNonFiniteLocal << endl;
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
  if (useMinSolidGeometryControl)
  {
    gsenshSolidMin=gsenshSolidMin*drho;
  }
  if (useMaxSolidGeometryControl)
  {
    gsenshSolidMax=gsenshSolidMax*drho;
  }
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

if (useMinSolidGeometryControl)
{
  Info<< "filter chainrule: solid-min filter solve" << endl;
  const auto gsensSolidMinFilterLinearSolvePerformance =
    solve(fvm::laplacian(gsensSolidMin)-fvm::Sp(b,gsensSolidMin)+gsenshSolidMin*b);
  recordLinearSolve
  (
    gsensSolidMinFilterLinearSolveStats,
    gsensSolidMinFilterLinearSolvePerformance.initialResidual(),
    gsensSolidMinFilterLinearSolvePerformance.finalResidual(),
    gsensSolidMinFilterLinearSolvePerformance.nIterations()
  );
}

if (useMaxSolidGeometryControl)
{
  Info<< "filter chainrule: solid-max filter solve" << endl;
  const auto gsensSolidMaxFilterLinearSolvePerformance =
    solve(fvm::laplacian(gsensSolidMax)-fvm::Sp(b,gsensSolidMax)+gsenshSolidMax*b);
  recordLinearSolve
  (
    gsensSolidMaxFilterLinearSolveStats,
    gsensSolidMaxFilterLinearSolvePerformance.initialResidual(),
    gsensSolidMaxFilterLinearSolvePerformance.finalResidual(),
    gsensSolidMaxFilterLinearSolvePerformance.nIterations()
  );
}

Info<< "filter chainrule: scale filtered sensitivities" << endl;
fsensMeanT.primitiveFieldRef()=fsensMeanT.primitiveFieldRef()*mesh.V()/gMax(mesh.V());
gsensPowerDiss.primitiveFieldRef()=gsensPowerDiss.primitiveFieldRef()*mesh.V()/gMax(mesh.V());
gsensVol.primitiveFieldRef()=gsensVol.primitiveFieldRef()*mesh.V()/gMax(mesh.V());
if (useMinSolidGeometryControl)
{
  gsensSolidMin.primitiveFieldRef()=gsensSolidMin.primitiveFieldRef()*mesh.V()/gMax(mesh.V());
}
if (useMaxSolidGeometryControl)
{
  gsensSolidMax.primitiveFieldRef()=gsensSolidMax.primitiveFieldRef()*mesh.V()/gMax(mesh.V());
}
