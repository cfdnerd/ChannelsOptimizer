Info<< "filter chainrule: sanitize sensitivities" << endl;
label fsenshNonFiniteLocal(0);
label gsenshPowerNonFiniteLocal(0);
label gsenshVolNonFiniteLocal(0);
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

  if (!std::isfinite(static_cast<double>(gsenshVol[i])))
  {
    gsenshVol[i] = 0.0;
    gsenshVolNonFiniteLocal++;
  }

  if (applyMinSolidGeometryControl && !std::isfinite(static_cast<double>(gsenshSolidMin[i])))
  {
    gsenshSolidMin[i] = 0.0;
    gsenshSolidMinNonFiniteLocal++;
  }

  if (applyMaxSolidGeometryControl && !std::isfinite(static_cast<double>(gsenshSolidMax[i])))
  {
    gsenshSolidMax[i] = 0.0;
    gsenshSolidMaxNonFiniteLocal++;
  }
}
reduce(fsenshNonFiniteLocal, sumOp<label>());
reduce(gsenshPowerNonFiniteLocal, sumOp<label>());
reduce(gsenshVolNonFiniteLocal, sumOp<label>());
reduce(gsenshSolidMinNonFiniteLocal, sumOp<label>());
reduce(gsenshSolidMaxNonFiniteLocal, sumOp<label>());

if
(
  Pstream::master()
  && (
    fsenshNonFiniteLocal > 0
    || gsenshPowerNonFiniteLocal > 0
    || gsenshVolNonFiniteLocal > 0
    || gsenshSolidMinNonFiniteLocal > 0
    || gsenshSolidMaxNonFiniteLocal > 0
  )
)
{
  Info<< "filter chainrule: clamped non-finite sensitivities"
  << " objective=" << fsenshNonFiniteLocal
      << " power=" << gsenshPowerNonFiniteLocal
      << " volume=" << gsenshVolNonFiniteLocal
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
gsenshVol = dimensionedScalar("one", gsenshVol.dimensions(), 1.0);
Info<< "filter chainrule: objective filter solve" << endl;
//***********************************//Heaviside_filter
if(opt>1)
{
  const scalar eta5Safe =
    Foam::min(Foam::max(eta5, scalar(1e-6)), scalar(1.0 - 1e-6));
  const scalar oneMinusEta5Safe =
    Foam::max(scalar(1.0) - eta5Safe, scalar(1e-6));
  label drhoNonFiniteLocal(0);
  for(i=0;i<n;i++)
  {
     scalar drhoValue = 0.0;
     if(xp[i]<=eta5Safe)
     {
       drhoValue =
         del*Foam::exp(-del*(1-xp[i]/eta5Safe))
         + Foam::exp(-del);
     }
     else
     {
       drhoValue =
         del*Foam::exp(-del*(xp[i]-eta5Safe)/oneMinusEta5Safe)
         + Foam::exp(-del);
     }

     drhoValue = (1.0 - 1e-4)*drhoValue + 1e-4;

     if (!std::isfinite(static_cast<double>(drhoValue)))
     {
       drhoValue = 0.0;
       drhoNonFiniteLocal++;
     }

     drho[i] = drhoValue;
  }
  reduce(drhoNonFiniteLocal, sumOp<label>());
  fsenshMeanT=fsenshMeanT*drho;
  gsenshPowerDiss=gsenshPowerDiss*drho;
  gsenshVol=gsenshVol*drho;
  if (applyMinSolidGeometryControl)
  {
    gsenshSolidMin=gsenshSolidMin*drhoGeom;
  }
  if (applyMaxSolidGeometryControl)
  {
    gsenshSolidMax=gsenshSolidMax*drhoGeom;
  }

  label fsenshProjectedNonFiniteLocal(0);
  label gsenshPowerProjectedNonFiniteLocal(0);
  label gsenshVolProjectedNonFiniteLocal(0);
  label gsenshSolidMinProjectedNonFiniteLocal(0);
  label gsenshSolidMaxProjectedNonFiniteLocal(0);
  forAll(fsenshMeanT, i)
  {
    if (!std::isfinite(static_cast<double>(fsenshMeanT[i])))
    {
      fsenshMeanT[i] = 0.0;
      fsenshProjectedNonFiniteLocal++;
    }

    if (!std::isfinite(static_cast<double>(gsenshPowerDiss[i])))
    {
      gsenshPowerDiss[i] = 0.0;
      gsenshPowerProjectedNonFiniteLocal++;
    }

    if (!std::isfinite(static_cast<double>(gsenshVol[i])))
    {
      gsenshVol[i] = 0.0;
      gsenshVolProjectedNonFiniteLocal++;
    }

    if (applyMinSolidGeometryControl && !std::isfinite(static_cast<double>(gsenshSolidMin[i])))
    {
      gsenshSolidMin[i] = 0.0;
      gsenshSolidMinProjectedNonFiniteLocal++;
    }

    if (applyMaxSolidGeometryControl && !std::isfinite(static_cast<double>(gsenshSolidMax[i])))
    {
      gsenshSolidMax[i] = 0.0;
      gsenshSolidMaxProjectedNonFiniteLocal++;
    }
  }
  reduce(fsenshProjectedNonFiniteLocal, sumOp<label>());
  reduce(gsenshPowerProjectedNonFiniteLocal, sumOp<label>());
  reduce(gsenshVolProjectedNonFiniteLocal, sumOp<label>());
  reduce(gsenshSolidMinProjectedNonFiniteLocal, sumOp<label>());
  reduce(gsenshSolidMaxProjectedNonFiniteLocal, sumOp<label>());

  if
  (
    Pstream::master()
    && (
      drhoNonFiniteLocal > 0
      || fsenshProjectedNonFiniteLocal > 0
      || gsenshPowerProjectedNonFiniteLocal > 0
      || gsenshVolProjectedNonFiniteLocal > 0
      || gsenshSolidMinProjectedNonFiniteLocal > 0
      || gsenshSolidMaxProjectedNonFiniteLocal > 0
    )
  )
  {
    Info<< "filter chainrule: clamped non-finite projected sensitivities"
        << " drho=" << drhoNonFiniteLocal
        << " objective=" << fsenshProjectedNonFiniteLocal
        << " power=" << gsenshPowerProjectedNonFiniteLocal
        << " volume=" << gsenshVolProjectedNonFiniteLocal
        << " solidMin=" << gsenshSolidMinProjectedNonFiniteLocal
        << " solidMax=" << gsenshSolidMaxProjectedNonFiniteLocal << endl;
  }
}
//***********************************//PDE filter
if (!std::isfinite(static_cast<double>(b.value())))
{
  FatalErrorInFunction
    << "Non-finite density-filter coefficient b before filter_chainrule solves."
    << " activeFilterR=" << activeFilterR
    << " b=" << b.value()
    << exit(FatalError);
}
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

if (applyMinSolidGeometryControl)
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

if (applyMaxSolidGeometryControl)
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
if (applyMinSolidGeometryControl)
{
  gsensSolidMin.primitiveFieldRef()=gsensSolidMin.primitiveFieldRef()*mesh.V()/gMax(mesh.V());
}
if (applyMaxSolidGeometryControl)
{
  gsensSolidMax.primitiveFieldRef()=gsensSolidMax.primitiveFieldRef()*mesh.V()/gMax(mesh.V());
}
