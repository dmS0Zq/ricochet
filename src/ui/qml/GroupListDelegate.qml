import QtQuick 2.0
import QtQuick.Controls 1.0
import im.ricochet 1.0

Rectangle {
    id: delegate
    color: highlighted ? "#c4e7ff" : "white"
    width: parent.width
    height: nameLabel.height + 8

    property bool highlighted: ListView.isCurrentItem
    onHighlightedChanged: {
        if (renameMode)
            renameMode = false
    }

    Label {
        id: nameLabel
        anchors {
            left: parent.left
            right: parent.right
            leftMargin: 6
            rightMargin: 8
            verticalCenter: parent.verticalCenter
        }
        text: model.name
        textFormat: Text.PlainText
        elide: Text.ElideRight
        font.pointSize: styleHelper.pointSize
        color: "black"
    }

    GroupActions {
        id: contextMenu
        group: model.group

        onRenameTriggered: renameMode = true
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: {
            if (!delegate.ListView.isCurrentItem)
                groupListView.currentIndex = model.index
        }
        onClicked: {
            if (mouse.button === Qt.RightButton) {
                contextMenu.openContextMenu()
            }
        }
        onDoubleClicked: {
            if (mouse.button === Qt.LeftButton) {
                groupListView.groupActivated(model.group, contextMenu)
            }
            groupsModel.testSendMessage(model.group)
        }
    }

    property bool renameMode
    property Item renameItem
    onRenameModeChanged: {
        if (renameMode && renameItem === null) {
            renameItem = renameComponent.createObject(delegate)
            renameItem.forceActiveFocus()
            renameItem.selectAll()
        } else if (!renameMode && renameItem !== null) {
            renameItem.visible = false
            renameItem.destroy()
            renameItem = null
        }
    }

    Component {
        id: renameComponent

        TextField {
            id: nameField
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: nameLabel.verticalCenter
            }
            text: model.group.name
            onAccepted: {
                model.group.name = text
                delegate.renameMode = false
            }
        }
    }
}
