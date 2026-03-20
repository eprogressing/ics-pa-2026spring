include $(AM_HOME)/Makefile.check

NAME = klib
ARCHIVE ?= $(shell pwd)/build/$(NAME)-$(ARCH).a

.DEFAULT_GOAL = archive

.PHONY: archive clean

archive:
	@test -f $(ARCHIVE) || (echo "Missing prebuilt archive: $(ARCHIVE)" && false)

clean:
	rm -rf build/
