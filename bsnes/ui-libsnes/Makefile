include $(snes)/Makefile
include $(gameboy)/Makefile

# We define the API version here, so it can be included in the code and passed
# to the linker.
libsnesRevisionMajor := 1
libsnesRevisionMinor := 3
libsnesRevisionFull  := $(libsnesRevisionMajor).$(libsnesRevisionMinor)

flags += -DLIBSNES_REVISION_MAJOR=$(libsnesRevisionMajor)
flags += -DLIBSNES_REVISION_MINOR=$(libsnesRevisionMinor)

ifeq ($(findstring i386,$(arch)),i386)
	# i386 does not need -fPIC
else ifeq ($(findstring i686,$(arch)),i686)
	# i686 does not need -fPIC
else
	# Most architectures need -fPIC
	flags += -fPIC
endif

#rules
objects := $(objects) libsnes
objects := $(patsubst %,obj/%.o,$(objects))

obj/libsnes.o: $(ui)/libsnes.cpp $(ui)/*

#targets
build: $(objects)
ifeq ($(platform),x)
	ar rcs out/libsnes.a $^
	$(cpp) -o out/libsnes.so -shared -Wl,-soname,libsnes.so.$(libsnesRevisionMajor) -Wl,--version-script=$(ui)/version-script $^
else ifeq ($(platform),osx)
	ar rcs out/libsnes.a $^
	$(cpp) -o out/libsnes.dylib -install_name @executable_path/../Libraries/libsnes.dylib -shared -dynamiclib $^
else ifeq ($(platform),win)
	$(cpp) -o out/snes.dll -shared -Wl,--out-implib,libsnes.a -Wl,--version-script=$(ui)/version-script $^
endif

install: build
ifeq ($(platform),x)
	install -D -m 644 out/libsnes.a $(DESTDIR)$(prefix)/lib/libsnes.a
	install -D -m 644 out/libsnes.so $(DESTDIR)$(prefix)/lib/libsnes.so.$(libsnesRevisionFull)
	cd $(DESTDIR)$(prefix)/lib; ln -sf libsnes.so.$(libsnesRevisionFull) libsnes.so
	ldconfig -n $(DESTDIR)$(prefix)/lib
	install -D -m 644 ui-libsnes/libsnes.hpp $(DESTDIR)$(prefix)/include/libsnes.hpp
else ifeq ($(platform),osx)
	cp out/libsnes.dylib /usr/local/lib/libsnes.dylib
endif

uninstall:
ifeq ($(platform),x)
	-rm $(DESTDIR)$(prefix)/lib/libsnes.a
	-rm $(DESTDIR)$(prefix)/lib/libsnes.so
	-rm $(DESTDIR)$(prefix)/lib/libsnes.so.$(libsnesRevisionFull)
	ldconfig
	-rm $(DESTDIR)$(prefix)/include/libsnes.hpp
else ifeq ($(platform),osx)
	rm /usr/local/lib/libsnes.dylib
endif
