# ZUN 代码风格参考（th07 调查结论，供 th08 反编译使用）

> 来源：调查子代理对 `d:/python_play/Touhou08/th07/src/th07/` 的源码分析。
> 用途：th08 反编译的目标是**逐字节匹配原版 exe**，必须复现 ZUN 的**实际写法**（哪怕"愚蠢"/冗余/滥用宏），不要"改正确"。

## 1. 取值宏（解释器读参数的核心）

th07 EclManager.cpp:22-38，整个 RunEcl 用：

```c
#define GET_INT_PTR(enemy, argIdx) \
    GetVar(enemy, &instr->args[argIdx].i, instr->paramMask, argIdx)

#define GET_FLOAT_PTR(enemy, argIdx) \
    GetFloatVar(enemy, &instr->args[argIdx].f, instr->paramMask, argIdx)

#define GET_INT_VALUE(enemy, argIdx) \
    (((instr->paramMask & (1 << argIdx)) != 0) ? GetVarValue(enemy, instr->args[argIdx].i) : instr->args[argIdx].i)

#define GET_FLOAT_VALUE(enemy, argIdx) \
    (((instr->paramMask & (1 << argIdx)) != 0) ? GetFloatVarValue(enemy, instr->args[argIdx].f) : instr->args[argIdx].f)

#define GET_INT_VALUE_D(enemy, args, argIdx, bitIdx) \
    (((instr->paramMask & (1 << bitIdx)) != 0) ? GetVarValue(enemy, args[argIdx].i) : args[argIdx].i)

#define GET_FLOAT_VALUE_D(enemy, args, argIdx, bitIdx) \
    (((instr->paramMask & (1 << bitIdx)) != 0) ? GetFloatVarValue(enemy, args[argIdx].f) : args[argIdx].f)
```

- `*_PTR` 系列：返回可写指针（变量→字段指针；字面量→&args[argIdx] 就地）。用于赋值目标。
- `*_VALUE` 系列：返回值。用于运算/比较。
- `*_D` 系列：参数下标与 bit 下标分离，用于子弹/激光 spawn（bit N 不对应 args[N]）。
- AnmManager 有同构版（vm 版，读 instr->flags）。

th08 现状：EclManager.cpp 有 `ECL_IVAL/ECL_FVAL` 宏（只用了一次）、`ECL_ARG_INT/ECL_ARG_FLT`（移动辅助函数用），其余 ~200 个 case 是手写 if/else 展开。**th07 证明三元宏形式编译后逐字节一致**。

## 2. 变量系统 = 4 个巨型 switch

| 函数 | 返回 | 语义 |
|---|---|---|
| GetVarValue | i32 | 读值。float 变量经 i32 返回（原始位模式）|
| GetFloatVarValue | f32 | 读值，int cast 成 (f32) |
| GetVar | i32* | 取指针。先 `idx>=0 && (mask & 1<<idx)==0` → 字面量地址；否则 switch |
| GetFloatVar | f32* | 同上浮点版 |

关键：
- `GetVar` 掩码检查 `idx >= 0 && ((u32)mask & 1<<idx) == 0`；**idx==-1 强制走变量解析**（ZUN 用魔数凑行为）
- GetVarValue 的 `default: return eclVar`（非已知 id 的普通数值原样返回）
- 两个"读值"函数 case 集合**故意不对称**；GetVar/GetFloatVar 只覆盖部分变量，漏的走 default —— 反编译时**不要"补全"** case
- th08 的 GetIntPtr/GetFloatPtr 内部硬编码 `args[0]`（`return &args[0].i`），th07 支持 `&args[argIdx]` —— **功能差距，若需写非 arg0 目标（如 DEC_JUMP 写 arg2）要按 th07 改**

## 3. 解释器 switch 典型形态

```c
case ECL_SET_INT:
    *GET_INT_PTR(enemy, 0) = GET_INT_VALUE(enemy, 1);
    break;
case ECL_ADD_FLOAT:
    *GET_FLOAT_PTR(enemy, 0) = GET_FLOAT_VALUE(enemy, 1) + GET_FLOAT_VALUE(enemy, 2);
    break;
case ECL_RAND_SIGN:
    *GET_INT_PTR(enemy, 0) = ((g_Rng.GetRandomU16() & 1) != 0 ? 1 : -1) * GET_INT_VALUE(enemy, 1);
    break;
case ECL_DEC_JUMP:
    *GET_INT_PTR(enemy, 2) -= 1;
    if (GET_INT_VALUE(enemy, 2) <= 0) break;
    // fallthrough
case ECL_JUMP:
    ... // goto jump / restart（改 instr 指针）
```

- 写用 PTR，读用 VALUE，一行 `*PTR = VALUE(...)` 是标准形
- 宏套宏（内层读本 instr 的 arg 当下标，外层解析）
- goto + 改 instr 指针，不是结构化循环
- case 之间有意 fallthrough
- 魔数常量（3.1415927f、1.5707964f、0.7853982f）**保留字面值，不用 ZUN_PI 替换**
- `AddNormalizeAngle(x, 0.0f)` 当纯 normalize 用（全库习惯）

## 4. "愚蠢"代码（照抄 + 注释，不清理）

th07 团队用 `// ZUN bloat` 标注的：重复赋值、声明未用局部变量、复制粘贴分支、冗余初始化累加器、`(x & 1U) != 0` 有符号/无符号混用、从 args 数组取下标、函数内重复的 if 块、三次同样公式（rank 函数）等。

## 5. 角度→向量：FromAngleMagnitude（fsincos）

th07 统一入口 `Float3::FromAngleMagnitude(angle, magnitude)`：
```c
__asm { mov eax, this; fld [angle]; fsincos; fmul [magnitude]; fstp [eax]; fmul [magnitude]; fstp [eax+4]; }
```
x=cos, y=sin。另有 `sincosf` 函数版 / `sincosf_macro` 宏版。

**th08 问题**：BulletManager.cpp:122 `ComputeSinCos` 用 `cos()`+`sin()` 两次 CRT 调用，原版 0x433880 是单条 fsincos（sincosf 内联汇编）→ **必须改**。th08 ZunMath.hpp 已有 `sincos` 宏（行 110-117）。

## 6. 变量命名

- 全局 `g_xxx`；成员/局部无前缀，动词短语函数名
- 反编译团队给未知字段：`unused_N`（偏移，只写不读）、`local_N`（反汇编栈偏移，非 ZUN 原名）、`param_1`（参数占位）
- th07 团队后来把 local_N 改成语义名（= th08 的"语义化"工作）

## 对 th08 RunEcl 的直接建议

1. 建 GET_INT_PTR/GET_FLOAT_PTR/GET_INT_VALUE/GET_FLOAT_VALUE 宏，替换手写 if/else（字节中性，th07 证明）
2. 若 th08 子弹 spawn 有 bit≠arg 偏移，加 *_D 变体
3. 填实 GetVarValue/GetEclFloatVar：巨型 switch（不是查表），default 兜底，case 故意不对称
4. GetIntPtr/GetFloatPtr 支持 &args[argIdx]，掩码检查与 th07 一致
5. 角度→向量统一 FromAngleMagnitude/sincos；替换 ComputeSinCos
6. 魔数保留字面值；AddNormalizeAngle(x,0) 习惯照抄
7. 遇 `// ZUN bloat` 类冗余一律照抄并注释，不清理

## 7. 编译选项混合单元（重要发现）

原版某些 .cpp 是 **/Od 与 /Os 混合**编译的（不同函数用不同选项）：
- **证据**：GameManager.cpp 若整体改 /Od，Gauge 系列（GaugeIsExtremelyYoukai 等）从 40% 升到 93%，但 IsStageCleared/CalcChecksum/RegisterChain 等原本 100% 的函数**回归**到 25-65%。说明这些函数原版分别属不同编译单元（或 ZUN 拆了文件）。
- **结论**：单文件编译选项无法同时匹配 /Od 和 /Os 混合的函数。configure.py 的 `/Os` 分配是团队调过的，别整体改。
- **影响**：原版 /Od 的函数（如 Gauge 系列 getter/setter）在 /Os 下语义正确但 epilogue 差一条指令（`leave` vs `mov/pop`），指令级 40-93%。这是反编译固有限制（除非拆文件），**接受语义正确 + 低指令匹配**。
- **辨识 /Od 特征**：epilogue 用 `mov esp,ebp; pop ebp`（vs /Os 的 `leave`）、f32 赋值用纯 `mov`（vs /Os 的 fld/fstp）、可能有冗余重读死代码（如 SetBossPresent 的 `mov al,[ebp+8]`）。

## 8. VC7 #pragma optimize 无效

VC7 的 `#pragma optimize("agstwy", off)` 在 /Os 编译下**不生效**（函数级关优化无法覆盖命令行 /Os）。要在 /Os 文件里复现 /Od 代码生成只能拆文件或改整体编译选项，不能靠 pragma。
