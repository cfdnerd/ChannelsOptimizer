scalar rawObjMinLocal = GREAT, rawObjMaxLocal = -GREAT, rawObjL2Local = 0.0;
scalar rawPowMinLocal = GREAT, rawPowMaxLocal = -GREAT, rawPowL2Local = 0.0;
scalar rawVolMinLocal = GREAT, rawVolMaxLocal = -GREAT, rawVolL2Local = 0.0;
scalar filtObjMinLocal = GREAT, filtObjMaxLocal = -GREAT, filtObjL2Local = 0.0;
scalar filtPowMinLocal = GREAT, filtPowMaxLocal = -GREAT, filtPowL2Local = 0.0;
scalar filtVolMinLocal = GREAT, filtVolMaxLocal = -GREAT, filtVolL2Local = 0.0;
scalar filtSolidMinMinLocal = GREAT, filtSolidMinMaxLocal = -GREAT, filtSolidMinL2Local = 0.0;
scalar filtSolidMaxMinLocal = GREAT, filtSolidMaxMaxLocal = -GREAT, filtSolidMaxL2Local = 0.0;
scalar rawObjDotPowLocal = 0.0, rawObjDotVolLocal = 0.0, rawPowDotVolLocal = 0.0;
scalar filtObjDotPowLocal = 0.0, filtObjDotVolLocal = 0.0, filtPowDotVolLocal = 0.0;
label rawObjNonFiniteLocal = 0, rawPowNonFiniteLocal = 0, rawVolNonFiniteLocal = 0;
label filtObjNonFiniteLocal = 0, filtPowNonFiniteLocal = 0, filtVolNonFiniteLocal = 0;
label filtSolidMinNonFiniteLocal = 0, filtSolidMaxNonFiniteLocal = 0;
scalar drhoMinGradLocal = GREAT, drhoMaxGradLocal = -GREAT;
scalar grayVolumeGradLocal = 0.0, solidVolumeGradLocal = 0.0, fluidVolumeGradLocal = 0.0;
label lowerBoundGradLocal = 0, upperBoundGradLocal = 0, moveLimitGradLocal = 0;
scalar maxAbsDesignChangeLocal = 0.0, l2DesignChangeLocal = 0.0;

forAll(fsensMeanTRaw, i)
{
  const scalar rawObj = fsensMeanTRaw[i];
  const scalar rawPow = gsensPowerDissRaw[i];
  const scalar rawVol = gsensVolRaw[i];
  const scalar filtObj = fsensMeanT[i];
  const scalar filtPow = gsensPowerDiss[i];
  const scalar filtVol = gsensVol[i];
  const scalar filtSolidMin = useMinSolidGeometryControl ? gsensSolidMin[i] : 0.0;
  const scalar filtSolidMax = useMaxSolidGeometryControl ? gsensSolidMax[i] : 0.0;
  const scalar drhoVal = drho[i];
  const scalar cellVolume = mesh.V()[i];
  const scalar designChange = x[i] - xOld[i];

  rawObjMinLocal = Foam::min(rawObjMinLocal, rawObj);
  rawObjMaxLocal = Foam::max(rawObjMaxLocal, rawObj);
  rawPowMinLocal = Foam::min(rawPowMinLocal, rawPow);
  rawPowMaxLocal = Foam::max(rawPowMaxLocal, rawPow);
  rawVolMinLocal = Foam::min(rawVolMinLocal, rawVol);
  rawVolMaxLocal = Foam::max(rawVolMaxLocal, rawVol);
  filtObjMinLocal = Foam::min(filtObjMinLocal, filtObj);
  filtObjMaxLocal = Foam::max(filtObjMaxLocal, filtObj);
  filtPowMinLocal = Foam::min(filtPowMinLocal, filtPow);
  filtPowMaxLocal = Foam::max(filtPowMaxLocal, filtPow);
  filtVolMinLocal = Foam::min(filtVolMinLocal, filtVol);
  filtVolMaxLocal = Foam::max(filtVolMaxLocal, filtVol);
  if (useMinSolidGeometryControl)
  {
    filtSolidMinMinLocal = Foam::min(filtSolidMinMinLocal, filtSolidMin);
    filtSolidMinMaxLocal = Foam::max(filtSolidMinMaxLocal, filtSolidMin);
  }
  if (useMaxSolidGeometryControl)
  {
    filtSolidMaxMinLocal = Foam::min(filtSolidMaxMinLocal, filtSolidMax);
    filtSolidMaxMaxLocal = Foam::max(filtSolidMaxMaxLocal, filtSolidMax);
  }
  drhoMinGradLocal = Foam::min(drhoMinGradLocal, drhoVal);
  drhoMaxGradLocal = Foam::max(drhoMaxGradLocal, drhoVal);

  rawObjL2Local += rawObj*rawObj;
  rawPowL2Local += rawPow*rawPow;
  rawVolL2Local += rawVol*rawVol;
  filtObjL2Local += filtObj*filtObj;
  filtPowL2Local += filtPow*filtPow;
  filtVolL2Local += filtVol*filtVol;
  if (useMinSolidGeometryControl)
  {
    filtSolidMinL2Local += filtSolidMin*filtSolidMin;
  }
  if (useMaxSolidGeometryControl)
  {
    filtSolidMaxL2Local += filtSolidMax*filtSolidMax;
  }

  rawObjDotPowLocal += rawObj*rawPow;
  rawObjDotVolLocal += rawObj*rawVol;
  rawPowDotVolLocal += rawPow*rawVol;
  filtObjDotPowLocal += filtObj*filtPow;
  filtObjDotVolLocal += filtObj*filtVol;
  filtPowDotVolLocal += filtPow*filtVol;

  if (!std::isfinite(static_cast<double>(rawObj))) rawObjNonFiniteLocal++;
  if (!std::isfinite(static_cast<double>(rawPow))) rawPowNonFiniteLocal++;
  if (!std::isfinite(static_cast<double>(rawVol))) rawVolNonFiniteLocal++;
  if (!std::isfinite(static_cast<double>(filtObj))) filtObjNonFiniteLocal++;
  if (!std::isfinite(static_cast<double>(filtPow))) filtPowNonFiniteLocal++;
  if (!std::isfinite(static_cast<double>(filtVol))) filtVolNonFiniteLocal++;
  if (useMinSolidGeometryControl && !std::isfinite(static_cast<double>(filtSolidMin)))
  {
    filtSolidMinNonFiniteLocal++;
  }
  if (useMaxSolidGeometryControl && !std::isfinite(static_cast<double>(filtSolidMax)))
  {
    filtSolidMaxNonFiniteLocal++;
  }

  if (xh[i] > 0.05 && xh[i] < 0.95) grayVolumeGradLocal += cellVolume;
  if (xh[i] <= 0.05) solidVolumeGradLocal += cellVolume;
  if (xh[i] >= 0.95) fluidVolumeGradLocal += cellVolume;
  if (x[i] <= 1e-8) lowerBoundGradLocal++;
  if (x[i] >= 1.0 - 1e-8) upperBoundGradLocal++;
  if (Foam::mag(designChange) >= 0.999*movlim) moveLimitGradLocal++;
  maxAbsDesignChangeLocal = Foam::max(maxAbsDesignChangeLocal, Foam::mag(designChange));
  l2DesignChangeLocal += designChange*designChange;
}

reduce(rawObjMinLocal, minOp<scalar>());
reduce(rawObjMaxLocal, maxOp<scalar>());
reduce(rawPowMinLocal, minOp<scalar>());
reduce(rawPowMaxLocal, maxOp<scalar>());
reduce(rawVolMinLocal, minOp<scalar>());
reduce(rawVolMaxLocal, maxOp<scalar>());
reduce(filtObjMinLocal, minOp<scalar>());
reduce(filtObjMaxLocal, maxOp<scalar>());
reduce(filtPowMinLocal, minOp<scalar>());
reduce(filtPowMaxLocal, maxOp<scalar>());
reduce(filtVolMinLocal, minOp<scalar>());
reduce(filtVolMaxLocal, maxOp<scalar>());
reduce(rawObjL2Local, sumOp<scalar>());
reduce(rawPowL2Local, sumOp<scalar>());
reduce(rawVolL2Local, sumOp<scalar>());
reduce(filtObjL2Local, sumOp<scalar>());
reduce(filtPowL2Local, sumOp<scalar>());
reduce(filtVolL2Local, sumOp<scalar>());
if (useMinSolidGeometryControl)
{
  reduce(filtSolidMinMinLocal, minOp<scalar>());
  reduce(filtSolidMinMaxLocal, maxOp<scalar>());
  reduce(filtSolidMinL2Local, sumOp<scalar>());
  reduce(filtSolidMinNonFiniteLocal, sumOp<label>());
}
if (useMaxSolidGeometryControl)
{
  reduce(filtSolidMaxMinLocal, minOp<scalar>());
  reduce(filtSolidMaxMaxLocal, maxOp<scalar>());
  reduce(filtSolidMaxL2Local, sumOp<scalar>());
  reduce(filtSolidMaxNonFiniteLocal, sumOp<label>());
}
reduce(rawObjDotPowLocal, sumOp<scalar>());
reduce(rawObjDotVolLocal, sumOp<scalar>());
reduce(rawPowDotVolLocal, sumOp<scalar>());
reduce(filtObjDotPowLocal, sumOp<scalar>());
reduce(filtObjDotVolLocal, sumOp<scalar>());
reduce(filtPowDotVolLocal, sumOp<scalar>());
reduce(rawObjNonFiniteLocal, sumOp<label>());
reduce(rawPowNonFiniteLocal, sumOp<label>());
reduce(rawVolNonFiniteLocal, sumOp<label>());
reduce(filtObjNonFiniteLocal, sumOp<label>());
reduce(filtPowNonFiniteLocal, sumOp<label>());
reduce(filtVolNonFiniteLocal, sumOp<label>());
reduce(drhoMinGradLocal, minOp<scalar>());
reduce(drhoMaxGradLocal, maxOp<scalar>());
reduce(grayVolumeGradLocal, sumOp<scalar>());
reduce(solidVolumeGradLocal, sumOp<scalar>());
reduce(fluidVolumeGradLocal, sumOp<scalar>());
reduce(lowerBoundGradLocal, sumOp<label>());
reduce(upperBoundGradLocal, sumOp<label>());
reduce(moveLimitGradLocal, sumOp<label>());
reduce(maxAbsDesignChangeLocal, maxOp<scalar>());
reduce(l2DesignChangeLocal, sumOp<scalar>());

const scalar rawObjL2 = std::sqrt(rawObjL2Local);
const scalar rawPowL2 = std::sqrt(rawPowL2Local);
const scalar rawVolL2 = std::sqrt(rawVolL2Local);
const scalar filtObjL2 = std::sqrt(filtObjL2Local);
const scalar filtPowL2 = std::sqrt(filtPowL2Local);
const scalar filtVolL2 = std::sqrt(filtVolL2Local);
const scalar filtSolidMinL2 =
  useMinSolidGeometryControl ? std::sqrt(filtSolidMinL2Local) : 0.0;
const scalar filtSolidMaxL2 =
  useMaxSolidGeometryControl ? std::sqrt(filtSolidMaxL2Local) : 0.0;
const scalar normL2DesignChange = std::sqrt(l2DesignChangeLocal)/std::sqrt(Foam::max(N, scalar(1)));
const scalar rawObjPowCos = rawObjDotPowLocal/(Foam::max(rawObjL2*rawPowL2, SMALL));
const scalar rawObjVolCos = rawObjDotVolLocal/(Foam::max(rawObjL2*rawVolL2, SMALL));
const scalar rawPowVolCos = rawPowDotVolLocal/(Foam::max(rawPowL2*rawVolL2, SMALL));
const scalar filtObjPowCos = filtObjDotPowLocal/(Foam::max(filtObjL2*filtPowL2, SMALL));
const scalar filtObjVolCos = filtObjDotVolLocal/(Foam::max(filtObjL2*filtVolL2, SMALL));
const scalar filtPowVolCos = filtPowDotVolLocal/(Foam::max(filtPowL2*filtVolL2, SMALL));

if (Pstream::master())
{
  std::ostringstream constraintStream;
  constraintStream
    << " constraints: power=" << gx[powerConstraintIndex]
    << " volume=" << gx[volumeConstraintIndex];
  if (useMinSolidGeometryControl)
  {
    constraintStream
      << " solidMinRaw=" << solidMinConstraintValue
      << " solidMinMMA=" << gx[minSolidConstraintIndex];
  }
  if (useMaxSolidGeometryControl)
  {
    constraintStream
      << " solidMaxRaw=" << solidMaxConstraintValue
      << " solidMaxMMA=" << gx[maxSolidConstraintIndex];
  }
  constraintStream << "  weightedLimit=" << activePowerLimit;

  std::ofstream gradientLogFile(gradientOptFile, std::ios::app);
  gradientLogFile
    << "\n================================================================================\n"
    << " Iteration " << opt << " gradient diagnostics\n"
    << "--------------------------------------------------------------------------------\n"
    << " objective: " << activeObjectiveLabel << "  value=" << objectiveValueCurrent << "\n"
    << constraintStream.str() << "\n"
    << " controls: qAlpha=" << qAlpha << " qKappa=" << qKappa << " qHeat=" << qHeat
    << " alphaMax=" << alphaMax.value() << " beta=" << del << "\n"
    << " geom stage: active=" << (geometryControlStageActivated ? "yes" : "no")
    << " weight=" << geometryControlStageWeight
    << " frozen=" << (geometryControlHardeningFrozen ? "yes" : "no")
    << " betaGeom=" << geometricSizeProjectionBeta
    << " etaGeom=" << geometryProjectionEtaActive << "\n"
    << " modes: wallDistance=" << activeWallDistanceMode
    << " turbulence=" << activeTurbulenceModelMode << "\n"
    << " raw objective: min=" << rawObjMinLocal << " max=" << rawObjMaxLocal
    << " l2=" << rawObjL2 << " nonFinite=" << rawObjNonFiniteLocal << "\n"
    << " raw power: min=" << rawPowMinLocal << " max=" << rawPowMaxLocal
    << " l2=" << rawPowL2 << " nonFinite=" << rawPowNonFiniteLocal << "\n"
    << " raw volume: min=" << rawVolMinLocal << " max=" << rawVolMaxLocal
    << " l2=" << rawVolL2 << " nonFinite=" << rawVolNonFiniteLocal << "\n"
    << " filtered objective: min=" << filtObjMinLocal << " max=" << filtObjMaxLocal
    << " l2=" << filtObjL2 << " nonFinite=" << filtObjNonFiniteLocal << "\n"
    << " filtered power: min=" << filtPowMinLocal << " max=" << filtPowMaxLocal
    << " l2=" << filtPowL2 << " nonFinite=" << filtPowNonFiniteLocal << "\n"
    << " filtered volume: min=" << filtVolMinLocal << " max=" << filtVolMaxLocal
    << " l2=" << filtVolL2 << " nonFinite=" << filtVolNonFiniteLocal << "\n"
    << " drho: min=" << drhoMinGradLocal << " max=" << drhoMaxGradLocal << "\n"
    << " cosine raw: obj-power=" << rawObjPowCos
    << " obj-vol=" << rawObjVolCos << " power-vol=" << rawPowVolCos << "\n"
    << " cosine filtered: obj-power=" << filtObjPowCos
    << " obj-vol=" << filtObjVolCos << " power-vol=" << filtPowVolCos << "\n"
    << " design change: maxAbs=" << maxAbsDesignChangeLocal
    << " l2/sqrtN=" << normL2DesignChange << "\n"
    << " bound counts: lower=" << lowerBoundGradLocal
    << " upper=" << upperBoundGradLocal
    << " moveLimit=" << moveLimitGradLocal << "\n"
    << " xh fractions: gray=" << grayVolumeGradLocal/area
    << " solid=" << solidVolumeGradLocal/area
    << " fluid=" << fluidVolumeGradLocal/area << "\n";
  if (useMinSolidGeometryControl)
  {
    gradientLogFile
      << " filtered solidMin: min=" << filtSolidMinMinLocal
      << " max=" << filtSolidMinMaxLocal
      << " l2=" << filtSolidMinL2
      << " nonFinite=" << filtSolidMinNonFiniteLocal << "\n";
  }
  if (useMaxSolidGeometryControl)
  {
    gradientLogFile
      << " filtered solidMax: min=" << filtSolidMaxMinLocal
      << " max=" << filtSolidMaxMaxLocal
      << " l2=" << filtSolidMaxL2
      << " nonFinite=" << filtSolidMaxNonFiniteLocal << "\n";
  }
}
