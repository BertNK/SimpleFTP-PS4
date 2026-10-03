# SFTP (SimpleFTP)

A small PS4 homebrew app for transferring files between a PS4 and a PC over your local network. The PS4 hosts an FTP server; connect to it from a PC using FileZilla or a file manager with FTP support.

The app uses regular, unencrypted FTP. It is not SSH File Transfer Protocol and should only be used on a trusted local network.

## What it does

- Transfers files both ways: upload from PC to PS4, or download from PS4 to PC.
- Uses its own FTP server. GoldHEN must be active to install and run this app, but its built-in FTP server is separate (commonly HEN port 2121).

The server accepts anonymous connections and exposes the PS4 filesystem from `/`. Anyone on the same reachable network can browse, upload, or overwrite files while the app is running. Use it only on a trusted LAN to prevent data theft, and avoid changing system files.

## Use it

1. Install and launch the PKG on a PS4 that can run homebrew with GoldHEN.
2. Connect the PS4 and PC to the same router. Note the PS4 IP address shown in the app.
3. In FileZilla, connect with **Host** set to the PS4 IP, **Port** `2122`, **Protocol** FTP, and **User** `anonymous`. Passive or active transfer mode is supported.
4. Drag files between the PC and PS4 panes. To install a transferred homebrew PKG, use the package installer available in your PS4 homebrew setup.

If your Windows File Explorer build supports FTP, enter `ftp://PS4-IP:2122/` in its address bar. FileZilla is the recommended option when Explorer does not support FTP.

### Requirements

- Windows x64.
- [OpenOrbis PS4 Toolchain **v0.5.4**](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases) installed. The installer sets `OO_PS4_TOOLCHAIN`.
- [LLVM for Windows](https://github.com/llvm/llvm-project/releases), including `clang++` and `ld.lld`. The build script checks `C:\Program Files\LLVM\bin` automatically; otherwise add LLVM’s `bin` folder to `PATH`.
- A .NET runtime for the OpenOrbis packaging utility. The build script enables major-version roll-forward to use a newer installed runtime where possible.

### Build steps

1. Copy the entire `lan_transfer` project folder out of the OpenOrbis SDK if you want to keep or publish it separately. Keep `assets`, `lan_transfer`, `sce_sys`, both build files, and this README together. The folder is self-contained for source and artwork, but building still requires an installed OpenOrbis toolchain and LLVM; the build script gets headers, libraries, utilities, and runtime modules from that installation.
2. Install the requirements above.
3. Double-click `build.bat`, or open Command Prompt in this folder and run `build.bat`.
4. The finished package is **`SFTP (SimpleFTP).pkg`** in this folder.

The build script changes to its own folder before building, so you can copy or rename the project folder and build it from its new location. To install on the PS4, copy only `SFTP (SimpleFTP).pkg` to a USB drive. Keep the whole project folder if you want to build again or edit the source.

## License

This app uses the Gontserrat font bundled with the OpenOrbis toolchain sample assets. The local graphics helper is based on the OpenOrbis toolchain sample helper.

Made by [BertNK](https://github.com/BertNK)