# Battlefield: Bad Company 2 Vita

<p align="center"><img src="./screenshots/game.png"></p>

This is a wrapper/port of *Battlefield: Bad Company 2 Android* for the *PS Vita*.

The port works by loading the official Android ARMv6 executable in memory, resolving its imports with native functions and patching it in order to properly run.

## Setup Instructions (For End Users)

In order to properly install the game, you'll have to follow these steps precisely:

- Install [kubridge](https://github.com/TheOfficialFloW/kubridge/releases/) and [FdFix](https://github.com/TheOfficialFloW/FdFix/releases/) by copying `kubridge.skprx` and `fd_fix.skprx` to your taiHEN plugins folder (usually `ux0:tai`) and adding two entries to your `config.txt` under `*KERNEL`:
  
```
  *KERNEL
  ux0:tai/kubridge.skprx
  ux0:tai/fd_fix.skprx
```

**Note** Don't install fd_fix.skprx if you're using repatch plugin

- Obtain your copy of *Battlefield: Bad Company 2* legally from the Amazon store in form of an `.apk` file and one or more `.obb` files (usually located inside the `/sdcard/android/obb/bc2/`) folder. [You can get all the required files directly from your phone](https://stackoverflow.com/questions/11012976/how-do-i-get-the-apk-of-an-installed-app-without-root-access) or by using an apk extractor you can find in the play store. The apk can be extracted with whatever Zip extractor you prefer (eg: WinZip, WinRar, etc...) since apk is basically a zip file. You can rename `.apk` to `.zip` to open them with your default zip extractor.
- Copy the `/sdcard/android/obb/bc2/` folder to `ux0:data/bc2`.
- Open the apk and extract `libbc2.so` from the `lib/armeabi` folder to `ux0:data/bc2`.
- Install [BC2.vpk](https://github.com/TheOfficialFloW/bc2_vita/releases/download/v1.0/BC2.vpk) on your *PS Vita*.

## Build Instructions (For Developers)

--

## Credits

- Once13One for providing LiveArea assets.