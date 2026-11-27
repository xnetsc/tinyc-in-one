# 全矩阵打包盘点：生态 × 宿主 × 目标

定版规则（2026-11-27）：任意平台宿主都必须能交叉编译出全部 9 个目标；
五生态（npm、Python 原生 wheel、Java jar、安卓 AAR、浏览器）同一规则；
发行按宿主平台分包，每个包自带全部 9 个目标的工具链与 sysroot；
可共用的不按组合名多发（判据见 §5）；每个包单独装上即完整工作（§5 末）。

本文是盘点与原型验证记录：现状依据仓库实物（build.sh、
vendor-tinycc/Makefile、packaging/stage_layout.py、
.github/workflows/release.yml、各 packages 的组装脚本）与本机实测；
本机验不了的宿主（mac/win/android）逐项点名留待 CI。

## 1. 维度

**目标**（arch × OS 的 9 个有效组合）：

| 目标 triple | arch × OS |
|---|---|
| x86_64-linux-gnu | x86_64 × linux |
| aarch64-linux-gnu | arm64 × linux |
| riscv64-linux-gnu | riscv64 × linux |
| x86_64-w64-mingw32 | x86_64 × win |
| aarch64-linux-android | arm64 × 安卓 |
| x86_64-linux-android | x86_64 × 安卓 |
| arm64-apple-darwin | arm64 × mac |
| x86_64-apple-darwin | x86_64 × mac |
| wasm32-wasi | wasm32（虚拟） |

不存在的组合（目标侧）：arm64 × win、riscv64 × win、riscv64 × 安卓、
riscv64 × mac——这四种组合既无目标也无宿主，不在矩阵内。

**宿主**（发包维度）：x86_64 × linux、arm64 × linux、x86_64 × win、
arm64 × mac、x86_64 × mac、arm64 × 安卓 + x86_64 × 安卓（同一 AAR）、
wasm（虚拟宿主，浏览器/Pyodide）。

空格子点名：

- **riscv64 × linux 宿主：定版排除**——riscv64 只作目标、不做宿主
  （2026-11-24 定版，2026-11-27 再次确认），不评估可行性。
- arm64 × win 宿主：不存在（同目标侧，无此组合）。

## 2. 网格总表：生态 × 宿主 → 包与现状

「带目标数」指该包当前自带的目标工具链数（满格 9）。

| 宿主 | npm | Python wheel | Java jar | AAR | 浏览器 / Pyodide |
|---|---|---|---|---|---|
| x86_64 × linux | @xnetx/tinyc-native-x86_64-linux-gnu，6/9 | manylinux_2_17_x86_64 wheel，6/9 | tinyc-x86_64-linux-gnu.jar，6/9 | — | — |
| arm64 × linux | @xnetx/tinyc-native-aarch64-linux-gnu，6/9 | manylinux_2_17_aarch64 wheel，6/9 | tinyc-aarch64-linux-gnu.jar，6/9 | — | — |
| x86_64 × win | @xnetx/tinyc-native-x86_64-w64-mingw32，1/9 | win_amd64 wheel，1/9 | tinyc-x86_64-w64-mingw32.jar，1/9 | — | — |
| arm64 × mac | @xnetx/tinyc-native-arm64-apple-darwin，1/9 | macosx_11_0_arm64 wheel，1/9 | tinyc-arm64-apple-darwin.jar，1/9 | — | — |
| x86_64 × mac | @xnetx/tinyc-native-x86_64-apple-darwin，1/9 | macosx_10_9_x86_64 wheel，1/9 | tinyc-x86_64-apple-darwin.jar，1/9 | — | — |
| arm64 × 安卓 + x86_64 × 安卓 | — | — | — | tinyc-android-release.aar（双 ABI 一包），包级 2/9、每 ABI 的 .so 各 1/9 | — |
| wasm（虚拟宿主） | — | — | — | — | tinyc-web（npm + zip）9/9 ✓；tinyc_web wheel（Pyodide）9/9 ✓ |

npm 主包 @xnetx/tinyc 是 JS API 层，靠 optionalDependencies 在一次
install 里带上当前宿主的子包（见 §5 独立性点名）。

## 3. 宿主 × 目标覆盖现状与二进制来源

| 宿主 \ 目标 | linux×3 | win×1 | 安卓×2 | mac×2 | wasm32 |
|---|---|---|---|---|---|
| x86_64 × linux | ✓ | ✓ | ✓ | ✗→§6 已补 | ✗→§6 已补 |
| arm64 × linux | ✓ | ✓ | ✓ | ✗（同 build.sh，待 CI） | ✗（同 build.sh，待 CI） |
| x86_64 × win | ✗ | ✓（仅原生） | ✗ | ✗ | ✗ |
| arm64 × mac | ✗ | ✗ | ✗ | ✓（仅原生） | ✗ |
| x86_64 × mac | ✗ | ✗ | ✗ | ✓（仅原生） | ✗ |
| 安卓（双 ABI） | ✗ | ✗ | ✓（各 ABI 仅本 arch） | ✗ | ✗ |
| wasm | ✓ | ✓ | ✓ | ✓ | ✓ |

二进制在哪条流水线构建：

- linux 宿主的 6 个：顶层 build.sh 在 linux-x64 / linux-arm64 两个 CI
  job 里构建 build/dist/<目标>——跑在构建机上的 ELF 双形态文件，
  目标为其 triple（TCC 一二进制一目标）。同一 dist 供 npm/wheel/jar
  三生态 staging（§5②）。
- win 宿主的 1 个：build.sh 的 host 子命令 build_host_mingw——Linux 上
  MinGW 交叉 gcc 构建，或 windows CI job 里原生 MinGW gcc 构建
  （dist-host/），目标 == 宿主。
- mac 宿主各 1 个：mac-test-bundle/build-mac.sh 在 macos CI job 里以
  clang 原生构建（双形态为 Mach-O + 内嵌 dylib 段形态），经 release.yml
  的 stage-dist 步组装成 dist 布局后供三生态 staging。
- 安卓的 2 个：packages/android/build.sh 的 native-libs 模式——NDK
  clang 逐 ABI 做 unity 构建（tcc.c + JNI 胶水），每个 libtinyc.so
  编译期固定一个目标（arm64-v8a 的 .so 带 -DTCC_TARGET_ARM64，
  x86_64 的 .so 带 -DTCC_TARGET_X86_64）。
- wasm 宿主的 9 个：packages/browser/build.sh 逐 TARGET 以 emcc 构建
  编译器信封（8 个通用目标用上游后端，wasm32 用 wasm-backend 后端），
  sysroot 信封由 build-sysroot-envelopes.mjs 组装。

## 4. 逐生态组装路径与补齐缺口

### npm（bundle_native.js → packaging/stage_layout.py）

staging 本身与宿主/目标数无关：stage_layout 把 dist 里「文件格式与
机器字长能在该宿主上跑」的二进制连同其 sysroot 装入。缺口全在
dist 供给侧（§6 已补 linux 侧）与两处 staging 限制：

- stage_layout 只读一个 dist 目录、只接受一个 --host；多机产物
  （mac/win job 的 dist）须先汇齐到同一 dist 再 staging（现 CI 是
  各 job 就地 staging 本机产物）。
- stage_layout 的 Mach-O 判别缺 arch：binary_machine 对 Mach-O 只
  返回格式不读 CPU 类型，arm64 的 Mach-O 二进制会被
  x86_64-apple-darwin 宿主误收。现各 mac job 只 staging 本机产物
  未触发；全矩阵合并 staging 前必须补 CPU 类型判别。

### Python 原生 wheel（bundle_native.py → stage_layout.py；
### build_wheel_matrix.sh 逐宿主出 wheel）

与 npm 共用同一 staging，缺口相同。wheel 自包含（staging 产物全量
打进 wheel），独立性 ✓。

### Java jar（packages/java/build.sh → stage_layout.py）

运行时已自选宿主：Native.java 在运行时检测宿主 triple（os.name/
os.arch，可由 TINYC_HOST_TRIPLE 覆盖），按 packaging-manifest.json
查 (host, target) 组合并解压使用。jar 的单宿主形态纯粹是组装侧
只 staging 了一个宿主。补齐缺口：多宿主 staging + 清单合并
（stage_layout 需多宿主模式），组装点移到 release 汇聚 job
（收齐 5 个宿主的 dist 后一次组装）——即 §5 的 fat jar。

### 安卓 AAR（packages/android/build.sh 自有组装，不经 stage_layout）

补齐其余 7 个目标需要四步：

1. **每 ABI × 每目标一份内核 .so**：现 build_so 按 ABI 各编一份
   单目标 .so；全矩阵是 2 ABI × 9 目标 = 18 份（NDK clang unity
   构建，逐目标换 -DTCC_TARGET_* 与 baked 路径）。wasm32 目标还须
   把 wasm-backend 的 wasm-gen.c/wasm-link.c 与其补丁并入 unity
   构建（现 JNI 构建只用 vendor 树 + 项目补丁）。
2. **Java 层目标选择**：TinycAssets.tripleForCurrentAbi 只按设备
   ABI 推导目标、无目标参数；Tinyc.java 按单一 triple 绑定
   toolchain/sysroot。须加目标入参并按目标解压/绑定（assets 布局
   toolchain/<triple> + sysroot/<triple> 本身已按 triple 分目录，
   结构可复用）。
3. **assets staging 扩容**：native-libs 里 toolchain 与 sysroot 的
   staging 循环写死两个 android triple；扩到 9 个（sysroot 来源
   与顶层 dist 对齐：linux-gnu 取 include-libc + libc、darwin 取
   vendor-darwin 头 + .tbd、win32 取 win32 树、wasm32 只取头），
   并按现机制递增 TinycAssets.ASSETS_VERSION（现为构建时临时
   bump、不回写仓库，扩容时须一并处理）。
4. **体积影响**（组件实测 + 估算）：现 AAR 1,957,003 B（build-92）。
   新增 16 份 .so ≈ +8.8 MB（未压缩，单份实测 535,672 /
   555,432 B）；新增 7 个 sysroot 子集 ≈ +6–9 MB（未压缩；单份
   组件实测：glibc 头树 1,309,677 B、bionic 头 836,490 B + crt/
   stub 约 0.83 MB、darwin 头 101,912 B + .tbd 23,909 B、win32 树
   3,288,605 B、wasm32 仅头约 0.3 MB）；新增 7 份 toolchain 资产
   ≈ +0.8 MB。压缩后 AAR 估计落在 10–14 MB 区间（估算）。

验证：android-emulator CI job（x86_64 API 30 模拟器）——现 job 只
覆盖 x86_64 ABI 的消费者测试，arm64 ABI 无模拟器覆盖，须点名。

### 浏览器（packages/browser/build.sh 逐目标 emcc + 信封）

已全矩阵 ✓，无组装缺口。一处名实不符点名：wasm32 sysroot 信封里
名为 libtcc1.a 的文件实为原生 x86_64 ELF 存档（实测 md5
9c46ec226a434e48643e65150db095ce，与 x86_64-linux-gnu dist 的
libtcc1.a 逐字节相同）——解析路径 build-sysroot-envelopes.mjs 的
findLibtcc1 对 wasm32 取了 build/wasm32-src/ 下原生 make 的副产物。
该目标本无 libtcc1.a（§6），wasm 链接路径不消费此文件（惰性无害），
但应清理：wasm32 的 libFiles 不应带这一项，或 findLibtcc1 对
wasm32 直接返回空。

### Pyodide（packages/pyodide/bundle_assets.py）

从浏览器包的 tinyc-web.zip 逐字复制 9 个编译器信封，sysroot 按与
build-sysroot-envelopes.mjs 相同的来源序解析重组——已全矩阵 ✓。
它依赖宿主提供 Pyodide 运行时（平台本身，非我方包，类同 jar 依赖
JVM），包内资产自包含。

## 5. 共用判据核查（一个包能跨组合跑 → 只发一个包）

- **Java：现 5 个 jar 可合并为一个 fat jar。** jar 内容是平台中立
  字节码 + 资源文件，运行时已自选宿主（§4）；5 个 jar 的差别只在
  装入的 natives 子集。合并后每个宿主仍只用自己那份 natives，
  且 sysroot（与宿主无关的目标数据，现复制 5 份、单份 3.3–5.0 MB）
  在 fat jar 内只存一份。前置改动：stage_layout 多宿主模式/清单
  合并 + release 汇聚组装（§4）。合并后独立性 ✓。
- **macOS 双 arch**：jar 侧由 fat jar 自然覆盖；wheel 侧可用
  universal2 平台标签把 arm64 与 x86_64 两 wheel 合为一个（pip 在
  两种 arch 的 mac 上都接受 universal2）；npm 侧两个 darwin 子包
  可合为一个（os=darwin、cpu 同列 arm64 与 x64，包内含两套
  Mach-O 二进制，index.js 已按 hostTriple() 运行时自选）。三生态
  结论均为可合并，未落地。
- **npm native 子包（跨 OS/arch 的 5 个）：必须分包。** 判据：
  npm 的交付按包的 os/cpu 字段门控，optionalDependencies 只把
  当前宿主的子包落进一次 install；子包内容是宿主原生二进制，
  在其它宿主上不可执行——「同一个包跨组合运行」在 npm 的安装
  单位层面不成立，与 jar 的单文件自选不是同一种形态。
- **原生 wheel（跨平台标签的 5 个）：必须分包。** 判据：pip 按
  wheel 平台标签（manylinux_2_17_x86_64、win_amd64、
  macosx_11_0_arm64 等）门控安装，跨平台标签的 wheel 在其它宿主
  上根本不会被安装。mac 双 arch 的 universal2 是标签层面的合并
  例外（见上）。
- **AAR 双 arch 一包：已符合，保持。** 一个 AAR 内含双 ABI 的
  jniLibs，消费端按设备 ABI 自选。
- **wasm 单包：已符合，保持。** 浏览器包与 Pyodide wheel 均为
  单包全目标。

**独立性点名**：npm 是唯一「靠包管理器一次性依赖安装才成套」的
生态——主包 @xnetx/tinyc 单独在场没有二进制、native 子包单独在场
没有 API 层，其独立单位是一次 npm install 的整体（主包 +
optionalDependencies 门控选中的子包）；主包 tarball 内嵌的本宿主
natives/sysroot 是非 registry 场景的回退，不改变这一性质。
其余生态（wheel、jar、AAR、浏览器、Pyodide wheel）均为自包含
单包，单独装上即完整工作；上述拟合并形态（fat jar、universal2
wheel、darwin 合并子包）不破坏独立性（darwin 子包仍属 npm 安装
整体的一部分，与现状同性质）。

**② toolchain 二进制跨生态同源性**：npm/wheel/jar 三生态的
staging 都由同一个 packaging/stage_layout.py 从同一棵 build/dist
产出（bundle_native.js、bundle_native.py、java/build.sh 三个入口
都调它）。同一轮构建内三生态拿到的是同一批文件的逐字拷贝
（实测：本机 dist 的 bin/tinyc 与 npm staging 副本 md5 同为
04183a58645959c11923b3bb999923c4、mtime 相同）；但每个生态各
staging、各嵌入一份物理副本，没有跨生态共用的单一 staging
产物——dist 在两次 staging 之间被重建时副本即分叉（实测：
python staging 副本为更早一轮 dist 的拷贝，md5 不同）。即
「同源同批」只在同一轮构建内成立，产物层面仍是三份。

## 6. Linux 宿主补齐 3 目标：原型验证（本 worktree 已集成）

本机（x86_64 × linux）实测，worktree 的 build.sh 已落地并验证：

### darwin ×2（arm64-apple-darwin、x86_64-apple-darwin）

- 构建方式现成：vendor Makefile 已有 cross 目标
  `cross-arm64-osx` / `cross-x86_64-osx`（Makefile:136-137 TCC_X
  含 x86_64-osx 与 arm64-osx；DEF-arm64-osx = $(DEF-arm64) +
  -DTCC_TARGET_MACHO，文件表含 tccmacho.c），产出跑在本机的 ELF
  交叉编译器（实测 arm64-osx-tcc 482,624 B、x86_64-osx-tcc
  753,776 B）与 <arch>-osx-libtcc1.a。无需新构建参数。
- libtcc1.a 的成员是 ELF 对象——上游设计（tcc.h:231-232
  ELF_OBJ_ONLY：Mach-O 目标写 ELF .o、最终链接时由 Mach-O 写入器
  消费），与 mac 原生构建的 dist 同形态，不是缺陷。
- sysroot：vendor-darwin 自写头（42 个）作 include-libc、
  vendor-darwin/lib 的 5 个 .tbd 存根作 lib/，由
  mac-test-bundle/darwin-dist.py 的 stage 子命令组装（与 mac 包
  同一条路）；Mach-O 无 crt 对象。
- 集成：build.sh 新增 build_darwin（build_one 的 cross 形态 +
  darwin staging + -DTCC_TARGET_MACHO 与包内路径宏），实跑
  `./build.sh arm64-apple-darwin x86_64-apple-darwin` exit 0。
- 编链验证：dist/bin/tinyc（不带 -B、靠 dist 自身路径）编链
  hello.c（stdio + math.h 的 sqrt）两目标均 exit 0；file 输出
  `Mach-O 64-bit arm64 executable, flags:<|DYLDLINK|PIE>` 与
  x86_64 对应行；llvm-objdump 核验 hello-arm64：LC_MAIN
  （entryoff 4096）、LC_LOAD_DYLIB 指向
  /usr/lib/libSystem.B.dylib（.tbd 的 install name）、_main 已
  定义、_printf/_sqrt 为 UND 导入；反证：引用未声明符号
  nosuchfn_xyz 时链接 exit 1 报 `unresolved reference`，
  .tbd 闭合真实生效。Mach-O 产物在本机跑不了，属预期。

### wasm32-wasi

- 构建方式：wasm-backend/build.sh 已在本机构建 wasm32-tcc
  （vendor 副本 + wasm-gen.c/wasm-link.c + 后端补丁，configure
  后改 config.h 守卫加 TCC_TARGET_WASM32，原生 make 后
  `make wasm32-tcc`），exit 0；其全回归套件实跑 exit 0、
  `regression result: PASS`（37 个阶段行）。
- 双形态集成探针：项目补丁与 wasm-backend 补丁在同一棵树上
  顺序叠加（项目补丁 → wasm 源码 → wasm 补丁）exit 0、无冲突；
  unity 编译 tcc.c + tcc_io.c（-DTCC_TARGET_WASM32）exit 0，
  导出 tinyc_main 与 tcc_new / tcc_compile_string 等 tcc_* API。
- sysroot 结论：头 = wasm-backend/include 的 14 个（C 库声明 +
  4 个 SIMD 兼容头等）+ vendor include 的 TCC builtin 集，
  合计 25 个，与浏览器 wasm32 sysroot 清单的头集一致；
  **libtcc1 无**——该目标在后端模型里运行时 helper 是宿主
  import（ASSESSMENT.md §4.1），dist 清单不带 libtcc1 字段；
  stage_layout 的 wasm32 组合因此没有 libPath（包运行时若要为
  无 libtcc1 的目标设 libPath，须在 staging/清单层定默认规则，
  列入 §7）。
- 集成：build.sh 新增 build_wasm32（源码组装同探针），实跑
  `./build.sh wasm32-wasi` exit 0。
- 运行验证：自写 standalone 程序（i64 斐波那契、整数开方、
  double 运算）经 wasm32-tcc 与经新 dist 的双形态 tinyc 两路
  编译均 exit 0、产物同为 1,271 B；在 Node 下零 import 实例化
  真跑，断言全过（fib_ll(20)=6765、isqrt(1000)=31、
  dsum(2.0,1.5)=6.5），node exit 0。

### staging 总装演示（x86_64 × linux 宿主）

worktree 的 build/dist 凑齐 9 个 dist（6 个为既有构建产物、
3 个为本次新构建）后：stage_layout 对 x86_64-linux-gnu 一次
staged 9/9 组合；bundle_native.js、bundle_native.py 重跑后
各自 packaging-manifest 均含 9 组合；
tests/check_outputs_consistency.py 对新目标无 outputs 不一致
（darwin 的 Mach-O 族、wasm32 的 module 族与浏览器清单逐字
相同），仅剩「jar 未构建」一项（本机无 JDK，jar 只在 CI 构建）。

arm64 × linux 宿主：同一 build.sh 在 aarch64 机上产出 aarch64
ELF 的 darwin/wasm32 交叉二进制，构建路径与宿主 arch 无关的
部分已在 x86_64 上验通；aarch64 宿主实机构建留待 CI
linux-arm64 job。

## 7. 其余宿主补齐路径与 CI 改动（本机不可验，留待 CI）

- **mac 宿主 ×2**：泛化 mac-test-bundle/build-mac.sh——clang 按
  其余 8 个目标的 defines 做双形态组装（现只组装原生目标），
  每目标 libtcc1 可由同树 `make cross-*` 产出（mac 上同理），
  或复用与宿主无关的目标数据；前置：stage_layout 补 Mach-O
  CPU 判别（§4）。验证：macos CI job 真机构建 + 现有 sdkfree
  电池扩跨目标编链用例（产物在本机只能验格式与闭合，原生目标
  之外不能跑）。
- **win 宿主**：泛化 build_host_mingw——MinGW gcc 按其余 8 个
  目标 defines 编 PE 双形态（导出表生成 gen_windows_def 与目标
  无关）；libtcc1/sysroot 复用目标数据。验证：windows CI job
  真机运行 + feature smoke 扩跨目标。
- **安卓宿主**：按 §4 AAR 四步落地；验证：android-emulator job
  （仅 x86_64 ABI），arm64 ABI 点名无覆盖。
- **CI 改动汇总**：linux 两 job 改跑全矩阵 build.sh（本机实测量
  级：两 darwin 目标约 108 秒、wasm32 约 30 秒，共享 vCPU 仅作
  参考）；mac/win/android 三 job 各自新增跨目标构建与冒烟；
  release job 若采纳 fat jar 需新增汇聚组装步。

## 8. 本 worktree 的提交

- build.sh：print_outputs 新增 macho 与 wasm 两族；新增
  build_darwin、build_wasm32；ALL 与 case 分发纳入
  arm64-apple-darwin、x86_64-apple-darwin、wasm32-wasi；头部
  注释订正（darwin 目标在本机构建、darwin 宿主变体仍只在
  macOS 构建）。
- packages/npm/packaging-manifest.json、
  packages/python/tinyc/packaging-manifest.json：对
  x86_64-linux-gnu 宿主重跑 bundle 后的 9 组合清单（生成物）。

## 9. 剩余缺口清单

1. stage_layout 的 Mach-O CPU 判别缺失——合并 staging 前必补。
2. stage_layout 单 --host、无多宿主清单合并——fat jar 前置，
   未落地（方案见 §5）。
3. wasm32 组合无 libPath——包运行时对无 libtcc1 目标的
   libPath 规则待定。
4. mac/win/android 三宿主的跨目标构建未实现（路径已明确，
   须在各自 CI job 落地与验证；本机不可验，未假装验过）。
5. AAR 的 Java 层目标选择 API 未实现；AAR 全矩阵体积为估算
   （§4，组件实测、总量估算）。
6. 浏览器 wasm32 sysroot 信封内的 libtcc1.a 名实不符
   （§4，惰性无害，待清理）。
7. jar 未在本机验证（无 JDK）；fat jar 为方案，未落地。
