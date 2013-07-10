#ifndef KERNEL_H
#define KERNEL_H

#include <QSqlDatabase>
#include <QCache>
#include <QFileInfo>
#include <QThreadStorage>
#include <QMutex>
#include <QMutexLocker>
#include <QStringList>
#include <QUrl>
#include <fstream>
#include "boost/current_function.hpp"
#include <time.h>
#include "Kernel_global.h"
#include "ilwis.h"
#include "issuelogger.h"
#include "module.h"
#include "publicdatabase.h"
#include "ilwisobject.h"
#include "factory.h"

namespace Ilwis {
// forwards
class Version;
class FactoryInterface;
class IssueLogger;
class Resource;

typedef QScopedPointer<Version> SPVersion;

/*!
 The Kernel class a singleton object that controls some essential resources in the system.
 - thread local variables. The kernel is registration place for all variables that are globally available in a thread. e.g. the workingcatalog.
 - internal database. The program maintains a number of tables that are available through what is called the publicdatabase
 - issues. errors, warnings and remarks are stored in the issuelogger(a singleton). The issuelogger is maintained by the kernel
 - masterfactory. the kernel is the place were the factory of factories (masterfactory) resides
 - operations. Though operations are executed in the modules, the publication of the operations takes place through the kernel
 */
class KERNELSHARED_EXPORT Kernel : public QObject
{
    Q_OBJECT
public:
    /*!
     Constructor. The kernel is constructed when the kernel() method, a global method , is called.
     * \param parent
     */
    explicit Kernel(QObject *parent = 0);

    ~Kernel();
     /*!
     *  Stores a thread local variable under a certain key in the thread storage
     * \param key name of the variable that is stored
     * \param data pointer to the data to be stored
     */
    void setTLS(const QString& key, QVariant* data);
    /*!
     *  deleteTLS deletes the memory of the thread local variable
     * \param key name of the variable to be deleted
     */
    void deleteTLS(const QString& key);
    /*!
     *  Returns the (pointer to) data of a thread local variable. The method will return a const reference because the client using this method may never delete this pointer. It is controlled by the TLS
     *
     * \param key name of the variable
     * \return const reference to the variable.
     */
    const QVariant *getFromTLS(const QString& key) const;
    /*!
      *  Adds a factory to the masterfactory. The masterfactory holds all the factories in the system but it is not visible to the outside world.
      * \param fac the factory to be added
      */
     void addFactory(FactoryInterface *fac);

    /*!
     *  Translates a string to local language. The default language is english and this will have a null translation
     * \param s
     * \return
     */
    QString translate(const QString& s) const;
    /*!
     *  Returns the version object of this ilwis instance. The version object contains all info about versioning of the kernel and its modules
     * \return  const pointer to the version object. The object can not be changed by clients of the kernel
     */
    const SPVersion &version() const;

    static Kernel *_kernel;
    /*!
     *  Initializes the resources maintained by the kernel
     */
    void init();
    /*!
     *  Returns a reference to the publicdatabase
     * \return
     */
    PublicDatabase &database();
    /*!
     *  issues returns a reference to the issuelogger. In the software the issuelogger is used to store all kinds of info about what is happening. This method exposes the logger to add/retrieve information.
     *
     * \return issuelogger
     */
    QScopedPointer<IssueLogger> &issues();
    /*!
     *  Adds an operation to the commandhandler. The commandhandler is the internal object that handles commands but it is not visible to the outside clients
     * \param info the structure containing the information about the new command
     */
    QString demangle(const char *mangled_name);
    /*!
     *  Convenience method that shortens the code to put errors in the issuelogger. The method is mainly used by the ERROR macros to pass messages to the logger
     *
     * \param message the actual error message
     * \param p1 (optional) error specific fill value
     * \param p2 (optional) error specific fill value
     * \return always return false. means the macros can be used directly in the return statement of the place were the error occurred. At these places usually a false is returned
     */
    bool error(const QString& message, const QString p1, const QString p2, const QString p3, const QString& file, int line, const QString& func);

    /*!
     * Returns a const reference to a factory of a certain combination of type and subtype.
     * \param type main typename of the factory. e.g. IlwisObjectFactory or CatalogFactory
     * \param subtype optional further specialization of the factory type. usualy for factories that are contained in a module  were the subtype identifies the module the. It is flexible though and could also be used in other combinations
     */
    template<class T> const T *factory(const QString& type, const QString& subtype="") const {
        QString key = subtype != "" ? QString(subtype + "::" + type).toLower() : type.toLower();
        QHash<QString, FactoryInterface *>::const_iterator iter = _masterfactory.find(key) ;
        if ( iter != _masterfactory.end())
            return dynamic_cast<T *>(iter.value());
        return 0;
    }

    /*!
     * Returns a to a factory of a certain combination of type and subtype
     * \param type main typename of the factory. e.g. IlwisObjectFactory or CatalogFactory
     * \param subtype optional further specialization of the factory type. usualy for factories that are contained in a module  were the subtype identifies the module the. It is flexible though and could also be used in other combinations
     * \return an instanciated factory or 0. In the case of 0 the issuelogger will log the error
     */
    template<class T> T *factory(const QString& type, const QString& subtype="") {
        QString key = subtype != "" ? QString(subtype + "::" + type).toLower() : type.toLower();
        QHash<QString, FactoryInterface *>::const_iterator iter = _masterfactory.find(key) ;
        if ( iter != _masterfactory.end())
            return dynamic_cast<T *>(iter.value());
        //this->_issues->log(TR(QString(ERR_NO_INITIALIZED_1).arg(kind)));
        return 0;
    }

    /*!
     * Returns a facotry for a certain kind of objects and resource. The method tries to find a factory of a certain kind that can handle that type of resource. It will query the known factories through the canUse method if it can handle the resource. If so it will return the factory
     * \return an instanciated factory or 0. In the case of 0 the issuelogger will log the error
     */
    template<class T> const T *factory(const QString& kind, const Resource& item) const {
        QString type = kind.toLower();
        for(QHash<QString, FactoryInterface *>::const_iterator iter = _masterfactory.begin(); iter != _masterfactory.end(); ++iter) {
            QStringList slist = iter.key().split("::");
            bool found = false;
            if ( slist.size() > 0) {
                QString back = slist.back();
                found =  back == type;
            }
            if (found ) {
                if ( iter.value()->canUse(item))
                    return dynamic_cast<T *>(iter.value());
            }
        }
        return 0;
    }
    //for debugging
    void startClock();
    void endClock();

private:
    QThreadStorage<QCache<QString, QVariant> *> _caches;
    ModuleMap _modules;
    SPVersion _version;
    PublicDatabase _dbPublic;
    QScopedPointer<IssueLogger> _issues;
    QHash<QString, FactoryInterface * > _masterfactory;
    mutable clock_t _start_clock;


signals:
    void doCommand(const QString& expr, ExecutionContext* ctx);

public slots:

};


}

KERNELSHARED_EXPORT Ilwis::Kernel* kernel();
#define ERROR0(m) ( kernel()->error(m,sUNDEF, sUNDEF, sUNDEF, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION))
#define ERROR1(m, p) ( kernel()->error(m, p, sUNDEF, sUNDEF, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION))
#define ERROR2(m, p1, p2) ( kernel()->error(m, p1, p2, sUNDEF, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION))
#define ERROR3(m, p1, p2, p3) ( kernel()->error(m,p1, p2, p3, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION))

typedef QList<IlwisTypes> IlwTypeList;



#endif // KERNEL_H
