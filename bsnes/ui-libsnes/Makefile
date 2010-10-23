ui_objects := libsnes

obj/libsnes.o: ui-libsnes/libsnes.cpp ui-libsnes/libsnes.hpp

ui_objects := $(patsubst %,obj/%.o,$(ui_objects))

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

ui_build: $(ui_objects) $(snes_objects)
ifeq ($(platform),x)
	ar rcs out/libsnes.a $^
	$(cpp) -o out/libsnes.so -shared -Wl,-soname,libsnes.so.1 $^
else ifeq ($(platform),osx)
	ar rcs out/libsnes.a $^
	$(cpp) -o out/libsnes.dylib -install_name @executable_path/../Libraries/libsnes.dylib -shared -dynamiclib $^
else ifeq ($(platform),win)
	$(cpp) -o out/snes.dll -shared -Wl,--out-implib,libsnes.a $^
endif

ui_install: ui_build
ifeq ($(platform),x)
	install -D -m 755 out/libsnes.a $(DESTDIR)$(prefix)/lib/libsnes.a
	install -D -m 755 out/libsnes.so $(DESTDIR)$(prefix)/lib/libsnes.so
	ldconfig
else ifeq ($(platform),osx)
	cp out/libsnes.dylib /usr/local/lib/libsnes.dylib
endif

ui_uninstall:
ifeq ($(platform),x)
	-rm $(DESTDIR)$(prefix)/lib/libsnes.a
	-rm $(DESTDIR)$(prefix)/lib/libsnes.so
	ldconfig
else ifeq ($(platform),osx)
	rm /usr/local/lib/libsnes.dylib
endif
