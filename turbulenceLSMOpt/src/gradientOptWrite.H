scalar rawObjMinLocal = GREAT, rawObjMaxLocal = -GREAT, rawObjL2Local = 0.0;
scalar rawPowMinLocal = GREAT, rawPowMaxLocal = -GREAT, rawPowL2Local = 0.0;
scalar rawVolMinLocal = GREAT, rawVolMaxLocal = -GREAT, rawVolL2Local = 0.0;
scalar filtObjMinLocal = GREAT, filtObjMaxLocal = -GREAT, filtObjL2Local = 0.0;
scalar filtPowMinLocal = GREAT, filtPowMaxLocal = -GREAT, filtPowL2Local = 0.0;
scalar filtVolMinLocal = GREAT, filtVolMaxLocal = -GREAT, filtVolL2Local = 0.0;
scalar rawObjDotPowLocal = 0.0, rawObjDotVolLocal = 0.0, rawPowDotVolLocal = 0.0;
scalar filtObjDotPowLocal = 0.0, filtObjDotVolLocal = 0.0, filtPowDotVolLocal = 0.0;
label rawObjNonFiniteLocal = 0, rawPowNonFiniteLocal = 0, rawVolNonFiniteLocal = 0;
label filtObjNonFiniteLocal = 0, filtPowNonFiniteLocal = 0, filtVolNonFiniteLocal = 0;
scalar drhoMinGradLocal = GREAT, drhoMaxGradLocal = -GREAT;
scalar grayVolumeGradLocal = 0.0, solidVolumeGradLocal = 0.0, fluidVolumeGradLocal = 0.0;
label lowerBoundGradLocal = 0, upperBoundGradLocal = 0, velocityCapGradLocal = 0;
scalar maxAbsDesignChangeLocal = 0.0, l2DesignChangeLocal = 0.0;
scalar phiMinGradLocal = GREAT, phiMaxGradLocal = -GREAT;
scalar maxAbsPhiChangeGradLocal = 0.0, l2PhiChangeGradLocal = 0.0;
scalar interfaceObjMinGradLocal = GREAT, interfaceObjMaxGradLocal = -GREAT, interfaceObjL2GradLocal = 0.0;
scalar interfacePowMinGradLocal = GREAT, interfacePowMaxGradLocal = -GREAT, interfacePowL2GradLocal = 0.0;
scalar interfaceVolMinGradLocal = GREAT, interfaceVolMaxGradLocal = -GREAT, interfaceVolL2GradLocal = 0.0;
scalar rawVelMinGradLocal = GREAT, rawVelMaxGradLocal = -GREAT, rawVelL2GradLocal = 0.0;
scalar regVelMinGradLocal = GREAT, regVelMaxGradLocal = -GREAT, regVelL2GradLocal = 0.0;

forAll(fsensMeanTRaw, i)
{
  const scalar rawObj = fsensMeanTRaw[i];
  const scalar rawPow = gsensPowerDissRaw[i];
  const scalar rawVol = gsensVolRaw[i];
  const scalar filtObj = fsensMeanT[i];
  const scalar filtPow = gsensPowerDiss[i];
  const scalar filtVol = gsensVol[i];
  const scalar drhoVal = drho[i];
  const scalar cellVolume = mesh.V()[i];
  const scalar designChange = x[i] - xOld[i];
  const scalar phiValue = phiLS[i];
  const scalar phiChange = phiLS[i] - phiOld[i];
  const scalar interfaceObj = interfaceObjectiveSensitivity[i];
  const scalar interfacePow = interfacePowerSensitivity[i];
  const scalar interfaceVol = interfaceVolumeSensitivity[i];
  const scalar rawVel = normalVelocityRaw[i];
  const scalar regVel = normalVelocity[i];

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
  drhoMinGradLocal = Foam::min(drhoMinGradLocal, drhoVal);
  drhoMaxGradLocal = Foam::max(drhoMaxGradLocal, drhoVal);
  phiMinGradLocal = Foam::min(phiMinGradLocal, phiValue);
  phiMaxGradLocal = Foam::max(phiMaxGradLocal, phiValue);

  rawObjL2Local += rawObj*rawObj;
  rawPowL2Local += rawPow*rawPow;
  rawVolL2Local += rawVol*rawVol;
  filtObjL2Local += filtObj*filtObj;
  filtPowL2Local += filtPow*filtPow;
  filtVolL2Local += filtVol*filtVol;

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

  if (xh[i] > 0.05 && xh[i] < 0.95) grayVolumeGradLocal += cellVolume;
  if (xh[i] <= 0.05) solidVolumeGradLocal += cellVolume;
  if (xh[i] >= 0.95) fluidVolumeGradLocal += cellVolume;
  if (x[i] <= 1e-8) lowerBoundGradLocal++;
  if (x[i] >= 1.0 - 1e-8) upperBoundGradLocal++;
  if (Foam::mag(rawVel) >= 0.999*maxNormalVelocity) velocityCapGradLocal++;
  maxAbsDesignChangeLocal = Foam::max(maxAbsDesignChangeLocal, Foam::mag(designChange));
  l2DesignChangeLocal += designChange*designChange;
  maxAbsPhiChangeGradLocal = Foam::max(maxAbsPhiChangeGradLocal, Foam::mag(phiChange));
  l2PhiChangeGradLocal += phiChange*phiChange;

  interfaceObjMinGradLocal = Foam::min(interfaceObjMinGradLocal, interfaceObj);
  interfaceObjMaxGradLocal = Foam::max(interfaceObjMaxGradLocal, interfaceObj);
  interfaceObjL2GradLocal += interfaceObj*interfaceObj;
  interfacePowMinGradLocal = Foam::min(interfacePowMinGradLocal, interfacePow);
  interfacePowMaxGradLocal = Foam::max(interfacePowMaxGradLocal, interfacePow);
  interfacePowL2GradLocal += interfacePow*interfacePow;
  interfaceVolMinGradLocal = Foam::min(interfaceVolMinGradLocal, interfaceVol);
  interfaceVolMaxGradLocal = Foam::max(interfaceVolMaxGradLocal, interfaceVol);
  interfaceVolL2GradLocal += interfaceVol*interfaceVol;
  rawVelMinGradLocal = Foam::min(rawVelMinGradLocal, rawVel);
  rawVelMaxGradLocal = Foam::max(rawVelMaxGradLocal, rawVel);
  rawVelL2GradLocal += rawVel*rawVel;
  regVelMinGradLocal = Foam::min(regVelMinGradLocal, regVel);
  regVelMaxGradLocal = Foam::max(regVelMaxGradLocal, regVel);
  regVelL2GradLocal += regVel*regVel;
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
reduce(phiMinGradLocal, minOp<scalar>());
reduce(phiMaxGradLocal, maxOp<scalar>());
reduce(grayVolumeGradLocal, sumOp<scalar>());
reduce(solidVolumeGradLocal, sumOp<scalar>());
reduce(fluidVolumeGradLocal, sumOp<scalar>());
reduce(lowerBoundGradLocal, sumOp<label>());
reduce(upperBoundGradLocal, sumOp<label>());
reduce(velocityCapGradLocal, sumOp<label>());
reduce(maxAbsDesignChangeLocal, maxOp<scalar>());
reduce(l2DesignChangeLocal, sumOp<scalar>());
reduce(maxAbsPhiChangeGradLocal, maxOp<scalar>());
reduce(l2PhiChangeGradLocal, sumOp<scalar>());
reduce(interfaceObjMinGradLocal, minOp<scalar>());
reduce(interfaceObjMaxGradLocal, maxOp<scalar>());
reduce(interfaceObjL2GradLocal, sumOp<scalar>());
reduce(interfacePowMinGradLocal, minOp<scalar>());
reduce(interfacePowMaxGradLocal, maxOp<scalar>());
reduce(interfacePowL2GradLocal, sumOp<scalar>());
reduce(interfaceVolMinGradLocal, minOp<scalar>());
reduce(interfaceVolMaxGradLocal, maxOp<scalar>());
reduce(interfaceVolL2GradLocal, sumOp<scalar>());
reduce(rawVelMinGradLocal, minOp<scalar>());
reduce(rawVelMaxGradLocal, maxOp<scalar>());
reduce(rawVelL2GradLocal, sumOp<scalar>());
reduce(regVelMinGradLocal, minOp<scalar>());
reduce(regVelMaxGradLocal, maxOp<scalar>());
reduce(regVelL2GradLocal, sumOp<scalar>());

const scalar rawObjL2 = std::sqrt(rawObjL2Local);
const scalar rawPowL2 = std::sqrt(rawPowL2Local);
const scalar rawVolL2 = std::sqrt(rawVolL2Local);
const scalar filtObjL2 = std::sqrt(filtObjL2Local);
const scalar filtPowL2 = std::sqrt(filtPowL2Local);
const scalar filtVolL2 = std::sqrt(filtVolL2Local);
const scalar normL2DesignChange = std::sqrt(l2DesignChangeLocal)/std::sqrt(Foam::max(N, scalar(1)));
const scalar normL2PhiChange = std::sqrt(l2PhiChangeGradLocal)/std::sqrt(Foam::max(N, scalar(1)));
const scalar rawObjPowCos = rawObjDotPowLocal/(Foam::max(rawObjL2*rawPowL2, SMALL));
const scalar rawObjVolCos = rawObjDotVolLocal/(Foam::max(rawObjL2*rawVolL2, SMALL));
const scalar rawPowVolCos = rawPowDotVolLocal/(Foam::max(rawPowL2*rawVolL2, SMALL));
const scalar filtObjPowCos = filtObjDotPowLocal/(Foam::max(filtObjL2*filtPowL2, SMALL));
const scalar filtObjVolCos = filtObjDotVolLocal/(Foam::max(filtObjL2*filtVolL2, SMALL));
const scalar filtPowVolCos = filtPowDotVolLocal/(Foam::max(filtPowL2*filtVolL2, SMALL));
const scalar interfaceObjL2Grad = std::sqrt(interfaceObjL2GradLocal);
const scalar interfacePowL2Grad = std::sqrt(interfacePowL2GradLocal);
const scalar interfaceVolL2Grad = std::sqrt(interfaceVolL2GradLocal);
const scalar rawVelL2Grad = std::sqrt(rawVelL2GradLocal);
const scalar regVelL2Grad = std::sqrt(regVelL2GradLocal);

if (Pstream::master())
{
  std::ofstream gradientLogFile(gradientOptFile, std::ios::app);
  gradientLogFile
    << "\n================================================================================\n"
    << " Iteration " << opt << " gradient diagnostics\n"
    << "--------------------------------------------------------------------------------\n"
    << " objective: " << activeObjectiveLabel << "  value=" << objectiveValueCurrent << "\n"
    << " constraints: power=" << gx[0] << " volume=" << gx[1]
    << "  weightedLimit=" << activePowerLimit << "\n"
    << " controls: qAlpha=" << qAlpha << " qKappa=" << qKappa << " qHeat=" << qHeat
    << " alphaMax=" << alphaMax.value() << " beta=" << del
    << " epsilonLSM=" << epsilonLSMActive << "\n"
    << " modes: wallDistance=" << activeWallDistanceMode
    << " turbulence=" << activeTurbulenceModelMode
    << " lsm=" << activeLSMUpdateMode << "\n"
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
    << " interface objective: min=" << interfaceObjMinGradLocal << " max=" << interfaceObjMaxGradLocal
    << " l2=" << interfaceObjL2Grad << "\n"
    << " interface power: min=" << interfacePowMinGradLocal << " max=" << interfacePowMaxGradLocal
    << " l2=" << interfacePowL2Grad << "\n"
    << " interface volume: min=" << interfaceVolMinGradLocal << " max=" << interfaceVolMaxGradLocal
    << " l2=" << interfaceVolL2Grad << "\n"
    << " velocity raw: min=" << rawVelMinGradLocal << " max=" << rawVelMaxGradLocal
    << " l2=" << rawVelL2Grad << "\n"
    << " velocity reg: min=" << regVelMinGradLocal << " max=" << regVelMaxGradLocal
    << " l2=" << regVelL2Grad << "\n"
    << " projection control: powerProjRaw=" << powerProjectionMultiplierAnalytical
    << " powerProjFloor=" << powerProjectionMultiplierFloor
    << " powerProjApply=" << powerProjectionMultiplier
    << " volumeProj=" << volumeProjectionMultiplier << "\n"
    << " power support: floor=" << powerSupportFloorValue
    << " reach=" << powerSupportFloorNegativeReach
    << " floorVol=" << powerSupportFloorVolumeFraction
    << " floorCells=" << powerSupportFloorCellCount << "\n"
    << " volume control: proj=" << volumeProjectionMultiplier
    << " phiShiftRaw=" << volumePhiShiftRaw
    << " phiShiftApplied=" << volumePhiShiftApplied
    << " phiShiftCap=" << volumePhiShiftCap << "\n"
    << " fluid shift: pre=" << fluidFractionBeforeVolumeShift
    << " post=" << fluidFractionAfterVolumeShift
    << " recovered=" << fluidFractionRecoveredByShift << "\n"
    << " drho: min=" << drhoMinGradLocal << " max=" << drhoMaxGradLocal << "\n"
    << " phiLS: min=" << phiMinGradLocal << " max=" << phiMaxGradLocal
    << " phiStepL2/sqrtN=" << normL2PhiChange
    << " phiStepMax=" << maxAbsPhiChangeGradLocal << "\n"
    << " cosine raw: obj-power=" << rawObjPowCos
    << " obj-vol=" << rawObjVolCos << " power-vol=" << rawPowVolCos << "\n"
    << " cosine filtered: obj-power=" << filtObjPowCos
    << " obj-vol=" << filtObjVolCos << " power-vol=" << filtPowVolCos << "\n"
    << " design change: maxAbs=" << maxAbsDesignChangeLocal
    << " l2/sqrtN=" << normL2DesignChange << "\n"
    << " bound counts: lower=" << lowerBoundGradLocal
    << " upper=" << upperBoundGradLocal
    << " velCap=" << velocityCapGradLocal << "\n"
    << " xh fractions: gray=" << grayVolumeGradLocal/area
    << " solid=" << solidVolumeGradLocal/area
    << " fluid=" << fluidVolumeGradLocal/area << "\n";
}
