import QtQuick 2.0

Item {
        id : abstractBasicModellerObject

        /*
         * Coordinates represent the center of the object
        */
       function setCoordinates(newX, newY) {
           x = newX;
           y = newY;
       }

       function getInputConnector() {
           return Qt.point(x, y);
       }

       function getOutputConnector() {
           return Qt.point(x, y);
       }
}
