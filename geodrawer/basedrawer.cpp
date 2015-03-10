#include "kernel.h"
#include "ilwisdata.h"
#include "basedrawer.h"
#include "coordinatesystem.h"
#include "rootdrawer.h"

using namespace Ilwis;
using namespace Geodrawer;

BaseDrawer::BaseDrawer(const QString& nme, DrawerInterface *parentDrawer, RootDrawer *rootdrawer, const IOOptions &) : DrawerInterface(0),Identity(nme,i64UNDEF,nme.toLower()), _rootDrawer(rootdrawer), _parentDrawer(parentDrawer)
{
    Identity::prepare();
}

void BaseDrawer::valid(bool yesno)
{
    _valid = yesno;
}

bool BaseDrawer::prepare(DrawerInterface::PreparationType prepType,  const IOOptions &)
{
    if ( code() == "RootDrawer") // rootdrawer for the moment has no need of shaders
        return true;

    if ( !rootDrawer()) // rootdrawer must be set
        return false;

    if ( hasType(prepType, ptSHADERS) && !isPrepared(ptSHADERS)){
        _shaders.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                         "attribute highp vec4 position;"
                                         "attribute mediump vec3 normal;"
                                         "uniform mat4 mvp;"
                                         "uniform vec3 scalecenter;"
                                         "uniform float scalefactor;"
                                         "uniform float alpha;"
                                         "attribute lowp vec4 vertexColor;"
                                         "varying lowp vec4 fragmentColor;"
                                         "void main() {"
                                         "    if ( scalefactor != 1) {"
                                             "    float x = scalecenter[0] + (position[0] - scalecenter[0]) * scalefactor;"
                                             "    float y = scalecenter[1] + (position[1] - scalecenter[1]) * scalefactor;"
                                             "    float z = position[2];"
                                            "    position[0] = x;"
                                            "    position[1] = y;"
                                            "    position[2] = z;"
                                            "    position[3] = 1;"
                                         "    }"
                                         "    vertexColor[3] = alpha;"
                                         "    gl_Position =  mvp * position;"
                                         "    fragmentColor = vertexColor;"
                                         "}");
        _shaders.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                         "varying lowp vec4 fragmentColor;"
                                         "void main() {"
                                         "    gl_FragColor = fragmentColor;"
                                         "}");


        if(!_shaders.link()){
            return ERROR2(QString("%1 : %2"),TR("Drawing failed"),TR(_shaders.log()));
        }
        if (!_shaders.bind()){
            return ERROR2(QString("%1 : %2"),TR("Drawing failed"),TR(_shaders.log()));
        }
        _prepared |= DrawerInterface::ptSHADERS;

        _vboPosition = _shaders.attributeLocation("position");
        _vboNormal = _shaders.attributeLocation("normal");
        _vboColor = _shaders.attributeLocation("vertexColor");
        _modelview = _shaders.uniformLocation("mvp");
        _scaleCenter = _shaders.uniformLocation("scalecenter");
        _scaleFactor = _shaders.uniformLocation("scalefactor");
        _vboAlpha = _shaders.uniformLocation("alpha");

    }


    return true;
}

void BaseDrawer::unprepare(DrawerInterface::PreparationType prepType )
{
    if ( hasType(_prepared, prepType))    {
        _prepared &= ~ prepType;
    }
}

bool BaseDrawer::isPrepared(quint32 type) const
{
    return hasType(_prepared, type);
}

bool BaseDrawer::draw(const IOOptions &) const
{
    return false;
}

RootDrawer *BaseDrawer::rootDrawer()
{
    return _rootDrawer;
}

const RootDrawer *BaseDrawer::rootDrawer() const
{
    return _rootDrawer;
}

DrawerInterface *BaseDrawer::parent()
{
    return _parentDrawer;
}

const DrawerInterface *BaseDrawer::parent() const
{
    return _parentDrawer;
}

bool BaseDrawer::isActive() const
{
    return _active;
}

void BaseDrawer::active(bool yesno)
{
    _active = yesno;
}

bool BaseDrawer::isValid() const
{
    return _valid;
}

void BaseDrawer::selected(bool yesno)
{
    _selected= yesno;
}

bool BaseDrawer::isSelected() const
{
    return _selected;
}

BaseDrawer::Containment BaseDrawer::containment() const
{
    if ( _envelope.isValid()){
        if ( rootDrawer()->zoomEnvelope().intersects(_envelope))
            return BaseDrawer::cINSIDE;
    }
    return BaseDrawer::cUNKNOWN;
}

void BaseDrawer::cleanUp()
{
    _shaders.removeAllShaders();
    unprepare(ptSHADERS);
}

void BaseDrawer::code(const QString &code)
{
    Identity::code(code);
}

QString BaseDrawer::code() const
{
    return Identity::code();
}

quint64 BaseDrawer::id() const
{
    return Identity::id();
}

QString BaseDrawer::name() const
{
    return Identity::name();
}

void BaseDrawer::name(const QString &n)
{
    Identity::name(n);
}

QString BaseDrawer::description() const
{
    return Identity::description();
}

void BaseDrawer::setDescription(const QString &desc)
{
    return Identity::setDescription(desc);
}

std::vector<QVariant> BaseDrawer::attributes(const QString &attrNames) const
{
    std::vector<QVariant> result;
    return result;
}

QVariant BaseDrawer::attribute(const QString &attrName) const
{
    if ( attrName == "alphachannel")
        return _alpha;

    return QVariant();
}

QVariant BaseDrawer::attributeOfDrawer(const QString &, const QString &) const
{
    return QVariant();
}

void BaseDrawer::setAttribute(const QString &attrName, const QVariant &value)
{
    if ( attrName == "alphachannel")
        _alpha = value.toFloat();
}

bool BaseDrawer::drawerAttribute(const QString , const QString &, const QVariant &)
{
    return false;
}

QColor BaseDrawer::color(const IRepresentation &rpr, double , DrawerInterface::ColorValueMeaning )
{
    return QColor();
}

quint32 BaseDrawer::defaultOrder() const
{
    return iUNDEF;
}

float BaseDrawer::alpha() const
{
    return _alpha;
}

void BaseDrawer::alpha(float alp)
{
    if ( alp >= 0 && alp <= 1.0)
        _alpha = alp;
}

void BaseDrawer::redraw()
{
    rootDrawer()->redraw();
}





