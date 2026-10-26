# Windows 云端 test EXE 构建指南与踩坑记录

最后验证日期：2026-10-27。

本文记录 `lu920115/chfsgui` 使用 GitHub Actions 构建 Windows x64 单文件静态 EXE 的可用流程，以及首次走通时实际遇到的问题。接手构建的 Agent 应先阅读根目录 `AGENTS.md`，再以现有工作流为准，不要凭记忆重新拼一套环境。

## 1. 已验证的构建入口

工作流文件：

```text
.github/workflows/build-windows-test.yml
```

已验证环境：

| 项目 | 当前可用配置 |
|---|---|
| Runner | `windows-latest` |
| MSYS2 环境 | `MINGW64`，不是 `UCRT64` |
| Qt | `mingw-w64-x86_64-qt5-static 5.15.19-1` |
| OpenSSL | `3.6.5-1` |
| zlib | `1.3.2-3` |
| zstd | `1.5.7-2` |
| 构建工具 | qmake + make |
| 链接参数 | `-static -static-libgcc -static-libstdc++` |
| 成功构建耗时 | 约 3 分钟 |

GCC 等工具链包的版本可能随 MSYS2 更新。每次构建都会生成 `build-environment.txt`，记录源码提交、Actions 运行 ID、GCC、qmake、make 和固定依赖版本。出现“上次能编译、这次不能编译”时，先比较这份文件，不要先改业务源码。

参考运行：

- [40415017885](https://github.com/lu920115/chfsgui/actions/runs/40415017885)：首个完整通过的 test 构建。
- [40416538137](https://github.com/lu920115/chfsgui/actions/runs/40416538137)：精简报告、构建环境记录及新版上传组件验证通过，用时 2 分 47 秒。

## 2. 触发、等待和下载

### 自动触发

以下推送会触发构建：

```yaml
on:
  push:
    paths:
      - chfsgui/**
      - .github/workflows/build-windows-test.yml
```

只修改 `README.md`、`docs/**` 或本文件不会触发构建，这是有意为之。

### 手动触发

可在 Actions 页面选择 **Build Windows test EXE → Run workflow**，也可使用 GitHub CLI：

```bash
gh workflow run build-windows-test.yml \
  --repo lu920115/chfsgui --ref master
```

找到刚触发的运行时，必须核对提交和事件，不要盲目把“最新一条”当作自己的运行：

```bash
gh run list --repo lu920115/chfsgui \
  --workflow build-windows-test.yml --limit 5 \
  --json databaseId,status,conclusion,headSha,event,url
```

等待并查看结果：

```bash
gh run watch <RUN_ID> --repo lu920115/chfsgui --exit-status
```

### 下载构件

版本来自 `chfsgui/appversion.h`：

```bash
VERSION=$(sed -n 's/^#define APP_VERSION "\([^"]*\)"$/\1/p' \
  chfsgui/appversion.h)

gh run download <RUN_ID> --repo lu920115/chfsgui \
  --name "chfsgui-v${VERSION}-test" --dir artifact
```

正常构件包含：

```text
CHFS3.1_GUI_Qrcode_v<版本>_test.exe
CHFS3.1_GUI_Qrcode_v<版本>_test.exe.sha256.txt
verification.txt
build-environment.txt
```

下载后先校验：

```bash
cd artifact
sha256sum -c CHFS3.1_GUI_Qrcode_v*_test.exe.sha256.txt
```

构件只保留 14 天。需要长期保留的测试文件应另行保存；正式版本仍发布到 `lu920115/chfs-gui-qrcode`，不要把 test 构件误当正式 Release。

## 3. 实际踩过的坑及正确处理方法

### 3.1 推送工作流返回 403：不是仓库写权限一定不够

实际错误：

```text
403 Forbidden: insufficient scopes
```

修改 `.github/workflows/**` 除了仓库权限，还需要 GitHub OAuth 的 `workflow` scope。只有 `repo` 的授权可能能推普通源码，却不能推工作流。

处理方法：通过 GitHub 官方设备授权补充 scope，例如已使用 CLI 登录时：

```bash
gh auth refresh -h github.com -s repo,workflow
```

不要让用户提供密码或私人 Token。授权完成后可用 `gh auth status` 确认包含 `repo`、`workflow`，再推送。

### 3.2 不能为了消除 MINGW64 提示切换 UCRT64

Windows 7 兼容路线依赖 MSVCRT。直接改成 UCRT64 可能引入 `ucrtbase.dll`、`api-ms-win-*` 等依赖，使 EXE 不能在原始 Windows 7 环境加载。

当前处理是保留：

```yaml
msystem: MINGW64
suppress-deprecation-warnings: msystem-mingw64
```

这只是抑制提示，不改变兼容目标。不要把它“优化”为 UCRT64。

每次构建还必须扫描 Win8+ API 和现代运行库依赖，至少包括 `GetSystemTimePreciseAsFileTime`、`ProcessPrng`、`api-ms-win-*`、`ucrtbase.dll`、`bcryptprimitives.dll`、`vulkan-1.dll` 和 `d3d12.dll`。不能仅凭 EXE 在 Windows 11 上能启动就判断它兼容 Windows 7。

### 3.3 Qt 必须用静态包，并显式静态链接 GCC 运行库

仅安装普通 Qt 或仅写一个 `-static` 不足以证明最终是单文件。当前工作流安装固定的 `qt5-static` 及依赖，并传入：

```bash
qmake ../chfsgui/chfsgui.pro \
  CONFIG+=release CONFIG-=debug \
  'QMAKE_LFLAGS+=-static -static-libgcc -static-libstdc++'
```

最终必须检查导入表，不能仅根据编译成功或文件较大判断静态链接成功。重点排除：

```text
libgcc_s_seh-1.dll
libstdc++-6.dll
libwinpthread-1.dll
libssl-3-x64.dll
libcrypto-3-x64.dll
Qt5*.dll
```

### 3.4 枚举名检查误报“快捷方式功能缺失”

失败运行：[40413995816](https://github.com/lu920115/chfsgui/actions/runs/40413995816)。

当时 EXE 已编译成功，但验证脚本使用：

```python
assert b'DesktopShortcuts' in data
```

于是误报失败。`DesktopShortcuts` 是 C++ 枚举名，不保证保留在编译后的二进制里；找不到它不代表界面或功能缺失。

改为检查实际界面文字：

```python
shortcut_text = '创建桌面快捷方式'
assert shortcut_text.encode('utf-8') in data
```

原则：功能验证应检查用户可见文字、配置、资源或实际行为，不要依赖会被编译器剥离的符号名。

### 3.5 二进制中文检查要选正确编码

同一次排查还确认了编码差异：

- `APP_ABOUT_TITLE` 经 `QStringLiteral` 使用，标题可按 UTF-16LE 检查。
- 部分 `tr("...")` 源字符串实际以 UTF-8 存在。

因此不要拿一种编码套所有字符串。若检查失败，先确认该字符串在源码中的声明和实际存储方式，不要直接认定功能没编进去。

### 3.6 Python 把 EXE 当脚本读取

失败运行：[40414545138](https://github.com/lu920115/chfsgui/actions/runs/40414545138)。

错误写法：

```bash
python "output/$TEST_EXE" <<'PY'
```

这会让 Python 将 EXE 当作源码读取，最终出现 `Non-UTF-8 code` 等错误。正确写法是使用 `-` 从标准输入读取脚本，并把 EXE 作为参数：

```bash
python - "output/$TEST_EXE" <<'PY'
import sys
filename = sys.argv[1]
PY
```

多个参数也一样，例如：

```bash
python - "$version" "output/$TEST_EXE" \
  chfsgui/chfs_resource/chfs.exe <<'PY'
import sys
version, filename, corename = sys.argv[1:]
PY
```

### 3.7 完整 `objdump -p` 不是合适的构件报告

最初直接执行：

```bash
objdump -p "output/$TEST_EXE" > output/imports.txt
```

结果报告达到 **23,159,803 字节、494,860 行**。原因是输出还包含大量函数表等内容，并不只是 DLL 导入表。

现在仍在内存中运行完整检查，但只把 PE 关键字段、检查结论和 DLL 列表写入 `verification.txt`。改进后的报告为 **611 字节**。不要为了保留“完整日志”再把几十 MB 的无用输出塞进每个构件。

### 3.8 官方核心要同时检查源文件和最终 EXE

只检查仓库中的 `chfs.exe` 哈希，不能证明构建产物确实内嵌了同一个核心。当前工作流同时执行：

1. 校验 `chfsgui/chfs_resource/chfs.exe` 的 SHA-256。
2. 将官方核心字节与最终 EXE 做包含检查，确认核心逐字节内嵌。

核心固定哈希：

```text
5ac710185ff59b59293620ab89ea1cc59927c506bb82de128c6e4a4885b6ca1e
```

不要重新压缩、UPX、重新签名或替换该核心后再声称“官方核心逐字节不变”。

### 3.9 Qt 资源压缩会导致“原文找不到”

`qr-notice.html` 通过 Qt 资源系统打包。资源可能压缩，所以不能简单要求完整 HTML 原文直接出现在 EXE 中，也不能因为搜索不到原文就认定网页增强丢失。

可靠做法是：

- 对比本地与远程资源源文件哈希；
- 检查必要的资源声明和运行时标记；
- 需要完全确认时，在 Windows 实机运行后验证网页行为。

官方 CHFS 核心目前在 EXE 中能逐字节匹配，但这是核心资源的验证方式，不应机械套用到所有可能被压缩的 Qt 资源。

### 3.10 两个 EXE 哈希不同、大小不同是正常现象

本地正式版与首次云端 test 版对比：

| 项目 | 本地正式版 | 云端 test 版 |
|---|---:|---:|
| 文件大小 | 38,337,536 字节 | 38,299,648 字节 |
| 大小差 | — | 少 37,888 字节 |
| DLL 集合 | 20 个 | 同样 20 个 |
| 官方核心 | 原样内嵌 | 原样内嵌 |
| 关键版本与功能标记 | 存在 | 存在 |

云端小 37,888 字节并不代表少了功能。本地 GCC 为 16.2.1，云端当时为 16.2.0；代码段、数据段、时间戳和链接布局也会变化。即使同一云端工作流再次构建，EXE 哈希也可能变化。

正确比较顺序：

1. 源码提交与资源哈希；
2. 官方核心是否原样内嵌；
3. DLL 集合和禁用 API；
4. 静态链接是否完整；
5. 版本和关键功能标记；
6. Windows 实机功能测试。

不要把“哈希不同”直接写成“构建不一致”或“功能有差异”。

### 3.11 `wlanapi.dll` 白名单警告不等于失败

本地 Win7 检查脚本可能输出：

```text
WARN non-whitelisted DLL: wlanapi.dll
RESULT: PASS
```

程序使用 Windows WLAN API 获取 Wi-Fi 信息，`wlanapi.dll` 是 Windows 系统 DLL。应核实目标系统是否原生提供及程序功能，而不是看到 WARN 就删除 Wi-Fi 功能或强行添加私有 DLL。

### 3.12 Actions 自身的运行时警告要单独处理

首次工作流使用 `actions/checkout@v4`，出现 Node.js 20 弃用提示后升级为 `actions/checkout@v5`。构件上传曾使用 `actions/upload-artifact@v4`，后来升级为 `actions/upload-artifact@v7`；改进后的运行未再出现旧 Node.js 警告。

不要凭猜测填写不存在的 action 版本。升级前查看对应官方 Releases，升级后重新运行并检查 Annotations。

### 3.13 云端检查通过不等于实机验证通过

当前自动检查覆盖：

- 编译成功；
- PE 文件头、版本标题和关键界面文字；
- 官方核心哈希及内嵌；
- 静态链接；
- Win7 禁用 API 和危险 DLL；
- SHA-256 构件校验。

它不能替代 Windows 11 / Windows 7 上的启动、共享目录、上传下载、账户权限、WebDAV、二维码地址、快捷方式和更新测试。交付时必须明确哪些已自动验证、哪些仍需实机验证。

## 4. 修改工作流时的最小回归清单

每次修改工作流后，至少重新验证一次完整构建：

- [ ] push 或手动触发指向预期提交。
- [ ] qmake、make 和静态链接成功。
- [ ] 官方核心两项检查通过。
- [ ] `verification.txt` 显示 Win7 与静态链接检查通过。
- [ ] 构件只包含需要的 EXE、校验文件和精简报告。
- [ ] `build-environment.txt` 记录完整。
- [ ] 下载后 `sha256sum -c` 通过。
- [ ] Actions Annotations 无新增且未解释的警告。
- [ ] 文件名含 `test`，未创建正式 Release。
- [ ] 回复用户时提供运行 ID、文件大小和最终 SHA-256。

## 5. 不建议重复走的弯路

- 不要先在本地重新下载整套交叉编译环境，再发现云端已有可用工作流。
- 不要把本地 Linux 交叉编译时的临时 `qt.conf`、主机 qmake 等修法机械搬进 Windows Runner；云端直接使用 MSYS2 环境。
- 不要逐个手工降级依赖来“碰运气”；先看 `build-environment.txt` 和失败步骤。
- 不要为了让断言变绿而删除核心、静态链接或 Win7 检查。
- 不要上传完整 PE 转储、构建目录或整套 MSYS2 作为构件。
- 不要修改正式更新源或发布 Release 来“测试一下”。test 构建只走 Actions Artifact。
