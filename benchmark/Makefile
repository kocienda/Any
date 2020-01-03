#
# Makefile for directory any benchmark tests
#

INCLUDE_CFLAGS := -I. -I..
ARCH_FLAGS := -arch x86_64
WARN_CFLAGS := -Werror -Wno-trigraphs -Wno-missing-field-initializers -Wno-missing-prototypes -Werror=return-type -Wdocumentation \
-Wunreachable-code -Wno-non-virtual-dtor -Wno-overloaded-virtual -Wno-exit-time-destructors -Wno-missing-braces -Wparentheses -Wswitch \
-Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wuninitialized \
-Wconditional-uninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-four-char-constants -Wno-conversion -Wconstant-conversion \
-Wint-conversion -Wbool-conversion -Wenum-conversion -Wno-float-conversion -Wnon-literal-null-conversion -Wobjc-literal-conversion \
-Wshorten-64-to-32 -Wno-newline-eof -Wno-c++11-extensions -Wdeprecated-declarations -Winvalid-offsetof -Wno-sign-conversion \
-Winfinite-recursion -Wmove -Wstrict-prototypes -Wrange-loop-analysis -Wunguarded-availability -Wno-nullability-completeness \
-fvisibility-inlines-hidden -fasm-blocks -fstrict-aliasing -fno-common

CC := cc
CPPFLAGS := -g -O3 -std=c++17 $(INCLUDE_CFLAGS) $(WARN_CFLAGS)
LFLAGS := -lc++ -lbenchmark

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
