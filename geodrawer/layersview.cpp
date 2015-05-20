#include <QtGui/QOpenGLFramebufferObject>
#include <QtQuick/QQuickWindow>
#include "layermanager.h"
#include "coverage.h"
#include "layersview.h"
#include "layersrenderer.h"
#include "rootdrawer.h"
#include "uicontextmodel.h"

#include <QtGui/QOpenGLFramebufferObject>

#include <QtQuick/QQuickWindow>
#include <qsgsimpletexturenode.h>

quint64 LayersView::_baseViewerId = 0;

LayersView::LayersView()
{
    _viewerId = _baseViewerId++;
    uicontext()->addViewer(this, _viewerId);
}

LayersView::~LayersView()
{
    uicontext()->removeViewer(_viewerId)    ;
}

QQuickFramebufferObject::Renderer *LayersView::createRenderer() const
{
    LayersRenderer *renderer = new LayersRenderer(this);
    connect(renderer,&LayersRenderer::synchronizeDone,this,&LayersView::synchronizeEnded);
    connect(renderer,&LayersRenderer::drawDone,this,&LayersView::drawDone);
    return renderer;
}

void LayersView::addCommand(const QString &command, const QVariantMap &params)
{

}

void LayersView::setAttribute(const QString &drawercode, const QVariantMap &values)
{
    _attributeQueue.push_back(std::pair<QString, QVariantMap>(drawercode, values));

}

void LayersView::copyAttribute(const QString &drawercode, const QString &attrName)
{
    _attributerequests.push_back(std::pair<QString, QString>(drawercode, attrName));
}

QString LayersView::attributeOfDrawer(const QString &drawercode, const QString &attrName)
{
    try {
        Ilwis::Geodrawer::RootDrawer *rootdrawer = rootDrawer();
        if ( !rootdrawer)
            return "";

        QVariant var = rootdrawer->attributeOfDrawer(drawercode, attrName);
        if ( !var.isValid())
            return "";
        QString result = var.toString();
        if ( result != "")
            return result;
        QString tpName = var.typeName();
        if ( tpName == "Ilwis::Envelope"){
            Envelope env = var.value<Envelope>();
            QString result = env.toString();
            return result;
        }
        if ( tpName == "Ilwis::BoundingBox"){
            auto bb = var.value<BoundingBox>();
            return bb.toString();
        }

        return "";
    } catch ( const ErrorObject&){

    } catch ( std::exception& ex){

    }
    return "";
}

void LayersView::addCommand(const QString &expression)
{
    OperationExpression expr(expression);
    if ( expr.isValid()){
        _commands.push_back(expr);
    }
}

void LayersView::setManager(LayerManager *manager)
{
    _manager = manager;
}

QString LayersView::layerInfo(const QString& pixelpair) const
{
    try {
        if ( _manager){
            QStringList parts = pixelpair.split("|");
            if ( parts.size() == 2 && rootDrawer()){
                Ilwis::Coordinate crd = rootDrawer()->pixel2Coord(Ilwis::Pixel(parts[0].toDouble(), parts[1].toDouble()));
                return _manager->layerInfo(crd);
            }
        }
        return "";
    }
    catch(const ErrorObject& ){}
    catch(const std::exception& ex){
        kernel()->issues()->log(ex.what());
    }
    return "";
}

QVariantMap LayersView::zoomEnvelope() const
{
    QVariantMap vmap;
    Geodrawer::RootDrawer *root = rootDrawer();
    if ( root){
        _manager->setScreenGeoReference(root->screenGrf());
        Envelope zoomenv = root->zoomEnvelope();
        vmap["minx"] = zoomenv.min_corner().x;
        vmap["miny"] = zoomenv.min_corner().y;
        vmap["maxx"] = zoomenv.max_corner().x;
        vmap["maxy"] = zoomenv.max_corner().y;
    }

    return vmap;
}

QVariantMap LayersView::viewEnvelope() const
{
    QVariantMap vmap;
    Geodrawer::RootDrawer *root = rootDrawer();
    if ( root){
        _manager->setScreenGeoReference(root->screenGrf());
        Envelope viewenv = root->viewEnvelope();
        vmap["minx"] = viewenv.min_corner().x;
        vmap["miny"] = viewenv.min_corner().y;
        vmap["maxx"] = viewenv.max_corner().x;
        vmap["maxy"] = viewenv.max_corner().y;
    }

    return vmap;
}

void LayersView::setViewEnvelope(const QVariantMap &var)
{
    bool ok;
    double minx = var["minx"].toDouble(&ok);
    if (!ok)
        return;
    double miny = var["miny"].toDouble(&ok);
    if (!ok)
        return;
    double maxx = var["maxx"].toDouble(&ok);
    if (!ok)
        return;
    double maxy = var["maxy"].toDouble(&ok);
    if (!ok)
        return;

    Envelope env(Coordinate(minx,miny), Coordinate(maxx, maxy));
    Geodrawer::RootDrawer *root = rootDrawer();
    if ( root){
        root->applyEnvelopeView(env,true);
    }
    emit viewEnvelopeChanged();

}

void LayersView::setZoomEnvelope(const QVariantMap &var)
{
    bool ok;
    double minx = var["minx"].toDouble(&ok);
    if (!ok)
        return;
    double miny = var["miny"].toDouble(&ok);
    if (!ok)
        return;
    double maxx = var["maxx"].toDouble(&ok);
    if (!ok)
        return;
    double maxy = var["maxy"].toDouble(&ok);
    if (!ok)
        return;

    Envelope env(Coordinate(minx,miny), Coordinate(maxx, maxy));
    Geodrawer::RootDrawer *root = rootDrawer();
    if ( root){
        root->applyEnvelopeZoom(env);
        root->redraw();
    }
    emit zoomEnvelopeChanged();

}



void LayersView::associate(const QString &name, const QString& event)
{
    for(const auto& association : _associates){
        if ( association.first == name)
            return;
    }
    _associates.push_back(std::pair<QString, QString>(name, event));
}

void LayersView::removeAssociate(const QString &name)
{
    auto iter = std::find_if(_associates.begin(), _associates.end(),[&name](const std::pair<QString, QString>& assoc)->bool{
        return name == assoc.first;
    });
    if ( iter != _associates.end())
        _associates.erase(iter);
}

LayerManager *LayersView::layerManager()
{
    return _manager;
}

bool LayersView::showLayerInfo() const
{
    return _showLayerInfo;
}

void LayersView::setShowLayerInfo(bool yesno)
{
    _showLayerInfo = yesno;
    emit showLayerInfoChanged();
}

void LayersView::synchronizeEnded()
{
    for(const auto& iter : _associates) {
        if ( iter.second == "synchronizeEnded"){
            QString name = iter.first;
            QObject *obj = uicontext()->rootObject()->findChild<QObject *>(name);
            if (obj){
                QVariant returnedValue;
                QMetaObject::invokeMethod(obj,"updateItem",Q_RETURN_ARG(QVariant, returnedValue));
            }
        }
    }

}

void LayersView::drawDone()
{
    for(const auto& iter : _associates) {
        if ( iter.second == "drawEnded"){
            QString name = iter.first;
            QObject *obj = uicontext()->rootObject()->findChild<QObject *>(name);
            if (obj){
                QVariant returnedValue;
                QMetaObject::invokeMethod(obj,"finalizeDraw",Q_RETURN_ARG(QVariant, returnedValue));
            }
        }
    }
}

QString LayersView::viewerId() const
{
    return QString::number(_viewerId);
}

Geodrawer::RootDrawer *LayersView::rootDrawer() const
{
    if ( !_manager)
        return 0;
    CoverageLayerModel *layer = _manager->layer(1); // layer 0 is the global layer, no 'real' drawer there
    if ( !layer)
        return 0;
    return layer->drawer()->rootDrawer();
}

QString LayersView::currentCoordinate() const
{
    if ( rootDrawer() && rootDrawer()->coordinateSystem().isValid()){
        if ( rootDrawer()->coordinateSystem()->isLatLon()){
            return _currentCoordinate.toString(6);
        }
    }
    return _currentCoordinate.toString(2);
}

void LayersView::setCurrentCoordinate(const QString &var)
{
    if ( var != ""){
        QStringList parts = var.split("|");
        if ( rootDrawer() && parts.size() == 2){
            _currentCoordinate = rootDrawer()->pixel2Coord(Ilwis::Pixel(parts[0].toDouble(), parts[1].toDouble()));
            emit currentCoordinateHasChanged();
        }
    }
}

QString LayersView::currentLatLon() const
{
    if ( rootDrawer() && rootDrawer()->coordinateSystem().isValid()){
        if ( rootDrawer()->coordinateSystem()->isLatLon()){
            LatLon ll(_currentCoordinate.y, _currentCoordinate.x);
            return ll.toString();
        }
        else if ( rootDrawer()->coordinateSystem()->canConvertToLatLon())
            return rootDrawer()->coordinateSystem()->coord2latlon(_currentCoordinate).toString();
    }
    return "";
}


