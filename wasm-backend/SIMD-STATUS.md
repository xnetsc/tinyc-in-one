# SIMD 状态（wasm 目标）

与 ASSESSMENT.md §8 映射表逐项对应。状态分三类：已实现（有
Node 数值断言覆盖）、不提供（头文件无声明，编译期失败）、
仍报错（编译或链接期给出指名错误）。

TCC 不做自动向量化；支持范围仅限显式 intrinsics 与向量类型代码。

## 1. 前端与发射层

已实现：

- `__attribute__((vector_size(16)))` 解析与 `VT_VECTOR` 类型；
  64 位（8 字节）与 128 位（16 字节）向量。
- C 向量语义：二元算术/位运算/比较、一元负号、向量字面量与
  花括号初始化、lvalue 下标读写、向量参数与返回值、向量↔向量
  cast（位重解释，与 GCC 一致）。
- 六个后端 builtin：`__builtin_wasm_splat`、`__builtin_wasm_extract`、
  `__builtin_wasm_replace`、`__builtin_wasm_vop1`、`__builtin_wasm_vop2`、
  `__builtin_wasm_shuffle`（含 swizzle 形式）。
- v128 发射：load/store（8/16 字节）、splat/extract/replace、
  shuffle/swizzle、表驱动的一元/二元操作（算术、饱和、min/max、
  比较、移位、abs/neg/sqrt/ceil/floor/trunc/nearest/popcnt、
  narrow、extend low/high、extmul low/high、dot_i16x8_s、
  trunc_sat/convert/demote/promote、bitmask/all_true/any_true）。
- 向量值在 4 个 v128 槽之间分配与 spill；向量参数/返回值走
  槽 0 约定。

限制：

- i64x2 splat、i64 lane 的 extract/replace 不支持：后端没有
  64 位标量寄存器，编译期指名报错。
- 向量 rvalue 可直接下标读取（`f()[0]`、`(a + b)[1]` 形式，
  后端把 rvalue 先落到帧内存）；对 rvalue 下标赋值
  （`f()[0] = x`）不报错，写入临时副本，不影响原值。

## 2. 头文件

随 build.sh 的 sysroot include 拷贝生效，`-I include` 亦可。

### include/wasm_simd128.h（原生层）

提供：load/store/load_splat、zero、const（i32/f32/i16/i8）、
splat（i8/i16/i32/f32/f64）、extract/replace（i8/i16/i32/f32/f64）、
shuffle/swizzle、位运算、整数算术（含 i8 乘的宽化合成、i64
加/减/乘/负）、饱和加减（i8/i16，有符号/无符号）、min/max
（整数至 i32、f32/f64、pmin/pmax）、avgr_u（i8/i16）、
dot_i16x8_s、extmul low/high、浮点算术、abs/neg/sqrt/ceil/
floor/trunc/nearest/popcnt、extend low/high（i8→i16、i16→i32、
i32→i64）、类型转换（trunc_sat、convert、demote、promote）、
全形状比较（无 u64 比较）、全形状移位（含 i64）、
bitmask/all_true/any_true。

不提供：i64x2 splat、i64 lane 读写、u64 比较、i32/i64 饱和
算术、i64 min/max（wasm SIMD 无对应指令）。

断言：tests/test-simd128.c + tests/run-simd128.mjs，19 项全过。

### include/arm_neon.h（NEON 子集，仅 §8.2「直接对应」行）

提供：vld1q/vst1q（s8/s16/s32/s64/u8/u16/u32/u64/f32/f64）、
vaddq/vsubq/vmulq（全形状）、vdivq_f32/f64、vqaddq/vqsubq
（s8/s16/u8/u16）、vminq/vmaxq（s8/s16/s32/u8/u16/u32/f32/f64）、
vceqq/vcgtq/vcgeq/vcltq/vcleq（同上形状，结果为同形状无符号
掩码）、vabsq（s8/s16/s32/s64/f32/f64）、vnegq、vdupq_n
（s8/s16/s32/u8/u16/u32/f32/f64）、vgetq_lane/vsetq_lane
（无 64 位 lane）、vdupq_laneq_s32/f32、vzip1q/vzip2q/vuzp1q/
vuzp2q/vtrn1q/vtrn2q（s32、s8，及 f32/u32 的部分形状）、
vtbl1q_u8、vshlq_n/vshrq_n（s32/u32/s64/u64，均匀标量计数）、
vreinterpretq（各形状互转）。

不提供：vmlaq/vfmaq（wasm 无 FMA；拆 mul+add 浮点多一次舍入，
语义不等价，不在头内合成）、每-lane 向量移位量、水平归约
（vaddvq/vpaddq）、vdotq、vcvt 系列（转换在 wasm_simd128.h）、
vtbx、vld1q_lane、64 位非 q 形、i64 lane 读/写/广播、
密码学扩展（vaeseq/vsha）。

断言：tests/test-neon.c + tests/run-neon.mjs，16 项全过。

### include/xmmintrin.h / include/emmintrin.h（SSE/SSE2 子集）

提供：__m128/__m128i/__m128d 的 load/store/set/setr/set1/
setzero；ps/pd 算术、sqrt、min/max；_ss 标量形（add/sub/mul/
div/sqrt/min/max 与比较 _ss 形、comi 形）；比较 ps/pd 全套；
movemask_ps/movemask_epi8；shuffle_ps、shuffle_epi32、
shufflelo/hi_epi16、shuffle_epi8（SSSE3）；unpacklo/hi
（ps 与 epi8/16/32/64）；movelh/movehl；整数 add/sub
（epi8/16/32/64）、mullo_epi16、madd_epi16、饱和 adds/subs
（epi8/16、epu8/16）、min/max（epi16、epu8）、avg（epu8/epu16）、
packs/packus（epi16/32）、cmpeq/cmpgt/cmplt（epi8/16/32）、
si128 位运算、元素移位（slli/srli/srai，epi16/32/64）、
字节移位（slli/srli_si128）；转换 cvtss_f32、cvtsi32_ss、
cvtss_si32、cvttss_si32、cvtsd_f64、cvtsi32_sd、cvtsd_ss、
cvtsd_si32、cvttsd_si32、cvtepi32_ps、cvttps_epi32、
cvtepi32_pd、cvttpd_epi32。

不提供：rcp/rsqrt 近似、mul_epu32、mulhi、sad_epu8、
set1_epi64x 等 64 位 splat、prefetch/fence、SSE4.1 及以后的
intrinsics（blend、dot 产品头函数、CRC32、字符串指令）。

注意：_mm_cvtss_si32/_mm_cvtsd_si32 的溢出行为是饱和
（trunc_sat 语义），与 x86 的 indefinite 值（0x80000000）不同。

断言：tests/test-sse.c + tests/run-sse.mjs，23 项全过。

## 3. §8.2 NEON 映射逐行状态

| §8.2 行 | 状态 |
|---|---|
| 整数/浮点加减乘 | 已实现（C 语义与三个头） |
| 饱和算术 | 已实现（i8/i16；wasm 无 i32/i64 饱和） |
| min/max | 已实现（整数至 i32、f32/f64）；i64 无对应指令，不提供。浮点 min/max 用 wasm `f*.min/max`，NaN 与 -0.0 语义与 NEON 有细差（§8.2 原注记） |
| 比较 | 已实现（整数有符号/无符号至 i32、i64 仅 eq/ne 与有符号序比较、f32/f64）；u64 序比较 wasm 无指令，不提供 |
| 取绝对/取负 | 已实现 |
| 固定模式重排 | 已实现（s32/s8 全套，f32/u32 部分形状，shuffle 立即数表） |
| 任意重排 | vtbl1q_u8 已实现（swizzle）；vtbx 不提供 |
| 移位（立即数/均匀量） | 已实现（含 i64） |
| 移位（每 lane 不同量） | 不提供（wasm 移位只接受全 lane 同一标量量；头内不做标量模拟） |
| 类型转换 | wasm_simd128.h 已实现（trunc_sat/convert/demote/promote/extend/narrow）；NEON 头不提供 vcvt |
| 乘加 | 不提供（无 FMA；拆分语义不等价） |
| 水平归约 | 不提供数值归约；bitmask/all_true/any_true 谓词归约在 wasm_simd128.h 已实现 |
| 点积 | NEON 头不提供 vdotq；wasm 原生 dot_i16x8_s 在 wasm_simd128.h 已实现 |
| lane 读写/广播 | 已实现（i64 lane 与 i64 splat 除外，见 §1 限制） |
| 加载/存储 | vld1q/vst1q 已实现；vld1q_lane 不提供 |
| 64 位 NEON 形 | 不提供 |
| 密码学扩展 | 不提供 |

## 4. §8.3 SSE/AVX 逐项状态

| §8.3 项 | 状态 |
|---|---|
| SSE/SSE2（128 位） | 已实现（清单见 §2） |
| SSSE3 shuffle_epi8 | 已实现 |
| SSE4.1 | 不提供（头内无声明） |
| SSE4.2 CRC32/字符串 | 不提供 |
| AVX/AVX2（256 位） | 不提供（无 __m256 类型与头文件） |
| AVX-512 与掩码操作 | 不提供（理由同 §8.3：掩码写回与掩码加载故障抑制在 wasm 无对应物） |

## 5. 标量侧（与 SIMD 相邻的部分）

已实现（tests/test-scalar2.c + tests/run-scalar2.mjs，
86 项数值断言覆盖）：

- 标量浮点算术（gen_opf 的 + - * / 与六种比较）；float 值以
  f64 提升形式存寄存器，f32 运算前后做 demote/promote。
- 标量转换（gen_cvt_itof/ftoi/ftof/sxtw）：int/long long/
  unsigned 与 float/double 互转；float→整数用 trunc_sat
  饱和语义；unsigned long long 互转走后端直通（补丁 28），
  不依赖 __floatundidf/__fixunsdfdi 等 helper。
- 比较结果物化与条件跳转：VT_CMP 协议 + 基本块分发
  （gjmp/gjmp_cond/gjmp_append/gsym_addr 按链协议实现），
  if/while/do-while/for、&&/|| 作值与作条件、三目、goto、
  switch、break/continue 均可。gen_opl 的高字比较舞步依赖
  标志位后端的环境条件，本后端以「最近一次整数比较的
  操作数」替代（cmp_r 为 0 的 VT_CMP 取环境操作数）。
- 标量 64 位：常量加载（含大于 2^32 的常量）、寄存器对的
  参数/返回值（与 wasm i64 签名互转）、加减（ADDC/SUBC
  进位协议）、乘（UMULL 合成）、常量计数移位、比较
  （gen_opl 高低字舞步）；i64 向量可花括号常量初始化，
  局部与静态均可。
- store/load 的 64 位整数按单字契约：通用层把寄存器对拆成
  PTRDIFF_T 单字逐个调用，save_reg_upstack 溢出 pair 成员
  时以完整 llong 类型、单寄存器调用，后端按一字存储。

剩余边界：

- long long 除法/取模、变量计数 long long 移位编译为对
  libtcc1 helper 的调用（__divdi3/__moddi3/__ashldi3 等），
  以 env 导入出现在模块里；本目标不带 libtcc1，宿主未提供
  时按缺失导入指名报错。
- wasm 链接的符号解析：定义在数据节的符号按节+偏移精确
  解析；其余情形按名查 ELF 符号表并有线性扫描兜底，兜底
  路径上同名的 block-scope static 符号取首个匹配。

## 6. 验证

全量回归：tests/run-regression.sh（构建 + 标量 + 标量 2 +
向量 + 三个头），记录在 tests/test-output.txt。

- 标量 tests/test-input.c：断言全过（add=5、triple=15、
  poly=29、mix=14），-run 按定版报错退出码 1。
- 标量 2 tests/test-scalar2.c：86 项断言全过（浮点算术、
  比较作值/作条件、控制流、转换、整浮混合、64 位整数、
  i64 向量初始化、向量前端用例）。
- 向量 tests/test-vec.c：21 项断言全过（19 个用例，其中
  t_cmp、t_shl 各含两组参数）。
- 三个头：19 + 16 + 23 项断言全过。
