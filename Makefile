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
EVALFILE = tsunami.bin

WFLAGS = -std=c++17 -Wall -Wextra -Wshadow -DEVALFILE=\"$(EVALFILE)\"
RFLAGS = -O3 $(WFLAGS) -DNDEBUG -static
CFLAGS = -O3 $(WFLAGS) -DNDEBUG -march=native

POPCNTFLAGS = -DUSE_POPCNT -mpopcnt
PEXTFLAGS = -DUSE_PEXT -mbmi2 $(POPCNTFLAGS)

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

basic:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(EXE)$(EXT)

none:
	$(CC) $(RFLAGS) $(SRC) $(LIBS) -o $(EXE)-none$(EXT)

popcnt:
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(POPCNTFLAGS) -o $(EXE)-popcnt$(EXT)

pext:
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(PEXTFLAGS) -o $(EXE)-pext$(EXT)

release: none popcnt pext
