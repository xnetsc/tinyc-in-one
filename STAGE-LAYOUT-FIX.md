# staging 层两处缺口修复（全矩阵合并前置）

基线：96109db（Linux 宿主 9/9 已合入）。只改 `packaging/stage_layout.py`，加
`packaging/test_stage_layout.py`，两份清单随 bundle 重跑更新。未改 release.yml、
未碰其它生态的包构建脚本。

## 改了什么

### 1. Mach-O CPU 类型识别

`binary_machine()` 之前对 Mach-O 只认文件头魔数、不读 cputype，`host_accepts()`
对 Mach-O 只看宿主名里有没有 apple/darwin——arm64 的 Mach-O 编译器会被
x86_64 mac 宿主误收（反之亦然）。

现在：

- 读 Mach-O 头的 cputype（小端/大端两种魔数都处理；`CPU_TYPE_X86_64 =
  0x01000007`，`CPU_TYPE_ARM64 = 0x0100000C`），thin 二进制返回一个 arch，
  fat（universal）二进制返回它包含的 arch 集合；cputype 不认识时返回
  None，且宿主永不收 None（不明 CPU 不盲目入包）。
- `host_accepts()` 对 Mach-O 按宿主 arch 精确匹配：x86_64 mac 宿主只收
  x86_64 Mach-O，arm64 mac 宿主只收 arm64 Mach-O；Linux 宿主不收任何
  Mach-O；32 位 Mach-O 不在布局内，仍不认。

注意：Linux 构建机上的交叉编译器全是 ELF（跑在构建机上、只负责输出
Mach-O），所以这项改动不影响 Linux 宿主的 staging；它只在 macOS 构建机
（编译器本体就是 Mach-O）上起作用。

### 2. 多宿主 staging 模式

- `--host` 可重复，`--dist` 也可重复。一次调用把每个给定宿主都 stage
  进同一个 root，写一份合并清单：每个组合的归属宿主就是它自己的
  `"host"` 字段，清单顶层新增 `"hosts"` 列出已 stage 的宿主（按命令
  行顺序）。
- dist 按给定顺序逐个查：对每个（宿主，目标），取第一个含有能在该宿主
  上跑的编译器二进制的 dist；sysroot 取自同一份 dist。形态：交叉编译
  dist 在先（跑在构建机上的编译器），宿主变体 dist（`build/dist-host`，
  跑在目标平台上的编译器，如 mingw32 的 PE `tinyc.exe`）在后。
- 单宿主调用行为不变：只传一个 `--host` 时清单内容与改前一致（组合
  对象逐字节相同，只多一个顶层 `"hosts"` 字段）；单个宿主 stage 出 0
  个组合时仍报错退出；多宿主时某个宿主 0 组合只告警、不致命（其余宿
  主照常），全部 0 组合才报错。

### pending 的账（run 45939297008 核查记的 `9 combinations, 4 pending`）

`"pending"` 是**宿主位**的账，不是目标组合的账：每项是一个尚未在本 root
内 stage 的计划宿主（`host == target` 的原生组合位）。单宿主跑一次，
stage 1 个宿主、pending 剩下 4 个；汇聚步把 5 个宿主全 stage 完，
pending 为空。组合归属看各组合的 `"host"` 字段；`"hosts"` 是已 stage
宿主的汇总。

## 怎么验的

`python3 packaging/test_stage_layout.py`：9 项全过。

- Mach-O：合成 thin 头（大小端两种魔数、两 arch）与 fat 头（双 arch /
  单 arch），`file(1)` 独立核对合成头确为 `Mach-O 64-bit arm64` /
  `Mach-O 64-bit x86_64`；`host_accepts` 矩阵（两 mac 宿主 × 两 arch ×
  fat × 未知 cputype × Linux 宿主）全断言；合成 dist（两 mac 目标各配
  对应 arch 的 Mach-O 编译器 + 真实 dist 清单）对两 mac 宿主各跑一次
  staging，错收与漏收都断言（错 arch 的二进制文件也不得出现在
  `natives/<host>/` 下）。
- Linux 9/9 不变：真实 dist（tinyc-cross-matrix 的 `build/dist`）对
  x86_64-linux-gnu 单跑仍 9 组合、全 included、pending 4；`bundle_native.js`
  与 `bundle_native.py` 各重跑一次，两份清单仍 9 组合、diff 只有新增的
  `"hosts"` 字段（组合对象逐字节不变）。
- 多宿主 = 逐宿主单跑之并：合成多宿主 dist（Linux 交叉编译器 + Mach-O
  两 mac 编译器 + PE mingw32 编译器）上，四宿主合跑与逐宿主单跑的组合
  对象逐项相等，pending 只剩 1（aarch64-linux-gnu）；`--host` 重复传
  同一宿主会去重。
- 多 dist：真实 dist（9 个 Linux 交叉编译器）+ 真实 dist-host（含 PE
  mingw32 编译器与 aarch64 原生编译器）合跑 5 宿主，得 11 组合
  （linux 9 + aarch64 原生 1 + mingw32 PE 1，darwin 0——此处无 Mach-O
  二进制可取，pending 如实记 2 个 darwin 宿主位）；mingw32 stage 进
  去的是 PE 文件（MZ 头核对）。
- 反验：`git stash` 回旧代码跑同一套测试，7/8 失败（唯一通过的是
  Linux 9/9，旧代码那面本来就没坏）——测试对这两处缺口确有判别力。
- 现有检查：`tests/check_outputs_consistency.py` 对 9 个目标全 OK
  （dist 与包清单 outputs 一致）；唯一 FAIL 是 Java jar 未构建（本
  机无 JDK、与本次改动无关，CI 覆盖）。

## 剩余缺口

- darwin 宿主的跨目标构建还没实现：macOS 构建机上目前每个目标只有
  本机 arch 的 Mach-O 编译器，另一 arch 的编译器要等 mac job 的交叉
  构建落地；届时本修复的 CPU 匹配才会在真机 CI 里被跑到——本机只能
  用合成 Mach-O 验逻辑。
- win 宿主同理：9 个 PE 编译器（每目标一个）要等 win job 的交叉构建；
  目前只有 mingw32 目标有 PE 形态（dist-host）。
- 汇聚步的 dist 供给：多宿主模式吃的是 dist 清单（可多个），各 CI job
  须把自己的 dist / dist-host 作为制品交上来，汇聚步按「交叉 dist 在
  先、宿主变体 dist 在后」传 `--dist`；这一步在 release.yml 里接线，
  不在本提交内。
- `aarch64-linux-gnu` 宿主的交叉编译器（跑在 aarch64 Linux 上的 9 个
  编译器）目前只在 linux-arm64 job 的 dist 里，未进过汇聚验证。
