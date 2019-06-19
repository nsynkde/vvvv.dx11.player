# vvvv.dx11.player

Supports image sequence streaming from disk to GPU. Supported file formats are:
- `.tga`
- `.dds`
- `.dpx`


### Building
Open `DX11Player.sln` from `plugins/DX11Player` and build the solution file.

- **Release** build will output to `patches/` directory — ready for usage in vvvv.
- **Debug** builds will output to a `/build` directory relative to the Solution file.


### Debugging
1. Open `DX11Player.sln`
2. Rightclick `Properties` on the StartUp project (should be DX11Player)
3. Choose `Debug`
    4. Select `Start external program` and select your `vvvv.exe`

5. Under `Command line arguments`, use

```/o PATH_TO_YOUR_EXAMPLE_PATCH```

You might want to add `/showecxeptions true`, too


### Installation
Build with configuration `Release` and copy `patches/` into your
`%VVVV%/packs` directory.


### Structure:
`Dependencies` servers the following third party libraries, because NuGet packages seem to be out of date for these:
- FeralTic.dll
- VVV.DX11.Core.dll
- VVV.DX11.Lib.dll

### Example
1. Extract `data\bunny.7z` to `data\bunny`
2. Open `patches/nsynk.player.example.v4p`

### Additional Data
If you need additional data (e.g. for the examples) you can download and unzip
the file `vvvv.dx11.player.additional-data.zip` from `BOX\Current\Ongoing\Test
Image Sequences`.

