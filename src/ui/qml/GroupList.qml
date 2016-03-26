import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import im.ricochet 1.0

ScrollView {
    id: scroll

    data: [
        GroupsModel {
            id: groupsModel
            manager: groupManager
        }
    ]
    property QtObject selectedGroup
    property ListView view: groupListView

    // Emitted for double click on group
    signal groupActivated(Group group, Item actions)

    onSelectedGroupChanged: {
        if (selectedGroup !== groupsModel.group(groupListView.currentIndex)) {
            groupListView.currentIndex = groupsModel.rowOfGroup(selectedGroup)
        }
    }

    ListView {
        id: groupListView
        model: groupsModel
        currentIndex: -1

        signal groupActivated(Group group, Item actions)
        onGroupActivated: scroll.groupActivated(group, actions)
        onCurrentIndexChanged: {
            // Not using a binding to allow writes to selectedGroup
            scroll.selectedGroup = groupsModel.group(groupListView.currentIndex)
        }
        data: [
            MouseArea {
                anchors.fill: parent
                z: -100
                onClicked: groupListView.currentIndex = -1
            }

        ]
        delegate: GroupListDelegate { }
    }
}
