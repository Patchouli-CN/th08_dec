# 更新日志 (Changelog)

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
