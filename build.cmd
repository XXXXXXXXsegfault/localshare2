mkdir tmp
bin\scpp.exe src/main.c tmp/cc.i
bin\scc.exe tmp/cc.i tmp/cc.bcode
bin\bcode.exe tmp/cc.bcode tmp/cc.asm
bin\asm.exe tmp/cc.asm localshare2.exe tmp/cc.map