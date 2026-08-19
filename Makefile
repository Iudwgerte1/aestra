# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Aestra is a UCI-compliant chess engine written in C++.                  #
# Copyright (C) 2026  Iudwgerte1 <a09701070@gmail.com>                    #
#                                                                         #
# This program is free software: you can redistribute it and/or modify    #
# it under the terms of the GNU General Public License as published by    #
# the Free Software Foundation, either version 3 of the License, or       #
# (at your option) any later version.                                     #
#                                                                         #
# This program is distributed in the hope that it will be useful,         #
# but WITHOUT ANY WARRANTY; without even the implied warranty of          #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           #
# GNU General Public License for more details.                            #
#                                                                         #
# You should have received a copy of the GNU General Public License       #
# along with this program.  If not, see <https://www.gnu.org/licenses/>.  #
#                                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

CC = clang++
SRC = src/*.cpp
LIBS = 
EXE = Aestra
EXT = .exe
EVALFILE = bouquet640.bin

WFLAGS = -std=c++17 -Wall -Wextra -Wshadow -DEVALFILE=\"$(EVALFILE)\"
RFLAGS = -O3 $(WFLAGS) -DNDEBUG -static
CFLAGS = -O3 $(WFLAGS) -DNDEBUG -march=native

POPCNTFLAGS = -DUSE_POPCNT -mpopcnt
PEXTFLAGS = -DUSE_PEXT -mbmi2 $(POPCNTFLAGS)

SSSE3FLAGS  = -DUSE_SSSE3 -msse -msse2 -msse3 -mssse3
AVXFLAGS    = -DUSE_AVX -mavx -msse4.1 $(SSSE3FLAGS)
AVX2FLAGS   = -DUSE_AVX2 -mavx2 -mfma $(AVXFLAGS)

# The following makefile lines are copied from Ethereal to detect CPU features automatically
PROPS = $(shell echo | $(CC) -march=native -E -dM -)

ifneq ($(findstring __POPCNT__, $(PROPS)),)
	CFLAGS += -DUSE_POPCNT
endif

ifneq ($(findstring __BMI2__, $(PROPS)),)
	ifeq ($(findstring __znver1, $(PROPS)),)
		ifeq ($(findstring __znver2, $(PROPS)),)
			CFLAGS += -DUSE_PEXT
		endif
	endif
endif

ifneq ($(findstring __AVX2__, $(PROPS)),)
	CFLAGS += -DUSE_AVX2
endif

ifneq ($(findstring __AVX__, $(PROPS)),)
	CFLAGS += -DUSE_AVX
endif

ifneq ($(findstring __SSSE3__, $(PROPS)),)
	CFLAGS += -DUSE_SSSE3
endif

basic:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(EXE)$(EXT)

builddir:
	mkdir release

ssse3-popcnt: builddir
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(POPCNTFLAGS) $(SSSE3FLAGS) -o release/$(EXE)-ssse3$(EXT)

ssse3-pext: builddir
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(PEXTFLAGS)	$(SSSE3FLAGS) -o release/$(EXE)-pext-ssse3$(EXT)

avx-popcnt: builddir
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(POPCNTFLAGS) $(AVXFLAGS) -o release/$(EXE)-avx$(EXT)

avx-pext: builddir
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(PEXTFLAGS)	$(AVXFLAGS) -o release/$(EXE)-pext-avx$(EXT)

avx2-popcnt: builddir
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(POPCNTFLAGS) $(AVX2FLAGS) -o release/$(EXE)-avx2$(EXT)

avx2-pext: builddir
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(PEXTFLAGS)	$(AVX2FLAGS) -o release/$(EXE)-pext-avx2$(EXT)

release: ssse3-popcnt avx-popcnt avx2-popcnt ssse3-pext avx-pext avx2-pext
