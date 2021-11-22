/*
 * Copyright (c) 2021 Group of Computer Architecture, University of Bremen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <iostream>

#include "coverage.h"

using namespace rv32;

BasicBlockList::~BasicBlockList(void) {
	for (size_t i = 0; i < blocks.size(); i++)
		delete blocks[i];
}

BasicBlock *BasicBlockList::add(uint64_t start, uint64_t end) {
	BasicBlock *block = new BasicBlock(start, end);
	blocks.push_back(block);
	return block;
}

// TODO: Use binary search
void BasicBlockList::visit(uint64_t addr) {
	for (auto block : blocks) {
		if (addr >= block->start && addr < block->end) {
			block->visited = true;
			return;
		}
	}

	std::cerr << "unknown block at: 0x" << std::hex << addr << std::endl;
}

size_t BasicBlockList::size(void) {
	return blocks.size();
}
