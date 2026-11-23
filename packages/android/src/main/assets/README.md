# assets 布局

AAR 运行时由 `TinycAssets.init()` 把当前 ABI 对应 triple 的两棵树解压到
应用 `filesDir/tinyc/<version>/<triple>/` 下（assets 不是真实文件，
TCC 读不了，必须先落地）。

```
assets/
├─ toolchain/
│  ├─ aarch64-linux-android/
│  │  ├─ include/    TCC 自带头：stdarg.h、stddef.h、stdbool.h、float.h、tcclib.h 等
│  │  └─ lib/        libtcc1.a
│  └─ x86_64-linux-android/
│     ├─ include/
│     └─ lib/
└─ sysroot/
   ├─ aarch64-linux-android/
   │  ├─ include/    bionic 系统头
   │  └─ lib/        bionic 系统库与导入库
   └─ x86_64-linux-android/
      ├─ include/
      └─ lib/
```

当前各目录只有 `.gitkeep` 占位。发布构建时由构建流水线把对应 triple 的
toolchain 与 sysroot 内容填入（规格第 9 节第 2、3 步：收集后裁剪），
`TinycAssets.ASSETS_VERSION` 随内容变化递增。
