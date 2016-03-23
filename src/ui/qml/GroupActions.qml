import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Dialogs 1.1
import "GroupWindow.js" as GroupWindow

Item {
    id: groupMenu

    property QtObject group

    function openWindow() {
        var window = GroupWindow.getWindow(group)
        window.raise()
        window.requestActivate()
    }

    function removeGroup() {
        removeGroupDialog.active = true
        if (removeGroupDialog.item !== null) {
            removeGroupDialog.item.open()
        } else if (uiMain.showRemoveGroupDialog(group)) {
            group.deleteGroup()
        }
    }
    function openContextMenu() {
        contextMenu.popup()
    }
    function openPreferences() {
        root.openPreferences("GroupPreferences.qml", { 'selectedGroup': group})
    }

    signal renameTriggered

    Menu {
        id: contextMenu
        MenuItem {
            text: qsTr("Open Window")
            onTriggered: openWindow()
        }
        MenuItem {
            text: qsTr("Details...")
            onTriggered: openPreferences()
        }
        MenuItem {
            text: qsTr("Rename")
            onTriggered: renameTriggered()
        }
        MenuSeparator { }
        MenuItem {
            text: qsTr("Remove")
            onTriggered: removeGroup()
        }
    }

    Loader {
        id: removeGroupDialog
        source: "MessageDialogWrapper.qml"
        active: false
    }
}
