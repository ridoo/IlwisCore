#include "kernel_global.h"
#include "ilwis.h"
#include "geos/geom/CoordinateArraySequence.h"
#include "geos/geom/Point.h"
#include "geos/geom/LineString.h"
#include "geos/geom/LinearRing.h"
#include "geos/geom/Polygon.h"
#include "geos/geom/GeometryFactory.h"
#include "geos/geom/Coordinate.h"
#include "geos/io/WKTReader.h"
#ifdef Q_OS_WIN
#include "geos/io/WKTReader.inl"
#endif
#include "geos/io/WKTWriter.h"
#include "geos/io/ParseException.h"
#include "coverage.h"
#include "coordinate.h"
#include "location.h"
#include "geometries.h"
#include "coordinatesystem.h"
#include "featurecoverage.h"
#include "feature.h"
#include "geos/geom/CoordinateFilter.h"
#include "csytransform.h"

#include "geometryhelper.h"


using namespace Ilwis;

QString GeometryHelper::toWKT(const geos::geom::Geometry* geom){
    geos::io::WKTWriter writer;
    return QString::fromStdString(writer.write(geom));
}

geos::geom::Geometry* GeometryHelper::fromWKT(const QString& wkt, const ICoordinateSystem& csy) {
    try{
        geos::io::WKTReader reader;
        geos::geom::Geometry* geom = reader.read(wkt.toStdString());
        if (geom){
            if ( csy.isValid()){
                GeometryHelper::setCoordinateSystem(geom, csy.ptr());
            }else
               GeometryHelper::setCoordinateSystem(geom,0);

            return geom;
        }
        ERROR2(ERR_NOT_COMPATIBLE2, "WKT", TR("this wkt parser"));
        return 0;
    } catch (const geos::io::ParseException& ex){
        ERROR0(ex.what());
        return 0;
    }
}

IlwisTypes GeometryHelper::geometryType(const geos::geom::Geometry *geom){
    if ( geom == 0)
        return itUNKNOWN;

    switch ( geom->getGeometryTypeId()){
    case geos::geom::GEOS_POINT:
    case geos::geom::GEOS_MULTIPOINT:
        return itPOINT;
    case geos::geom::GEOS_LINESTRING:
    case geos::geom::GEOS_MULTILINESTRING:
        return itLINE;
    case geos::geom::GEOS_POLYGON:
    case geos::geom::GEOS_MULTIPOLYGON:
	case geos::geom::GEOS_LINEARRING:		    
        return itPOLYGON;
    case geos::geom::GEOS_GEOMETRYCOLLECTION:
        return itPOLYGON|itLINE|itPOINT;
    default:
        return  itUNKNOWN;
    }
}

void GeometryHelper::transform(geos::geom::Geometry *geom, const ICoordinateSystem& source, const ICoordinateSystem& target){
    CsyTransform trans(source, target);
    geom->apply_rw(&trans);
}

Ilwis::CoordinateSystem* GeometryHelper::getCoordinateSystem(geos::geom::Geometry *geom){
    if ( geom == nullptr)
        return 0;
    void *ptr = geom->getUserData();
    if (ptr == 0)
        return 0;
    return reinterpret_cast<Ilwis::CoordinateSystem *>(ptr);
}

void GeometryHelper::setCoordinateSystem(geos::geom::Geometry* geom, Ilwis::CoordinateSystem* csy){
    geom->setUserData(csy);
}

