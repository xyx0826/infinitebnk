# infinitebnk

Utility for extracting Wwise SoundBanks from Halo Infinite modules.

## Credits

This project heavily uses [Coreforge/libInfinite](https://github.com/Coreforge/libInfinite) for reading module data.

This project uses tag file path information from [Gamergotten/Infinite-runtime-tagviewer](https://github.com/Gamergotten/Infinite-runtime-tagviewer).

## Usage

First, download [`tagnames.txt`](https://github.com/Gamergotten/Infinite-runtime-tagviewer/raw/master/files/tagnames.txt)
from Gamergotten/Infinite-runtime-tagviewer.
This file helps map some SoundBank tag asset IDs to real paths instead of hexadecimal placeholders, but is not mandatory.

On a command line, call infinitebnk with `infinitebnk.exe <path-to-deploy-folder> [path-to-tagnames]`.
Usual command line tips apply:

- `<path-to-deploy-folder>` is the path to the `deploy` folder within your Halo Infinite installation.
  - E.g. `"E:/Steam/steamapps/common/Halo Infinte/deploy"`
- `[path-to-tagnames]` is the optional path to your downloaded `tagnames.txt`.
- If a path contains whitespaces, wrap it in double quotes.

If everything goes well, SoundBanks will be extracted to the `soundbanks` folder under your command line's current working directory.

## Troubleshooting

If the app crashes, capture its full output by running the same command line but with `> output.txt` added to the end.
Examine and provide `output.txt` for debugging.

## Building

Below instructions apply to Windows.
I couldn't get jsoncpp and a few other libInfinite dependencies to work, so I butchered them. Sorry Coreforge :/

- Initialize and update all git submodules recursively
- Find a Windows build of libpng (1.2.37 tested) and extract it to `3rd_party/libpng-1.2.37`
- Find a Windows build of zlib (1.2.3 tested) and extract it to `3rd_party/zlib-1.2.3`
- Configure and build with CMake
