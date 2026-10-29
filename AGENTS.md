# Agent 操作约定

本文件适用于整个仓库。修改源码、工作流或构建 Windows EXE 前，先阅读本文件；完整流程和已踩过的坑见 [docs/windows-cloud-build.md](docs/windows-cloud-build.md)。

## 仓库边界

- 完整修改版源码仓库：`lu920115/chfsgui`。
- 正式 EXE 在本仓库和 `lu920115/chfs-gui-qrcode` 各保存一份；Releases 和默认更新源仍使用 `lu920115/chfs-gui-qrcode`。
- 不向上游 `docblue/chfsgui` 推送，不创建上游 PR。
- 云端 test 构件不是正式 Release，不修改更新源，不把 test 当作已发布版本。
- 不索取用户密码或 Token。需要 GitHub 授权时使用官方设备授权流程。

## 默认构建方式

优先复用现有工作流：`.github/workflows/build-windows-test.yml`，不要另起一套构建系统。

- 平台：GitHub Actions `windows-latest` + MSYS2 `MINGW64`。
- 目标：Windows x64 单文件静态 EXE，同时保持 Windows 7 兼容路线。
- 推送 `chfsgui/**` 或工作流文件会自动触发；也可用 `workflow_dispatch` 手动触发。
- 产物名：`CHFS3.1_GUI_Qrcode_v<版本>_test.exe`。
- 构件名：`chfsgui-v<版本>-test`，保留 14 天。
- 下载后必须核对 SHA-256，并重新检查 PE 导入、内嵌核心和关键版本标记。

正式构建使用 `.github/workflows/build-windows-exe.yml`，只手动触发。它从本仓库编译并向本仓库 `master` 提交正式 EXE 与 SHA-256 文件，不自动创建 Release。随后由 `lu920115/chfs-gui-qrcode` 的同步工作流下载、校验并提交同一份文件到 `main`，不重复编译，也不需要跨仓库 Token。

## 不可随意改变的约束

1. **不要切换 UCRT64。** 当前使用 MINGW64/MSVCRT 是为了 Windows 7；消除弃用提示不能成为切换运行库的理由。
2. **不要修改或替换官方 CHFS 3.1 核心。** `chfsgui/chfs_resource/chfs.exe` 的 SHA-256 必须为：
   `5ac710185ff59b59293620ab89ea1cc59927c506bb82de128c6e4a4885b6ca1e`。
3. 保留 `-static -static-libgcc -static-libstdc++`，最终交付不依赖 Qt、OpenSSL、libstdc++、libgcc、winpthread 等外部 DLL。
4. 禁止出现 `GetSystemTimePreciseAsFileTime`、`ProcessPrng`、`api-ms-win-*`、`ucrtbase.dll`、`bcryptprimitives.dll` 等 Win7 不兼容依赖。
5. `wlanapi.dll` 是程序使用的 Windows 原生 WLAN API；检查工具可能给出白名单警告，但不能仅凭这条警告判定失败。
6. 不为了“让检查通过”删除功能、绕过断言或关闭核心验证。检查脚本有误时修正检查方法，并说明原因。

## 验证注意事项

- 不要用两个 EXE 的 SHA-256 是否相同判断源码或功能是否一致。编译器小版本、时间戳和链接布局都会改变哈希与文件大小。
- 不要查找 `DesktopShortcuts` 等 C++ 枚举名判断功能是否存在，名称可能在编译后被剥离。应检查实际界面文字或做运行测试。
- `QStringLiteral` 标题常以 UTF-16LE 出现，部分 `tr()` 源文字以 UTF-8 出现；二进制字符串检查需选对编码。
- Qt 资源可能被压缩，不能要求网页 HTML 原文一定直接出现在 EXE 字节流中。
- 不上传完整 `objdump -p` 输出作为常规报告；它可能超过 20 MB，且绝大部分不是导入表。
- 云端编译和静态检查通过不等于 Windows 实机测试通过。未经实机验证时必须明确说明。

## 完成标准

一次云端 test 构建只有同时满足以下条件才算完成：

1. Actions 所有步骤成功，无未解释的警告。
2. 官方核心哈希及“核心已逐字节内嵌”检查通过。
3. 静态链接与 Win7 导入检查通过。
4. test EXE 已下载到本地，SHA-256 与构件校验文件一致。
5. 记录运行 ID、最终文件大小和 SHA-256。
6. 明确区分“自动检查通过”和“Windows 11 / Windows 7 实机测试通过”。
