import QtQuick
import QtQuick3D

Item {
    id: road
    required property real distanceKm
    readonly property real travelMeters: distanceKm * 1000
    readonly property real stripeOffset: ((travelMeters % 12) + 12) % 12
    readonly property real postOffset: ((travelMeters % 24) + 24) % 24
    readonly property real edgeOffset: ((travelMeters % 6) + 6) % 6

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: DriverTheme.night ? "#10252c" : "#91b2b6" }
            GradientStop { position: 0.7; color: DriverTheme.night ? "#344e51" : "#d9e3da" }
            GradientStop { position: 1; color: DriverTheme.night ? "#273e37" : "#afc0b2" }
        }
    }

    View3D {
        anchors.fill: parent
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Transparent
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
            fog: Fog {
                enabled: true
                color: DriverTheme.night ? "#344e51" : "#b5c8c0"
                depthEnabled: true
                depthNear: 100
                depthFar: 650
            }
        }
        camera: driverCamera
        PerspectiveCamera {
            id: driverCamera
            position: Qt.vector3d(1.55, 1.45, 6)
            eulerRotation.x: -3
            clipNear: 0.1
            clipFar: 900
            fieldOfView: 72
        }

        Node {
            scale: Qt.vector3d(0.5, 1, 1)
            PrincipledMaterial {
                id: groundMaterial
                baseColor: DriverTheme.night ? "#1f352f" : "#8b9d8a"
                lighting: PrincipledMaterial.NoLighting
            }
            PrincipledMaterial {
                id: asphaltMaterial
                baseColor: DriverTheme.night ? "#20353a" : "#536467"
                lighting: PrincipledMaterial.NoLighting
            }
            PrincipledMaterial {
                id: lineMaterial
                baseColor: DriverTheme.night ? "#99baab" : "#e1e8db"
                lighting: PrincipledMaterial.NoLighting
            }
            PrincipledMaterial {
                id: reflectorMaterial
                baseColor: DriverTheme.night ? "#76e0d6" : "#518980"
                lighting: PrincipledMaterial.NoLighting
            }
            Model {
                source: "#Cube"
                position: Qt.vector3d(0, -0.2, -300)
                scale: Qt.vector3d(14, 0.002, 14)
                materials: groundMaterial
            }
            Model {
                source: "#Cube"
                position: Qt.vector3d(0, 0, -300)
                scale: Qt.vector3d(0.14, 0.001, 8)
                materials: asphaltMaterial
            }
            Repeater3D {
                model: [-6.5, 6.5]
                Model {
                    required property real modelData
                    source: "#Cube"
                    position: Qt.vector3d(modelData, 0.065, -300)
                    scale: Qt.vector3d(0.0012, 0.0001, 8)
                    materials: lineMaterial
                }
            }
            Repeater3D {
                model: 60
                Model {
                    required property int index
                    source: "#Cube"
                    position: Qt.vector3d(0, 0.07, 24 - index * 12 + road.stripeOffset)
                    scale: Qt.vector3d(0.0018, 0.0001, 0.045)
                    materials: lineMaterial
                }
            }
            Repeater3D {
                model: 60
                Node {
                    required property int index
                    position: Qt.vector3d(index % 2 ? 7.5 : -7.5, 0, 36 - Math.floor(index / 2) * 24 + road.postOffset)
                    Model {
                        source: "#Cube"
                        y: 0.5
                        scale: Qt.vector3d(0.0013, 0.01, 0.0013)
                        materials: lineMaterial
                    }
                    Model {
                        source: "#Cube"
                        y: 0.84
                        scale: Qt.vector3d(0.0015, 0.0026, 0.0015)
                        materials: reflectorMaterial
                    }
                }
            }
            Repeater3D {
                model: [-7.9, 7.9]
                Model {
                    required property real modelData
                    source: "#Cube"
                    position: Qt.vector3d(modelData, 0.65, -300)
                    scale: Qt.vector3d(0.001, 0.0028, 8)
                    materials: lineMaterial
                }
            }
            Repeater3D {
                model: 2
                Node {
                    required property int index
                    z: 40 - index * 150 + ((road.travelMeters % 150) + 150) % 150
                    Repeater3D {
                        model: [-8.8, 8.8]
                        Model {
                            required property real modelData
                            source: "#Cube"
                            position: Qt.vector3d(modelData, 2.7, 0)
                            scale: Qt.vector3d(0.002, 0.054, 0.002)
                            materials: lineMaterial
                        }
                    }
                    Model {
                        source: "#Cube"
                        y: 5.4
                        scale: Qt.vector3d(0.178, 0.003, 0.003)
                        materials: lineMaterial
                    }
                    Repeater3D {
                        model: [-3.1, 3.1]
                        Model {
                            required property real modelData
                            source: "#Cube"
                            position: Qt.vector3d(modelData, 4.7, 0)
                            scale: Qt.vector3d(0.036, 0.012, 0.0015)
                            materials: reflectorMaterial
                        }
                    }
                }
            }
            Repeater3D {
                model: 144
                Model {
                    required property int index
                    source: "#Cube"
                    position: Qt.vector3d(index % 2 ? 6.85 : -6.85, 0.065,
                                         18 - Math.floor(index / 2) * 6 + road.edgeOffset)
                    scale: Qt.vector3d(0.0024, 0.0005, 0.009)
                    materials: reflectorMaterial
                }
            }
            Repeater3D {
                model: 18
                Model {
                    required property int index
                    source: "#Cone"
                    position: Qt.vector3d(index % 2 ? 150 + index * 14 : -150 - index * 14, 0, -440 - Math.floor(index / 2) * 32)
                    scale: Qt.vector3d(1.3 + index % 3 * 0.25, 0.6 + index % 4 * 0.12, 1.3)
                    materials: groundMaterial
                }
            }
        }
    }
}
