ui_objects := libsnes libco

obj/libsnes.o: ui-libsnes/libsnes.cpp ui-libsnes/libsnes.hpp

ui_objects := $(patsubst %,obj/%.o,$(ui_objects))

# We define the API version here, so it can be included in the code and passed
# to the linker.
libsnesRevisionMajor := 1
libsnesRevisionMinor := 1
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

ui_clean:
	-@$(call delete,$(ui_objects))

ui_build: $(ui_objects) $(snes_objects) $(gameboy_objects)
ifeq ($(platform),x)
	ar rcs out/libsnes.a $^
	$(cpp) -o out/libsnes.so -shared -Wl,-soname,libsnes.so.$(libsnesRevisionMajor) $^
else ifeq ($(platform),osx)
	ar rcs out/libsnes.a $^
	$(cpp) -o out/libsnes.dylib -install_name @executable_path/../Libraries/libsnes.dylib -shared -dynamiclib $^
else ifeq ($(platform),win)
	$(cpp) -o out/snes.dll -shared -Wl,--out-implib,libsnes.a $^
endif

ui_install: ui_build
ifeq ($(platform),x)
	install -D -m 644 out/libsnes.a $(DESTDIR)$(prefix)/lib/libsnes.a
	install -D -m 644 out/libsnes.so $(DESTDIR)$(prefix)/lib/libsnes.so.$(libsnesRevisionFull)
	cd $(DESTDIR)$(prefix)/lib; ln -sf libsnes.so.$(libsnesRevisionFull) libsnes.so
	ldconfig
	install -D -m 644 ui-libsnes/libsnes.hpp $(DESTDIR)$(prefix)/include/libsnes.hpp
else ifeq ($(platform),osx)
	cp out/libsnes.dylib /usr/local/lib/libsnes.dylib
endif

ui_uninstall:
ifeq ($(platform),x)
	-rm $(DESTDIR)$(prefix)/lib/libsnes.a
	-rm $(DESTDIR)$(prefix)/lib/libsnes.so
	-rm $(DESTDIR)$(prefix)/lib/libsnes.so.$(libsnesRevisionFull)
	ldconfig
	-rm $(DESTDIR)$(prefix)/include/libsnes.hpp
else ifeq ($(platform),osx)
	rm /usr/local/lib/libsnes.dylib
endif
