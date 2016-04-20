import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import im.ricochet 1.0

ScrollView {
    id: groupScroll

    data: [
        Rectangle {
            anchors.fill: groupScroll
            z: -1
            color: palette.base
        },
        GroupsModel {
            id: groupsModel
            manager: groupsManager
        }
    ]

    property QtObject selectedGroup
    property ListView view: groupListView

    // Emitted for double click on group
    signal groupActivated(Group group, Item actions)
    signal selectedContactChanged(ContactUser contact)

    onSelectedGroupChanged: {
        if (selectedGroup !== groupsModel.group(groupListView.currentIndex)) {
            groupListView.currentIndex = groupsModel.rowOfGroup(selectedGroup)
        }
    }
    onSelectedContactChanged: {
        groupListView.currentIndex = -1
    }

    ListView {
        id: groupListView
        model: groupsModel
        currentIndex: -1

        signal groupActivated(Group group, Item actions)
        onGroupActivated: groupScroll.groupActivated(group, actions)

        onCurrentIndexChanged: {
            // Not using a binding to allow writes to selectedGroup
            groupScroll.selectedGroup = groupsModel.group(groupListView.currentIndex)
            contactList.selectedGroupChanged(groupsModel.group(groupListView.currentIndex))
        }

        data: [
            MouseArea {
                anchors.fill: parent
                z: -100
                onClicked: groupListView.currentIndex = -1
            }

        ]
        section.property: "name"
        section.delegate: Row {
            width: parent.width - x
            height: label.height + 4
            x: 8
            spacing: 6
            Label {
                id: label
                y: 2
                font.pointSize: styleHelper.pointSize
                font.bold: true
                font.capitalization: Font.SmallCaps
                textFormat: Text.PlainText
                color: "black"
                text: "Group Chats"
            }
            Rectangle {
                height: 1
                width: parent.width - x
                anchors {
                    top: label.verticalCenter
                    topMargin: 1
                }
                color: "black"
                opacity: 0.1
            }
        }

        delegate: GroupListDelegate { }
    }
}
