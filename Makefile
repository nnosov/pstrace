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

CXX 	= g++
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
VPATH = ./

SRC 		= $(shell find . -name '*.cpp')
#OBJ 		= $(patsubst %.cpp,%.o,$(addprefix $(BUILD_DIR)/,$(notdir $(SRC))))
OBJ 		= $(BUILD_DIR)/main.o

LIBS 		= -L./ -lpthread -luuid -ldl -ldw -lunwind -lunwind-x86_64
ATR_LIBS	= ./build/libframework.a

INCS		= -I./ \
-I/usr/include/postgresql -I /usr/include/libxml2 -I./thirdparty -I/opt/swifttest/include  -I/usr/include/GraphicsMagick \
-I./framework -I./framework/swi -I./framework/utils -I./framework/rmq -I./framework/logger -I$(BUILD_DIR)
#FLAGS		= -Wall -gdwarf-4 -fPIC -O3 -rdynamic -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -std=c++11
FLAGS		= -Wall -ggdb -fPIC -O3 -rdynamic -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -std=c++11


.PHONY: all clean $(BIN)

all: $(BIN)

$(ATR_LIBS):
	@make -C ./framework

clean:
	${RM} $(BUILD_DIR)/*.o $(BUILD_DIR)/*.dep $(BIN) $(BUILD_DIR)/prepare.bld $(RESULT_DIR)/prepare.res $(BUILD_DIR)/version.h
	@if [ -z "$$(ls -A $(BUILD_DIR) 2>&1)" ]; then ${RM} -r $(BUILD_DIR); fi
	@if [ -z "$$(ls -A $(RESULT_DIR) 2>&1)" ]; then ${RM} -r $(RESULT_DIR); fi

$(BUILD_DIR)/prepare.bld:
	@if [ ! -e $(BUILD_DIR) ]; then mkdir -vp $(BUILD_DIR); fi
	@touch $@
	
$(RESULT_DIR)/prepare.res:
	@if [ ! -e $(RESULT_DIR) ]; then mkdir -vp $(RESULT_DIR); fi
	@touch $@

$(BIN): $(BUILD_DIR)/prepare.bld $(RESULT_DIR)/prepare.res $(OBJ) $(ATR_LIBS)
	gcc -ggdb -O3 -o $(BUILD_DIR)/simple simple.cpp
	@make -C ./framework
	@printf "Create   %-60s" $@
	@OUT=$$($(CXX) $(COLOR) -o $@ $(OBJ) $(ATR_LIBS) $(LIBS) 2>&1); \
	if [ $$? -ne "0" ]; \
	  then echo -e "${RED}[FAILED]${NC}"; echo -e "$$OUT"; \
	else \
	  if [ -n "$$OUT" ]; \
	  	then echo -e "${YELLOW}[DONE]${NC}"; echo -e "'$$OUT'"; \
	  else \
	    echo -e "${GREEN}[DONE]${NC}"; \
	  fi; \
	fi

$(BUILD_DIR)/%.o: %.cpp
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
