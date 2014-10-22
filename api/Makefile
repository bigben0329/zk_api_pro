C := gcc
CXX := g++
AR := ar
C_FLAGS := -DTHREADED
C_FLAGS := -g -ansi -pipe -lpthread
SOURCES = $(wildcard *.cpp)
RELEASE_OBJS = $(patsubst %.cpp, %_release.o, $(SOURCES))

INC = -I. -I.. -I../include \
	  -I../include/zookeeper \
	  -I../include/generated

RELEASE_TARGETS := ../libs/lib32/libzkapi.a
MACHINE_PLATFORM = $(shell $(CC) -dumpmachine)
ifneq ($(findstring x86_64,$(MACHINE_PLATFORM)),)
C_FLAGS += -fPIC
RELEASE_TARGETS := ../libs/lib64/libzkapi.a
endif

%.o: %.c
	$(CXX) -c -o $@ $< $(CXX_DEVNET_FLAGS) $(INC)

%.d: %.c
	$(CXX) -MM -MF $@ -MT $@ -MT $* $< $(INC)

%_release.o: %.cpp
	    $(CXX) -c -o $@ $< $(CXX_RELEASE_FLAGS) $(INC)

all: $(RELEASE_TARGETS)

clean:
	@echo ${SOURCES}
	@echo ${RELEASE_OBJS}
	@echo ${RELEASE_TARGETS}
	rm *.o ${RELEASE_TARGETS}

$(RELEASE_TARGETS): $(RELEASE_OBJS)
	$(AR) rc $@ $?
