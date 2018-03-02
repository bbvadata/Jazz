
help:
	@echo ""
	@echo "make [nothing],"
	@echo "make help           : Show this help."
	@echo ""
	@echo "make jazz           : Make the Jazz executable."

	@echo ""


# Variables:
# ----------

LINUX           := Ubuntu_16.10
HOME            := $(shell pwd)
LD_LIBRARY_PATH := $(HOME)/libs
CXXFLAGS        := -std=c++11 -I$(HOME) -I$(HOME)/include/R/ -I$(HOME)/include/libssl/

RFLAGS := -DNDEBUG

CPPFLAGS := $(RFLAGS)

# echo src/*
VPATH   =  src/catch2 src/curl src/include src/jazz_blocks src/jazz_main src/jazz_misc src/jazz_utils src/lmdb

# find src/ |grep -e '.*jazz_.*cpp$' | sed 's/.*\(jazz_.*cpp\)$/\1/' | sed 's/cpp/o/'
objects = jazz_logger.o jazz_config.o jazz_miscutils.o jazz_blocks.o jazz_blockconv.o jazz_alloc.o jazz_websource.o jazz_commons.o jazz_http.o jazz_api.o jazz_main.o jazz_instances.o


# Header file dependencies:
# -------------------------

jazz_logger.o:     jazz_commons.h
jazz_config.o:     jazz_commons.h
jazz_miscutils.o:  jazz_commons.h
jazz_blocks.o:     jazz_commons.h
jazz_blockconv.o:  jazz_commons.h
jazz_alloc.o:      jazz_commons.h
jazz_websource.o:  jazz_commons.h
jazz_commons.o:    jazz_commons.h
jazz_http.o:       jazz_commons.h
jazz_api.o:        jazz_commons.h
jazz_main.o:       jazz_commons.h
jazz_instances.o:  jazz_commons.h


bin_release:
	@mkdir bin_release
	$(eval CPPFLAGS = $(RFLAGS))


jazz: bin_release $(objects)
	@echo "Making RELEASE Jazz ..."
	g++ -o bin_release/rjazz $(objects) -Ilibs -Llibs -lmicrohttpd -lpthread -lR -llmdb -lcrypto -lssl -lcurl


.PHONY : info
info   :
	@echo "Current Jazz home           : $(HOME)"
	@echo "Current Linux platform      : $(LINUX)"
	@echo "Current CPPFLAGS            : $(CPPFLAGS)"
	@echo "Current CXXFLAGS            : $(CXXFLAGS)"
	@echo "Current LD_LIBRARY_PATH     : $(LD_LIBRARY_PATH)"
