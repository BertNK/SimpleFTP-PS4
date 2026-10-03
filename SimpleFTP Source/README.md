# SFTP (SimpleFTP)

A small PS4 homebrew app for transferring files between a PS4 and a PC over your local network. The PS4 hosts an FTP server; connect to it from a PC using FileZilla or a file manager with FTP support.

> **About the name:** SFTP here means “SimpleFTP.” The app uses regular, unencrypted FTP. It is not SSH File Transfer Protocol and should only be used on a trusted local network.

## What it does

- Transfers files both ways: upload from PC to PS4, or download from PS4 to PC.
- Uses FTP control port **2122** with dynamic passive data ports, active-mode fallback, and up to four simultaneous PC connections.
- Streams file data in 256 KiB blocks to keep memory use small while transferring large files.
- Shows the PS4’s network address and beginner setup steps on screen.
- Uses `sce_sys/icon0.png` for the home screen icon and includes `sce_sys/pic1.png` as optional background art when that file is present at build time.
- Shows connection instructions each time it opens. Press controller **X** or **Circle**, or TV remote **OK/center** if the PS4 forwards it as a confirm button; press again on the main screen to reopen the instructions.
- Uses its own FTP server. GoldHEN must be active to run homebrew, but its built-in FTP server is separate (commonly port 2121).

The server accepts anonymous connections and exposes the PS4 filesystem from `/`. Anyone on the same reachable network can browse, upload, or overwrite files while the app is running. Use it only on a trusted LAN, and avoid changing system files.

## Use it

1. Install and launch the PKG on a PS4 that can run homebrew with GoldHEN.
2. Connect the PS4 and PC to the same router. Note the PS4 IP address shown in the app.
3. In FileZilla, connect with **Host** set to the PS4 IP, **Port** `2122`, **Protocol** FTP, and **User** `anonymous`. Passive or active transfer mode is supported.
4. Drag files between the PC and PS4 panes. To install a transferred homebrew PKG, use the package installer available in your PS4 homebrew setup.

If your Windows File Explorer build supports FTP, enter `ftp://PS4-IP:2122/` in its address bar. FileZilla is the recommended option when Explorer does not support FTP.

### Sony TV remote

Enable **Settings > System > Enable HDMI Device Link** on the PS4 and enable HDMI-CEC/device control on the TV. The app accepts both PS4 confirm buttons (Cross and Circle), so it works with either **Use X Button for Enter** preference when the TV remote's center/OK button is passed through to the foreground app. The PS4 controller API exposes button states, not a separate TV-remote/CEC event. Sony notes HDMI Device Link may be unavailable during some activities, including games or video, and functions vary by TV model. If OK does not dismiss the instructions, the PS4 is not forwarding it to this app; use a supported controller.

## Build on Windows

### Requirements

- Windows x64.
- OpenOrbis PS4 Toolchain **v0.5.4** installed. The installer sets `OO_PS4_TOOLCHAIN`.
- LLVM for Windows, including `clang++` and `ld.lld`. The build script checks `C:\Program Files\LLVM\bin` automatically; otherwise add LLVM’s `bin` folder to `PATH`.
- A .NET runtime for the OpenOrbis packaging utility. The build script enables major-version roll-forward to use a newer installed runtime where possible.

### Build steps

1. Copy the entire `lan_transfer` project folder out of the OpenOrbis SDK if you want to keep or publish it separately. Keep `assets`, `lan_transfer`, `sce_sys`, both build files, and this README together. The folder is self-contained for source and artwork, but building still requires an installed OpenOrbis toolchain and LLVM; the build script gets headers, libraries, utilities, and runtime modules from that installation.
2. Install the requirements above.
3. Double-click `build.bat`, or open Command Prompt in this folder and run `build.bat`.
4. The finished package is **`SFTP (SimpleFTP).pkg`** in this folder.

For custom art, replace `sce_sys/icon0.png` with a **512 × 512, 24-bit PNG with no transparency**. For the app’s background image, add `sce_sys/pic1.png` as a **1920 × 1080 PNG**. The build scripts include `pic1.png` automatically when present. The icon dimensions and color format follow the [OpenOrbis homebrew packaging guide](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/docs/MD/Building%20Homebrew.md); the background dimensions follow common PS4 package artwork conventions.

The interface palette is:

| Use | Hex | RGB |
|---|---|---|
| Main background | `#0C131F` | `12, 19, 31` |
| Help panel | `#141F30` | `20, 31, 48` |
| Cyan accent | `#49CDDC` | `73, 205, 220` |
| Main text | `#EEF4FA` | `238, 244, 250` |
| Secondary text | `#97ABBE` | `151, 171, 190` |

The build script changes to its own folder before building, so you can copy or rename the project folder and build it from its new location. To install on the PS4, copy only `SFTP (SimpleFTP).pkg` to a USB drive. Keep the whole project folder if you want to build again or edit the source.

## Repository layout

```text
lan_transfer/
├── assets/fonts/          # App font
├── lan_transfer/main.cpp  # UI and FTP server
├── lan_transfer/graphics.cpp, graphics.h, log.h  # Local rendering helper
├── sce_sys/               # PS4 app icon and package metadata location
├── sce_module/            # OpenOrbis runtime modules, copied from the SDK during build
├── .gitignore              # Excludes generated binaries and SDK module copies
├── build.bat              # Windows build and package script
├── Makefile               # Linux/macOS build script
└── README.md
```

## License

This sample uses the Gontserrat font bundled with the OpenOrbis toolchain sample assets. See `assets/fonts/OFL.txt` for its license. The local graphics helper is based on the OpenOrbis toolchain sample helper.
