# `Auxilium`

![Platform](https://img.shields.io/badge/paltform-win10--64-brightgreen)
![Qt Version](https://img.shields.io/badge/_Qt_5.15.2-yellowgreen)
![Build](https://img.shields.io/badge/build-MSVC_2019_x64-blue)

### 一、简介

`Auxilium` 程序用于打包 `QWidget` 程序 和 `Quick` 程序。

* 简洁模式：收集 `*.exe` 程序的依赖库，不继续寻找这些依赖库的后续依赖。
* 循环模式：收集 `*.exec` 程序的依赖和每一个 `*.dll` 的后续依赖。

系统能正常工作于 `Windows` 系统和 `Linux` 系统。

### 二、研发事件

- [x] 2022/12/05  配置窗口等待完成
- [x] 改名为 `Auxilium`，中文含义是 “辅助装置、打辅助的人”
- [x] 新增依赖库检索路径管理功能
- [x] 新增 `QML` 类型程序打包功能
- [x] 新增 `Linux` 系统下的打包逻辑（Linux 系统下，手动配置的检索路径无效）



### 三、安装包制作注意事项

* 使用 `Inno Setup` 制作安装包的注意事项

  ```
  1、如果主目录下有文件夹，通过导航加入文件夹后，需要手动修改该文件夹的 DestDir 属性；
  	例子：Source: "C:\ISMIFFMonitor_Plus\styles\*"; DestDir: "{app}\styles"; x...x...x
  	
  2、默认的安装包安装在 `C 盘`后只能使用管理员权限运行，需要加入以下命令
  	[Dirs]
  	Name: {app}; Permissions: users-full
  	
  	可以放到 [Run] 字段上面
  ```
  
  
