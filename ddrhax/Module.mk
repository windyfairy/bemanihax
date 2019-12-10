avsdlls            += ddrhax

deplibs_ddrhax    := \
    avs \

ldflags_ddrhax    := \
	-lwinmm -lpsapi

libs_ddrhax       := \
	util \
	minhook

srcpp_ddrhax      := \
    dllmain.cc
