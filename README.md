# EasyPutty
易用 多标签 支持 putty

1. 生成putty证书
2. 证书管理
3. Tags管理
4. 启动pageant.exe

## Cmder + PuTTY
- 优点：内存占用少
- 缺点：机器ip多了不好配置

## SuperPuTTY
- 优点：机器ip管理比较方便，但没有Solar-PuTTY配置证书优
- 缺点：操作有点卡，任务栏唤醒有时要点好几次，最小化最大化会白屏闪一下，打开多机器，有机器超时断开时标签选中页会跳

## Solar-PuTTY
- 优点：机器ip管理方便
- 缺点：内存占用多，有广告，GPU占用大

## 标签样式
开始编程时tabcontrol标签头显示旧系统样式，为显示新版本系统样式，修改`EasyPuTTY.vcxproj`在`<Link>`节点下增加下面配置实现
```
      <AdditionalManifestDependencies>type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'</AdditionalManifestDependencies>
      <AdditionalDependencies>comctl32.lib;comdlg32.lib;shlwapi.lib;%(AdditionalDependencies)</AdditionalDependencies>
      <AdditionalLibraryDirectories>$(WindowsSdkDir)Lib\$(Platform)\um;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
```

## 第三方下载
- [putty] http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html
- [winscp] https://winscp.net/download/WinSCP-5.15.3-Automation.zip
