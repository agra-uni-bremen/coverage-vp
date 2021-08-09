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
