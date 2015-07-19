CC       ?= gcc
CXX      ?= g++
RM       ?= rm -f
CP       ?= cp -a
MKDIR    ?= mkdir -p
WINDRES  ?= windres
INSTALL  ?= install
CFLAGS   ?= -Wall
CXXFLAGS ?= -Wall
LDFLAGS  ?= -Wall
ifneq "$(COVERAGE)" "yes"
  CFLAGS   += -O2
  CXXFLAGS += -O2
  LDFLAGS  += -O2
endif
LDFLAGS  += -Wl,-undefined,error
CAT      ?= $(if $(filter $(OS),Windows_NT),type,cat)

ifneq (,$(findstring /cygdrive/,$(PATH)))
	UNAME := Cygwin
else
	ifneq (,$(findstring WINDOWS,$(PATH)))
		UNAME := Windows
	else
		ifneq (,$(findstring mingw32,$(MAKE)))
			UNAME := MinGW
		else
			ifneq (,$(findstring MINGW32,$(shell uname -s)))
				UNAME = MinGW
			else
				UNAME := $(shell uname -s)
			endif
		endif
	endif
endif

SASS_SASSC_PATH ?= $(shell pwd)
SASS_LIBSASS_PATH ?= $(shell pwd)/..

ifeq ($(SASSC_VERSION),)
	ifneq ($(wildcard ./.git/ ),)
		SASSC_VERSION ?= $(shell git describe --abbrev=4 --dirty --always --tags)
	endif
endif

ifeq ($(SASSC_VERSION),)
	ifneq ($(wildcard VERSION),)
		SASSC_VERSION ?= $(shell $(CAT) VERSION)
	endif
endif

ifneq ($(SASSC_VERSION),)
	CFLAGS   += -DSASSC_VERSION="\"$(SASSC_VERSION)\""
	CXXFLAGS += -DSASSC_VERSION="\"$(SASSC_VERSION)\""
endif

# enable mandatory flag
ifeq (MinGW,$(UNAME))
	ifneq ($(BUILD),shared)
		STATIC_ALL     ?= 1
	endif
	STATIC_LIBGCC    ?= 1
	STATIC_LIBSTDCPP ?= 1
	CXXFLAGS += -std=gnu++0x
	LDFLAGS  += -std=gnu++0x
else
	STATIC_ALL       ?= 0
	STATIC_LIBGCC    ?= 0
	STATIC_LIBSTDCPP ?= 0
	CXXFLAGS += -std=c++0x
	LDFLAGS  += -std=c++0x
endif

ifneq ($(SASS_LIBSASS_PATH),)
	CFLAGS   += -I $(SASS_LIBSASS_PATH)/include
	CXXFLAGS += -I $(SASS_LIBSASS_PATH)/include
    # only needed to support old source tree
    # we have moved the files to src folder
	CFLAGS   += -I $(SASS_LIBSASS_PATH)
	CXXFLAGS += -I $(SASS_LIBSASS_PATH)
endif

ifneq ($(EXTRA_CFLAGS),)
	CFLAGS   += $(EXTRA_CFLAGS)
endif
ifneq ($(EXTRA_CXXFLAGS),)
	CXXFLAGS += $(EXTRA_CXXFLAGS)
endif
ifneq ($(EXTRA_LDFLAGS),)
	LDFLAGS  += $(EXTRA_LDFLAGS)
endif

LDLIBS = -lm

ifneq ($(BUILD),shared)
	LDLIBS += -lstdc++
endif

# link statically into lib
# makes it a lot more portable
# increases size by about 50KB
ifeq ($(STATIC_ALL),1)
	LDFLAGS += -static
endif
ifeq ($(STATIC_LIBGCC),1)
	LDFLAGS += -static-libgcc
endif
ifeq ($(STATIC_LIBSTDCPP),1)
	LDFLAGS += -static-libstdc++
endif

ifeq ($(UNAME),Darwin)
	CFLAGS += -stdlib=libc++
	CXXFLAGS += -stdlib=libc++
	LDFLAGS += -stdlib=libc++
endif

ifneq (MinGW,$(UNAME))
	LDFLAGS += -ldl
	LDLIBS += -ldl
endif

ifneq ($(BUILD),shared)
	BUILD = static
endif

ifeq (,$(PREFIX))
	ifeq (,$(TRAVIS_BUILD_DIR))
		PREFIX = /usr/local
	else
		PREFIX = $(TRAVIS_BUILD_DIR)
	endif
endif

SOURCES = sassc.c

# shell invocation makes problem in mingw64
LIB_STATIC = $(SASS_LIBSASS_PATH)/lib/libsass.a
LIB_SHARED = $(SASS_LIBSASS_PATH)/lib/libsass.so

ifeq (MinGW,$(UNAME))
	ifeq (shared,$(BUILD))
		CFLAGS     += -D ADD_EXPORTS
		CXXFLAGS   += -D ADD_EXPORTS
		LIB_SHARED  = $(SASS_LIBSASS_PATH)/lib/libsass.dll
	endif
else
	CFLAGS   += -fPIC
	CXXFLAGS += -fPIC
	LDFLAGS  += -fPIC
endif

OBJECTS = $(SOURCES:.c=.o)
SASSC_EXE = bin/sassc
SPEC_PATH = $(SASS_SPEC_PATH)

RESOURCES =
ifeq (MinGW,$(UNAME))
	RESOURCES = libsass.res
	SASSC_EXE = bin/sassc.exe
endif
ifeq (Windows,$(UNAME))
	RESOURCES = libsass.res
	SASSC_EXE = bin/sassc.exe
endif

all: sassc

sassc: $(SASSC_EXE)

$(SASSC_EXE): libsass build-$(BUILD)

$(DESTDIR)$(PREFIX):
	$(MKDIR) $(DESTDIR)$(PREFIX)

$(DESTDIR)$(PREFIX)/bin:
	$(MKDIR) $(DESTDIR)$(PREFIX)/bin

$(DESTDIR)$(PREFIX)/bin/%: bin/%
	$(INSTALL) -D -v -m0755 "$<" "$@"

install: $(DESTDIR)$(PREFIX)/$(SASSC_EXE)

build-static: $(RESOURCES) $(OBJECTS) $(LIB_STATIC)
	$(CC) $(LDFLAGS) -o $(SASSC_EXE) $^ $(LDLIBS)

build-shared: $(RESOURCES) $(OBJECTS) $(LIB_SHARED)
	$(MKDIR) bin/include
	$(CP) $(LIB_SHARED) bin/
    # headers are now installed by libsass makefile
	# $(CP) $(SASS_LIBSASS_PATH)/include/sass.h bin/include
	# $(CP) $(SASS_LIBSASS_PATH)/include/sass2scss.h bin/include
	# $(CP) $(SASS_LIBSASS_PATH)/include/sass_values.h bin/include
	# $(CP) $(SASS_LIBSASS_PATH)/include/sass_version.h bin/include
	# $(CP) $(SASS_LIBSASS_PATH)/include/sass_context.h bin/include
	# $(CP) $(SASS_LIBSASS_PATH)/include/sass_functions.h bin/include
	$(CC) $(LDFLAGS) -o $(SASSC_EXE) $^ $(LDLIBS)

$(LIB_STATIC): libsass-static
$(LIB_SHARED): libsass-shared

libsass: libsass-$(BUILD)

libsass-static:
ifdef SASS_LIBSASS_PATH
	BUILD="static" $(MAKE) -C $(SASS_LIBSASS_PATH)
else
	$(error SASS_LIBSASS_PATH must be defined)
endif

libsass-shared:
ifdef SASS_LIBSASS_PATH
	BUILD="shared" $(MAKE) -C $(SASS_LIBSASS_PATH)
else
	$(error SASS_LIBSASS_PATH must be defined)
endif

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: all
	$(MAKE) -C $(SASS_LIBSASS_PATH) version

libsass.res:
	$(WINDRES) res/libsass.rc -O coff libsass.res

specs: all
ifdef SASS_LIBSASS_PATH
	$(MAKE) -C $(SASS_LIBSASS_PATH) test_build
else
	$(error SASS_LIBSASS_PATH must be defined)
endif

clean:
	rm -f $(OBJECTS) $(SASSC_EXE) bin/*.so bin/*.dll bin/*.h
ifdef SASS_LIBSASS_PATH
	$(MAKE) -C $(SASS_LIBSASS_PATH) clean
endif

.PHONY: test specs clean sassc \
        all build-static build-shared \
        libsass libsass-static libsass-shared
.DELETE_ON_ERROR:
