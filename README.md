# rcbot
This is a fork of Cheeseh's RCBot source for Sven Co-op 4.5.  
http://rcbot.bots-united.com/forums/index.php?s=&showtopic=1653&view=findpost&p=10905

To maintain compatibility with future releases of Sven Co-op, the private entity headers must be updated. In the past, this was done by asking Sniper to send `cbase.h` from the private game code. Since he [no longer seems willing](http://rcbot.bots-united.com/forums/index.php?s=&showtopic=2057&view=findpost&p=14303) to do this, the API must be reverse engineered instead. That can be done relatively easily now with this [ApiGenerator](https://github.com/wootguy/ApiGenerator).

## Building the source
Windows:  
1. Intall Git for windows, CMake, and Visual Studio 2022 (select "Desktop development with C++")
2. Update `SVEN_ROOT_PATH` in `CMakeLists.txt` to point to your Sven Co-op folder.
```
git clone --recurse-submodules https://github.com/wootguy/rcbot
cd rcbot && mkdir build && cd build
cmake -A Win32 ..
cmake --build . --config Release
```
Linux:
```
git clone --recurse-submodules https://github.com/wootguy/rcbot
cd rcbot && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make
```
