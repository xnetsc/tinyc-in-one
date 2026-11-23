# TinyC wasm32 编译目标后端 — 可行性评估与骨架说明

对象：上游 TinyCC 0.9.28rc（vendor-tinycc，git HEAD `5cef7f3`）。
目标：新增 `wasm-gen.c + wasm-link.c`，目标 triple `wasm32-wasi`，宿主为本地平台的交叉编译器。
本文所有行号均为 vendor-tinycc 树内的真实行号；骨架代码在 `src/`，构建脚本 `build.sh`，补丁 `patches/apply-patches.py`，验证在 `tests/`。

---

## 1. 结论总览

- **可行**。TCC 的后端接口是编译期静态绑定的函数集（tcc.h:1631-1658 的 `xxx-gen.c` 区 + tcc.h:1606-1628 的 `xxx-link.c` 区），没有任何插件注册机制，新增目标 = 新增两个文件 + 在四处包含/分发链上加分支（见 §5）。riscv64 是最近加进来的目标，其改法可逐处对照。
- 骨架已按此接口实现并**构建成功**（`make wasm32-tcc`），能把整数子集 C 编成可被 Node 实例化执行的 .wasm（见 §9 的实测结果）。
- 三个与原生后端本质不同的点，先说清：
  1. **输出层整体旁路**。TCC 内部仍用 ELF section/符号表组织中间结果，但最终写出由 `wasm_output_file()` 接管（tccelf.c:3223 的 `tcc_output_file` 加分发），ELF 的 `tcc_output_elf`（tccelf.c:2678）、重定位（tccelf.c `relocate_sections` 等）全部不走；输入 .o 也不支持（单趟全程序输出）。
  2. **控制流**。WASM 是结构化控制流、无任意跳转；骨架采用基本块分发方案的第一步：函数体包一层 `block $exit`，return 跳转以 `br $exit` 发射（见 §6）。条件分支等其余控制流在骨架里是明确报错的桩，不是假装支持。
  3. **前端没有向量类型**。上游这个版本根本没有 vector 类型（实测见 §8.1），SIMD 的后端发射已按映射表落进骨架，但要等前端补上向量类型后才能从 C 源码到达。

---

## 2. gen 后端接口核对（tcc.h:1631-1658）

`libtcc.c:24-55` 的 ONE_SOURCE 包含链按目标宏二选一地 `#include "<target>-gen.c"` 和 `"<target>-link.c"`；tcc.h:367-390 在定义 `TARGET_DEFS_ONLY` 时再包含一遍，只取其中的宏定义部分。因此每个 `*-gen.c` 文件都是两段结构：`#if defined TARGET_DEFS_ONLY` 给宏，`#else` 给实现（对照 riscv64-gen.c:1-90）。

### 2.1 TARGET_DEFS_ONLY 段必须给出的宏（对照 riscv64-gen.c / i386-gen.c）

| 宏 | riscv64 出处 | wasm 取值 | 说明 |
|---|---|---|---|
| `NB_REGS` | riscv64-gen.c:4（19） | 18（0-7 整数、8-15 浮点、16/17 为 fp/sp 伪寄存器） | |
| `RC_INT` / `RC_FLOAT` | riscv64-gen.c:11-12 | `1<<0` / `1<<1` | 寄存器类位 |
| `RC_R(x)` / `RC_F(x)` | riscv64-gen.c:13-14 | `1<<(2+x)` / `1<<(10+x)` | |
| `RC_IRET` / `RC_IRE2` / `RC_FRET` | i386-gen.c:39-41 | `RC_R(0)` / `RC_R(1)` / `RC_F(0)` | tccgen.c:282 的 `RC_RET()` 据此选返回值寄存器 |
| `REG_IRET` / `REG_IRE2` / `REG_FRET` | i386-gen.c:57-59 | 0 / 1 / 8 | tccgen.c:2107 起的通用 `gen_opl`（PTR_SIZE==4 时启用）在 2111-2112 行引用这两个宏 |
| `PTR_SIZE` | riscv64-gen.c:24（8） | 4 | 连带决定 tcc.h:416 起 `ElfW` 取 32 位、`addr_t` 为 32 位 |
| `LDOUBLE_SIZE` / `LDOUBLE_ALIGN` | riscv64-gen.c:26 | 8 / 8 | long double 降为 f64（定版要求） |
| `MAX_ALIGN` | riscv64-gen.c:29 | 8 | tccgen.c:4114 引用，无此宏直接编译失败（骨架第一轮构建实测） |

另有数据符号两个（tcc.h:1631-1632 声明）：

- `target_machine_defs`（riscv64-gen.c:47）：wasm 给 `"__wasm__\0__wasm32__\0__WASI__\0__wasm32_wasi__\0"`。
- `reg_classes[NB_REGS]`（riscv64-gen.c:63）：伪寄存器 16/17 的类位仿 riscv 的 ra/sp 写 `1 << 本位`，不带 RC_INT/RC_FLOAT，保证 `get_reg` 永不分配到它们。

### 2.2 必须实现的函数（tcc.h:1634-1658 逐项）

| 函数（签名以 tcc.h 为准） | tcc.h 行 | 骨架状态 |
|---|---|---|
| `void gsym_addr(int t, int a)` | 1634 | **真实现**（链遍历，见 §6） |
| `void gsym(int t)` | 1635 | 不实现——由 tccgen.c:173 通用实现，它转调 `gsym_addr`。骨架曾误定义导致重定义报错，已删 |
| `void load(int r, SValue *sv)` | 1636 | **真实现**（整数/浮点标量 + v128 分支，见 §3、§8.4） |
| `void store(int r, SValue *v)` | 1637 | **真实现**（同上） |
| `int gfunc_sret(CType *vt, int variadic, CType *ret, int *align, int *regsize)` | 1638 | **真实现**（≤8 字节按 1-2 个 i32 寄存器返回；>8 返回 0 走隐式指针，prolog 对此报错） |
| `void gfunc_call(int nb_args)` | 1639 | **真实现**（直接调用；间接调用报错桩） |
| `void gfunc_prolog(Sym *func_sym)` | 1640 | **真实现**（可变参数函数报错桩） |
| `void gfunc_epilog(void)` | 1641 | **真实现**（组装最终函数体） |
| `void gen_fill_nops(int)` | 1642 | **真实现**（发射 wasm nop 0x01） |
| `int gjmp(int t)` | 1643 | **真实现**（return 链，见 §6） |
| `void gjmp_addr(int a)` | 1644 | 桩（报错） |
| `int gjmp_cond(int op, int t)` | 1645 | 桩（报错）——条件分支未实现 |
| `int gjmp_append(int n, int t)` | 1646 | 桩（报错） |
| `void gen_opi(int op)` | 1647 | **真实现**（i32 全套算术/移位/比较 + v128 分支） |
| `void gen_opf(int op)` | 1648 | 桩（报错）——浮点算术未实现 |
| `void gen_cvt_ftoi(int t)` / `gen_cvt_itof(int t)` / `gen_cvt_ftof(int t)` | 1649-1651 | 桩（报错） |
| `void ggoto(void)` | 1652 | 桩（报错） |
| `void o(unsigned int c)` | 1654 | **真实现**（单字节发射，见 §3） |
| `void gen_vla_sp_save(int addr)` / `gen_vla_sp_restore(int addr)` / `gen_vla_alloc(CType *type, int align)` | 1656-1658 | 桩（报错）——VLA/alloca 未实现，设计上应调 `$__stack_pointer`，见 §6 |
| `void gen_cvt_csti(int t)` | 按目标节声明（riscv 节 tcc.h:1734） | **真实现**（字节/短整的符号/零扩展，整数代码常用） |
| `void gen_cvt_sxtw(void)` | 按目标节声明 | 桩（报错，64 位对才需要） |
| `void gen_va_start(void)` / `gen_va_arg(CType *t)` | 按目标节声明（riscv 节 tcc.h:1731-1732） | 桩（报错）——可变参数不支持。注：tccgen.c:6079 的 `__builtin_va_start` 分发只对 RISCV64/X86_64 写了分支，wasm 走不到这两个函数，但声明与定义仍备齐以防引用 |
| `void gen_clear_cache(void)` | 按目标节声明 | **真实现**（空操作，wasm 无 icache） |
| `void gen_opl(int op)` | 按目标节声明 | 不实现——PTR_SIZE==4 时 tccgen.c:2107 自带通用实现（寄存器对分解），再定义会冲突 |

补丁在 tcc.h 的 riscv 节（tcc.h:1729-1737）之后新增 wasm 节，声明 `gen_va_start / gen_va_arg / gen_clear_cache / wasm_output_file`（见 §5 补丁 3）。

### 2.3 gen 契约里三个容易踩的点（骨架实测踩过，有案底）

1. **`gen_opi` 要自己 gv + 弹栈**。tccgen.c 的 `gen_op` 通用路径（tccgen.c:2504）不预先把操作数装入寄存器；riscv 的做法（riscv64-gen.c:1108-1116）是 `gv2(RC_INT, RC_INT)` 后 `vtop -= 2; d = get_reg(RC_INT); vtop++; vtop[0].r = d;`。骨架照此实现，常量操作数由 gv 经 `load` 装入寄存器（wasm 无立即数形态可利用，接受这一点）。
2. **`gfunc_call` 尾部契约**：`vtop -= nb_args + 1`（riscv64-gen.c:786），返回值由 tccgen 在调用点（tccgen.c:6483 之后）用 `vsetc(&ret.type, ret.r, ...)` 自己压栈，后端只需保证返回值已在 REG_IRET/REG_FRET 里。
3. **调用前必须 `save_regs(nb_args + 1)`**（tccgen.c:1398）。它把函数+参数组之下的存活寄存器值溢出到内存 home 并改写其 SValue。wasm 的寄存器是 per-function 的 locals，callee 无法破坏它们，但**本函数自己的返回值写入 REG_IRET 会覆盖恰好住在 reg 0 的存活中间结果**——实测 `y + y - add(3,4)` 得 0（应为 11），加上 save_regs 后正确。此条是 wasm 模型下 save_regs 仍然必需的准确原因。
4. **符号生命周期**：`Sym*` 在 `tccgen_finish`（tccgen.c:446）后被 dynarray 释放，输出阶段再经 `elfsym()` 读它就是 use-after-free（ASan 实测定位，栈见 §9）。骨架的注册表与 fixup 全部改存名字副本，输出时按名字走 `find_elf_sym`（tcc.h:1569）查 symtab section。
5. **函数签名返回类型的位置**：不在 VT_FUNC 类
...[truncated 20867 chars]型本身，在 `ft->ref->type`（tccgen.c:8819 `func_vt = sym->type.ref->type`）。骨架曾据 VT_FUNC 算签名，返回值类型全错，已按此修正。

---

## 3. gen 骨架的发射模型（src/wasm-gen.c）

- **寄存器 = wasm locals**：每函数 locals 布局（参数之后）为 i32 寄存器 0-7、f64 寄存器 8-15、i32 组 43 个（伪 fp/sp 槽、`$fp`、调用 scratch 40 个）、v128 槽 4 个；声明在 wasm-link.c 的 code section 写出，两边布局常量必须一致（已注释互指）。
- **`o()` 的双写**：每发射一个字节，同时写入当前函数体缓冲和 ELF text section 镜像（保持 `ind` 语义，tccgen.c:8856 的 st_size 修补等逻辑照常工作）。跳转链的 link word 只写镜像、不进 wasm 体（§6）。
- **帧与栈**：参数由 prolog 按 `gfunc_set_param` 给的负偏移存入帧（`$fp + c` 寻址，与 riscv64-gen.c:807-885 同构）；帧大小在 epilog 才知道（`loc` 定型），故函数 setup 字节在 epilog 回填组装：`block` + setup（`$fp` = 旧 SP、全局 SP 减帧长）+ 函数体 + `end block` + 恢复 SP + 取 REG_IRET 返回。栈在线性内存里向下生长，全局 0 是可变全局 `$__stack_pointer`。
- **load/store**（对照 riscv64-gen.c:251/408）：VT_LVAL 时先 `wasm_push_addr`（VT_LOCAL/LLOCAL 走 `$fp+c`、寄存器间接走寄存器、VT_SYM 走待修补常量、VT_CONST 即地址本身），再按类型尺寸选 load/store opcode（1/2/4 字节整数、f32/f64）；非 LVAL 处理 VT_CONST（含浮点常量位模式、VT_SYM 地址）、VT_LOCAL（取地址）、寄存器拷贝。VT_CMP/VT_JMP/VT_JMPI 的 load 明确报错——骨架从不产生 VT_CMP（比较在 gen_opi 里直接物化成 0/1 寄存器值，与 riscv 的惰性 VT_CMP 不同，这是为绕开 gjmp_cond 而作的已声明分歧）。
- **gen_opi**：i32 的 `+ - * / % & | ^`、TOK_SHL/SAR/SHR、TOK_UDIV/UMOD/PDIV、比较 TOK_EQ/NE/ULT/UGT/ULE/UGE/LT/GT/LE/GE 全部真发射；除法按 vtop 类型符号性选 div_s/div_u（wasm 除零 trap、C 是 UB，语义差已注记）。浮点走 gen_opf 桩。
- **gfunc_call**：实参逐个 `load` 到 scratch locals 后按序压栈，`call` 指令的操作数用 5 字节填充 LEB 占位、输出时按名修补为最终函数索引；未定义符号自动登记为 import（模块名按 WASI 名单选 `wasi_snapshot_preview1`，其余 `env`）。调用前 `save_regs(nb_args + 1)`（§2.3 第 3 条）。

---

## 4. 输出层核对与 wasm-link 的替代关系

### 4.1 被替代的部分

| 上游位置 | 作用 | wasm 处理 |
|---|---|---|
| tccelf.c:3223 `tcc_output_file` | 输出总分发（OBJ→elf_output_obj、PE→pe、MachO→macho、其余→elf_output_file） | 补丁 5 插入 `TCC_TARGET_WASM32` 分支，全部分发到 `wasm_output_file()` |
| tccelf.c:2678 `tcc_output_elf` / 2794 `tcc_output_binary` / 2821 `tcc_write_elf_file` | ELF/裸二进制写出 | 不走 |
| tccelf.c `relocate_sections` / `relocate_syms`（tcc.h:1574-1575）及 `relocate()`（tcc.h:1628，riscv64-link.c:203 实现） | 输入重定位处理 | wasm-link 的 `relocate()` 为报错桩：输入 .o 不支持 |
| tccelf.c:3268 起 `tcc_object_type` / `tcc_load_object_file` | 按 ELF 头 + `e_machine != EM_TCC_TARGET` 校验输入对象 | wasm 目标下输入 .o 本就不通；`EM_TCC_TARGET` 在 wasm-link.c 顶部定义为私有值 `0x5741`（'WA'），仅供此校验编译通过，wasm 输出从不写 ELF 头 |
| GOT/PLT 一套：tcc.h:1609-1622 的 `code_reloc` / `gotplt_entry_type` / `create_plt_entry` / `relocate_plt`（riscv64-link.c:27/69/114/131） | 动态链接支持 | 全部桩：`code_reloc` 返 0、`gotplt_entry_type` 返 `NO_GOTPLT_ENTRY`、`relocate_plt` 空操作、`create_plt_entry` 报错。另有一组 tccelf.c 编译期引用的常量宏（`R_DATA_32`、`R_DATA_PTR`、`R_JMP_SLOT`、`R_GLOB_DAT`、`R_COPY`、`R_RELATIVE`、`R_NUM`、`ELF_START_ADDR`、`ELF_PAGE_SIZE`、`PCRELATIVE_DLLPLT`、`RELOCATE_DLLPLT`，对照 riscv64-link.c:6-18）必须在 link 文件顶部给出，wasm 给私有互异值 0x5700 起——第一轮构建曾因缺这组宏与 `MAX_ALIGN` 报一串未声明错误，是真实的接口组成部分 |
| libtcc1（`lib/libtcc1.c`） | 目标运行时支持库（64 位除法、alloca 等 helper） | 未构建：`make wasm32-tcc` 不依赖它（Makefile:280 的 `%-tcc` 规则与 Makefile:329 的 `%-libtcc1.a` 是分开的目标）。i32/i64 基本运算 wasm 都有原生指令，骨架子集不引用 helper；将来若前端产生 `__divdi3` 类调用，会以 `env` import 形式出现在模块里，由宿主补或后续自举一份 wasm 版 libtcc1 |

### 4.2 wasm_output_file 的模块组装（src/wasm-link.c，真实现）

- 函数索引分配：imports 在前、定义在后；type section 对签名去重（参数/返回值只含 i32/i64/f32/f64/void，结构体与可变参数在登记时报错）。
- section 顺序齐全：type(1)、import(2)、function(3)、table(4)、memory(5)、global(6，`$__stack_pointer` 初值 = 数据段末 + 64 KiB 栈空间)、export(7，非 static 定义按名导出 + `memory`)、element(9，表以全部函数恒等填充，`call_indirect` 的地基)、code(10)、data(11)。
- 静态数据布局：`.rodata`/`.data`/`.bss` 按 section 名匹配，从线性内存 1024 起 16 字节对齐依次排布，地址 = 基址 + `st_value`；符号地址 fixup（函数符号解析为其表索引、数据符号解析为内存地址 + addend）在输出时按名字查 symtab 完成。
- LEB128 与 section 结构在 wasm-gen.c 的 `WBuf` 工具里（最小 LEB 与 5 字节填充 LEB 两个版本，后者保证修补不改变尺寸），模块写出全部走它，无占位实现。

---

## 5. 新增目标要改的位置（configure / Makefile / config.h）

骨架的 `patches/apply-patches.py` 对副本树做 7 处精确字符串替换（每处断言出现次数，不匹配即停），与上游位置一一对应：

| # | 文件与位置 | 改动 |
|---|---|---|
| 1 | tcc.h:163-165 | 宿主默认目标回退条件加 `&& !defined(TCC_TARGET_WASM32)`。不改则在 x86_64 宿主上命令行的 `-DTCC_TARGET_WASM32` 会被回退逻辑再定义出 `TCC_TARGET_X86_64`，双目标冲突 |
| 2 | tcc.h:386-390 | TARGET_DEFS_ONLY 包含链在 riscv 分支后加 wasm 分支（只含 gen/link，无 asm 文件） |
| 3 | tcc.h:1737 后（riscv 节之后） | 新增 wasm 节：声明 `gen_va_start`、`gen_va_arg`、`gen_clear_cache`、`wasm_output_file` |
| 4 | libtcc.c:52-55 | ONE_SOURCE 包含链加 wasm 分支（无 `wasm-asm.c`；内联汇编不支持，tccasm.c 对未知目标的解析路径不引用后端符号，本构建已验证可链接） |
| 5 | tccelf.c:3223 | `tcc_output_file` 开头加 wasm 分发（§4.1） |
| 5b | libtcc.c:2028 `TCC_OPTION_run` 的 cross 分支 | -run 报错换成 wasm 专属文案（§7） |
| 6 | Makefile:119 / 136-137 / 219 | `DEF-wasm32 = -DTCC_TARGET_WASM32`、`TCC_X += wasm32`、`wasm32_FILES = $(CORE_FILES) wasm-gen.c wasm-link.c` |

**config.h 生成**（configure 侧）：configure:646-647 的模板生成守卫 `#if !(TCC_TARGET_I386 || ... || TCC_TARGET_C67)`，其后由 configure:660-664 的 `print_num` 按原生 cpu 写目标宏。这个守卫有个真实的坑：交叉构建时命令行已定义 `TCC_TARGET_WASM32`，但守卫名单里没有它，`!(...)` 仍为真，生成的 config.h 会把原生目标宏也写进去——`build.sh` 在 configure 后用 sed 给生成出的 config.h 守卫补上 `|| TCC_TARGET_WASM32` 并 grep 断言。若要正式进上游，同一改动应落在 configure:646-647 的模板与 660-664 的映射表（加 `CONFIG_wasm32=yes) print_num TCC_TARGET_WASM32 1`），以及 configure:351-353 的 cpu 识别 case 旁加 `wasm32` 条目（供 `--cpu=wasm32` 的原生化构建，本骨架不需要）。Makefile:171-174 的 `TRIPLET-*` 骨架未加 wasm 条目——交叉编译器命名 `wasm32-tcc` 不依赖 triplet 变量，目标 triple `wasm32-wasi` 目前只体现在模块的 WASI import 命名上，记录在此以免误以为漏改。

---

## 6. 基本块分发框架的现状与剩余工作

定版方案是每函数一个 `loop + switch(块号)` 调度。骨架已落地的是它的地基部分，且是真发射：

- 函数体最外层包一个 `block $exit`，`gjmp` 发射 `br $exit`——这正是分发结构里"跳到出口块"的特例；return 链是目前唯一走通的链。
- 跳转链协议与 tccgen 通用层对齐：tccgen.c:173 的 `gsym(t)` 转调后端 `gsym_addr(t, ind)`，链以 32 位 link word 串在 text section 镜像里（对照 riscv64-gen.c:156 的 `gsym_addr` 读 `read32le(ptr)` 取下一环）。骨架的 `gjmp` 在镜像里按此协议存链（site+1 处写回前任），wasm 体对应位置是 `br` + 3 个 nop 补齐 5 字节；`gsym_addr` 只遍历链、不修补 wasm 体（br 目标在发射时已定）。
- 函数索引表已恒等填充（element section），`call_indirect` 所需的表地基齐了。

未落地（均为明确报错桩，按实现顺序）：`gjmp_cond`（条件分支——需要给每条链分配块号、函数头插入 `loop` + `br_table` 调度、条件求值后设块号并 `br loop`）、`gjmp_addr`、`gjmp_append`、标签与 `ggoto`、循环/switch 的 bsym 链。落地路径是把现在的 `$exit` 单块扩成调度循环：epilog 组装时在 block 外再包一层 loop，`gjmp_cond` 分配块号并登记"块号→体偏移"表，块尾统一跳回调度点——gen 侧的 fixup 机制（名字键、输出时修补）可直接复用来修补块偏移表。`alloca`/VLA 的桩按设计应发射"读 `$__stack_pointer`、减长度、对齐、写回"并把旧值存帧槽，`gen_vla_sp_save/restore` 存取该槽即可，与帧模型不冲突。

---

## 7. run 平台约束（定版要求 1）

**tcc_run 对 wasm 目标明确不支持。** 理由是结构性的：生成的 wasm 代码不能在 TCC 进程内直接执行，运行是宿主用自己的 WASM 引擎（浏览器、Node、wasmtime、嵌入式运行时等）实例化模块的事，TCC 不内嵌 wasm 引擎。

上游现状核对：

- `tcc_run` 本体只在 `#ifdef TCC_IS_NATIVE` 内定义（tccrun.c:24 起、tccrun.c:215 函数、文件在 1587 行以 `#endif /* TCC_IS_NATIVE */` 收尾），其后**没有**非原生桩——交叉构建的 libtcc 里这个符号干脆不存在，API 调用方会在链接期拿到未定义符号，而不是一条明确报错。
- CLI 路径上游倒是有明确报错：libtcc.c:2028 `TCC_OPTION_run` 的 `#else`（非原生）分支返回 `"-run is not available in a cross compiler"`。

骨架的处理（三处，无空桩）：

1. `src/wasm-link.c` 里给出 wasm 目标专属的 `tcc_run` 定义：进入 state 后以 `tcc_error_noabort` 返回 `"wasm: tcc_run is not supported for the wasm target; instantiate the generated module with a host wasm engine"`。此定义只在 wasm 目标构建里存在（wasm-link.c 只被 wasm 分支包含），与 tccrun.c 的原生定义不冲突（原生构建不含此文件；wasm 构建里 TCC_IS_NATIVE 未定义，原生定义也不存在）。
2. 补丁 5b 把 libtcc.c:2028 的 CLI 报错换成同样的 wasm 专属文案。实测：`./wasm32-tcc -run tests/test-input.c` 输出该文案、退出码 1（留档于 tests/test-output.txt）。
3. `wasm_output_file` 对 `TCC_OUTPUT_MEMORY` 在写出前明确报错（与 OBJ 不支持并列），堵住"绕过选项解析、直接以内存输出类型调 API"的路径。

---

## 8. 向量加速映射（定版要求 2）

### 8.1 前端现状：上游 TCC 没有向量类型（实测，不是推测）

- 全树 grep `vector_size`、`VT_VECTOR`、`__vector` 在 tcc.h / tccgen.c / tccpp.c / tcctok.h 中**零命中**（仅 arm 的 asm 注释、elf.h 的 auxiliary vector 等无关词）。
- 属性被解析后静默忽略：用原生 tcc 编 `typedef float v4f __attribute__((vector_size(16)));`，编译无警告通过，但 `-run` 实测 `sizeof(v4f) == 4`——它就是个 float。所以"给现有 TCC 加个头文件就能用 NEON intrinsics"是假的：类型层面什么都不存在。
- 结论：SIMD 支持的第一步在前端，不在后端。要补的是：属性解析认 `vector_size(N)`（tccpp/tccgen 的属性解析路径）、CType 里给向量一个表示（骨架在 gen 的 TARGET_DEFS 段预留了私有标志 `VT_VECTOR (1 << 24)` 作占位，该位仅在 VT_STRUCT 的 kind 编码里被解释，向量类型不带 struct btype，暂不冲突；正式实现时应由前端统一分配）、vstack 对 16 字节值的尺寸/对齐处理、以及 GCC 向量扩展语义（一元/二元运算作用于向量类型、向量字面量、lane 下标）在 tccgen 表达式路径上的分发。
- 头文件路线（照 Emscripten 的分层，但顺序倒过来：先类型、后头文件）：
  1. 原生头 `<wasm_simd128.h>` 形态：typedef 出 `v128_t` 等向量类型，intrinsics 写成 `static inline` 函数，函数体用 GCC 向量扩展表达式（`a + b` 作用于向量类型）或少数需要后端认的 builtin——让后端只需认"向量类型的 load/store/二元运算"这组最小面，intrinsics 的丰富度由头文件堆。
  2. 兼容头子集：`arm_neon.h` 子集与 SSE 子集（`emmintrin.h` 等）按 Emscripten 的做法在其 NEON-over-wasm 头的思路上，用原生向量操作逐条实现常用子集；不追求全量，清单以 §8.2/§8.3 的"直接对应"行为准。
- **边界：TCC 不做自动向量化。** tccgen 是单趟直译，没有任何向量化 pass（全树无此代码），标量 C 代码永远生成标量 wasm 指令。支持范围仅限显式 intrinsics 与向量类型的代码。

### 8.2 NEON（128 位）→ v128 映射

| NEON 操作类 | 代表 intrinsics | 映射 | 判定 |
|---|---|---|---|
| 整数/浮点加减乘 | `vaddq/vsubq/vmulq_*` | `i8x16/i16x8/i32x4/i64x2.add/sub/mul`、`f32x4/f64x2.add/sub/mul/div` | 直接对应 |
| 饱和算术 | `vqaddq/vqsubq_*` | `*.add_sat_s/u`、`*.sub_sat_s/u` | 直接对应 |
| min/max | `vminq/vmaxq_*` | 整数 `*.min_s/u`、`*.max_s/u`；浮点 `f*.min/max` | 直接对应（浮点 NaN 与 -0.0 语义与 NEON 有细差，逐案注记） |
| 比较 | `vceqq/vcgtq/vcleq_*` | 各 lane 类型的 `eq/ne/lt/gt/le/ge`，结果同为全 1/全 0 掩码向量 | 直接对应 |
| 取绝对/取负 | `vabsq/vnegq_*` | `*.abs`、`*.neg`（整数/浮点都有） | 直接对应 |
| 固定模式重排 | `vzipq/vuzpq/vtrnq_*` | `i8x16.shuffle` 以立即数 lane 表表达（模式在编译期已知） | 直接对应 |
| 任意重排 | `vtbl1q_*`（寄存器控制查表） | `i8x16.swizzle`（越界给 0 与 vtbl 一致；`vtbx` 的保留原值语义需再 blend 一步） | 直接对应（vtbx 为部分） |
| 移位（立即数） | `vshlq_n/vshrq_n_*` | 各 lane 类型 `shl/shr_s/shr_u`，移位量是标量 | 直接对应 |
| 移位（每 lane 不同量） | `vshlq/vshrq_*`（向量移位量） | wasm 移位只接受全 lane 同一标量量 | **只能标量模拟**（逐 lane extract/移位/insert，或查表） |
| 类型转换 | `vcvtq_*`（int↔float、f16↔f32、窄化/宽化） | `i32x4.trunc_sat_f32x4_*`、`f32x4.convert_i32x4_*`、`f64x2.promote_low_f32x4`、`f32x4.demote_f64x2_zero`、窄化 `*.narrow_*`、宽化 `*.extend_low/high_*` | 多数直接对应；带舍入模式选择的 vcvt 变体 wasm 只有 trunc/nearest 两种，部分对应 |
| 乘加 | `vmlaq/vfmaq_*` | 无 FMA（core SIMD 无；relaxed-simd 是另一提案） | 部分：拆 `mul + add` 两条，浮点多一次舍入，语义不等价，头文件里必须标注 |
| 水平归约 | `vaddvq/vpaddq_*` | wasm 无水平加（浮点尤其没有） | **只能标量模拟**（extract_lane 后标量连加） |
| 点积 | `vdotq_*`（ARMv8.2+） | 无对应（relaxed-simd 亦无通用 dot） | **只能标量模拟** |
| lane 读写/广播 | `vgetq_lane/vsetq_lane/vdupq_n` | `*.extract_lane/replace_lane/*.splat` | 直接对应 |
| 加载/存储 | `vld1q/vst1q_*`、`vld1q_lane` | `v128.load/store`、`v128.load_lane/store_lane` | 直接对应 |
| 64 位 NEON（双 lane 型） | `vadd_*`（64 位形） | v128 运算的高半置之不理不可靠（输入高半任意） | 部分：用对应 128 形并在头文件层保证高半清零，或标量模拟 |
| 密码学扩展 | `vaeseq/vsha*` | wasm SIMD 无 | **不支持**（标量实现已不是"映射"，不在头文件子集内） |

### 8.3 x86 SSE/AVX → v128 映射

- **SSE/SSE2/SSSE3/SSE4.1（128 位）**：与 v128 近 1:1。`_mm_add_ps` 等浮点、`_mm_add_epi*` 等整数直接对应 §8.2 同类行；`_mm_shuffle_ps/_mm_shuffle_epi32`（立即数）→ `i8x16.shuffle` 直接对应；`_mm_shuffle_epi8`（寄存器控制）→ `i8x16.swizzle`，但 PSHUFB 越界是"置零"、swizzle 也是置零，直接对应；`_mm_movemask_ps/_mm_movemask_epi8` → `f32x4.bitmask`/`i8x16.bitmask` 直接对应（位序一致）；`_mm_cvt*` → trunc_sat/convert 家族直接对应；SSE4.2 的 CRC32、字符串指令（`_mm_cmpestri` 类）**不支持**（无对应且语义复杂）。
- **AVX/AVX2（256 位）表示策略**：一个 `__m256/__m256i` 用**两个 v128 局部变量（lo/hi 对）**表示，签名与变量布局在前端/头文件层按此约定。逐 lane 运算成对发射（如 `_mm256_add_ps` → 2 条 `f32x4.add`），三操作数形式对栈机无影响（AVX 的非破坏性目的寄存器在 wasm 里由 locals 天然满足）。跨 128 lane 的操作是分水岭：`_mm256_shuffle_ps` 的 lane 内 shuffle 可成对 shuffle 解决（直接），`_mm256_permute*`、`_mm256_broadcastss`（内存广播可由 splat 解决、寄存器广播要 extract+splat）、`_mm256_extractf128/insertf128`（就是取/换 lo/hi 局部变量，直接）里，凡涉及任意跨 lane 重排的，用 `i8x16.shuffle` 对 lo/hi 分别选源 + 必要时经内存中转（store 到帧槽再 load_lane）实现，列**部分支持**并在头文件注释里标明代价。
- **AVX-512 与掩码操作**：列**不支持**（数据并行、无掩码的 512 位子集理论上可 4×v128 分半，但不列入支持范围）。原因不是宽度而是掩码语义：掩码寄存器、merging/zeroing 两种写回、以及**掩码加载的故障抑制**（被掩掉的 lane 不得触发内存访问错误）在 wasm 里没有对应物——逐条用 blend 模拟写回尚可，故障抑制无法模拟（wasm 的 load 要么整条执行要么不执行）。头文件层面对掩码 intrinsics 直接不提供声明，让编译期就失败，而不是生成语义错的代码。

### 8.4 已落地的 v128 发射（src/wasm-gen.c）

- 前端三步（vector_size 属性解析、`VT_VECTOR` 类型表示、tccgen 向量语义分发）与六个后端 builtin 已落地，向量操作可从 C 源码端到端触发；元素类型取自 `type->ref->type`。
- 发射面：`v128.load/store`（8/16 字节）、splat/extract/replace、shuffle/swizzle、表驱动的一元/二元操作（算术、饱和、min/max、比较、移位、abs/neg/sqrt/ceil/floor/trunc/nearest/popcnt、narrow、extend、extmul、dot、trunc_sat/convert/demote/promote、bitmask/all_true/any_true）；向量值住 4 个 v128 槽（伪寄存器 `WREG_VEC + slot`，含 spill），向量参数/返回值走槽 0 约定。
- 三个头文件已落地：`include/wasm_simd128.h`、`include/arm_neon.h`（§8.2 直接对应子集）、`include/xmmintrin.h`/`include/emmintrin.h`（SSE/SSE2 直接对应子集）。逐项状态（已实现/不提供/仍报错）见 SIMD-STATUS.md，与 §8.2/§8.3 表格逐行对应。
- 端到端验证：tests/test-vec.c 21 项、test-simd128.c 19 项、test-neon.c 16 项、test-sse.c 23 项 Node 数值断言全过，标量回归不坏；全量记录在 tests/test-output.txt（tests/run-regression.sh）。

---

## 9. 交叉目标与上游 cross 构建的关系（定版要求 3）

上游的模型是**一个二进制一个目标，构建期写死**：Makefile:136-137 的 `TCC_X` 是交叉编译器名单，`DEF-$T`（Makefile:102-121）给每个目标的 `-DTCC_TARGET_*`，`$T_FILES`（Makefile:201-219）选源文件，config.h 的守卫保证原生目标宏只在一个目标生效；运行时不存在目标切换（TCCState 里没有后端函数表，gen/link 都是静态符号）。wasm 后端完全顺此模型：`wasm32-tcc` 是一个宿主为 x86_64 ELF、目标为 wasm32-wasi 的交叉编译器，与 `riscv64-tcc` 在名单里平级（补丁 6）。推论写明两条，免得后续实现时走偏：

- 想要"一个 TCC 同时编 x86_64 与 wasm"，在上游架构里不存在低成本路径——那需要把 gen/link 接口虚表化，是伤筋动骨的前端改造，不在本方案范围。
- 我们的包方案（双形态内核按 triple 分发）与此正好同构：每个 triple 一份构建产物，包按宿主/目标选文件，不需要单二进制多目标。

---

## 10. 构建与验证的实际结果

环境：x86_64 Linux，gcc 13.3，Node v24.22.1。构建入口 `./build.sh`（拷贝 vendor 树 → 打补丁 → configure → 先原生构建 → `make wasm32-tcc`），全程日志 `build.log`。

- **构建：成功。** `BUILD OK: build-src/wasm32-tcc`。构建过程共三轮，前两轮的失败都已在上文对应章节记录成接口事实（缺 R_*/MAX_ALIGN 宏、gsym 重定义、config.h 守卫冲突、Sym* 生命周期、save_regs 缺失），不是含糊带过的"调试过程"——它们就是 §2.3/§4.1/§5 里那些结论的来源。
- **端到端验证：通过**（留档 `tests/test-output.txt`，可由 `./build.sh` 后重跑 tests 复现）：
  - `tests/test-input.c`（整数子集：参数、局部变量、嵌套调用、除/模/移位/位运算）经 `./build-src/wasm32-tcc -nostdlib -o tests/test.wasm` 编译成功，产物 834 字节。（`-nostdlib` 是因为 wasm 的 crt1.o/libtcc1 尚未提供，见 §4.1；EXE 输出默认会去找 crt1.o/crti.o 并明确报错，这一点行为正确。）
  - Node `WebAssembly.instantiate` 实例化成功，导出 `add, memory, mix, poly, triple`；断言全过：`add(2,3)=5`、`triple(5)=15`、`poly(3)=29`、`mix(17,5)=14`。
  - 回归项 p8（存活值在 REG_IRET 中跨调用，§2.3 第 3 条）：`poly(3)=11` 正确。
  - `-run`：输出 wasm 专属报错、退出码 1（§7）。
- 未验证/未完成清单（与 §6、§8.4 的桩清单一致，不重复列）：控制流全套、浮点算术与转换、64 位整数运算、间接调用、全局变量读写端到端（地址 fixup 代码已在，未有用例覆盖）、多文件编译、libtcc1、SIMD 端到端（等前端）。

## 11. 后续实现顺序建议

1. `gjmp_cond` + 基本块分发调度（§6 路径）——解锁 if/循环/switch，是从"子集骨架"变"能用编译器"的分界线。
2. gen_opf 与三组 gen_cvt、64 位整数（寄存器对或 i64 locals 二选一，建议后者：把整数寄存器 locals 改 i64 并在 i32 操作处加 wrap/extend，代价集中在 load/gen_opi）。
3. 间接调用（call_indirect 发射，表已就绪）与全局变量用例补测。
4. 前端向量类型三步（§8.1）→ SIMD 端到端测试 → NEON/SSE 兼容头子集。
5. wasm 版 libtcc1 自举与 `_start`/WASI 入口约定（main 的 argc/argv 在线性内存中的布局），到这一步才谈得上编完整程序而非函数库模块。
