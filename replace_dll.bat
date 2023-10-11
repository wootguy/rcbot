cd "C:\Games\Steam\steamapps\common\Sven Co-op\svencoop\addons\metamod\dlls"

if exist rcbot_old.dll (
    del rcbot_old.dll
)
if exist rcbot.dll (
    rename rcbot.dll rcbot_old.dll 
)

exit /b 0