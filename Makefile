#use Bash instead of SH
export SHELL := /bin/bash

# echo command color definitions
ifndef NO_COLOR
RED=\e[0;31m
GREEN=\e[0;32m
YELLOW=\e[1;33m
NC=\e[0m # No Color
COLOR=-fdiagnostics-color
else
RED=
GREEN=
YELLOW=
NC=
COLOR=
endif

CXX 	= gcc
CC  	= gcc
RM 		= rm -f
AR		= ar rvs

BUILD_DIR		= ./build
RESULT_DIR		= ./build

BIN  			= $(RESULT_DIR)/trace

# Generate module software version and build version
#$(shell \
#	STR=`cat $(BUILD_DIR)/version.h 2> /dev/null | grep "SOFT_BUILD" | sed 's/[a-z A-Z#\-\._]//g'`; \
#	let "STR += 1";\
#	echo "#ifndef VERSION_H_" > $(BUILD_DIR)/version.h; \
#	echo "#define VERSION_H_" >> $(BUILD_DIR)/version.h; \
#	echo "/* Automatically generated file, DO NOT EDIT it !!! */" >> $(BUILD_DIR)/version.h; \
#	echo "" >> $(BUILD_DIR)/version.h; \
#	echo "#define SOFT_BUILD $$STR" >> $(BUILD_DIR)/version.h; \
#	echo "#define USER_BUILD \"$${USER}\"" >> $(BUILD_DIR)/version.h; \
#	echo "#define DATE_BUILD \"`date +"%D %T"`\"" >> $(BUILD_DIR)/version.h; \
#	echo "#define REV_BUILD  \"`git rev-parse --short HEAD`\"" >> $(BUILD_DIR)/version.h; \
#	echo "" >> $(BUILD_DIR)/version.h; \
#	echo "#endif /* VERSION_H_ */" >> $(BUILD_DIR)/version.h \
#	)

#source file search path. add here new directories which contains files specified for $(SRC)
VPATH = ./tests

SRC 		= $(shell find . -name '*.c')
#OBJ 		= $(patsubst %.cpp,%.o,$(addprefix $(BUILD_DIR)/,$(notdir $(SRC))))
OBJ 		= $(BUILD_DIR)/main.o

LIBS 		= -L./ -lpthread -ldl -ldw -lunwind -lunwind-x86_64 -lstdc++ -liberty
LIB_STATIC	= $(RESULT_DIR)/libpst.a
LIB_DYNAMIC	= $(RESULT_DIR)/libpst.so

INCS		= -I./include -I./src
FLAGS		= -Wall -ggdb -fPIC -O3 -rdynamic -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS


.PHONY: all clean $(BIN)

all: $(BIN)

$(LIB_STATIC):
	@make -C ./src

clean:
	${RM} $(BUILD_DIR)/*.o $(BUILD_DIR)/*.dep $(BIN) $(BUILD_DIR)/prepare.bld $(RESULT_DIR)/prepare.res $(BUILD_DIR)/version.h \
	    $(LIB_STATIC) $(LIB_DYNAMIC)
	@make clean -C ./src
	@if [ -z "$$(ls -A $(BUILD_DIR) 2>&1)" ]; then ${RM} -r $(BUILD_DIR); fi
	@if [ -z "$$(ls -A $(RESULT_DIR) 2>&1)" ]; then ${RM} -r $(RESULT_DIR); fi

$(BUILD_DIR)/prepare.bld:
	@if [ ! -e $(BUILD_DIR) ]; then mkdir -vp $(BUILD_DIR); fi
	@touch $@
	
$(RESULT_DIR)/prepare.res:
	@if [ ! -e $(RESULT_DIR) ]; then mkdir -vp $(RESULT_DIR); fi
	@touch $@

$(BIN): $(BUILD_DIR)/prepare.bld $(RESULT_DIR)/prepare.res $(OBJ) $(LIB_STATIC)
	gcc -ggdb -O3 -o $(BUILD_DIR)/simple ./tests/simple.c
	@make -C ./src
	@printf "Create   %-60s" $@
	@OUT=$$($(CXX) $(COLOR) -o $@ $(OBJ) $(LIB_STATIC) $(LIBS) 2>&1); \
	if [ $$? -ne "0" ]; \
	  then echo -e "${RED}[FAILED]${NC}"; echo -e "$$OUT"; \
	else \
	  if [ -n "$$OUT" ]; \
	  	then echo -e "${YELLOW}[DONE]${NC}"; echo -e "'$$OUT'"; \
	  else \
	    echo -e "${GREEN}[DONE]${NC}"; \
	  fi; \
	fi

$(BUILD_DIR)/%.o: %.c
#compile source code directly to $BUILD_DIR directory
	@printf "Building %-60s" $@
	@OUT=$$($(CXX) $(COLOR) -o $@ -c $< $(FLAGS) $(INCS) 2>&1); \
	if [ $$? -ne "0" ]; \
	  then echo -e "${RED}[FAILED]${NC}"; echo -e "$$OUT"; \
	else \
	  if [ -n "$$OUT" ]; \
	  	then echo -e "${YELLOW}[DONE]${NC}"; echo -e "'$$OUT'"; \
	  else \
	    echo -e "${GREEN}[DONE]${NC}"; \
	  fi; \
	fi
#create dependencies
	@$(CXX) -MM -MT '$@' -c $< > $@.dep $(FLAGS) $(INCS)

#include dependencies for track changes in source code and related header files
DEPEND := $(OBJ:.o=.o.dep)
-include $(DEPEND)
