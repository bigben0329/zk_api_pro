C :=gcc                                                                                                                             
CXX := g++
AR := ar
C_FLAGS := -DTHREADED
C_FLAGS := -g -ansi -pipe -lpthread
SOURCES = $(wildcard *.c)
RELEASE = $(patsubst %.c, %, $(SOURCES))

INC = -I. -I./include \
	  -I./include/zookeeper \
	  -I./include/generated \
	  -I./api

LIB = \
	libs/lib32/libzkapi.a \
	libs/lib32/libzookeeper_mt.a \

MACHINE_PLATFORM = $(shell $(CC) -dumpmachine)
ifneq ($(findstring x86_64,$(MACHINE_PLATFORM)),)
C_FLAGS += -fPIC
LIB = \
	  libs/lib64/libzkapi.a \
	  libs/lib64/libzookeeper_mt.a \

endif
#LIB = lib/libzookeeper_mt.a

RELEASE_TARGETS:${RELEASE}

%: %.c
	$(CXX) $(C_FLAGS) -o $@ $< $(INC) $(LIB)

%.o: %.c
	$(CXX) -c -o $@ $< $(CXX_DEVNET_FLAGS) $(INC)

%.d: %.c
	$(CXX) -MM -MF $@ -MT $@ -MT $* $< $(INC)

%_release.o: %.cpp
	    $(CXX) -c -o $@ $< $(CXX_RELEASE_FLAGS) $(INC)

all:RELEASE_TARGETS

clean:
	@echo ${SOURCES}
	@echo ${RELEASE}
	-rm -f *.d *.o *_unittest.xml $(patsubst %.c, %, $(SOURCES)) *.gcno *.gcda
