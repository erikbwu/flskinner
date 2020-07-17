# ![](https://i.imgur.com/3ivYF1X.png) 
# flskinner

## A tool to apply basic skins to FL Studio 12+ ***without piracy or dll replacements***.

### Download

[Latest release](https://google.com)

### What versions does this support?

This should support any FL studio version that is 12.5 and above.
FL 12.5 may have issues with the default skin that ships with this.
It only works on Windows and with the 64-bit version of FL studio.

### Media

![](https://i.imgur.com/m0pAfJC.png)
![](https://i.imgur.com/yiZNz56.png)

### Is this piracy?

No. See below.

I do not condone piracy and will not offer any support for pirated versions of the software.

### How does it work?

Classic third-party skins for FL studio work by editing the Delphi Form Files in the resources of the FL Engine DLL. This usually involves a patched/cracked DLL which means it always ends up being piracy.

This tool takes another approach. With the introduction of code virtualization on newer FL studio versions, it has became much more difficult to edit the resource files.

Quick rundown:

* It launches FL studio in a suspended state, and injects a payload DLL which will place detours on the essential WINAPI calls required to load resources.
* When FL studio goes to load one of the Delphi Form Files, it is intercepted, and parsed. 
* After the parsing is done, it will search for variables with colors and apply color replacements from a user-defined JSON file.
* Once the modifications have been applied, the DFM is put back into binary form, and then FL studio receives the modified file, all within memory and without replacing any files.
* Other hooks are installed on a couple other functions *(it is unknown to me what they actually are)* used to apply colors to various things such as the mixer and certain panels that use a different style of rendering.

### Instructions

* Extract the zip in its own folder, you cannot launch it out of the zip.
* Upon launch, it will search for FL studio in its default location **(C:\Program Files (x86)\Image-Line\FL Studio 20)**
* If it cannot find FL studio, it will prompt you to select the FL studio folder. Pick the folder that contains **FL64.exe**.
* Just select the skin and press **Launch FL Studio**.
* You ***MUST*** launch FL studio using the tool, or else it will not apply the skin. **This does not modify your FL studio files in any way.**
* Feel free to join our [Discord](https://discord.gg/4N3EZqa) for help.

### Caveats

* You must launch FL studio using the flskinner UI.

* Only supports 64-bit Windows for the time being.

* Right now this only does basic color replacements, and not everything can be replaced. Lots of things in FL now have user color customization, such as playlist tracks, patterns, mixer tracks. This makes it difficult to just "replace the color". Some of these can be changed by creating your own FLP files with preset colors.

### More info

This was just a fun little project. This isn't intended to be some super serious tool. Image-Line has already said that there will be user-customization in a 20.x update.

[Discord](https://discord.gg/4N3EZqa)