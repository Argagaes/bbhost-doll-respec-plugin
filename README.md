# Doll Rebirth for bbhost

A Windows plugin for [bbhost](https://github.com/droogie/bbhost) 0.2.35 and Bloodborne 1.09. It adds **Rebirth in the Nightmare** to the Doll's menu in the Hunter's Dream. After confirmation, your hunter returns to the origin's level and attributes, and the Blood Echoes spent on attribute points are refunded. The level-up menu follows, with an option to accept or undo.

## Installation

1. Download `doll_rebirth.dll` from Releases and place it in the `plugins` folder next to the original `bbhost.exe`.
2. Restart bbhost. The plugin is enabled by default; the Doll must be available to talk to.
3. Back up your saves before trying it. The default Windows save folder is `%LOCALAPPDATA%\bbhost\data\saves\SPRJ0005`.

This plugin needs bbhost's normal decrypted 1.09 eboot. It does not require the Yharnam Stone or bbhost's altar rebirth setting. The original game files are not modified. To uninstall, remove the DLL or disable the plugin in bbhost's Plugins menu, then restart.

## Status and limitations

This version was tested against the 1.09 Doll dialogue and a simulated host covering reset, refund, undo, and rejection when the echo cap would be exceeded. **It has not yet been tested in-game.** On failure it reuses an existing bbhost message that can incorrectly mention the altar. A mod overriding the same Hunter's Dream talk archive can prevent the plugin from working. An unsigned plugin will not load if `[plugins] signed_only = true`.

## Building

The release DLL was built with Clang in MSYS2 UCRT64 and a static zlib 1.3.1 library. The exact zlib source archive used for the build is in `third_party/`. After extracting and building it as a static library, run from `source/` with the zlib build prefix substituted:

```sh
clang++ -std=c++17 -O2 -shared -static -static-libstdc++ -static-libgcc \
  -I. -I<zlib-prefix>/include doll_rebirth.cpp doll_script.cpp \
  engine/esd.cpp core/sha256.cpp gcn/container.cpp \
  <zlib-prefix>/lib/libzlibstatic.a -o doll_rebirth.dll
```

`plugins/doll_rebirth.dll` is the same binary distributed as the release asset. No Bloodborne game archives, eboot, save files, or example mod files are included.

## Credits and licenses

The plugin includes the plugin API, ESD, container, and SHA-256 code from [droogie/bbhost](https://github.com/droogie/bbhost), source revision `daea6d09479b6d87f3fb3a1fad22c9c304f77055`. bbhost and this plugin are distributed under GPL-3.0-or-later; see [LICENSE.txt](LICENSE.txt). bbhost's developers are credited for those components and the original rebirth implementation.

The DLL statically links [zlib 1.3.1](https://github.com/madler/zlib/tree/v1.3.1), copyright Jean-loup Gailly and Mark Adler. Its original source archive and license are in [third_party/](third_party/).
