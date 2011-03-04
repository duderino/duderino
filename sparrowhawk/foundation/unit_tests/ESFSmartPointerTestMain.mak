BASE=../../..

INCLUDE = \
-I$(BASE)/sparrowhawk/foundation\
-I$(BASE)/sparrowhawk/test_framework\
-I$(BASE)/sparrowhawk/foundation/unit_tests

SOURCES=\
ESFSmartPointerTest.cpp \
ESFSmartPointerTestMain.cpp

STATIC_LIBS =\
$(BASE)/sparrowhawk/foundation/libfoundation \
$(BASE)/sparrowhawk/test_framework/libtest_framework

DEBUG_ONLY=1
EXE=smart_pointer_test

all: $(EXE)

run:
	@echo "32 bit test"
	./$(EXE)_debug_32
	@echo "64 bit test"
	./$(EXE)_debug_64

include $(BASE)/GNUmakevars
