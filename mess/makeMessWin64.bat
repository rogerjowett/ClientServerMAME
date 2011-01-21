set PATH=C:\mingw-mame\mingw64-w32\bin;%PATH%
make -j4 NOWERROR=1 MSVC_BUILD=1 DIRECT3D=9 DIRECTINPUT=8 PTR64=1 TARGET=mess
make -j4 NOWERROR=1 MSVC_BUILD=1 DIRECT3D=9 DIRECTINPUT=8 PTR64=1 TARGET=mess DEBUG=1 SYMBOLS=1
copy vmess64.exe ..\mame\Hub\src\
copy vmess64.pdb ..\mame\Hub\src\
copy vmess64.exe Installer\ClientServerMESS\
copy vmess64.pdb Installer\ClientServerMESS\

