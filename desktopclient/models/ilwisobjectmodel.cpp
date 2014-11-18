#include "ilwisobjectmodel.h"
#include "coverage.h"
#include "projection.h"
#include "ellipsoid.h"
#include "geodeticdatum.h"
#include "conventionalcoordinatesystem.h"
#include "featurecoverage.h"
#include "feature.h"
#include "table.h"
#include "raster.h"


using namespace Ilwis;

IlwisObjectModel::IlwisObjectModel()
{
}

IlwisObjectModel::IlwisObjectModel(const Ilwis::Resource &source, QObject *parent) : ResourceModel(source, parent)
{
    _ilwisobject.prepare(resource());
}

QString IlwisObjectModel::creationDate() const
{
    if ( _ilwisobject.isValid()){
        Time creationTime = _ilwisobject->createTime();
        if ( creationTime.isValid())
            return creationTime.toString();
    }

    return "";
}

QString IlwisObjectModel::modifiedDate() const
{
    if ( _ilwisobject.isValid()){
        Time modifiedTime = _ilwisobject->modifiedTime();
        if ( modifiedTime.isValid())
            return modifiedTime.toString();
    }

    return "";
}

bool IlwisObjectModel::readonly() const
{
    if ( _ilwisobject.isValid()){
        return _ilwisobject->isReadOnly();
    }
    return true;
}

void IlwisObjectModel::readonly(bool yesno) const
{
    if ( _ilwisobject.isValid()){
        _ilwisobject->readOnly(yesno);
    }
}

QString IlwisObjectModel::description() const
{
    if ( _ilwisobject.isValid()){
        QString desc = _ilwisobject->description();
        if ( desc != sUNDEF)
            return desc;
    }
    return "";
}

QString IlwisObjectModel::externalFormat() const
{
    if ( _ilwisobject.isValid()){
        return _ilwisobject->externalFormat();
    }
    return "";
}

bool IlwisObjectModel::externalReadOnly() const
{
   if ( _ilwisobject.isValid()){
       return _ilwisobject->outputConnectionReadonly();
   }
   return true;
}

void IlwisObjectModel::description(const QString &desc) const
{
    if ( _ilwisobject.isValid())
        _ilwisobject->setDescription(desc);
}

bool IlwisObjectModel::isProjectedCoordinateSystem() const
{
    try {
        if ( hasType(_ilwisobject->ilwisType(), itCOVERAGE | itCOORDSYSTEM)){
            ICoordinateSystem csy = hasType(_ilwisobject->ilwisType(), itCOVERAGE) ? _ilwisobject.as<Coverage>()->coordinateSystem() : _ilwisobject.as<CoordinateSystem>();
            if ( csy.isValid()){
                if ( csy->isLatLon())
                    return false;

                if (csy->ilwisType() != itCONVENTIONALCOORDSYSTEM)
                    return false;

                return true;
            }
        }
        return false;
    }catch(const ErrorObject& ){
        // no exceptions may escape here
    }
    return "";
}

QString IlwisObjectModel::projectionInfo() const
{
    try {
        if ( isProjectedCoordinateSystem()){
            IConventionalCoordinateSystem csy = hasType(_ilwisobject->ilwisType(), itCOVERAGE) ? _ilwisobject.as<Coverage>()->coordinateSystem().as<ConventionalCoordinateSystem>()
                                                                                               : _ilwisobject.as<ConventionalCoordinateSystem>();
            QString projection = Projection::projectionCode2Name(csy->projection()->code());
            QVariant var = csy->projection()->parameter(Projection::pvZONE);
            if ( var.isValid() && !var.toInt() == 1){
                projection += " zone=" + var.toString();
            }
            QString ellipsoid = Ellipsoid::ellipsoidCode2Name(csy->ellipsoid()->name());
            if ( ellipsoid == sUNDEF){
                QString wkt = csy->ellipsoid()->wktShort();
                if ( wkt != sUNDEF && wkt != "")
                    ellipsoid = wkt;
                else {
                    QString authority = csy->ellipsoid()->authority();
                    if ( wkt != sUNDEF && wkt != "")
                        ellipsoid = authority;
                    else
                        ellipsoid = QString("custom(%1,%2)").arg(csy->ellipsoid()->majorAxis(),0,'f',3).arg(csy->ellipsoid()->flattening(),0,'f',6);
                }
            }
            return projection + "; ellipsoid=" + ellipsoid;
        }
        return "";
    } catch(const ErrorObject& ){
        // no exceptions may escape here
    }
    return "";
}

QQmlListProperty<AttributeModel> IlwisObjectModel::attributes()
{
    try {
        if ( _attributes.size() == 0){
            if ( _ilwisobject.isValid())    {
                IlwisTypes objecttype = _ilwisobject->ilwisType();
                if ( objecttype == itRASTER){
                    IRasterCoverage raster = _ilwisobject.as<RasterCoverage>();
                    if ( raster->hasAttributes()){
                        for(int i = 0; i < raster->attributeTable()->columnCount(); ++i){
                            AttributeModel *attribute = new AttributeModel(raster->attributeTable()->columndefinition(i), this);
                            _attributes.push_back(attribute);
                        }
                    }
                } else if ( hasType(objecttype,itFEATURE)){
                    IFeatureCoverage features = _ilwisobject.as<FeatureCoverage>();
                    for(int i = 0; i < features->attributeDefinitions().definitionCount(); ++i){
                        AttributeModel *attribute = new AttributeModel(features->attributeDefinitions().columndefinition(i), this);
                        _attributes.push_back(attribute);
                    }
                } else if ( hasType(objecttype,itTABLE)){
                    ITable tbl = _ilwisobject.as<Table>();
                    for(int i = 0; i < tbl->columnCount(); ++i){
                        AttributeModel *attribute = new AttributeModel(tbl->columndefinition(i), this);
                        _attributes.push_back(attribute);
                    }
                }
            }
            if ( _attributes.size() > 0){
                return QQmlListProperty<AttributeModel>(this, _attributes) ;
            }
        }
    }
    catch(const ErrorObject& ){
        // no exceptions may escape here
    }
    return QQmlListProperty<AttributeModel>();

}

QString IlwisObjectModel::valuetype() const
{
    try{
        if ( !_ilwisobject.isValid())
            return "";

        IlwisTypes objectype = _ilwisobject->ilwisType();
        IlwisTypes valueType = itUNKNOWN;
        if ( hasType( objectype, itCOVERAGE|itDOMAIN)){
            if ( objectype == itRASTER){
                IRasterCoverage raster = _ilwisobject.as<RasterCoverage>();
                valueType = raster->datadef().domain()->valueType();
            } else if ( hasType( objectype , itFEATURE)){
                IFeatureCoverage features = _ilwisobject.as<FeatureCoverage>();
                ColumnDefinition coldef = features->attributeDefinitions().columndefinition(COVERAGEKEYCOLUMN);
                if ( coldef.isValid()){
                    valueType = coldef.datadef().domain()->valueType();
                }

            } else if ( hasType( objectype , itDOMAIN)){
                IDomain dom = _ilwisobject.as<Domain>();
                valueType = dom->valueType();

            }
        }
        QString typeName =  TypeHelper::type2HumanReadable(valueType);

        return typeName == sUNDEF ? "" : typeName;
    }catch(const ErrorObject& ){
        // no exceptions may escape here
    }
    return "";
}

QString IlwisObjectModel::rangeDefinition(bool defaultRange) {
    try {
        IlwisTypes objectype = _ilwisobject->ilwisType();
        QString rangeString;
        if ( hasType( objectype, itCOVERAGE|itDOMAIN)){
            if ( objectype == itRASTER){
                IRasterCoverage raster = _ilwisobject.as<RasterCoverage>();
                rangeString = defaultRange ? raster->datadef().domain()->range()->toString() : raster->datadef().range()->toString();
            } else if ( hasType( objectype , itFEATURE)){
                IFeatureCoverage features = _ilwisobject.as<FeatureCoverage>();
                ColumnDefinition coldef = features->attributeDefinitions().columndefinition(COVERAGEKEYCOLUMN);
                if ( coldef.isValid()){
                    rangeString = defaultRange ? coldef.datadef().domain()->range()->toString() : coldef.datadef().range()->toString();
                }

            } else if ( hasType( objectype , itDOMAIN)){
                rangeString = _ilwisobject.as<Domain>()->range()->toString();
            }
        }
        return rangeString;
    }catch(const ErrorObject& ){
        // no exceptions may escape here
    }
    return "";

}

QString IlwisObjectModel::pixSizeString() const{
    if (hasType(_ilwisobject->ilwisType(), itRASTER | itGEOREF)){
        double pixsizex=rUNDEF, pixsizey=rUNDEF;
        if ( hasType(_ilwisobject->ilwisType(), itRASTER)){
            Size<> sz = _ilwisobject.as<RasterCoverage>()->size();
            Envelope envelope = _ilwisobject.as<RasterCoverage>()->envelope();
            if ( envelope.isValid()){
                pixsizex = envelope.xlength() / sz.xsize();
                pixsizey = envelope.ylength() / sz.ysize();
                if ( _ilwisobject.as<RasterCoverage>()->coordinateSystem()->isLatLon()){
                    pixsizex *= 360.0;
                    pixsizey *= 360.0;
                }
            }
        }
        if ( hasType(_ilwisobject->ilwisType(), itGEOREF)){
            double pixsz = _ilwisobject.as<GeoReference>()->pixelSize();
            if ( _ilwisobject.as<GeoReference>()->coordinateSystem()->isLatLon()){
                pixsz *= 360.0;
            }
            pixsizex = pixsizey = pixsz;
        }
        auto pixszstring = [&](double pixsize) ->QString{
            if ( pixsize > 50){
                return QString::number((int)pixsize);
            }else {
                return QString("%1").arg(pixsize,0,'f',1);
            }
        };
        return pixszstring(pixsizex) + ' x ' + pixszstring(pixsizey);

    }
    return "";
}

QString IlwisObjectModel::centerPixelLocation() const{
    Envelope envLatLon(Coordinate(-180,-61), Coordinate(180,81));
    QString code = QString("code=georef:type=corners,csy=epsg:4326,envelope=%1,gridsize=301 120,name=templocal").arg(envLatLon.toString());
    IGeoReference grf(code);
    if ( grf.isValid()){
        Envelope envelope =hasType(_ilwisobject->ilwisType(), itCOVERAGE) ? _ilwisobject.as<Coverage>()->envelope(true) : _ilwisobject.as<CoordinateSystem>()->envelope(true);
        if ( !envelope.isValid())
            return "";

        Coordinate crdCenter = envelope.center();
        Pixel pixCenter = grf->coord2Pixel(crdCenter);
        if ( pixCenter.isValid())
            return QString::number(pixCenter.x) + " " + QString::number(pixCenter.y);
    }
    return "";
}

QString IlwisObjectModel::getProperty(const QString &propertyname)
{
    try{
        QString property = ResourceModel::getProperty(propertyname);
        if ( property != sUNDEF)
            return property;
        if ( !_ilwisobject.isValid())
            return "";
        if ( propertyname == "latlonenvelope" || propertyname == "envelope"){
            if (hasType(_ilwisobject->ilwisType(), itCOVERAGE)){
                return _ilwisobject.as<Coverage>()->envelope(propertyname == "latlonenvelope").toString();
            } if ( hasType(_ilwisobject->ilwisType(), itCOORDSYSTEM)){
                return _ilwisobject.as<CoordinateSystem>()->envelope(propertyname == "latlonenvelope").toString();
            }
        }
        if ( propertyname == "pixelsize"){
            return pixSizeString();
        }
        if ( propertyname == "centerpixelboundingbox"){
            return centerPixelLocation();
        }
        return "";
    } catch(const ErrorObject& ){
        // no exceptions may escape here
    }
    return "";
}

bool IlwisObjectModel::isValid() const
{
    return _ilwisobject.isValid();
}
