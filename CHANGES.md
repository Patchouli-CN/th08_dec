# 更新日志 (Changelog)

## 2026-08-15 — 第十一~十二轮：dummy 类清除 + 未登记函数清零 + IsSpellcardActive 真实现

- **4 个 dummy 类错配清除**（CSV 用 dummy 类名登记 → 调用点不归一化、真实现被埋没）：
  - `Float3::Length` @ 0x40b4c0（新声明+Background.cpp 定义，**100%**）
  - `Rng::GetRandomF32InRange` @ 0x40d390（CSV 改名 + EclManager 自然调用，**100%**）
  - `Enemy::FUN_0042adb0`（Player.cpp StubThiscall42adb0 删除，调用点改 `((Enemy*)...)->FUN_0042adb0`）
  - `AsciiManager::CreateFamiliarPopup`（Gui.cpp StubThiscallAsciiManagerCreatePopup4 删除，97.69%）
- **8 个未登记函数补登**（隐式 ctor 用数组构造/析构链定位）：UpdateChargeShotTimer(97.75%)、
  Float3::Float3 双 ctor、FUN_00416130、EclContext::EclContext(95.24%)、~AnmManager(100%)、
  PlayerOption::PlayerOption、PlayerBulletVm::PlayerBulletVm。**"已实现未登记"清零**。
- **IsSpellcardActive 方案 A**：删除返回 0 的 stub（EnemyManager.cpp），保留 Spellcard.cpp
  真实现（flags&1），10 处调用点统一——**100% 匹配 + ~20 处原版调用点归一化**。
- **GetFlag0/1/3/14 冗余删除**（源码重复实现，对应位已确认）：8 处 GetFlag14 调用点改
  IsSpellPractice（**100%**）、IsDemoMode **100%**、IsReplay 84.2%、IsPracticeMode 82.4%。
- **Progress 41.04% → 43.03%**；经验入库：隐式 ctor 定位法（数组构造 ctor 指针）、
  scalar deleting destructor 定位法、CSV 名必须 = PDB demangle 名（带参名导致长期未匹配）。
- 已知限制：Float3 双 ctor 重名串位（reccmp 重名匹配 pop 顺序，Ambiguous match 警告，
  字节正确，接受）。

## 2026-08-15 — 第十轮：CSV 大扫除（找回"已实现未登记"函数）+ Float3 运算符修复

- **系统性扫描发现**：PDB 有 178 个源码已实现函数在 reccmp-functions.csv 无对应名
  （语义化改名后 CSV 未同步 → 完全不参与匹配，"白做"）。两轮共补登/改名 **38 处**，
  其中 7 个直接 100%、多个 90%+。**Progress 40.80% → 41.04%**。
- **coordinator 直接修 20 个**（有 FUNCTION marker 高置信）：
  ResetStringsCount、InterpolateCamera(99.5%)、UpdateStageTint、GetSubEnemyChainCount、
  SetSubVmAnm(95.8%)、SetMoveAngleToPlayer、SetBossLifeBarSegment、SetBossLifeSegmentColor、
  ComputeSinCos、IsBossPortraitVisible、ShowClock、UpdateClockNoon、ResetClock、
  IsStopped、FinalBCleared、UpdateInvulnerability(100%)、UpdateBoundaryIndicatorTargets(100%)、
  ResetShotSlot(100%)、UpdateBullets、UpdateBulletVms。
- **子代理 B 补登 18 个**（原版反汇编 + PDB demangle 双重验证）：
  EclGlobalObj::SetGlobalFlag/SetGlobalFlag2/SetTargetPos、RemoveEnemiesByScore（修双 th08::）、
  SetupLaserMove（补类）、GetDifficultyFromSpellCard(100%)、AddSpellcardTime(100%)、
  ResetSpellcard、UpdateReplay、StartRecording、StartReplay、StopRecording、CreateFamiliarPopup、
  5 个 FUN_xxx 加 th08:: 前缀。存疑清单：源码 stub 冲突（IsSpellcardActive 等）、未定位 ctor、
  CRT/x3d_D3DX 库函数合理跳过。
- **Float3::operator+/operator- 假匹配清除**：Player.cpp/EclManager.cpp 的
  D3DVectorOps::Sub/Add dummy stub 删除（假实现顶掉了 Background.cpp 的真实现），
  8 处调用点改自然 `positionCenter -/+ *(Float3*)&shotSpeed3d4` 形式——
  operator+/-/+= 全部恢复 100% 匹配。
- **经验入库**：CSV 名必须 = 我方 PDB demangle 名；marker 回溯定位不可靠（跨函数误配），
  地址必须反汇编原版验证语义；源码 STUB/FUNCTION 注释是可靠定位线索。

## 2026-08-15 — 第五~九轮：弹型 helper 全量反编译 + EnemyManager 绘制/回调对齐

- **弹型行为 helper 全部反编译（14 个，其中 11 个 100%）**（Accuracy 89.40% / Progress 40.80%）：
  - **100%**：FUN_004337f0、FUN_00432170、FUN_00432210、FUN_004322b0（目标角转向：
    vel+=fc0×speed + EclAngleFromDxDy）、FUN_00432390（匀速螺旋）、FUN_00432460
    （减速+转向递增）、FUN_004325a0、FUN_004326e0（瞄准玩家：AngleToPlayer 嵌套
    AddNormalizeAngle）、FUN_004329f0（x 环绕，**double 比较+float 加法精度混用怪癖**）、
    FUN_00432830 99.15%（边界反弹：双向反弹+flagsDAC&0x400 垂直条件）
  - **FUN_00432aa0 97.83%**（y 环绕；剩余为 reccmp 符号解析伪影，字节正确）
  - **FUN_0042ffc0 70.22%**（0x81b 最大 helper：行为安装分发器——0x18 结构数组遍历、
    16 个 case 分派（跳表+cmp 链）、-990/-999 哨兵、LaserData pack(2)+0x24 间隙、
    this@-0x21c 对齐；剩余：fbcTmp 槽位互斥、cmp 0x3e/0x3f、符号伪影）
  - **FUN_004321b0 68.42%**（链头倒序清零；剩余为符号伪影，字节正确）
  - **DrawSingleBullet 59.89%**（state 跳表选 vms、视口偏移、白化、SetZRotation）
- **EnemyManager::OnDrawImpl (0x42e140) → 84.18%**：var_order 18 变量、块变量
  liveness 复用（sinA→angle2 槽、fade→angle 槽、dataPtr 内联、alpha/b→saved1fc
  高字节）、angle2 参数去 j（原版 aiParam2*0x1c 不含 j）、dataPtr 指针算术 bug 修复
  （i32* 步进 → u8* 字节偏移，6 处）。
- **EnemyManager::DeletedCallback (0x42ee80) → 92.42%**：var_order(i, enemy,
  markerPos)（markerPos 12 字节外层变量必须进列表）。
- **EnemyManager::AddedCallback (0x42ebf0)**：结构差异清单已记录（0x42ec72 if 条件区、
  0x42ed24 memset 死代码区），待后续任务。
- **经验入库**：reccmp 符号解析不对称伪影（RECOMP 有 PDB size vs ORIG CSV 无 size →
  大位移/立即数被误解析为 g_X+OFFSET，字节正确不可 cpp 修复）、standalone 验证必须用
  项目 VC7 cl（VS2022 cl 槽位/代码生成不同导致错误结论）、var_order 必须包含全部外层
  变量（漏 12 字节 Float3 会把后续变量推深）。

## 2026-08-15 — 第四轮：弹型 helper 反编译 + EnemyManager 栈对齐

- **3 个 helper 达成 100% 完美匹配**（Accuracy 88.87% / Progress 40.56%）：
  - **`FUN_004337f0` (0x4337f0, ItemManager 初始化)**：`memset(0x17b094)` + `itemListTail=&itemListHead`。
  - **`Bullet::FUN_00432170` (0x432170, 槽复位)**：state=0 + 两个 timer SetCurrent(0)。
  - **`Bullet::FUN_00432210` (0x432210, flagsDAC bit0 行为)**：0xf80 计时器≤0x10 时 5.0f
    减速曲线 + `vel.FromAngleMagnitude(angle, (tmp+unkD68)×g_ShotSpeed)`，否则翻转 bit0。
- **`BulletManager::FUN_004321b0` (0x4321b0) 68.42%**：6 个弹型链头倒序清零
  （chainHeads[5]..[0] 独立赋值，正序 for 形状不符）。
- **`BulletManager::DrawSingleBullet` (0x432f20) 59.89%**（stub 36.36% →）：state 跳表选
  vms[1..4]/vms[0]、视口偏移坐标（左右显式 `.Float3::Float3()` 构造迫使表达式物化
  -0x10/-0x14）、color1 白化、`SetZRotation(AddNormalizeAngle(π/2+angle, 0))`、Draw2D。
  5 槽 var_order 对齐；残余差异仅判别序列（store-sub-store，编译器限制）。
- **`EnemyManager::OnUpdate` (0x42c660) 42.21% → 58.94%**：39 槽位表全提取，
  `#pragma var_order` 19 变量逐槽对齐、声明顺序照原版构造序、循环变量拆分
  （loopVar64/-0x64、loopVar78/-0x78）、新增 deadFloat70（-0x70，ZUN bloat 死构造）。
- **7 个未登记全局补登**（reccmp-globals.csv + EnemyManager.cpp 17 处裸地址替换）：
  g_Unknown164d0a8/g_Unknown164d0ac（人类/总帧计数器，比率写入 0x164cfb8）、
  g_17d6ed4（全库只读状态标志）、g_BossPos/g_BossEnemy/g_BossPosFlag
  （boss 位置跟踪，0x18b899c 系列）、g_18b89ec（playerState 切换计时器）。

## 2026-08-15 — BulletManager 三函数 + 工具链修复

- **工具链修复：var_order 插件恢复**。发现 scripts/prefix 的 C1XX.DLL 被 create_devenv.py
  的 install_compiler_sdk 覆盖为原版（插件丢失），导致所有 `#pragma var_order` 失效
  （C4068 警告、变量按声明序分配）——RemoveAllBullets 从 87.83% 掉到 59.35% 的元凶。
  重新编译安装 pragma_var_order 插件后：RemoveAllBullets 恢复 87.83%、
  BulletManager::OnUpdate 38.00% → **58.70%**、OnDraw 83.33% → **86.67%**、
  Accuracy 86.46% → 88.68%。
- **BulletManager::OnUpdate (0x431240) 58.70%**：switch 跳表 case 0 直接指向
  state1_update 体（消除多余 jmp 导致的 290 行失配）、stateTmp 显式化
  （store→reload→sub→store）、state 2/3/4 早退形状、`Float3 tmp = vel / X` 拷贝初始化
  （隐藏返回槽复用）、补上缺失的 `b->vms[0].SetZRotation(AddNormalizeAngle(1.5707964f +
  b->angle, 0.0f))`（0x431d1f-431d40）、var_order 槽位对齐原版 19 槽。
- **BulletManager::OnDraw (0x432b50) 86.67%**：var_order 6 外层槽、
  第二层绘制独立槽 varD8/varD4、10 处 `.Float3::Float3()` 显式构造、补第二层条件
  `(unk599[0]==0 || runState!=0)`。
- **BulletManager::AddedCallback (0x433070) 53.29%**：删 slot/sprite 局部、
  BULLET_TEMPLATE() 宏复刻 imul+add 寻址、碰撞尺寸 switch 修正（表值 4 属 default
  6/6/3）、spriteId 原地 `-=2`/`-=8`。另修复 FUNCTION marker 行号错配
  （VC7 起始行=签名前最后非注释行导致 AddedCallback 的 marker 匹配到 DeletedCallback
  实体、DeletedCallback 100% 丢失）——移除该 marker 后 DeletedCallback 恢复 100%。
- **跨类 helper stub 全部成员化对齐 CSV**：EnemyManager 的 8 个帮手
  （Player::FUN_00451670、Spellcard::spellcard_fun_004178a0/FUN_0042dff0、
  EclManager::GetTimelineCount/GetTimeline、ZunTimer::operator%、
  BulletManager::FUN_00430aa0(0x1f40,1) 参数顺序修正、AnmManager::DrawVertices、
  Gui::FUN_00437ddd 新增）——EnemyManager::OnUpdate 调用点全部归一化。
- **CSV 语义名同步**（reccmp-functions.csv + mapping.csv）：0x41fd20→HasOwnerEnemy、
  0x42bc90→ClearDataSlots、0x418450→EclInterruptTable::SetupEclContext、
  0x4230c0→Gui::SetBossLifeBarMaxSize、0x4358bb→Gui::IsMsgActive、
  0x44a470→Player::FUN_0044a470、删除 0x40d3b0 重复行。
- **15 处裸地址全局语义化**（EnemyManager）：g_PlayerCharacter、g_PlayerFlags
  （附带 i32→u32 修复 sar→shr）、g_EclExitLeftBound、g_Player.isYoukaiMode/
  playerState、g_GameManager.unk2C。未登记符号清单（0x164d0a8/0x164d0ac、
  0x17d6ed4、0x18b899c/0x18b89b4/0x18b89b8、0x18b89ec）待登记。
- **Float3::operator/ 改为 out-of-line**（0x40c7d0 是按值返回的独立实体，
  原内联版形状错误），定义移至 Background.cpp。

## 2026-08-10

- **一批小函数反编译完成（100%）**：`Supervisor::PlayMusic`（MIDI/WAV 分支完整）、
  `CStreamingSound::PartialFadeIn`（fade-type 3 + SetVolume(-1000)）、
  `ResultScreen::LinkScoreEx`/`FreeScore`/`SetState`、`Supervisor::HideLoadingVms`。
- **又一批 100%**：`AnmManager::SetTextureCaptureParams`、`ResultScreen::MoveCursor`/
  `MoveShotTypeCursor`/`MoveCursorHorizontally`（`WAS_PRESSED_SCROLLING` 边沿/重复移动）、
  `Gui::MsgWait`、`ResultScreen::DeletedCallback`（链清理 + delete）。
- **`Spellcard::RegisterChain`** 反编译完成：0% → **100%**（ChainElem 注册 +
  `FUN_00414590` 初始化检查）。
- **`BulletManager::Initialize`** 反编译完成：0% → **100%**（memset + 字段初始化 +
  子弹池 0x600 元素循环置 -1）。新增全局 `g_BulletPool`。
- 函数实现率提升至 **82.04%**，byte acc 提升至 **13.83%**。
- **`Supervisor::PlayAudio`** 反编译完成：16.8% → **100%**。实现了 MIDI 分支
  （通过 `MidiOutput` 停止/加载/播放）和 WAV 分支（改写扩展名为 `.wav` 后经
  `SoundPlayer` 排队播放），以及回放/演示模式下跳过音乐播放标记的逻辑。
- **`Supervisor::OnDraw2`** 大幅完善：18.2% → **92.8%**。补全了加载界面
  （loading VM）的淡入淡出逻辑——根据 `loadingVmsHaveBeenSetup` 计算 alpha
  渐变、"Press Shot Button" 提示文字、缩放与字符串重置。
- **`Background::AddedCallback`** 反编译完成：26.7% → **100%**。实现了背景
  关卡数据加载回调（预加载背景 ANM、加载 .std 关卡数据、设置雾/相机/每关特效指针）。
- **`ResultScreen::RegisterChain`** 反编译完成：0% → **100%**。实现链注册
  （分配 ResultScreen、按链 ID 选择屏幕模式、注册 OnUpdate/OnDraw 回调）。
- **`Player::LoadShtFile`** 反编译完成：25% → **100%**。实现 .sht 文件加载
  （重定位 entry 指针、把 4 张回调表索引解析成函数/数据指针），并补充
  `AnmManager::GetAnm`。
- 登记了 **168 个此前已实现但未登记**的函数，函数实现率提升至 **73.06%**，
  byte acc 提升至 **7.38%**。
- 再次扫描又登记了 **28 个 fn_diff 100%** 的函数，函数实现率提升至 **77.99%**，
  byte acc 提升至 **9.31%**。
- **`AsciiManager::OnUpdate`** 反编译完成：0% → **100%**。实现分数/时间弹幕的
  位置漂移与计时过期、暂停/重试菜单路由、demoIcon VM 驱动。byte acc 提升至 **9.5%**。
- **`ResultScreen::FormatDate`**（0x456938）、**`FUN_00426d10`**（0x426d10）、
  **`Float3::operator+=`**（0x410a70）反编译完成（均 100%）。FormatDate 即
  `strftime(buf, 6, "%m/%d", localtime(&t))`；FUN_00426d10 遍历 `g_EffectManager`
  中 512 个粒子（+0x351 标记 == '3' 时对 +0x2d4 的 Float3 做 `+=`）；
  `Float3::operator+=` 由 hpp 内联改为 out-of-line（原版 0x410a70 是独立函数，
  调用处 `call`）。func 88.73% / size 14.3%。
- **`EffectManager::ResetEffects`**（0x425410）、**`AsciiManager::FUN_00407160`**
  反编译完成（均 100%）。ResetEffects 即 `memset(this, 0, 0x8b05c)`（rep stos
  0x22c17 dwords），EffectManager 结构体补齐到 0x8b05c；FUN_00407160 写
  this+0x8264 = 0。
- **`EffectManager::FUN_00425870`**（0x425870）、**`EffectManager::FUN_00425430`**
  （0x425430）反编译完成（均 100%）。效果粒子 spawn 系列：定位粒子槽（+0x1c 起
  512×0x360）、释放旧 sprite、memset 清零、写类型/位置、调 AnmLoaded::SetAndExecuteScriptIdx、
  应用效果模板（g_EffectTemplates 表 0x4c6d30）、标记回放事件。展开
  EffectManagerParticle 结构体（含 unk1f0/1f8/288/2a4/2d4/328/348/350/351/358）、
  EffectManager 加 unk0/unk8b054 字段、新增 g_EffectTemplates 全局。
- **`ResultScreen::HandleHighScoreCharacterSelect`**（0x45567d）反编译完成（100%）。
  角色选择状态机（unk10==0 初始化、==1 MoveCursor/WAS_PRESSED 确认返回/进入成绩屏）。
- **`ResultScreen::HandleSpellCardDifficultySelect`**（0x455a33，86%→100%）、
  **`ResultScreen::HandleSpellCardCharacterSelect`**（0x455cb0）反编译完成。
  同难度/角色选择状态机模式（switch this->unk10 + MoveCursor + WAS_PRESSED 确认）。
- **`EnemyManager::Initialize`**（0x429e00）反编译完成（100%）。初始化 EnemyManager
  （0x9cef10 清零 + 各数组/标志位/定时器/Float3 初始化，p 变量复用 this）。

## 2026-08-11

- **`Player::AddedCallback`** 反编译完成：32.7% → **100%**。重写为逐段对照反汇编：
  三次构造器调用（helper `PlayerPosCenter` 模拟 D3DXVECTOR3 空构造器）、0x180 shot 槽
  循环、sht 表数据除法拷贝、GetFlag14 条件 SetCurrent、bullets state 清零、两张射击
  回调表 memcpy、角色颜色覆盖、0x10 位置拷贝、4 个 option 初始化、结尾 e2b2c/全局
  0x57ad30。新增全局 `g_Unknown57ad30`（0x57ad30）。

## 2026-08-11

- **Player 一批小函数 100%**：`IsHuman`/`IsYoukai`、`FUN_0044e370`（shot 槽清零）、
  `FUN_0044e350`、`FUN_0044c5b0`（0x180 槽计时）、`FUN_0044d420`（gauge 分层设置）、
  `FUN_0044e0f0`/`FUN_0044e120`（AnmVm flags）、`FUN_0044d180`（重生初始化）。
- **ZunTimer 方法系统性修复**：Supervisor.cpp 的 `/Os` 使简单方法 epilogue 生成
  `leave`，与原版 `mov esp,ebp; pop ebp` 不符；将 SetCurrent/SetCurrentImpl/Tick/
  TickImpl/AsFrames/AsFramesFloat 移到 Player.cpp（`/Od`）后全部 100%。
- `FUN_0040bc20`/`FUN_0040bc40` 更名 `Player::IsHuman`/`IsYoukai`（统一 mapping/reccmp/源码）。

## 2026-08-11

- **可读性重构**：定义 `ShotSlot` 结构并拆分 `Player::shots[0x180]`，玩家射击相关
  函数改用字段访问替代裸偏移。
- **`FUN_00451500`** 反编译完成：0 → **100%**（决死结界检测，条件分支+音效）。
- **`FUN_0044c650`** 射击状态机实现至 70%（FIXME：寄存器/跳板布局差异）。

## 2026-08-11

- **`Player::FUN_0044aec0`（0x12a1 巨型主更新）9.5% → 66%**：方向检测 + 换机（双人系统）
  完整实现。逆向出 TH08 换机逻辑：`unkFdc`/`unkFe0` 换机输入、`unk8` 换机动画帧计数、
  `isYoukaiMode` 换机中标志、`unk5` human/youkai 形态标记、`moveSpeedNormal`(0xe2a74)/
  `moveSpeedSpirit`(0xe2a78) 双速度表。movementDirection switch → 速度、动画选择
  （当前帧 vs 上帧 x 速度）、位置更新+边界钳制、6 个方向向量、option 回调表更新、
  妖力槽逻辑（决死结界判断）、结界特效、残影轨迹。剩余差异为寄存器分配/switch 判别式
  中转（编译器决策）。
- **关键逆向发现**：6 个方向向量（0x38c-0x3c8）就是 `shotVector1-4` +
  `grabItemTopLeft/BottomRight`（第 5/6 个向量）；option 回调表是二维
  `[g_PlayerCharacter][option]`（`shl eax,4` 寻址，改二维数组声明后 fn_diff 提升 36%）。
- **GaugeIsModeratelyHuman/Youkai 返回类型 bool→i32**：原版 `setle al` 但调用处无
  `movzx`（i32 返回），bool 生成 movzx 不匹配。改 i32 后 FUN_0044aec0 提升约 4%。
- **Player 结构字段化**：0x7/0x8/0xc、`trailPos[0x10]`、`shotVector1-4`、`moveSpeedX/Y`、
  `velocityX/Y`、`moveSpeedNormal/Spirit` 指针、`effectVm`(0xbe834)、PlayerMoveSpeed 结构。
- 新增全局：`g_PlayerBoundaryLeft/Top/Width/Height`(0x164d2ec-2f8)、
  `g_OptionInitCallbacks[8][4]`/`g_OptionUpdateCallbacks[8][4]`/`g_SpiritOptionInitCallbacks[4]`/
  `g_SpiritOptionUpdateCallbacks[4]`。
- **新 stub**：`Player::FUN_00451640`(0x451640)、`D3DVectorOps`（D3DXVECTOR3 operator-/+ 0x4090d0/409080 的 thiscall stub）、`GameManager::GaugeIsModeratelyYoukai`。

- **`Player::FUN_0044cbf0`（0x44cbf0 死亡/重生）stub → 98.7%**：死亡处理（消耗时间点、
  shotIndex 递减、清结界、死亡特效+音效、掉落物 spawn、Lives/Power/Bomb 恢复）+ 重生
  （invulnerabilityTimer 插值 alpha、30 帧后 playerState=1、重生点设置、形态切换动画、
  扣命、SetBombCount）。剩余差异为 goto 跳板布局（AsFrames/形态切换分支）。
- 逆向发现：死亡掉落物为 1 个大 P + 5 个小 P（第一 SpawnItem itemType=2，后 5 个=0）；
  时间点惩罚 `-(currentTimeOrbs/10)`（>5000 时 -500）；`0x160f510+0x3c`=currentTimeOrbs。
- 新增：`g_PlayerUnknown0b0`(0x164d0b0)、`Spellcard::FUN_0044d150`、`ItemManager::FUN_00441530`。

- **`Player::FUN_00449ff0`（0x449ff0 弹幕碰撞检测）stub → 96.7%**：遍历 shots[0xc0..]，
  圆形碰撞（dx²+dy² < r²）、旋转矩形（Rotate 后 AABB）、AABB 矩形碰撞，命中更新
  `unkE2a90`（碰撞类型）+ `unk30++` 返回 2。剩余差异为 AABB 独立 if 的跳板布局。
- 逆向发现：`ShotSlot.unk20` 是 f32（旋转标志，原版 fcomp/fchs）；`unk8==0.0` 走矩形碰撞；
  原版 4 次 Float3 构造（diff/rotated + box/box2 复用 halfW/left 槽）。

- **进度条修正**：implemented.csv 曾含 7 个 fn_diff 非 100% 的函数（历史遗留 FIXME），
  导致进度虚高（93.84/15.28）。移除后进度准确：**92.61% func / 13.30% size**。
  进度条现只统计完全匹配（fn_diff 100%）的函数。

## 2026-08-11 — GameManager::OnUpdate（0x439bc7）stub -> 98.26%

攻下了全项目最大的状态机之一（3644 字节）。完整实现了：
- 关卡完成记录（无续关/有续关通关表、extra 通关计数）
- 暂停键 + 特殊模式暂停检测
- 符卡收集 BGM 切换
- focus 慢速切换（含 RNG 保存/恢复）
- 换机/demo/slowMode 三套降速逻辑
- 防篡改完整性检查（csumFloat 越界置 -9999）
- 分数插值动画

新增 12 个语义化全局，魔法常量全部命名（PAUSE_KEY_MASK、
ANTITAMPER_RANGE_MIN/MAX、DEMO_*_FRAME 等）。剩余 1.74% 是 MSVC
跳转 trampoline / u8 位或的系统性 codegen 差异（已标 FIXME）。
