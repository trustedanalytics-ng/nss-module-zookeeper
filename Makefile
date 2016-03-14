INCLUDE_DIR=./include
OBJECT_DIR=./obj
LIBS=-lm

_DEPS=zk_nss.h configuration.h
_OBJ=zk_nss_group.o configuration.o zk.o zk_nss_passwd.o
SO_FILE=libnss_zk.so.2
SO_FILE_LOC=/lib64/$(SO_FILE)
EXE_FILE=zk_nss_test

CC=gcc
CF_FLAGS=-I$(INCLUDE_DIR) $(LIBS) -I/include

DEPS=$(patsubst %,$(INCLUDE_DIR)/%,$(_DEPS))

OBJ=$(patsubst %,$(OBJECT_DIR)/%,$(_OBJ))

$(OBJECT_DIR)/%.o: %.c
	$(CC) -c -fPIC -o $@ $< $(CF_FLAGS)

$(SO_FILE): $(OBJ)
	$(CC) $(OBJ) -shared -o $(SO_FILE) -l:libzookeeper_mt.so.2 $(CF_FLAGS)

compile: $(SO_FILE)
	$(info "Compiled")

$(SO_FILE_LOC): $(SO_FILE)
	$(shell cp $(SO_FILE) $(SO_FILE_LOC))

$(EXE_FILE): $(OBJECT_DIR)/$(EXE_FILE).o $(SO_FILE_LOC)
	$(CC) $(OBJECT_DIR)/$(EXE_FILE).o -o$(EXE_FILE) -l:$(SO_FILE) -l:libzookeeper_mt.so.2 $(CF_FLAGS)

install: $(SO_FILE_LOC)
	$(info "installed")

test: $(EXE_FILE)
	$(info $(shell ./$(EXE_FILE)))
	#TODO: create java tests

clean:
	rm -f $(OBJECT_DIR)/*.o *~ core $(INCLUDE_DIR)/*~ $(SO_FILE) $(EXE_FILE)
