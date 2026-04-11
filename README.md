# Rimoo (PC + Android)

Rimoo 是一个开源输入法项目，当前同时维护 PC（Windows）与 Android 两端，第一阶段聚焦简体中文用户的稳定输入体验。

## 当前版本

- 产品名：`Rimoo`
- 版本：`0.1.0`

## 已完成（0.1.0）

- PC 端统一应用名称为 `Rimoo`
- Android 端统一应用名称为 `Rimoo`
- 默认输入方向调整为简体中文场景
- PC 端移除自动检查更新模块
- Android 端保留九键输入并完成当前阶段修复
- 补充偏大陆场景的简中高频词/口语词

## 后续计划

- 词库同步功能（PC <-> Android）
  - 目标：跨设备同步用户词库与常用短语
  - 状态：规划中，`0.1.0` 尚未提供该功能

## 仓库结构

- `WeaselServer` / `WeaselTSF` / `WeaselDeployer` / `WeaselSetup`：PC 输入法核心组件
- `android-yuyanime/`：Android 输入法工程（含 `yuyansdk`）
- `output/install.nsi`：Windows 安装包脚本
- `resource/*.ico`：PC 端图标资源

## 构建与产物

### PC（Windows）

前提：

- Visual Studio Build Tools 2022
- Windows SDK
- Boost
- NSIS

命令：

```bat
call env.bat
build.bat release installer
```

安装包输出目录：

- `output\archives\`

### Android

前提：

- Android Studio（建议稳定版）
- JDK 17
- Android SDK（minSdk 23）

命令：

```bat
cd android-yuyanime
gradlew.bat :app:assembleOfflineDebug
```

APK 输出目录：

- `android-yuyanime\app\build\outputs\apk\offline\debug\`

## Release

统一发布地址（PC + Android 同仓）：

- https://github.com/SumiRayki/Rimoo/releases

## 致谢

- [Rime](https://github.com/rime)
- [Weasel](https://github.com/rime/weasel)
- [YuyanIme](https://github.com/gurecn/YuyanIme)
