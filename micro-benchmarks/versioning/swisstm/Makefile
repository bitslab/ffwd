include Makefile.in

CPPFLAGS += -DUSE_STANDARD_IOSTREAM

OBJFILES = $(OBJ_DIR)/wlpdstm.o

####################################
# choose files to compile and link #
####################################

ifeq ($(STM), swisstm)
	OBJFILES += $(OBJ_DIR)/transaction.o $(OBJ_DIR)/privatization_tree.o
endif

ifeq ($(STM), epochstm)
	OBJFILES += $(OBJ_DIR)/transaction.o
endif

ifeq ($(STM), tlrw)
	OBJFILES += $(OBJ_DIR)/transaction.o
endif

ifeq ($(STM), rw)
	OBJFILES += $(OBJ_DIR)/transaction.o
endif

ifeq ($(STM), p-tlrw)
	OBJFILES += $(OBJ_DIR)/transaction.o
endif

ifeq ($(STM), dynamic)
	OBJFILES += $(OBJ_DIR)/transaction.o $(OBJ_DIR)/mixed.o $(OBJ_DIR)/eager.o $(OBJ_DIR)/lazy.o
endif

MUBENCH_RBTREE_OBJFILES = $(OBJ_DIR)/intset-rbtree.o
MUBENCH_RBFOREST_OBJFILES = $(OBJ_DIR)/rbforest.o

.PHONY: clean

all: $(INCLUDE_OUT_FILE) $(LIB_DIR)/libwlpdstm.a $(OBJ_DIR)/intset-rbtree $(OBJ_DIR)/rbforest

###############
# create dirs #
###############

OBJ_DIR_TS = $(OBJ_DIR)/.ts
LIB_DIR_TS = $(LIB_DIR)/.ts
INCLUDE_DIR_TS = $(INCLUDE_DIR)/.ts

$(OBJ_DIR_TS):
	mkdir -p $(OBJ_DIR)
	touch $@

$(LIB_DIR_TS):
	mkdir -p $(LIB_DIR)
	touch $@

$(INCLUDE_DIR_TS):
	mkdir -p $(INCLUDE_DIR)
	touch $@

#######################
# create include file #
#######################

$(INCLUDE_OUT_FILE): $(INCLUDE_DIR_TS)
	echo "#ifndef STM_H_" > $(INCLUDE_OUT_FILE)
	echo "#define STM_H_" >> $(INCLUDE_OUT_FILE)
	echo >> $(INCLUDE_OUT_FILE)
	echo | awk '{input = "$(LPDSTM_CPPFLAGS)";split(input, defs, " ");for(idx in defs) if(defs[idx] != "-D") print "#define " defs[idx]}' >> $(INCLUDE_OUT_FILE)
	cat $(INCLUDE_IN_FILE) >> $(INCLUDE_OUT_FILE)
	echo >> $(INCLUDE_OUT_FILE)
	echo "#endif" >> $(INCLUDE_OUT_FILE)

##################
# create library #
##################

# create lib
$(LIB_DIR)/libwlpdstm.a: $(LIB_DIR_TS) $(OBJFILES)
	$(AR) cru $@ $(OBJFILES)

# compile
#$(OBJ_DIR)/wlpdstm.o: $(OBJ_DIR) $(STM_API_DIR)/wlpdstm.cc $(STM_API_DIR)/wlpdstm.h
#	$(CPP) $(CPPFLAGS) $(STM_API_DIR)/wlpdstm.cc -c -o $@

$(OBJ_DIR)/wlpdstm.o: $(OBJ_DIR_TS) $(STM_API_DIR)/wlpdstm.cc $(STM_API_DIR)/wlpdstm.h
	$(CPP) $(CPPFLAGS) $(STM_API_DIR)/wlpdstm.cc -c -o $@

ifeq ($(STM), swisstm)
$(OBJ_DIR)/transaction.o: $(OBJ_DIR_TS) $(STM_SRC_DIR)/transaction.cc $(STM_SRC_DIR)/transaction.h
	$(CPP) $(CPPFLAGS) $(STM_SRC_DIR)/transaction.cc -c -o $@

$(OBJ_DIR)/privatization_tree.o: $(OBJ_DIR_TS) $(STM_SRC_DIR)/privatization_tree.cc $(STM_SRC_DIR)/privatization_tree.h
	$(CPP) $(CPPFLAGS) $(STM_SRC_DIR)/privatization_tree.cc -c -o $@
endif

ifeq ($(STM), epochstm)
$(OBJ_DIR)/transaction.o: $(OBJ_DIR_STMAP) $(EPOCHSTM_SRC_DIR)/transaction.cc $(EPOCHSTM_SRC_DIR)/transaction.h
	$(CPP) $(CPPFLAGS) $(EPOCHSTM_SRC_DIR)/transaction.cc -c -o $@
endif

ifeq ($(STM), tlrw)
$(OBJ_DIR)/transaction.o: $(OBJ_DIR_TS) $(TLRW_SRC_DIR)/transaction.cc $(TLRW_SRC_DIR)/transaction.h
	$(CPP) $(CPPFLAGS) $(TLRW_SRC_DIR)/transaction.cc -c -o $@
endif

ifeq ($(STM), rw)
$(OBJ_DIR)/transaction.o: $(OBJ_DIR_TS) $(RW_SRC_DIR)/transaction.cc $(RW_SRC_DIR)/transaction.h
	$(CPP) $(CPPFLAGS) $(RW_SRC_DIR)/transaction.cc -c -o $@
endif

ifeq ($(STM), p-tlrw)
$(OBJ_DIR)/transaction.o: $(OBJ_DIR_TS) $(P_TLRW_SRC_DIR)/transaction.cc $(P_TLRW_SRC_DIR)/transaction.h
	$(CPP) $(CPPFLAGS) $(P_TLRW_SRC_DIR)/transaction.cc -c -o $@
endif

ifeq ($(STM), dynamic)
$(OBJ_DIR)/transaction.o: $(OBJ_DIR_TS) $(DYNAMIC_SRC_DIR)/transaction.cc $(DYNAMIC_SRC_DIR)/transaction.h
	$(CPP) $(CPPFLAGS) $(DYNAMIC_SRC_DIR)/transaction.cc -c -o $@

$(OBJ_DIR)/mixed.o: $(OBJ_DIR_TS) $(DYNAMIC_SRC_DIR)/mixed/mixed.cc $(DYNAMIC_SRC_DIR)/mixed/mixed.h
	$(CPP) $(CPPFLAGS) $(DYNAMIC_SRC_DIR)/mixed/mixed.cc -c -o $@

$(OBJ_DIR)/eager.o: $(OBJ_DIR_TS) $(DYNAMIC_SRC_DIR)/eager/eager.cc $(DYNAMIC_SRC_DIR)/eager/eager.h
	$(CPP) $(CPPFLAGS) $(DYNAMIC_SRC_DIR)/eager/eager.cc -c -o $@

$(OBJ_DIR)/lazy.o: $(OBJ_DIR_TS) $(DYNAMIC_SRC_DIR)/lazy/lazy.cc $(DYNAMIC_SRC_DIR)/lazy/lazy.h
	$(CPP) $(CPPFLAGS) $(DYNAMIC_SRC_DIR)/lazy/lazy.cc -c -o $@
endif

##################
# create mubench #
##################

$(OBJ_DIR)/intset-rbtree: $(OBJ_DIR_TS) $(LIB_DIR)/libwlpdstm.a $(MUBENCH_RBTREE_OBJFILES)
	$(CC) $(MUBENCH_CPPFLAGS) -o $@ $(MUBENCH_RBTREE_OBJFILES) $(MUBENCH_LDFLAGS)
	cp $(OBJ_DIR)/intset-rbtree .

$(OBJ_DIR)/intset-rbtree.o: $(OBJ_DIR_TS) $(MUBENCH_SRC_DIR)/intset-rbtree.c $(INCLUDE_OUT_FILE)
	$(CC) $(MUBENCH_CPPFLAGS) $(MUBENCH_SRC_DIR)/intset-rbtree.c -c -o $@

$(OBJ_DIR)/rbforest: $(OBJ_DIR_TS) $(LIB_DIR)/libwlpdstm.a $(MUBENCH_RBFOREST_OBJFILES)
	$(CPP) $(MUBENCH_CPPFLAGS) -o $@ $(MUBENCH_RBFOREST_OBJFILES) $(MUBENCH_LDFLAGS)
	cp $(OBJ_DIR)/rbforest .

$(OBJ_DIR)/rbforest.o: $(OBJ_DIR_TS) $(OBJ_DIR_TS) $(MUBENCH_SRC_DIR)/rbforest.cc
	$(CPP) $(MUBENCH_CPPFLAGS) $(MUBENCH_SRC_DIR)/rbforest.cc -c -o $@

$(OBJ_DIR)/intset-list.o: $(OBJ_DIR_TS) $(MUBENCH_SRC_DIR)/intset-rbtree.c $(INCLUDE_OUT_FILE)
	$(CC) $(MUBENCH_CPPFLAGS) $(MUBENCH_SRC_DIR)/intset-rbtree.c -c -o $@

$(OBJ_DIR)/intset-list: $(OBJ_DIR_TS) $(LIB_DIR)/libwlpdstm.a $(OBJ_DIR)/intset-list.o
	$(CC) $(MUBENCH_CPPFLAGS) -o $@ $(OBJ_DIR)/intset-list.o $(MUBENCH_LDFLAGS)

$(OBJ_DIR)/intset-list-mv.o: $(OBJ_DIR_TS) $(MUBENCH_SRC_DIR)/intset-list-mv.c $(INCLUDE_OUT_FILE)
	$(CC) $(MUBENCH_CPPFLAGS) $(MUBENCH_SRC_DIR)/intset-list-mv.c -c -o $@

$(OBJ_DIR)/intset-list-mv: $(OBJ_DIR_TS) $(LIB_DIR)/libwlpdstm.a $(OBJ_DIR)/intset-list-mv.o
	$(CC) $(MUBENCH_CPPFLAGS) -o $@ $(OBJ_DIR)/intset-list-mv.o $(MUBENCH_LDFLAGS)

$(OBJ_DIR)/intset-tree.o: $(OBJ_DIR_TS) $(MUBENCH_SRC_DIR)/intset-rbtree.c $(INCLUDE_OUT_FILE)
	$(CC) $(MUBENCH_CPPFLAGS) -DUSE_RBTREE $(MUBENCH_SRC_DIR)/intset-rbtree.c -c -o $@

$(OBJ_DIR)/intset-tree: $(OBJ_DIR_TS) $(LIB_DIR)/libwlpdstm.a $(OBJ_DIR)/intset-tree.o
	$(CC) $(MUBENCH_CPPFLAGS) -o $@ $(OBJ_DIR)/intset-tree.o $(MUBENCH_LDFLAGS)

################
# common tasks #
################

clean:
	rm -rf $(TARGET_DIR)
	rm -rf $(LIB_DIR)
	rm -rf $(INCLUDE_DIR)
	rm -rf intset-rbtree


