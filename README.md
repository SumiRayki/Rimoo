# Rimoo (PC)

Rimoo 是一个基于 Rime / Weasel 深度定制的 Windows 输入法项目，当前阶段聚焦简体中文用户的稳定输入体验。

## 当前版本

- Product: `Rimoo`
- Version: `0.1.0`
- 基线：`rime/weasel` fork

## 第一阶段目标（已落地）

- 应用名称统一为 `Rimoo`
- 版本号统一为 `0.1.0`
- 移除自动检查更新模块（WinSparkle）
- 默认词库组合调整为更适合简体中文用户
- 其余功能保持 Weasel 原有能力，不做额外删减

## 后续计划

- 词库同步功能（PC <-> Android）：
  - 目标：支持跨设备同步用户词库与常用短语
  - 状态：规划中，`0.1.0` 尚未提供该功能

## 仓库结构

- `WeaselServer` / `WeaselTSF` / `WeaselDeployer` / `WeaselSetup`：PC 输入法核心组件
- `output/install.nsi`：Windows 安装包脚本
- `resource/*.ico`：托盘、程序与安装器图标资源

## 本地构建（Windows）

前提：

- Visual Studio Build Tools 2022
- Windows SDK
- Boost（按仓库脚本配置）
- NSIS（用于 installer 打包）

示例命令：

```bat
call env.bat
build.bat release installer
```

默认输出：

- 安装包目录：`output\archives\`

## 安装与运行

- 双击 installer 安装，或静默安装：

```bat
rimoo-0.1.0.x-installer.exe /S
```

- 安装后默认程序目录：
  - `C:\Program Files\Rime\weasel-0.1.0`

## 许可证

- 本项目沿用上游 `rime/weasel` 与其依赖组件的开源许可证体系（详见仓库内 `LICENSE.txt` 及各组件声明）。

## 致谢

- [Rime](https://github.com/rime)
- [Weasel](https://github.com/rime/weasel)
