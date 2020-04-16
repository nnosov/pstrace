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

CXX = gcc
CC  = gcc
RM 	= rm -f
AR	= ar rvs

BUILD_DIR	= ./build
RESULT_DIR	= ../build
LIB_STATIC	= $(RESULT_DIR)/libpst.a
LIB_SHARED	=  $(RESULT_DIR)/libpst.so

#Software LABEL, BUILD number, and DATE of build 
$(shell mkdir -p ${BUILD_DIR})
SOFT_BUILD	= $(shell STR=`cat $(BUILD_DIR)/version.h 2> /dev/null | grep 'SOFT_BUILD' | sed 's/[a-z A-Z\#\-\._]//g'`; let "STR += 1"; echo $$STR; echo "SOFT_BUILD $$STR" > $(BUILD_DIR)/version.h;)
DATE_BUILD	= $(shell date +"%D %T")

#source and header files search path. add here new directories which contains files specified for $(SRC)
VPATH 		= ./ ./dwarf ./arch ./utils

SRC			= $(wildcard $(addsuffix /*.c,${VPATH}))
OBJ			= $(patsubst %.c,%.o,$(addprefix $(BUILD_DIR)/,$(notdir $(SRC))))

INCS		= -I"./dwarf" -I"./" -I"./utils" -I"./arch" -I"../include"
FLAGS		= -Wall -ggdb -fPIC -O3 -rdynamic -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -DSOFT_BUILD=$(SOFT_BUILD) -DUSER_BUILD=$(USER) -DDATE_BUILD="$(DATE_BUILD)"

.PHONY: all clean

all: $(LIB_STATIC) $(LIB_SHARED)

clean:
	${RM} $(BUILD_DIR)/*.o $(BUILD_DIR)/*.dep $(LIB_STATIC) $(BUILD_DIR)/prepare.bld $(RESULT_DIR)/prepare.res $(BUILD_DIR)/version.h \
	    $(LIB_STATIC) $(LIB_SHARED) $(RESULT_DIR)/simple
	@if [ -z "$$(ls -A $(BUILD_DIR) 2>&1)" ]; then ${RM} -r $(BUILD_DIR); fi
	@if [ -z "$$(ls -A $(RESULT_DIR) 2>&1)" ]; then ${RM} -r $(RESULT_DIR); fi

$(BUILD_DIR)/prepare.bld:
	@if [ ! -e $(BUILD_DIR) ]; then mkdir -vp $(BUILD_DIR); fi
	@touch $@

$(RESULT_DIR)/prepare.res:
	@if [ ! -e $(RESULT_DIR) ]; then mkdir -vp $(RESULT_DIR); fi
	@touch $@

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

#create static library
$(LIB_STATIC): $(RESULT_DIR)/prepare.res $(BUILD_DIR)/prepare.bld $(OBJ)
	@rm -fv $@
	@printf "Create   %-60s" $@
	@OUT=$$($(AR) $@ $(OBJ) 2>&1); \
	if [ $$? -ne "0" ]; \
	  then echo -e "${RED}[FAILED]${NC}";  echo -e "$$OUT"; \
	else \
	  echo -e "${GREEN}[DONE]${NC}"; \
	fi

#create shared library
$(LIB_SHARED): $(RESULT_DIR)/prepare.res $(BUILD_DIR)/prepare.bld $(OBJ)
	@rm -fv $@
	@printf "Create   %-60s" $@
	@OUT=$$($(CC) -shared $(OBJ) -o $@ 2>&1); \
	if [ $$? -ne "0" ]; \
	  then echo -e "${RED}[FAILED]${NC}";  echo -e "$$OUT"; \
	else \
	  echo -e "${GREEN}[DONE]${NC}"; \
	fi

#include dependencies for track changes in sourcecode and related header files
DEPEND := $(OBJ:.o=.o.dep)
-include $(DEPEND)
