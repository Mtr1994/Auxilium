# `Auxilium`

![Platform](https://img.shields.io/badge/paltform-win10--64-brightgreen)
![Qt Version](https://img.shields.io/badge/_Qt_5.15.2-yellowgreen)
![Build](https://img.shields.io/badge/build-MSVC_2019_x64-blue)
![GitHub](https://img.shields.io/github/license/Mtr1994/Auxilium)

### 一、简介

`Auxilium` 程序用于打包 `QWidget` 程序 和 `Quick` 程序。

* 简洁模式：收集 `*.exe` 程序的依赖库，不继续寻找这些依赖库的后续依赖。
* 循环模式：收集 `*.exec` 程序的依赖和每一个 `*.dll` 的后续依赖。

系统能正常工作于 `Windows` 系统和 `Linux` 系统。

### 二、研发事件

- [x] 2022/12/05  配置窗口等待完成
- [x] 改名为 `Auxilium`，中文含义是 “辅助装置、打辅助的人”
