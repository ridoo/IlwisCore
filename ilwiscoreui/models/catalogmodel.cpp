#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QQmlContext>
#include <QThread>
#include <QCoreApplication>
#include "coverage.h"
#include "representation.h"
#include "connectorinterface.h"
#include "resource.h"
#include "geometries.h"
#include "georeference.h"
#include "mastercatalog.h"
#include "catalogview.h"
#include "resourcemodel.h"
#include "catalogmodel.h"
#include "tranquilizer.h"
#include "layermanager.h"
#include "ilwiscontext.h"
#include "oshelper.h"


using namespace Ilwis;
//using namespace Desktopclient;

CatalogModel::~CatalogModel()
{

}

CatalogModel::CatalogModel(QObject *parent) : ResourceModel(Resource(), parent)
{
    _initNode = true;
    _isScanned = false;
    _level = 0;
}

CatalogModel::CatalogModel(const Resource &res, QObject *parent) : ResourceModel(res,parent)
{
    _initNode = true;
    _isScanned = false;
    _level = 0;
    if ( res.url().toString() == Catalog::DEFAULT_WORKSPACE){
        _view = CatalogView(context()->workingCatalog()->source());
        setDisplayName("default");
    }else{
        _view = CatalogView(res);
        _displayName = _view.name();
    }
}

CatalogModel::CatalogModel(quint64 id, QObject *parent) : ResourceModel(mastercatalog()->id2Resource(id), parent)
{
    _initNode = true;
    _isScanned = false;
    _level = 0;
    if ( item().url().toString() == Catalog::DEFAULT_WORKSPACE){
        _view = CatalogView(context()->workingCatalog()->source());
        setDisplayName("default");
    }else{
        _view = CatalogView(item());
        _displayName = _view.name();
    }
}

void CatalogModel::setView(const CatalogView &view, bool threading){
    _view = view;
    resource(view.resource());
    bool inmainThread = QThread::currentThread() == QCoreApplication::instance()->thread();
    bool useThread = threading && inmainThread;
    if ( useThread){
        if ( !mastercatalog()->knownCatalogContent(OSHelper::neutralizeFileName(view.resource().url().toString()))){
            QThread* thread = new QThread;
            CatalogWorker2* worker = new CatalogWorker2(view.resource().url());
            worker->moveToThread(thread);
            thread->connect(thread, &QThread::started, worker, &CatalogWorker2::process);
            thread->connect(worker, &CatalogWorker2::finished, thread, &QThread::quit);
            thread->connect(worker, &CatalogWorker2::finished, worker, &CatalogWorker2::deleteLater);
            thread->connect(thread, &QThread::finished, thread, &QThread::deleteLater);
            thread->connect(worker, &CatalogWorker2::updateContainer, this, &CatalogModel::updateContainer);
            thread->start();
        }
    }else
        mastercatalog()->addContainer(view.resource().url());
    _displayName = view.resource().name();
    if ( _displayName == sUNDEF)
        _displayName = view.resource().url().toString();
}

CatalogView CatalogModel::view() const
{
    return _view;
}

bool CatalogModel::isScanned() const
{
    return _isScanned;
}

bool CatalogModel::initNode() const {
    return _initNode;
}

int CatalogModel::level() const
{
    return _level;
}

QQmlListProperty<ResourceModel> CatalogModel::resources() {

    try{
        gatherItems();

        _objectCounts.clear();
        for(auto *resource : _currentItems){
            _objectCounts[resource->type()]+= 1;
        }


        return  QQmlListProperty<ResourceModel>(this, _currentItems);
    }
    catch(const ErrorObject& err){

    }
    return  QQmlListProperty<ResourceModel>();
}

QQmlListProperty<CatalogMapItem> CatalogModel::mapItems()
{
   return  QQmlListProperty<CatalogMapItem>(this, _catalogMapItems);
}

void CatalogModel::makeParent(QObject *obj)
{
    setParent(obj);
}

void CatalogModel::filterChanged(const QString& typeIndication, bool state){
    QString objectType = typeIndication;
    bool exclusive = objectType.indexOf("|exclusive") != -1;
    if ( exclusive)
        objectType = objectType.split("|")[0];
    if ( objectType == "all" || exclusive){
        _filterState["rastercoverage"] = exclusive ? false : state;
        _filterState["featurecoverage"] = exclusive ? false :state;
        _filterState["table"] = exclusive ? false :state;
        _filterState["coordinatesystem"] = exclusive ? false :state;
        _filterState["georeference"] = exclusive ? false :state;
        _filterState["domain"] = exclusive ? false :state;
        _filterState["representation"] = exclusive ? false :state;
        _filterState["projection"] = exclusive ? false :state;
        _filterState["ellipsoid"] = exclusive ? false :state;
    }else
        _filterState[objectType] = state;

    QString filterString;
    if ( exclusive){
        if ( state)
            filterString = QString("type") + "& '" + objectType.toLower() + "' !=0";
    } else {
        for(auto iter : _filterState){
            if ( !iter.second){
                if ( filterString != "")
                    filterString += " and ";
                filterString += QString("type") + "& '" + iter.first + "' =0";
            }
        }
    }
    filter(filterString);
}

void CatalogModel::filter(const QString &filterString)
{
    if ( _view.filter() == filterString)
        return;

    _refresh = true;
    _view.filter(filterString);
    contentChanged();
}

void CatalogModel::refresh(bool yesno)
{
    _refresh = yesno;
}


QStringList CatalogModel::objectCounts()
{
    try{
        gatherItems();

        _objectCounts.clear();
        for(auto *resource : _currentItems){
            _objectCounts[resource->type()]+= 1;
        }
        QStringList counts;
        for(auto item : _objectCounts)    {
            QString txt = Ilwis::TypeHelper::type2HumanReadable(item.first) + "|" + QString::number(item.second);
            counts.push_back(txt);
        }
        return counts;
    }
    catch(const Ilwis::ErrorObject& err){
    }

    return QStringList();
}

void CatalogModel::nameFilter(const QString &filter)
{
    if ( _nameFilter == filter)
        return;

    _nameFilter = filter;
    _currentItems.clear();
    emit contentChanged();
}

QString CatalogModel::nameFilter() const
{
    return _nameFilter;
}

void CatalogModel::prepareMapItems(LayerManager *manager, bool force)
{
    try{
        if ( force){
            _catalogMapItems.clear();
            _refresh = true;
            gatherItems();
        }
        std::map<qint64, std::vector<Resource>> hashes;
        if ( _catalogMapItems.size() == 0){
            kernel()->issues()->silent(true);
            for (auto iter  = _currentItems.begin(); iter != _currentItems.end(); ++iter){
                if(hasType((*iter)->type(), itCOVERAGE)){
                    Ilwis::Resource res =(*iter)->resource();
                    if ( res.isValid() && res.hasProperty("latlonenvelope"))    {
                        Envelope env = res["latlonenvelope"].toString();
                        double hash = env.min_corner().x + env.max_corner().x + env.min_corner().y + env.max_corner().y + env.area();
                        hash = hash * 1e9;
                        hashes[(qint64)hash].push_back(res);
                       //

                    }
                }
            }
            for(auto& lst : hashes ){
                    _catalogMapItems.push_back(new CatalogMapItem({lst.second},manager->screenGrf(),this));
            }
            kernel()->issues()->silent(false);
        }
    } catch (const Ilwis::ErrorObject& ){

    } catch (std::exception& ex){
        Ilwis::kernel()->issues()->log(ex.what());
    }
    kernel()->issues()->silent(false);
}

void CatalogModel::gatherItems() {
    if ( _currentItems.isEmpty() || _refresh) {
        if ( !_view.isValid())
            return;

        _view.prepare();

        _currentItems.clear();
        _refresh = false;

        std::vector<Resource> items = _view.items();
        for(const Resource& resource : items){
            if ( _nameFilter != ""){
                if ( resource.name().indexOf(_nameFilter) == -1){
                    if ( resource["longname"].toString().indexOf(_nameFilter) == -1)
                        continue;
                }
            }
            _currentItems.push_back(new ResourceModel(resource, this));

        }
        if ( _view.hasParent()){
            _currentItems.push_front(new ResourceModel(Resource(_view.resource().url().toString() + "/..", itCATALOG), this));
        }
    }
}

void CatalogModel::updateContainer()
{
    _refresh = true;
    emit contentChanged();
}
 //-------------------------------------------------
CatalogWorker2::CatalogWorker2(const QUrl& url) : _container(url)
{
}

void CatalogWorker2::process(){
    try {
            mastercatalog()->addContainer(_container);
            emit updateContainer();
            emit finished();
    } catch(const ErrorObject& ){

    } catch ( const std::exception& ex){
        kernel()->issues()->log(ex.what());
    }

    emit finished();
}





