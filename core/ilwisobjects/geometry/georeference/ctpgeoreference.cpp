#include <QString>

#include "kernel.h"
#include "ilwis.h"
#include "geometries.h"
#include "ilwisdata.h"
#include "coordinatesystem.h"
#include "controlpoint.h"
#include "georeference.h"
#include "ctpgeoreference.h"

using namespace Ilwis;


//-------------------------------------------------------
CTPGeoReference::CTPGeoReference() : GeoReference()
{
}

CTPGeoReference::CTPGeoReference(const Resource& res) : GeoReference(res) {

}

bool CTPGeoReference::isValid() const
{
    return true;
}

ControlPoint &CTPGeoReference::controlPoint(quint32 index)
{
    _invalidCP = ControlPoint(true);
    if ( index < _controlPoints.size())
        return _controlPoints[index];
    return _invalidCP;

}

quint32 CTPGeoReference::nrControlPoints() const
{
    return _controlPoints.size();
}

qint32 CTPGeoReference::index(const Pixel &pix) const
{
    for(qint32 i=0; i < (qint32)_controlPoints.size(); ++i) {
        Pixel cp( _controlPoints[i].gridLocation());
        if ( cp == pix)
            return i;
    }
    return iUNDEF;
}

qint32 CTPGeoReference::index(const Coordinate &crd) const
{
    for(qint32 i=0; i < (qint32)_controlPoints.size(); ++i) {
        Coordinate c( _controlPoints[i]);
        if ( c == crd)
            return i;
    }
    return iUNDEF;
}

qint32 CTPGeoReference::setControlPoint(const ControlPoint &pnt)
{
    int rec = index(Pixel(pnt.gridLocation()));
    if ( rec == iUNDEF) {
        _controlPoints.push_back(pnt);
        return _controlPoints.size() - 1;
    }
    controlPoint(rec) = pnt;
    return rec;
}

bool CTPGeoReference::compute()
{
    return true;
}

