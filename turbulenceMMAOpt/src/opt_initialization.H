Info<< "Initialization\n" << endl;
const char* optimizationHistoryFile = "optimization.hst";
const char* debugOptimizerJsonFile = "debugOptimizer.jsonl";
const char* debugOptimizerLogFile = "debugOptimizer.log";
const char* gradientOptFile = "gradientOpt.log";
const char* solverConvergenceFile = "solverConvergences.log";
if (Pstream::master())
{
  std::remove(optimizationHistoryFile);
  std::remove(debugOptimizerJsonFile);
  std::remove(debugOptimizerLogFile);
  std::remove(gradientOptFile);
  std::remove(solverConvergenceFile);
  std::ofstream historyFile(optimizationHistoryFile, std::ios::out);
  historyFile
    << "# turbulenceMMAOpt optimization history\n"
    << "# Cols: Iter PDActLog PDCon V MeanT MaxT\n"
    << "# PDActLog=PDCon*PowerDiss0, PDCon=PowerDiss used by MMA\n"
    << std::left
    << std::setw(8) << "Iter"
    << std::setw(16) << "PDActLog"
    << std::setw(16) << "PDCon"
    << std::setw(16) << "V"
    << std::setw(16) << "MeanT"
    << std::setw(16) << "MaxT"
    << '\n';

  std::ofstream debugLogFile(debugOptimizerLogFile, std::ios::out);
  debugLogFile
    << "# turbulenceMMAOpt optimizer debug log\n"
    << "# Structured per-iteration diagnostics for optimizer development\n"
    << "# Format: sectioned human-readable report, one block per optimization iteration\n"
    << "\n[Runtime Option Dump]\n"
    << "\n--- optProperties ---\n"
    << optProperties
    << "\n--- tuneOptParameters ---\n"
    << tuneOptParameters
    << "\n--- transportProperties ---\n"
    << transportProperties
    << "\n--- thermalProperties ---\n"
    << thermalProperties
    << "\n";

  std::ofstream gradientLogFile(gradientOptFile, std::ios::out);
  gradientLogFile
    << "# turbulenceMMAOpt gradient and sensitivity diagnostics\n"
    << "# Structured one-block-per-iteration report\n";

  std::ofstream solverConvergenceLogFile(solverConvergenceFile, std::ios::out);
  solverConvergenceLogFile
    << "# turbulenceMMAOpt solver convergence log\n"
    << "# Linear-solver statistics grouped by optimization iteration\n";
}

struct LinearSolveStats
{
  label callCount;
  scalar initialResidualSum;
  scalar initialResidualMax;
  scalar finalResidualSum;
  scalar finalResidualMax;
  label iterationSum;
  label iterationMax;
  scalar lastInitialResidual;
  scalar lastFinalResidual;
  label lastIterations;

  LinearSolveStats()
  :
    callCount(0),
    initialResidualSum(0),
    initialResidualMax(0),
    finalResidualSum(0),
    finalResidualMax(0),
    iterationSum(0),
    iterationMax(0),
    lastInitialResidual(0),
    lastFinalResidual(0),
    lastIterations(0)
  {}
};

auto resetLinearSolveStats =
[](LinearSolveStats& stats)
{
  stats.callCount = 0;
  stats.initialResidualSum = 0;
  stats.initialResidualMax = 0;
  stats.finalResidualSum = 0;
  stats.finalResidualMax = 0;
  stats.iterationSum = 0;
  stats.iterationMax = 0;
  stats.lastInitialResidual = 0;
  stats.lastFinalResidual = 0;
  stats.lastIterations = 0;
};

auto recordLinearSolve =
[](LinearSolveStats& stats, const scalar initialResidual, const scalar finalResidual, const label iterations)
{
  stats.callCount++;
  stats.initialResidualSum += initialResidual;
  stats.initialResidualMax = Foam::max(stats.initialResidualMax, initialResidual);
  stats.finalResidualSum += finalResidual;
  stats.finalResidualMax = Foam::max(stats.finalResidualMax, finalResidual);
  stats.iterationSum += iterations;
  stats.iterationMax = Foam::max(stats.iterationMax, iterations);
  stats.lastInitialResidual = initialResidual;
  stats.lastFinalResidual = finalResidual;
  stats.lastIterations = iterations;
};

const scalar linearSolveObsResidual = 1e-6;
const scalar linearSolveWarnResidual = 1e-4;

auto classifyLinearSolveStatus =
[&](const LinearSolveStats& stats) -> const char*
{
  if (stats.callCount == 0)
  {
    return "SKIP";
  }
  if (stats.finalResidualMax > linearSolveWarnResidual)
  {
    return "WARN";
  }
  if (stats.finalResidualMax > linearSolveObsResidual)
  {
    return "OBS";
  }
  return "OK";
};

auto linearSolveSeverity =
[](const char* status) -> label
{
  if (std::strcmp(status, "WARN") == 0)
  {
    return 2;
  }
  if (std::strcmp(status, "OBS") == 0)
  {
    return 1;
  }
  return 0;
};

auto linearSolveSeverityFromStats =
[&](const LinearSolveStats& stats) -> label
{
  return linearSolveSeverity(classifyLinearSolveStatus(stats));
};

LinearSolveStats ULinearSolveStats;
LinearSolveStats pLinearSolveStats;
LinearSolveStats TLinearSolveStats;
LinearSolveStats TbLinearSolveStats;
LinearSolveStats UbLinearSolveStats;
LinearSolveStats pbLinearSolveStats;
LinearSolveStats UaLinearSolveStats;
LinearSolveStats paLinearSolveStats;
LinearSolveStats xpFilterLinearSolveStats;
LinearSolveStats fsensFilterLinearSolveStats;
LinearSolveStats gsensPowerFilterLinearSolveStats;
LinearSolveStats gsensVolFilterLinearSolveStats;

int i=0,opt=0;
double del=0;
scalar n(0),N(0),eta0(0),eta5(0.5),eta1(1),y0(0),y5(0);
n=x.size();//n:length of x in the i-th core
N=x.size();//N:design freedom
reduce(N, sumOp<scalar>()); 

scalar V(0),area=gSum(mesh.V()),MeanT(0),PowerDiss(0),PowerDissActual(0),PDActLog(0),PowerDissActualOverBase(0),maxT(0),sumT(0);
scalar objectiveValueCurrent(0);
scalar ksObjectiveValue(0);
scalar varianceObjectiveValue(0);
scalar previousObjectiveValue(0);
bool hasPreviousObjectiveValue(false);
bool stopOptimization(false);
label optimizerConvergedStreak(0);
word optimizerStopReason("running");
word activeObjectiveLabel("MeanT");
word activeWallDistanceMode("meshWave");
word activeTurbulenceModelMode("kEpsilon");
double len=std::pow(area/N, 1.0/3);
b.value()=1.0/(filterR*len/3.464)/(filterR*len/3.464);
del=beta0;
//***********************************//MMA
int m=2;//number of constraint
MMA mma(n,m);
mma.asyminit = mma_init;
mma.asymdec = mma_dec;
mma.asyminc = mma_inc;
mma.raa0 = raa0;
mma.movelimit = movlim;
std::vector<double> xmma(n);
std::vector<double> dfdx(n);
std::vector<double> gx(m);
std::vector<std::vector<double>> dgdx(m);
for (int i = 0; i < m; i++)
{
  dgdx[i].resize(n);
}
