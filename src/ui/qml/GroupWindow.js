.pragma library

var windows = { }
var createWindow = function() { console.log("BUG!") }

function getWindow(group) {
    var id = group.uniqueID
    var window = windows[group.uniqueID]

    if (window === undefined || window === null) {
        window = createWindow(group)
        window.closed.connect(function() { windows[id] = undefined })
        windows[id] = window
    }
    return window
}

function windowExists(group) {
    return windows[group.uniqueID] !== undefined && windows[group.uniqueID] !== null
}
