import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    width: 1360
    height: 860
    minimumWidth: 1050
    minimumHeight: 700
    visible: true
    title: "Phase Correlation Tester"
    color: "#101318"
    font.pixelSize: 15

    property color panel: "#181d24"
    property color panel2: "#212832"
    property color panel3: "#2a3340"
    property color field: "#0c1015"
    property color border: "#3b4655"
    property color borderStrong: "#59697c"
    property color textMain: "#f4f7fb"
    property color textMuted: "#aeb9c8"
    property color accent: "#78b7ff"
    property color accentSoft: "#1f78b7ff"

    palette.window: panel
    palette.windowText: textMain
    palette.base: field
    palette.alternateBase: panel2
    palette.text: textMain
    palette.button: panel2
    palette.buttonText: textMain
    palette.highlight: accent
    palette.highlightedText: "#0b1520"
    palette.placeholderText: textMuted

    function loadDroppedImages(urls) {
        if (!urls || urls.length === 0)
            return

        if (urls.length >= 2) {
            correlationEngine.setImageA(urls[0])
            correlationEngine.setImageB(urls[1])
            return
        }

        if (correlationEngine.imageAUrl.length === 0)
            correlationEngine.setImageA(urls[0])
        else
            correlationEngine.setImageB(urls[0])
    }

    FileDialog {
        id: imageADialog
        title: "Choose Image A"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Images (*.png *.jpg *.jpeg *.bmp *.webp *.tif *.tiff)", "All files (*)"]
        onAccepted: correlationEngine.setImageA(selectedFile)
    }

    FileDialog {
        id: imageBDialog
        title: "Choose Image B"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Images (*.png *.jpg *.jpeg *.bmp *.webp *.tif *.tiff)", "All files (*)"]
        onAccepted: correlationEngine.setImageB(selectedFile)
    }

    component Card: Rectangle {
        radius: 12
        color: root.panel
        border.color: root.border
        border.width: 1
    }

    component ActionButton: Button {
        id: control
        property bool primary: false

        implicitWidth: 82
        implicitHeight: 34
        leftPadding: 14
        rightPadding: 14

        HoverHandler { id: hover }

        contentItem: Text {
            text: control.text
            color: !control.enabled
                   ? "#778291"
                   : control.primary ? "#08131f" : root.textMain
            font: control.font
            font.weight: control.primary ? Font.DemiBold : Font.Medium
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 7
            color: !control.enabled
                   ? "#252c35"
                   : control.down
                     ? (control.primary ? "#5c9fe9" : "#303947")
                     : hover.hovered
                       ? (control.primary ? "#8bc1ff" : "#2b3440")
                       : (control.primary ? root.accent : root.panel2)
            border.color: !control.enabled
                          ? "#313946"
                          : control.primary ? "#a7d1ff" : root.borderStrong
            border.width: 1
        }
    }

    component StyledCheckBox: CheckBox {
        id: control
        implicitHeight: 30
        spacing: 9

        indicator: Rectangle {
            implicitWidth: 19
            implicitHeight: 19
            x: control.leftPadding
            y: control.topPadding + (control.availableHeight - height) / 2
            radius: 5
            color: control.checked ? root.accent : root.field
            border.color: control.checked ? "#a7d1ff" : root.borderStrong
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: "✓"
                visible: control.checked
                color: "#08131f"
                font.pixelSize: 13
                font.weight: Font.Bold
            }
        }

        contentItem: Text {
            leftPadding: control.indicator.width + control.spacing
            text: control.text
            color: control.enabled ? root.textMain : "#748090"
            font: control.font
            verticalAlignment: Text.AlignVCenter
        }
    }

    component StyledTextField: TextField {
        id: control
        implicitHeight: 34
        color: root.textMain
        placeholderTextColor: root.textMuted
        selectionColor: root.accent
        selectedTextColor: "#08131f"
        leftPadding: 10
        rightPadding: 10

        background: Rectangle {
            radius: 7
            color: root.field
            border.color: control.activeFocus ? root.accent : root.border
            border.width: control.activeFocus ? 1.5 : 1
        }
    }

    component StyledSpinBox: SpinBox {
        id: control
        editable: true
        implicitHeight: 34

        contentItem: TextInput {
            z: 2
            text: control.textFromValue(control.value, control.locale)
            color: root.textMain
            selectionColor: root.accent
            selectedTextColor: "#08131f"
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
            leftPadding: 10
            rightPadding: 38
            readOnly: !control.editable
            validator: control.validator
            inputMethodHints: Qt.ImhFormattedNumbersOnly
        }

        up.indicator: Rectangle {
            x: control.width - width
            y: 0
            implicitWidth: 30
            implicitHeight: control.height / 2
            color: control.up.pressed ? "#364150" : root.panel2
            border.color: root.border

            Text {
                anchors.centerIn: parent
                text: "▲"
                color: root.textMain
                font.pixelSize: 9
            }
        }

        down.indicator: Rectangle {
            x: control.width - width
            y: control.height / 2
            implicitWidth: 30
            implicitHeight: control.height / 2
            color: control.down.pressed ? "#364150" : root.panel2
            border.color: root.border

            Text {
                anchors.centerIn: parent
                text: "▼"
                color: root.textMain
                font.pixelSize: 9
            }
        }

        background: Rectangle {
            radius: 7
            color: root.field
            border.color: control.activeFocus ? root.accent : root.border
            border.width: control.activeFocus ? 1.5 : 1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2

                Text {
                    text: "Phase Correlation Tester"
                    color: root.textMain
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                }

                Text {
                    text: "Compare two same-sized frames and inspect the full 2D phase-correlation response."
                    color: root.textMuted
                    font.pixelSize: 14
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                implicitWidth: statusText.implicitWidth + 20
                implicitHeight: 30
                radius: 15
                color: correlationEngine.hasResult ? "#18304a" : root.panel2
                border.color: correlationEngine.hasResult ? "#3f7fb8" : root.border

                Text {
                    id: statusText
                    anchors.centerIn: parent
                    text: correlationEngine.hasResult ? "Heatmap ready" : "Awaiting analysis"
                    color: correlationEngine.hasResult ? "#a9d4ff" : root.textMuted
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
            }

            ActionButton {
                text: "Analyze"
                primary: true
                enabled: correlationEngine.imageAUrl.length > 0 && correlationEngine.imageBUrl.length > 0
                onClicked: correlationEngine.analyze()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 226
                    spacing: 14

                    Card {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true

                                ColumnLayout {
                                    spacing: 0
                                    Text {
                                        text: "Image A"
                                        color: root.textMain
                                        font.weight: Font.DemiBold
                                    }
                                    Text {
                                        text: "Reference frame"
                                        color: root.textMuted
                                        font.pixelSize: 11
                                    }
                                }

                                Item { Layout.fillWidth: true }
                                ActionButton { text: "Load"; onClicked: imageADialog.open() }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 8
                                color: root.field
                                border.color: "#2c3541"
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    source: correlationEngine.imageAUrl
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                }

                                Text {
                                    anchors.centerIn: parent
                                    visible: correlationEngine.imageAUrl.length === 0
                                    text: "Drop or load image A"
                                    color: root.textMuted
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }

                    Card {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true

                                ColumnLayout {
                                    spacing: 0
                                    Text {
                                        text: "Image B"
                                        color: root.textMain
                                        font.weight: Font.DemiBold
                                    }
                                    Text {
                                        text: "Shifted frame"
                                        color: root.textMuted
                                        font.pixelSize: 11
                                    }
                                }

                                Item { Layout.fillWidth: true }
                                ActionButton { text: "Load"; onClicked: imageBDialog.open() }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 8
                                color: root.field
                                border.color: "#2c3541"
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    source: correlationEngine.imageBUrl
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                }

                                Text {
                                    anchors.centerIn: parent
                                    visible: correlationEngine.imageBUrl.length === 0
                                    text: "Drop or load image B"
                                    color: root.textMuted
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }
                }

                Card {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true

                            ColumnLayout {
                                spacing: 1

                                Text {
                                    text: "Phase-correlation heatmap"
                                    color: root.textMain
                                    font.pixelSize: 17
                                    font.weight: Font.DemiBold
                                }

                                Text {
                                    text: "2D inverse-FFT response; brighter/hotter regions indicate stronger translation candidates."
                                    color: root.textMuted
                                    font.pixelSize: 12
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Rectangle {
                                visible: correlationEngine.hasResult && correlationEngine.peaks.length > 0
                                implicitWidth: bestShiftText.implicitWidth + 18
                                implicitHeight: 30
                                radius: 7
                                color: "#12263a"
                                border.color: "#315d86"

                                Text {
                                    id: bestShiftText
                                    anchors.centerIn: parent
                                    text: correlationEngine.peaks.length > 0
                                          ? "Best: dx " + Number(correlationEngine.peaks[0].dx).toFixed(2)
                                            + ", dy " + Number(correlationEngine.peaks[0].dy).toFixed(2)
                                          : ""
                                    color: "#b9dcff"
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                }
                            }

                            Text {
                                visible: correlationEngine.hasResult
                                text: correlationEngine.runtimeMs.toFixed(2) + " ms"
                                color: root.textMuted
                                font.pixelSize: 12
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 10
                            color: "#080b0f"
                            border.color: correlationEngine.hasResult ? "#46627e" : "#26303b"
                            border.width: 1
                            clip: true

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 8
                                radius: 6
                                color: "#06080b"
                                border.color: "#1f2832"
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    source: correlationEngine.heatmapUrl
                                    fillMode: Image.PreserveAspectFit
                                    cache: false
                                    asynchronous: true
                                }

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 7
                                    visible: !correlationEngine.hasResult

                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "No heatmap yet"
                                        color: root.textMain
                                        font.pixelSize: 16
                                        font.weight: Font.DemiBold
                                    }

                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: "Load two images, then run Analyze"
                                        color: root.textMuted
                                        font.pixelSize: 13
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: correlationEngine.hasResult
                            spacing: 9

                            Text {
                                text: "Low"
                                color: root.textMuted
                                font.pixelSize: 11
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 9
                                radius: 4
                                gradient: Gradient {
                                    orientation: Gradient.Horizontal
                                    GradientStop { position: 0.00; color: "#30123b" }
                                    GradientStop { position: 0.20; color: "#4669e8" }
                                    GradientStop { position: 0.40; color: "#1bcfd4" }
                                    GradientStop { position: 0.60; color: "#a4fc3c" }
                                    GradientStop { position: 0.80; color: "#f9b41b" }
                                    GradientStop { position: 1.00; color: "#b40426" }
                                }
                            }

                            Text {
                                text: "High"
                                color: root.textMuted
                                font.pixelSize: 11
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "White cross = zero shift. Numbered white rings = ranked local peaks. The optional white rectangle is the active translation-search region."
                            color: root.textMuted
                            font.pixelSize: 11
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }

            Card {
                Layout.preferredWidth: 342
                Layout.fillHeight: true

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 14
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

                        Text {
                            text: "Analysis settings"
                            color: root.textMain
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "Tune the correlation surface and the peak search without changing the input images."
                            color: root.textMuted
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }

                        StyledCheckBox {
                            text: "Apply Hann window"
                            checked: correlationEngine.hannWindow
                            onToggled: correlationEngine.hannWindow = checked
                        }

                        StyledCheckBox {
                            id: limitSearchBox
                            text: "Limit translation search"
                            checked: correlationEngine.limitSearch
                            onToggled: correlationEngine.limitSearch = checked
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 92
                            radius: 9
                            color: root.panel2
                            border.color: root.border
                            enabled: limitSearchBox.checked
                            opacity: enabled ? 1.0 : 0.45

                            GridLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                columns: 2
                                rowSpacing: 8
                                columnSpacing: 10

                                Text { text: "± X px"; color: root.textMuted }
                                StyledTextField {
                                    Layout.fillWidth: true
                                    text: correlationEngine.maxDx.toString()
                                    validator: IntValidator { bottom: 0; top: 16384 }
                                    onEditingFinished: correlationEngine.maxDx = parseInt(text || "0")
                                }

                                Text { text: "± Y px"; color: root.textMuted }
                                StyledTextField {
                                    Layout.fillWidth: true
                                    text: correlationEngine.maxDy.toString()
                                    validator: IntValidator { bottom: 0; top: 16384 }
                                    onEditingFinished: correlationEngine.maxDy = parseInt(text || "0")
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 92
                            radius: 9
                            color: root.panel2
                            border.color: root.border

                            GridLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                columns: 2
                                rowSpacing: 8
                                columnSpacing: 10

                                Text { text: "Peaks"; color: root.textMuted }
                                StyledSpinBox {
                                    Layout.fillWidth: true
                                    from: 1
                                    to: 20
                                    value: correlationEngine.peakCount
                                    onValueModified: correlationEngine.peakCount = value
                                }

                                Text { text: "Suppress radius"; color: root.textMuted }
                                StyledSpinBox {
                                    Layout.fillWidth: true
                                    from: 1
                                    to: 512
                                    value: correlationEngine.suppressionRadius
                                    onValueModified: correlationEngine.suppressionRadius = value
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 1
                            color: root.border
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "Detected peaks"
                                color: root.textMain
                                font.pixelSize: 17
                                font.weight: Font.DemiBold
                            }

                            Item { Layout.fillWidth: true }

                            Rectangle {
                                implicitWidth: peakCountText.implicitWidth + 14
                                implicitHeight: 26
                                radius: 13
                                color: root.panel2
                                border.color: root.border

                                Text {
                                    id: peakCountText
                                    anchors.centerIn: parent
                                    text: correlationEngine.peaks.length.toString()
                                    color: root.textMuted
                                    font.pixelSize: 12
                                }
                            }
                        }

                        Repeater {
                            model: correlationEngine.peaks

                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: 86
                                radius: 9
                                color: modelData.rank === 1 ? "#1d2a37" : root.panel2
                                border.color: modelData.rank === 1 ? "#4a779f" : root.border

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true

                                        Text {
                                            text: "#" + modelData.rank
                                            color: modelData.rank === 1 ? root.accent : root.textMain
                                            font.weight: Font.Bold
                                        }

                                        Item { Layout.fillWidth: true }

                                        Text {
                                            text: (modelData.relative * 100).toFixed(1) + "% of peak 1"
                                            color: root.textMuted
                                            font.pixelSize: 11
                                        }
                                    }

                                    Text {
                                        text: "dx " + Number(modelData.dx).toFixed(2)
                                              + " px    dy " + Number(modelData.dy).toFixed(2) + " px"
                                        color: root.textMain
                                        font.weight: Font.Medium
                                    }

                                    Text {
                                        text: "raw strength " + Number(modelData.strength).toExponential(4)
                                        color: root.textMuted
                                        font.pixelSize: 11
                                    }
                                }
                            }
                        }

                        Text {
                            visible: correlationEngine.peaks.length === 0
                            Layout.fillWidth: true
                            text: "Peak candidates will appear here after analysis."
                            color: root.textMuted
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }

                        Item { Layout.preferredHeight: 4 }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 42
            radius: 9
            color: root.panel2
            border.color: root.border

            Text {
                anchors.fill: parent
                anchors.margins: 11
                verticalAlignment: Text.AlignVCenter
                text: correlationEngine.statusMessage
                color: root.textMuted
                font.pixelSize: 13
                elide: Text.ElideRight
            }
        }
    }

    DropArea {
        id: imageDropArea
        anchors.fill: parent
        z: 10

        onDropped: function(drop) {
            root.loadDroppedImages(drop.urls)
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 8
        z: 9
        visible: imageDropArea.containsDrag
        radius: 12
        color: root.accentSoft
        border.color: root.accent
        border.width: 2

        Text {
            anchors.centerIn: parent
            text: "Drop one image to fill the next slot, or drop two images for A and B"
            color: root.textMain
            font.pixelSize: 18
            font.weight: Font.DemiBold
        }
    }
}
