
CC = gcc
OUTDIR = build/native

CFLAGS = -Wall -Wno-trigraphs -ffunction-sections -fdata-sections -std=gnu++14 -pthread
LFLAGS = -Wl,--gc-sections -lstdc++ -lrt -lm -pthread \
		 -lboost_serialization -lboost_system -lboost_filesystem -lboost_thread -lboost_date_time

INCLUDES = -I../include/
SRCDIR = ../src

BUILDCMD = $(CC) -c $(INCLUDES) $(CFLAGS) -c -o $@ $< -g
BUILDCMD += -DCOLLECTION_ENERGY_PRODUCTION_HOST='"127.0.0.1"' -DCOLLECTION_ENERGY_PRODUCTION_PORT=5000
BUILDCMD += -DCOLLECTION_WEATHER_HOST='"127.0.0.1"' -DCOLLECTION_WEATHER_PORT=5010
BUILDCMD += -DINFERENCE_HOST='"127.0.0.1"' -DINFERENCE_PORT=5020
BUILDCMD += -DUI_PORT=5555
BUILDCMD_SQLITE = $(CC) -DSQLITE_DEFAULT_FOREIGN_KEYS=1 -c -I../include/sqlite/ -c -o $@ $<

MODULE = $(patsubst $(SRCDIR)/hems/modules/$(1)/%.cpp,$(OUTDIR)/hems/modules/$(1)/%.o, \
		 	$(filter-out $(SRCDIR)/hems/modules/$(1)/main.cpp,$(wildcard $(SRCDIR)/hems/modules/$(1)/*.cpp)) \
		 )
COMMON = $(patsubst $(SRCDIR)/hems/common/%.cpp,$(OUTDIR)/hems/common/%.o,$(wildcard $(SRCDIR)/hems/common/*.cpp))
TESTS  = $(patsubst $(1)/$(2).cpp,$(OUTDIR)/tests/$(1)/$(2).o,$(wildcard $(1)/$(2).cpp))
EXTRAS = $(patsubst extras/$(1).cpp,$(OUTDIR)/tests/extras/$(1).o,$(wildcard extras/$(1).cpp))
SQLITE = $(patsubst $(SRCDIR)/sqlite/sqlite3.c,$(OUTDIR)/sqlite/sqlite3.o,$(SRCDIR)/sqlite/sqlite3.c)


$(shell mkdir -p $(OUTDIR)/hems/common)
$(shell mkdir -p $(OUTDIR)/hems/modules/launcher)
$(shell mkdir -p $(OUTDIR)/hems/modules/automation)
$(shell mkdir -p $(OUTDIR)/hems/modules/collection)
$(shell mkdir -p $(OUTDIR)/hems/modules/inference)
$(shell mkdir -p $(OUTDIR)/hems/modules/storage)
$(shell mkdir -p $(OUTDIR)/hems/modules/training)
$(shell mkdir -p $(OUTDIR)/hems/modules/ui)

$(shell mkdir -p $(OUTDIR)/tests/common)
$(shell mkdir -p $(OUTDIR)/tests/extras)
$(shell mkdir -p $(OUTDIR)/tests/launcher)
$(shell mkdir -p $(OUTDIR)/tests/automation)
$(shell mkdir -p $(OUTDIR)/tests/collection)
$(shell mkdir -p $(OUTDIR)/tests/inference)
$(shell mkdir -p $(OUTDIR)/tests/storage)
$(shell mkdir -p $(OUTDIR)/tests/training)
$(shell mkdir -p $(OUTDIR)/tests/ui)

$(shell mkdir -p $(OUTDIR)/sqlite)


.PHONY: clean all
clean:
	rm -r $(OUTDIR)/*
all: common launcher storage collection


# Build targets for extras

dummy_module_0: $(call EXTRAS,dummy_module_0)
	$(CC) -o $(OUTDIR)/tests/extras/$@ $^ $(LFLAGS) $(INCLUDES)
	cp $(OUTDIR)/tests/extras/$@ $(OUTDIR)/tests/

dummy_module_1: $(call EXTRAS,dummy_module_1)
	$(CC) -o $(OUTDIR)/tests/extras/$@ $^ $(LFLAGS) $(INCLUDES)
	cp $(OUTDIR)/tests/extras/$@ $(OUTDIR)/tests/

dummy_module_2: $(call EXTRAS,dummy_module_2)
	$(CC) -o $(OUTDIR)/tests/extras/$@ $^ $(LFLAGS) $(INCLUDES)
	cp $(OUTDIR)/tests/extras/$@ $(OUTDIR)/tests/

# Tests for common classes and functions

common: test_messenger test_settings test_types

test_messenger: $(COMMON) $(call TESTS,common,test_messenger)
	$(CC) -o $(OUTDIR)/tests/common/$@ $^ $(LFLAGS) $(INCLUDES)

test_settings: $(COMMON) $(call TESTS,common,test_settings)
	$(CC) -o $(OUTDIR)/tests/common/$@ $^ $(LFLAGS) $(INCLUDES)

test_types: $(COMMON) $(call TESTS,common,test_types)
	$(CC) -o $(OUTDIR)/tests/common/$@ $^ $(LFLAGS) $(INCLUDES)

# Tests for the Launcher Module

launcher: test_launcher test_local_logger

test_launcher: $(COMMON) $(call MODULE,launcher) $(call TESTS,launcher,test_launcher) \
			   dummy_module_0 dummy_module_1 dummy_module_2
	$(CC) -o $(OUTDIR)/tests/launcher/$@ $(filter-out dummy_module_0 dummy_module_1 dummy_module_2,$^) \
		$(LFLAGS) $(INCLUDES)
	cp -r $(OUTDIR)/tests/extras/dummy_module_* $(OUTDIR)/tests/launcher/

test_local_logger: $(COMMON) $(call MODULE,launcher) $(call TESTS,launcher,test_local_logger)
	$(CC) -o $(OUTDIR)/tests/launcher/$@ $^ $(LFLAGS) $(INCLUDES)

# Tests for the Automation and Recommendation Module

# Tests for the Measurement Collection Module

collection: test_download_data

test_download_data: $(COMMON) $(call MODULE,collection) $(call TESTS,collection,test_download_data)
	$(CC) -o $(OUTDIR)/tests/collection/$@ $^ $(LFLAGS) $(INCLUDES)
	cp -r extras/energy_production_provider $(OUTDIR)/tests/collection/
	cp -r extras/weather_provider $(OUTDIR)/tests/collection/

# Tests for the Knowledge Inference Module.

# Tests for the Data Storage Module.

storage: test_storage test_handler_settings test_handler_msg_set test_handler_msg_del test_handler_msg_get

test_storage: $(COMMON) $(SQLITE) $(call MODULE,storage) $(call TESTS,storage,test_storage)
	$(CC) -o $(OUTDIR)/tests/storage/$@ $^ $(LFLAGS) -ldl $(INCLUDES) -Iinclude/sqlite/

test_handler_settings: $(COMMON) $(SQLITE) $(call MODULE,storage) \
						$(call TESTS,storage,test_handler_settings)
	$(CC) -o $(OUTDIR)/tests/storage/$@ $^ $(LFLAGS) -ldl $(INCLUDES) -Iinclude/sqlite/

test_handler_msg_set: $(COMMON) $(SQLITE) $(call MODULE,storage) \
						$(call TESTS,storage,test_handler_msg_set)
	$(CC) -o $(OUTDIR)/tests/storage/$@ $^ $(LFLAGS) -ldl $(INCLUDES) -Iinclude/sqlite/

test_handler_msg_del: $(COMMON) $(SQLITE) $(call MODULE,storage) \
						$(call TESTS,storage,test_handler_msg_del)
	$(CC) -o $(OUTDIR)/tests/storage/$@ $^ $(LFLAGS) -ldl $(INCLUDES) -Iinclude/sqlite/

test_handler_msg_get: $(COMMON) $(SQLITE) $(call MODULE,storage) \
						$(call TESTS,storage,test_handler_msg_get)
	$(CC) -o $(OUTDIR)/tests/storage/$@ $^ $(LFLAGS) -ldl $(INCLUDES) -Iinclude/sqlite/

# Tests for the Model Training Module.

# Tests for the User Interface Module.


$(OUTDIR)/%.o: $(SRCDIR)/%.cpp
	$(BUILDCMD)

$(OUTDIR)/tests/%.o: %.cpp
	$(BUILDCMD)

$(OUTDIR)/extras/%.o: %.cpp
	$(CC) -o $(OUTDIR)/extras/$@ $^ $(LFLAGS) $(INCLUDES)

$(OUTDIR)/sqlite/%.o: $(SRCDIR)/sqlite/%.c
	$(BUILDCMD_SQLITE)
