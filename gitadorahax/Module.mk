avsdlls            += gitadorahax

deplibs_gitadorahax    := \
    avs avs-ea3 \

ldflags_gitadorahax    := \
	-lwinmm -lpsapi

libs_gitadorahax       := \
	util \
	minhook

srcpp_gitadorahax      := \
    dllmain.cc \
	gitadorascorehook.cc \
