import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.1
import RepresentationSetter 1.0
import UIContextModel 1.0
import VisualizationManager 1.0
import "../../Global.js" as Global

Rectangle {
    width: parent.width
    height: 200
    property RepresentationSetter representation
    Loader{
        id : rprNumeric

    }
    Loader{
        id : rprThematic
    }

    Component.onCompleted: {
        representation = displayOptions.manager.layer(layertools.currentIndex).propertyEditor(editorName)
        if ( representation.activeValueType === "number"){
            rprNumeric.source = "NumericRepresentation.qml"
        }
    }
}