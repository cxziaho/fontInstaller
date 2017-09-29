# fontIntaller
An app for installing the [fontRedirect](https://github.com/cxziaho/fontRedirect) plugin, and selecting fonts using a GUI.  Hopefully it isn't too buggy, but if you can do better please feel free to rewrite it!  
## Important Notes  
Currently, the `fontRedirect` plugin causes certain games to crash on openening them (Gravity Rush, Ys 8 JP English Patched and Hyperdimension Neptunia Re;Birth1 for example) and I wont be able to fix this for a while.  This is not permanent, just uninstall the plugin.  If you manage to fix this, submit a pull request and I will accept it! :)  
### Info
fontInstaller reads fonts in `ux0:data/font/`, which can be either **.otf** or **.ttf**.  It reads up to 256 font files before stopping.  
### Getting Started
Install the VPK file, open it and wait for the plugin to be installed.  Add fonts to `ux0:data/font/` and open the app again (this can be done before initial install as well).  Select a font in the app and press start, then your enter button, CIRCLE or CROSS.  Your Vita should restart with this new font, and will keep this font unless you boot without plugins or change font.  
## Building   
Assuming you have the [VitaSDK](http://vitasdk.org) toolchain:  
Before this, install my custom [libvita2d Fork](https://github.com/cxziaho/libvita2d) which adds font info functions (`make` then `make install`).
```
git clone https://github.com/cxziaho/fontInstaller.git  
cd fontInstaller/module
mkdir build && cd build
cmake ..
make
cd ../
mkdir build && cd build
cmake ..
make
```
### Other
Your config.txt is backed up to `ux0:data/font/backup.txt`, so if this corrupts it for some reason your config will be there.  
The current font name is stored in `ux0:data/font/config.txt` so you can change that directly.
