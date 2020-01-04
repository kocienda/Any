#
# Makefile for directory any benchmark tests
#

INCLUDE_CFLAGS := -I. -I.. -I/usr/local/include
ARCH_FLAGS := -arch x86_64
WARN_CFLAGS := -Werror -Wno-trigraphs -Wno-missing-field-initializers -Werror=return-type \
-Wunreachable-code -Wno-non-virtual-dtor -Wno-overloaded-virtual -Wno-missing-braces -Wparentheses -Wswitch \
-Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wuninitialized \
-Wno-uninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-conversion \
-Wdeprecated-declarations -Winvalid-offsetof -Wno-sign-conversion \
-fvisibility-inlines-hidden -fstrict-aliasing -fno-common

# To compile with Clang/LLVM
# CC := cc
# CPPFLAGS := -O3 -std=c++17 $(INCLUDE_CFLAGS) $(WARN_CFLAGS)
# LFLAGS := -L/usr/local/lib -lc++ -lbenchmark

# To compile with GCC
CC := g++-9
CPPFLAGS := -O3 -std=gnu++17 $(INCLUDE_CFLAGS) $(WARN_CFLAGS)
LFLAGS := -L/usr/local/lib -lstdc++ -lbenchmark

SRCS := $(wildcard *.cpp)
BINS := $(SRCS:%.cpp=bin/%)
DEPS := ../xgcc-any.h ../xllvm-any.h ../cyto-any.h  ../any-types.h

.PHONY: all
all: bin $(BINS)

bin:
	@mkdir $@

bin/% : %.cpp $(DEPS)
	@echo $(CC) $<
	@$(CC) $(CPPFLAGS) $(LFLAGS) -o $@ $<
   
.INTERMEDIATE: $(notdir $(BINS))
.DELETE_ON_ERROR:

.PHONY: clean
clean:
	rm -rf bin
