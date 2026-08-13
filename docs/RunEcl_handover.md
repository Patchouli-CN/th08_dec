# RunEcl 槽对齐细化 — 交接文档

## 目标
把 `th08::EclManager::RunEcl` (0x4184b0) 的相似度从 **25.23%** 提高到尽可能高。
方法：**逐 opcode case 对照原版反汇编，匹配 ZUN 的 EBP 临时变量槽分配**。
这是字节级反编译项目（重编译 th08.exe 与原版逐字节匹配）。

## 关键机制（必须理解）

### fn_diff 的比较规则（决定什么必须精确）
- **call 目标归一化为 T**：helper 函数内部不用管，只需调用约定/参数/返回值正确。
- **EBP 槽不归一化**：`mov [ebp-0x3e4], eax` 与 `mov [ebp-0x568], eax` 是**不同指令**，直接比较。槽必须精确匹配。
- 全局地址归一化为 G。

### var_order 编译器补丁
项目用自制 C1XX.DLL 补丁（`scripts/pragma_var_order.cpp`），`#pragma var_order(...)` 强制局部变量按序分配：
- var1 → EBP-4, var2 → EBP-8, var3 → EBP-12 ...
- RunEcl 当前列表：`(arg, subCtxIdx, instr, i, p5, p6, ..., p57)` —— p5..p57 是 53 个 i32 槽。
- 槽分配是**整个函数级**的活变量分析：任何一个 case 用错临时变量模式，会让后面所有 case 的槽整体偏移。

## RunEcl 当前结构（src/EclManager.cpp）

```cpp
#pragma var_order(arg, subCtxIdx, instr, i, p5..p57)
ZunResult EclManager::RunEcl(Enemy *enemy)
{
    EclRawInstr *instr;
    i32 arg;
    i32 subCtxIdx = -1;
    i32 i;
    i32 p5..p57;
    ...
restart:
#define ECL_IVAL(n) ((instr->paramMask & (1 << (n))) ? GetVarValue(enemy, instr->args[n].i) : instr->args[n].i)
#define ECL_FVAL(n) ((instr->paramMask & (1 << (n))) ? enemy->GetEclFloatVar(instr->args[n].i) : instr->args[n].f)
#define IS_BOSS_MODE() ((g_PlayerFlags & 0x4000) && (((g_PlayerFlags >> 7) & 3) != 0))
    ...
    switch (instr->id - 1)  // case N = opcode N+1
    { ... 184 cases ... }
```

### helper 签名（EclManager.cpp 顶部声明，stub 返回 0/NULL）
- `i32 __fastcall GetVarValue(Enemy*, i32 varId)` — ecx=enemy, edx=varId
- `f32 Enemy::GetEclFloatVar(i32 varId)` — thiscall, 参数栈传
- `i32 *__fastcall GetIntPtr(Enemy*, AnyArg*, u16 paramMask)`
- `f32 *__fastcall GetFloatPtr(Enemy*, AnyArg*, u16 paramMask, i32 unused)`

## 诊断方法（先看现状）

```bash
# 反汇编原版（必须用 resources/th08.exe，不是 build/th08.exe）
objdump -d -M intel --start-address=0x41bd03 --stop-address=0x41bd20 resources/th08.exe

# 看 RunEcl 的字节 diff（- 原版, + 我的）
reccmp-reccmp --paths resources/th08.exe build/th08.exe build/th08.pdb . --verbose 0x4184b0 --no-color

# 测相似度
reccmp-reccmp --paths resources/th08.exe build/th08.exe build/th08.pdb . --no-color | grep RunEcl
```

**已确认的槽错位案例（op116）**：
- 原版：`mov dword ptr [ebp-0x3e4], eax`（GetVarValue 结果存槽 -0x3e4）
- 我的：`mov dword ptr [ebp-0x568], eax`
- 后续 `mov edx, [ebp-0x3e4]` vs `[ebp-0x568]` 同样错位。

## 方法论（逐 case 对齐）

对**每个** opcode case：
1. **反汇编原版对应区域**：opcode→地址映射在 `RunEcl_opcode_calls.txt`（如 `op116 @0x41bcd3`）。该文件列出全部 184 opcode 的起始地址和 call 目标。
2. **读原版反汇编**，看它用了哪些 EBP 槽、槽存什么、参数读取顺序（ECL paramMask 分支：bitN 置位则 argN 是变量 id，调用 GetVarValue/GetFloatVar；否则是字面量）。
3. **调整我的 C++** 让编译器产出相同槽：
   - **优先用显式局部变量**匹配原版槽，不要依赖 ECL_IVAL/ECL_FVAL 宏内联。例如原版 `mov [ebp-0x3e4], eax` 表示它把 GetVarValue 结果存在一个局部变量，我应写：
     ```cpp
     i32 v0 = GetVarValue(enemy, instr->args[0].i);   // 或按 paramMask 分支
     enemy->unk3300 = v0;
     ```
   - 注意：ECL_IVAL(0) 的宏内联会让 MSVC 在 var_order 槽外另分配临时，导致错位。显式局部变量（在 var_order 列表里）能固定槽。
   - **参数求值顺序**：函数调用参数从右到左求值。ECL_IVAL/FVAL 内部的三元运算符 + 函数调用要匹配原版的调用顺序。
4. **build + reccmp 验证**每次改动：
   ```bash
   python scripts/build.py --build-type normal
   ```
   build 报 error C2118（结构超大小）说明改坏布局，立即回退。看 verbose diff 确认该 case 的槽开始匹配。
5. **不要一次性改太多**：每改 1-2 个 case 就 build+测，确认相似度单调不降。若降，回退该 case 的改动。

## 硬性约束
- **绝不改结构布局**：Enemy.hpp / EclManager.hpp 里的结构 C_ASSERT 必须保持成立。只改 RunEcl 函数体里的**变量声明/临时变量写法**。
- **绝不改语义**：不能改变任何逻辑/行为，只调代码生成（槽分配）。
- **不要改 helper 内部**（GetVarValue 等 stub），它们是 T 归一化。
- 若某 case 无法对齐（原版写法特殊），保留当前实现 + 注释，继续下一个。**尽力而为，别在一个 case 上死磕太久**。
- 代码可读性保持（已有命名，别破坏）。

## 已知细节（主代理实测）
- 原版 op116 `enemy->unk3300 = GetVarValue(args[0])` 结果槽 -0x3e4。
- `GetVarValue(enemy, args[0].i)` 是 __fastcall：ecx=enemy, edx=varId。
- ECL 参数：args[n] 在 instr+0xc+n*4；paramMask 在 instr+0xa；instr->id 在 instr+0x0。
- case 布局：switch(instr->id-1)，所以"opcode N" = "case N-1"。
- 主循环每帧 `enemy->movePos = enemy->pos + enemy->moveVec;`。
- exit 块处理 interp 收尾；handleInterrupt 处理中断；skipInstr 跳到下条。
- DID.md 有此前进度记录，可参考。

## 验证节奏
每完成一批，报告：改了多少 case、相似度变化（before/after）、有哪些 case 无法对齐及原因。
