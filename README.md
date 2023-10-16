# rcbot
This is a fork of Cheeseh's RCBot source for Sven Co-op 4.5.  
http://rcbot.bots-united.com/forums/index.php?s=&showtopic=1653&view=findpost&p=10905

To maintain compatibility with future releases of Sven Co-op, the private entity headers must be updated. In the past, this was done by asking Sniper to send `cbase.h` from the private game code. Since he [no longer seems willing](http://rcbot.bots-united.com/forums/index.php?s=&showtopic=2057&view=findpost&p=14303) to do this, the API must be reverse engineered instead. That can be done relatively easily now with this [ApiGenerator](https://github.com/wootguy/ApiGenerator).

## Building the source
Windows:  
1. Intall Git for windows, CMake, and Visual Studio 2022 (select "Desktop development with C++")
2. Open a command prompt somewhere and run this command to download the code:
```
git clone --recurse-submodules https://github.com/wootguy/rcbot
```
3. Open the rcbot folder and run `build.bat`

Linux:
```
git clone --recurse-submodules https://github.com/wootguy/rcbot
cd rcbot
sh build.sh
```