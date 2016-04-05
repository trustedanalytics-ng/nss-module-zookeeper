#  Name Service Switch module for zookeeper
#  Copyright (C) 2016, Intel Corporation.
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
#  USA

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
	$(info "Installed")

clean:
	rm -f $(OBJECT_DIR)/*.o *~ core $(INCLUDE_DIR)/*~ $(SO_FILE) $(EXE_FILE)
