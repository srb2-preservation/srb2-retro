#
# Makefile options for unices (linux, bsd...)
#

ifeq (${DEDICATED},1)
EXENAME?=lsrb2dretro
else
EXENAME?=lsdl2srb2retro
endif

opts+=-DUNIXCOMMON -DLUA_USE_POSIX
libs+=-lm

ifndef nasm_format
nasm_format:=elf -DLINUX
endif

ifndef NOHW
opts+=-I/usr/X11R6/include
libs+=-L/usr/X11R6/lib
endif

SDL=1

# In common usage.
ifdef LINUX
libs+=-lrt
passthru_opts+=NOTERMIOS
endif

# Tested by Steel, as of release 2.2.8.
ifdef FREEBSD
opts+=-I/usr/X11R6/include -DLINUX -DFREEBSD
libs+=-L/usr/X11R6/lib -lkvm
endif

ifdef OPENBSD
libs+=-lexecinfo -lpthread
endif

ifdef MACOS
libs+=-framework CoreFoundation
endif
