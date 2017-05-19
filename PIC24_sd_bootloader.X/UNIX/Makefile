#
#  Copyright (c) 2017, Naoya Miyamoto. All rights reserved.
#

include sourcelist.make
include settings.make

# file lists
OBJ_LIST := $(patsubst %.c,%.o,$(SOURCE_LIST))
SOURCES := $(addprefix $(SOURCE_DIR_PATH)/,$(SOURCE_LIST))
HEADERS := $(addprefix $(HEADER_DIR_PATH)/,$(HEADER_LIST))
OBJS := $(addprefix $(BUILD_WORK_DIR_PATH)/,$(OBJ_LIST))


.PHONY: all
all: $(EXECUTABLE_FILE_PATH)
	@echo 'done.'

folders:
	@if [ ! -d $(OUTPUT_DIR_PATH) ]; \
 		then mkdir -p $(OUTPUT_DIR_PATH); \
 		fi
	@if [ ! -d $(BUILD_WORK_DIR_PATH) ]; \
 		then mkdir -p $(BUILD_WORK_DIR_PATH); \
 		fi

$(BUILD_WORK_DIR_PATH)/%.o: $(SOURCE_DIR_PATH)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(EXECUTABLE_FILE_PATH): folders $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $(EXECUTABLE_FILE_PATH)

.PHONY: clean
clean:
	rm -rf $(OBJS) $(EXECUTABLE_FILE_PATH) 
	@echo 'clean done.'

