<p align="center"><img src="assets/branding/logo.png" width="128" alt="QML Cockpit Simulator 标志"></p>
<h1 align="center">QML Cockpit Simulator</h1>
<p align="center"><strong>面向智能座舱的整车状态模拟与联调工具</strong></p>

---

<p align="center">
  <a href="https://github.com/woniu101/qml-cockpit-simulator/releases"><img src="https://img.shields.io/badge/Version-0.2.0-527761" alt="版本 0.2.0"></a>
  <img src="https://img.shields.io/badge/Qt-6.12-41CD52" alt="Qt 6.12">
  <img src="https://img.shields.io/badge/Windows-x64-357ABD" alt="Windows x64">
  <a href="https://github.com/woniu101/qml-cockpit"><img src="https://img.shields.io/badge/车机项目-协议_v1-527761" alt="车机项目，协议 v1"></a>
</p>

---

<p align="center">
  <a href="#下载与快速开始">下载</a> · <a href="#实操演示">实操演示</a> · <a href="#当前能力">功能</a> · <a href="#开发版运行">开发指南</a> · <a href="#协议与实现">协议</a>
</p>

独立运行的 Qt / C++ 整车模拟器，为 [QML Cockpit](https://github.com/woniu101/qml-cockpit) 提供实时车辆状态和控制回执。两个项目独立构建，通过本机进程通信联调。

## 下载与快速开始

Windows x64 便携包的发布入口为 [GitHub Releases](https://github.com/woniu101/qml-cockpit-simulator/releases)。获取压缩包后解压，运行 `QmlCockpitSimulator.exe`，或按[构建说明](#开发版运行)从源码运行。打开 [QML Cockpit](https://github.com/woniu101/qml-cockpit)，在顶栏将数据源切换为「外部模拟器」。

## 实操演示

[![模拟器驾驶主屏](docs/screenshots/driver.jpg)](https://github.com/woniu101/qml-cockpit#实操演示)

查看[车机与模拟器实操演示](https://github.com/woniu101/qml-cockpit#实操演示)。右后车窗从 0% 调到 35% 的片段展示了目标下发、开度变化与结果回传。

开发程序为 `build/QmlCockpitSimulator.exe`，窗口标题为“QML Cockpit Simulator · 整车模拟器”。图标与车机共用浅灰绿底，使用带脉冲线的芯片轮廓。多尺寸 Windows 图标可用 `scripts/create-app-icon.ps1` 重新生成。

## 开发版运行

当前脚本适配开发机器：Visual Studio 2022 Community、Qt 6.12.0 MSVC x64、CMake、Ninja，安装在 `D:\Microsoft Visual Studio` 和 `D:\Qt`。其他环境需要调整 `CMakePresets.json` 与脚本中的工具路径。

```powershell
Set-Location D:\Projects\car_terminal\qml-cockpit-simulator
.\scripts\build.cmd
.\scripts\run.cmd
```

在车机顶部点击“数据源”，选择“外部模拟器”。默认服务名称为 `qml-cockpit.vehicle.v1`，两端必须一致。

默认进入驾驶主屏。三维道路、标线与路侧设施随实际模拟里程移动，停车时保持静止。`F11` 切换全屏，`Esc` 退出全屏，`F1` 打开按键设置。右上角的车辆操作抽屉提供四窗、门锁和后视镜操作，详细门窗与备厢状态在车身页。

默认 **← 刹车 / → 油门**，**1 / 2 / 3 / 4** 对应 P / R / N / D，**Z / X** 升降主驾车窗。可切换 **W 油门 / S 刹车** 预设或逐项改键，配置自动保存。踩住刹车再从 P 挡挂 D/R，松开油门后滑行。踏板和车窗持续操作在切页、打开弹层或窗口失焦时释放。

补能页可开盖、插枪、开始或暂停充电，并快进 5 分钟。联调页可查看日志、打开原控制台，也可直接用 `scripts/run.cmd --console` 启动原控制台。

驾驶舱的实体换挡有停稳和踩刹车要求，原控制台与车机仍可使用固定车速演示。两端沿用协议 v1，P/N 速度为零。日间与夜间是模拟器本地显示设置。空调和灯光目前仍由车机管理。

平级目录布局下，也可在车机仓库中执行：

```powershell
..\qml-cockpit\scripts\run-with-simulator.ps1
```

该脚本打开模拟器和车机开发窗口，车机自动选择外部模式。两个进程独立退出；关闭或停止模拟器后，车机自动重连。

## 当前能力

- SOC 直接注入、P/R/N/D、模拟车速、累计行驶距离、快进；D/R 行驶自动消耗电量，D 最高 120 km/h，R 最高 15 km/h。
- 四门状态与开合互锁；四窗实际开度、目标和运动状态。输入数字直接注入实际值，车机下发的目标按约 1.8 秒全行程平滑执行，支持中途停止和后续指令替换。
- 驻车开充电口、插枪、设置 50–100% 目标、开始/暂停充电；55 kW / 60 kWh 示意能量模型，到达目标自动停止。
- 指令日志，以及暂停遥测、拒绝、丢弃请求、延迟执行和主动断开连接等故障注入。
- 控制台支持锁车、前后备厢、后视镜和全部关门，操作条件与服务端校验共用同一规则。
- 按状态启用开始/暂停充电、拔枪、开口、换挡、行驶和批量操作；灰色操作提供条件说明。窗口较窄时自动换为单列，可滚动访问完整内容，按钮文字保持完整。
- “清除全部故障”恢复遥测及后续请求的正常处理；“重置模拟车辆”恢复车辆并取消已排队的延迟指令。车辆重置与故障清除分别操作。

这是虚拟车辆数据源。Windows 电池、电源、蓝牙和网络由车机程序独立接入；模拟器不操作这些系统设备，也没有接入真实车辆硬件。音响、空调和灯光仍在车机侧处理。

## 验证

```powershell
.\scripts\test.cmd
# 先分别构建两个仓库，再在车机仓库验证真实双进程交互：
..\qml-cockpit\scripts\verify-link.ps1
..\qml-cockpit\scripts\verify-link.ps1 -Scale 1.25
```

测试包含分帧/粘包/非法数据、窗位目标与实际值、行驶互锁、充电目标及真实界面输入。驾驶舱测试覆盖踏板、键盘、车窗翘板、补能和联调页。双进程测试覆盖 UI 指令、实际玻璃位置、停止/替换、充电、拒绝/超时、失联冻结、导航补算、重连和源切换。测试使用独立服务名称，不接管正在运行的演示实例。

截图和日志保存在各仓库的 `artifacts/`，构建产物在 `build/`，均不纳入 Git；开发阶段无需生成便携包。

## 协议与实现

- [协议 v1](protocol/README.md)：消息格式、字段、回执、超时和边界。
- `protocol/VehicleProtocol.h`：分帧与快照校验，和车机仓库中的副本保持一致。
- `src/SimVehicle.*`：独立车辆模型。
- `src/SimServer.*`：本机服务、会话、指令调度与回执。
- `src/SimulatorWindow.*`：Qt Widgets 控制台。

无界面运行：`scripts/run.cmd --headless --server qml-cockpit.vehicle.v1`。可以指定不同服务名启动多套互不影响的联调实例。
