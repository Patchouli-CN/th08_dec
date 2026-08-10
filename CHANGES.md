# 更新日志 (Changelog)

## 2026-08-10

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
