set commoncompilerflags=-O2 -Z7 -nologo -W4 -WX -wd4201 -wd4505 -wd4996 -wd4100 -wd4702 -wd4189 -wd4706 -wd4127 -wd4311 -wd4302 -wd4312 -wd4310 -FC -GR- -EHa- -MTd -Gm-
set commoncompilerflags= -DFORGIVENESS_STREAMING=0 -DFORGIVENESS_SLOW=1 -DFORGIVENESS_INTERNAL=0 -DDEFAULT_WIDTH=1920 %commoncompilerflags%
set commonlinkerflags=-opt:ref -incremental:no
