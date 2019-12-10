cflags          += -DWIN32_LEAN_AND_MEAN -DCOBJMACROS -Ipkcs11 -Wno-attributes

avsvers_32      := 1700 1508
avsvers_64      := 1700 1509

imps            += avs avs-ea3

include util/Module.mk
include minhook/Module.mk
include iidxhax/Module.mk
include popnhax/Module.mk
include jubeathax/Module.mk
include gitadorahax/Module.mk
include ddrhax/Module.mk

#
# Distribution build rules
#

zipdir          := $(BUILDDIR)/zip

$(zipdir)/:
	mkdir -p $@

$(BUILDDIR)/iidxhax.zip: \
		build/bin/avs2_1700-64/iidxhax.dll \
		dist/iidxhax/iidxhax.xml \
		| $(zipdir)/
	echo ... $@
	zip -j $@ $^

$(BUILDDIR)/popnhax.zip: \
		build/bin/avs2_1508-32/popnhax.dll \
		dist/popnhax/popnhax.xml \
		| $(zipdir)/
	echo ... $@
	zip -j $@ $^

$(BUILDDIR)/jubeathax.zip: \
		build/bin/avs2_1700-32/jubeathax.dll \
		dist/jubeathax/jubeathax.xml \
		| $(zipdir)/
	echo ... $@
	zip -j $@ $^

$(BUILDDIR)/gitadorahax.zip: \
		build/bin/avs2_1509-64/gitadorahax.dll \
		dist/gitadorahax/gitadorahax.xml \
		| $(zipdir)/
	echo ... $@
	zip -j $@ $^

$(BUILDDIR)/ddrhax.zip: \
		build/bin/avs2_1508-32/ddrhax.dll \
		dist/ddrhax/ddrhax.xml \
		| $(zipdir)/
	echo ... $@
	zip -j $@ $^

$(BUILDDIR)/bemanihax.zip: \
		$(BUILDDIR)/iidxhax.zip \
		$(BUILDDIR)/popnhax.zip \
		$(BUILDDIR)/jubeathax.zip \
		$(BUILDDIR)/gitadorahax.zip \
		$(BUILDDIR)/ddrhax.zip \
		| $(zipdir)/
	echo ... $@
	zip -9 -q -j $@ $^

all: $(BUILDDIR)/bemanihax.zip
