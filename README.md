# chfs / chfsgui — Cute HTTP File Server 3.1 镜像仓库

本仓库收集整理 [CuteHttpFileServer/chfs](http://iscute.cn/chfs)（v3.1）的官方程序文件，方便国内用户直接下载。

- **chfs**：免费的 HTTP 协议文件共享服务器，单文件运行，浏览器即可访问
- **chfsgui**：Windows 图形界面封装版（本仓库 fork 自 [docblue/chfsgui](https://github.com/docblue/chfsgui)）

## 简介

CuteHttpFileServer/chfs 是一个免费的、HTTP 协议的文件共享服务器，使用浏览器即可快速访问。特点：

- 单个文件，核心功能无需其他文件
- 跨平台运行，支持主流平台：Windows、Linux 和 Mac
- 界面简洁，简单易用
- 支持扫码下载和手机端访问，手机与电脑之间共享文件非常方便
- 支持账户权限控制和地址过滤
- 支持快速分享文字片段
- 支持 WebDAV 协议

与其他常用文件共享方式（FTP、飞秋、网盘、自己建站）相比，chfs 使用简单、适用场景更广，无论是个人使用还是共享给他人，都非常方便快捷。

## 下载

### GitHub Release 直链（推荐）

| 平台 | 文件 |
|---|---|
| Windows 图形界面 | [chfsgui-Windows-3.1.exe](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfsgui-Windows-3.1.exe) |
| Windows x64 | [chfs-windows-x64-3.1.exe](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfs-windows-x64-3.1.exe) |
| Windows x86 | [chfs-windows-x86-3.1.exe](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfs-windows-x86-3.1.exe) |
| macOS (Intel) | [chfs-mac-amd64-3.1](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfs-mac-amd64-3.1) |
| Linux amd64 | [chfs-linux-amd64-3.1](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfs-linux-amd64-3.1) |
| Linux 386 | [chfs-linux-386-3.1](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfs-linux-386-3.1) |
| Linux arm64 | [chfs-linux-arm64-3.1](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfs-linux-arm64-3.1) |
| Linux arm | [chfs-linux-arm-3.1](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfs-linux-arm-3.1) |
| Linux mips | [chfs-linux-mips-3.1](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfs-linux-mips-3.1) |
| Linux mips64 | [chfs-linux-mips64-3.1](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfs-linux-mips64-3.1) |
| Linux mips64le | [chfs-linux-mips64le-3.1](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfs-linux-mips64le-3.1) |
| Linux mipssoftfloat | [chfs-linux-mipssoftfloat-3.1](https://github.com/lu920115/chfsgui/releases/download/v3.1/chfs-linux-mipssoftfloat-3.1) |

> 完整列表见 [Releases 页面](https://github.com/lu920115/chfsgui/releases)。

### 其他下载方式

- 百度网盘分流：https://pan.baidu.com/s/1QDhTWb-CRUQaHkITSSrF1w （提取码：`chfs`）
- Docker 镜像：https://hub.docker.com/r/docblue/chfs

## 使用方法

### 1. GUI 方式（Windows，最简单）

直接运行 `chfsgui-Windows-3.1.exe`，在图形界面中设置共享目录和端口即可。

### 2. 命令行方式（CLI）

chfs 是控制台程序，可直接双击运行，也可在命令行中通过参数配置：

```
usage: chfs.exe [<options>]
  -file string   Configuration file.
  -path string   Shared directories, separated by '|'. (default ".")
  -port uint     HTTP listening port. (default 80)
  -version       Print version.
```

| 参数 | 说明 |
|---|---|
| `-path` | 要共享的目录，默认为程序运行目录；多个目录用 `|` 隔开。**路径带空格时用引号包住** |
| `-port` | 监听端口，默认 80 |
| `-file` | 配置文件，可覆盖上述配置项；页面自定义、SSL 证书等需在配置文件中设置 |
| `-version` | 显示程序版本号 |

示例：

```bash
# 共享 /Users/guoguo/Share 目录，端口 8080
./chfs-mac-amd64-3.1 -path "/Users/guoguo/Share" -port 8080
```

然后在浏览器访问 `http://<本机IP>:8080` 即可，手机扫码也可访问。

**Windows 小技巧**：下载注册表模板，编辑其中的 chfs.exe 路径后双击导入，即可通过右键菜单快捷共享目录。

### 3. 以系统服务运行（Windows + NSSM）

chfs 本身不是服务程序，如需作为系统服务常驻，可借助 [NSSM](http://www.nssm.cc/download) 注册：

```
1. 将 chfs.exe 放在指定目录，如 d:\program\cutehttpfileserver
2. 下载 nssm 并解压到同一目录
3. 命令行 CD 至该目录
4. 执行：nssm install cute_http_file_service
5. 在弹出的对话框中输入程序路径及运行参数
6. 启动服务：nssm start cute_http_file_service
```

### 4. 高级用法

**启用 HTTPS**：在配置文件中设置 `ssl.cert` 和 `ssl.key` 两个键值，指向对应的证书文件。注意：证书格式与 nginx 兼容；最低支持 TLS 1.0；推荐监听 443 端口。

**启用 WebDAV**：程序默认支持 WebDAV，与 HTTP 共用同一套访问规则，地址为 `http://host:port/webdav`。

## 版本更新说明

**v3.1**
- 修复无法启用 HTTPS 的缺陷
- 修复拖拽文件方式上传文件的目的目录不正确的缺陷
- 修复账户名称不能包含大写字母的缺陷
- 缩略图算法由 Lanczos 降为 NearestNeighbor，并取消后台缓存
- 优化 chfs 专用回收站（~/.chfs_trashbin）逻辑，增强平台兼容性

<details>
<summary>历史版本</summary>

**v3.0**
- 修复若干已知 BUG，细节优化
- 支持英语、繁体
- 优化文件上传，降低资源占用，加速上传速度
- 支持安全删除文件（回收站）
- 重新设计目录访问权限：设置默认访问权限后，可对任意子目录设置独立访问权限

**v2.0**
- 修复若干已知 BUG
- 增加视频播放按钮（仅支持 webm / ogg / mp4）
- 默认关闭图片缩略图
- 性能优化，降低进程资源占用

**v1.10**
- 所有 API 使用同一前缀：/chfs
- 增加上传文件夹功能
- 增加文本文件简单编辑功能
- 增加全局搜索文件功能

</details>

## 原作者与致谢

- **chfs 原作者网站**：http://iscute.cn/chfs
- **chfsgui GUI 封装**：[docblue/chfsgui](https://github.com/docblue/chfsgui)（LGPL v3）
- 本仓库仅为收集镜像，方便下载，所有权利归原作者所有。
