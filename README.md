<p align="center"><img src="assets/branding/logo.png" width="128" alt="QML Cockpit Simulator 标志"></p>
<h1 align="center">QML Cockpit Simulator</h1>
<p align="center"><strong>面向智能座舱的整车模拟与联调工具</strong></p>

---

<p align="center">
  <a href="https://github.com/woniu101/qml-cockpit-simulator/releases/tag/v0.2.0"><img src="https://img.shields.io/badge/Release-v0.2.0-527761" alt="Release v0.2.0"></a>
  <img src="https://img.shields.io/badge/Windows-x64-357ABD" alt="Windows x64">
  <img src="https://img.shields.io/badge/Qt-6.12-41CD52" alt="Qt 6.12">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL--3.0-blue" alt="GPL-3.0"></a>
</p>

---

<p align="center">
  <a href="#快速开始">下载</a> · <a href="#实操演示">实操演示</a> · <a href="#主要功能">主要功能</a> · <a href="#联调流程">车机联调</a> · <a href="#从源码构建">源码构建</a>
</p>

## 快速开始

下载 [Windows x64 便携包](https://github.com/woniu101/qml-cockpit-simulator/releases/download/v0.2.0/QmlCockpitSimulator-v0.2.0-windows-x64.zip)，完整解压后运行 `QmlCockpitSimulator.exe`。可单独体验驾驶、车身和补能，也可连接[同版本车机](https://github.com/woniu101/qml-cockpit/releases/tag/v0.2.0)。

| 默认按键 | 操作 |
| --- | --- |
| `←` / `→` | 刹车 / 油门 |
| `1` / `2` / `3` / `4` | P / R / N / D |
| `Z` / `X` | 升起 / 降下主驾车窗 |
| `F1` / `F11` | 按键设置 / 切换全屏 |

起步时按住刹车，从 P 挡切入 D 或 R，松开刹车后再踩油门。也可以直接使用界面上的挡位与踏板。

## 实操演示

https://github.com/user-attachments/assets/ce32dbd2-25b4-4e8e-ae83-a713da0a0295

片中右后窗从 0% 调到 35%，可以看到指令下发、开度变化和回传。

[在哔哩哔哩观看车机与模拟器实操演示](https://www.bilibili.com/video/BV1ewW46FEaL/)

## 主要功能

- **驾驶模拟**：操作挡位和踏板，实时更新车速、里程和电量。
- **车身与补能**：模拟门窗、门锁、后视镜、插枪和充电流程。
- **联调诊断**：查看指令回执，可模拟遥测暂停、延迟、拒绝和断线。

![模拟器驾驶主屏](docs/screenshots/driver.jpg)

## 联调流程

1. 启动模拟器和 [QML Cockpit](https://github.com/woniu101/qml-cockpit)。默认服务名为 `qml-cockpit.vehicle.v1`，联调进程需在同一台电脑、同一 Windows 用户下运行。
2. 在车机驻车时选择「数据源 → 外部模拟器」，等待顶栏显示「联调 · 已连接」。
3. 在车机将右后窗目标设为 35%，在模拟器的车身页查看实际开度，在联调页查看回执。也可从模拟器修改挡位、车速或充电状态，观察车机端的反馈。

验证异常时，可在联调页暂停遥测，再恢复遥测或点击「清除故障」，观察车机从等待状态恢复。需要单独查看控制台时，可用 `--console` 启动；自动化联调用 `--headless` 启动无界面服务。要运行多个实例，可用 `--server <名称>` 区分服务，车机端应配置相同名称。消息格式与回执语义见[联调协议](protocol/README.md)。

## 从源码构建

使用 Qt 6.12、Visual Studio 2022 的 MSVC x64 工具链、CMake 和 Ninja。若安装路径与仓库预设不同，先调整 [CMake 预设](CMakePresets.json)及相关脚本。

```powershell
.\scripts\build.cmd
.\scripts\run.cmd
```

构建产物为 `build/QmlCockpitSimulator.exe`。运行 `scripts/test.cmd` 可验证车辆逻辑、控制台和驾驶主屏。两个源码仓库平级放置并完成构建后，也可从车机仓库用 `scripts/run-with-simulator.ps1` 一起启动。

## 许可

项目源码采用 [GPL-3.0-only](LICENSE)。Qt 等组件的许可与源码见[第三方声明](THIRD_PARTY_NOTICES.md)。
