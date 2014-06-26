#include <functional>
#include <future>
#include "kernel.h"
#include "raster.h"
#include "columndefinition.h"
#include "basetable.h"
#include "flattable.h"
#include "domainitem.h"
#include "itemdomain.h"
#include "identifieritem.h"
#include "identifierrange.h"
#include "symboltable.h"
#include "ilwisoperation.h"
#include "transposeraster.h"

using namespace Ilwis;
using namespace RasterOperations;

REGISTER_OPERATION(TransposeRaster)

TransposeRaster::TransposeRaster()
{
}

TransposeRaster::TransposeRaster(quint64 metaid, const Ilwis::OperationExpression &expr) : OperationImplementation(metaid, expr)
{

}

bool TransposeRaster::execute(ExecutionContext *ctx, SymbolTable &symTable)
{

}

Ilwis::OperationImplementation *TransposeRaster::create(quint64 metaid, const Ilwis::OperationExpression &expr)
{

}

Ilwis::OperationImplementation::State TransposeRaster::prepare(ExecutionContext *ctx, const SymbolTable &)
{

}

quint64 TransposeRaster::createMetadata()
{
    OperationResource operation({"ilwis://operations/transposeraster"});
    operation.setSyntax("transposeraster(inputraster,mirrhor | mirrvert | mirrdiag | transpose | rotate90 | rotate180 | rotate270)");
    operation.setDescription(TR("transpose the raster according to the method indicated by the second parameter"));
    operation.setInParameterCount({2});
    operation.addInParameter(0,itRASTER,  TR("input raster"),TR("ratser to be transposed"));
    operation.addInParameter(1,itSTRING, TR("transpose method"),TR("rotation or mirror of the input map"));
    operation.setOutParameterCount({1});
    operation.addOutParameter(0,itRASTER, TR("output raster"), TR("output raster with a new georef"));
    operation.setKeywords("raster, geometry");

    mastercatalog()->addItems({operation});
    return operation.id();
}
