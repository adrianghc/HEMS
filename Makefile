
ifeq ($(arch),arm)
# 	BOOST = /PATH/TO/COMPILED/BOOST/
	BOOST_INC = -I$(BOOST)
	BOOST_LIB = -L$(BOOST)stage/lib -Wl,-rpath-link=$(BOOST)stage/lib,-rpath=$$ORIGIN/lib
	CC = arm-linux-gcc
	OUTDIR = build/arm
else
	CC = gcc
	OUTDIR = build/native
	COLOR = -DCOLOR
endif

SRCDIR = src
RSRCDIR = resources

CFLAGS = -Wall -Wno-trigraphs -ffunction-sections -fdata-sections -std=gnu++14 -pthread
LFLAGS = -Wl,--gc-sections -lstdc++ -lrt -lm $(BOOST_LIB) \
		-lboost_serialization -lboost_system -lboost_thread -lboost_date_time -pthread
INCLUDES = -Iinclude/ $(BOOST_INC)
BUILDCMD = $(CC) $(COLOR) -c $(INCLUDES) $(CFLAGS) -c -o $@ $<
ifeq ($(debug),yes)
	BUILDCMD += -g
endif
BUILDCMD += -DCOLLECTION_ENERGY_PRODUCTION_HOST='"127.0.0.1"' -DCOLLECTION_ENERGY_PRODUCTION_PORT=2020
BUILDCMD += -DCOLLECTION_WEATHER_HOST='"127.0.0.1"' -DCOLLECTION_WEATHER_PORT=2022
BUILDCMD += -DINFERENCE_HOST='"127.0.0.1"' -DINFERENCE_PORT=2024
BUILDCMD += -DUI_PORT=2222
BUILDCMD_SQLITE = $(CC) -DSQLITE_DEFAULT_FOREIGN_KEYS=1 -c -Iinclude/sqlite/ -c -o $@ $<

MODULE = $(patsubst $(SRCDIR)/hems/modules/$(1)/%.cpp,$(OUTDIR)/hems/modules/$(1)/%.o,$(wildcard $(SRCDIR)/hems/modules/$(1)/*.cpp))
COMMON = $(patsubst $(SRCDIR)/hems/common/%.cpp,$(OUTDIR)/hems/common/%.o,$(wildcard $(SRCDIR)/hems/common/*.cpp))
SQLITE = $(patsubst $(SRCDIR)/sqlite/sqlite3.c,$(OUTDIR)/sqlite/sqlite3.o,$(SRCDIR)/sqlite/sqlite3.c)


$(shell mkdir -p $(OUTDIR)/hems/common)
$(shell mkdir -p $(OUTDIR)/hems/modules/launcher)
$(shell mkdir -p $(OUTDIR)/hems/modules/automation)
$(shell mkdir -p $(OUTDIR)/hems/modules/collection)
$(shell mkdir -p $(OUTDIR)/hems/modules/inference)
$(shell mkdir -p $(OUTDIR)/hems/modules/storage)
$(shell mkdir -p $(OUTDIR)/hems/modules/training)
$(shell mkdir -p $(OUTDIR)/hems/modules/ui)
$(shell mkdir -p $(OUTDIR)/sqlite)


.PHONY: clean all
clean:
	rm -r $(OUTDIR)/*
all: launcher storage collection ui inference automation training

launcher: $(COMMON) $(call MODULE,launcher)
	$(CC) -o $(OUTDIR)/hems/modules/launcher/$@ $^ $(LFLAGS) $(INCLUDES)
	cp $(OUTDIR)/hems/modules/launcher/$@ $(OUTDIR)/hems/

automation: $(COMMON) $(call MODULE,automation)
	$(CC) -o $(OUTDIR)/hems/modules/automation/$@ $^ $(LFLAGS) $(INCLUDES)
	cp $(OUTDIR)/hems/modules/automation/$@ $(OUTDIR)/hems/

collection: $(COMMON) $(call MODULE,collection)
	$(CC) -o $(OUTDIR)/hems/modules/collection/$@ $^ $(LFLAGS) $(INCLUDES)
	cp $(OUTDIR)/hems/modules/collection/$@ $(OUTDIR)/hems/

inference: $(COMMON) $(call MODULE,inference)
	$(CC) -o $(OUTDIR)/hems/modules/inference/$@ $^ $(LFLAGS) $(INCLUDES)
	cp $(OUTDIR)/hems/modules/inference/$@ $(OUTDIR)/hems/

storage: $(COMMON) $(SQLITE) $(call MODULE,storage)
	$(CC) -o $(OUTDIR)/hems/modules/storage/$@ $^ $(LFLAGS) -ldl $(INCLUDES) -Iinclude/sqlite/
	cp $(OUTDIR)/hems/modules/storage/$@ $(OUTDIR)/hems/

training: $(COMMON) $(SQLITE) $(call MODULE,training)
	$(CC) -o $(OUTDIR)/hems/modules/training/$@ $^ $(LFLAGS) $(INCLUDES)
	cp $(OUTDIR)/hems/modules/training/$@ $(OUTDIR)/hems/

ui: $(COMMON) $(call MODULE,ui)
	$(CC) -o $(OUTDIR)/hems/modules/ui/$@ $^ $(LFLAGS) -ldl $(INCLUDES)
	cp $(OUTDIR)/hems/modules/ui/$@ $(OUTDIR)/hems/
	cp -r $(RSRCDIR) $(OUTDIR)/hems

$(OUTDIR)/%.o: $(SRCDIR)/%.cpp
	$(BUILDCMD)

$(OUTDIR)/sqlite/%.o: $(SRCDIR)/sqlite/%.c
	$(BUILDCMD_SQLITE)
