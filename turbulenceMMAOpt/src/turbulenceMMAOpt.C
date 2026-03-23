//Author: Yu Minghao    Updated: May 2020 
#include "fvCFD.H"
#include "singlePhaseTransportModel.H"
#include "turbulentTransportModel.H"
#include "simpleControl.H"
#include "fvOptions.H"//
#include "MMA/MMA.h"
#include <cstring>
#include <iomanip>
#include <sstream>
#include <diff.c>

int main(int argc, char *argv[])
{
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "createControl.H"
    #include "createFvOptions.H"//
    #include "createFields.H"
    #include "readTransportProperties.H" 
    #include "initContinuityErrs.H"
    #include "readThermalProperties.H" 
    #include "opt_initialization.H"
    while (simple.loop(runTime))
    {
        #include "solverConvergenceReset.H"
        #include "update.H"
        #include "wallDistanceAlpha.H"
        #include "Primal_U.H"
        #include "Primal_kEpsilon.H"
        #include "Primal_T.H"
        if (resetAdjointsEachIteration)
        {
            Ua = dimensionedVector("zero", Ua.dimensions(), vector::zero);
            Ub = dimensionedVector("zero", Ub.dimensions(), vector::zero);
            pa = dimensionedScalar("zero", pa.dimensions(), 0.0);
            pb = dimensionedScalar("zero", pb.dimensions(), 0.0);
            phia = dimensionedScalar("zero", phia.dimensions(), 0.0);
            phib = dimensionedScalar("zero", phib.dimensions(), 0.0);
            Tb = dimensionedScalar("zero", Tb.dimensions(), 0.0);

            Ua.correctBoundaryConditions();
            Ub.correctBoundaryConditions();
            pa.correctBoundaryConditions();
            pb.correctBoundaryConditions();
            phia.correctBoundaryConditions();
            phib.correctBoundaryConditions();
            Tb.correctBoundaryConditions();
        }
        #include "AdjointHeat_Tb.H"
        #include "AdjointHeat_Ub.H"
        #include "AdjointFlow_Ua.H"
        #include "costfunction.H"              
        #include "sensitivity.H"
        runTime.write();
        if (stopOptimization)
        {
            Info<< "Optimizer stop criteria met at iteration " << opt
                << " (" << optimizerStopReason << ")." << endl;
            break;
        }
    }
    #include "finalize.H"
    return 0;
}
