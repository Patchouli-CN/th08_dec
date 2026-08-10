# 更新日志 (Changelog)

## 2026-08-10

- **`Supervisor::PlayAudio`** 反编译完成：16.8% → **100%**。实现了 MIDI 分支
  （通过 `MidiOutput` 停止/加载/播放）和 WAV 分支（改写扩展名为 `.wav` 后经
  `SoundPlayer` 排队播放），以及回放/演示模式下跳过音乐播放标记的逻辑。
- **`Supervisor::OnDraw2`** 大幅完善：18.2% → **92.8%**。补全了加载界面
  （loading VM）的淡入淡出逻辑——根据 `loadingVmsHaveBeenSetup` 计算 alpha
  渐变、"Press Shot Button" 提示文字、缩放与字符串重置。
- 进度条更新：函数实现率提升至 **43.13%**。
