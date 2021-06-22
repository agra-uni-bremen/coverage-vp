#include <assert.h>
#include <iostream>

#include "coverage.h"

using namespace rv32;

void BasicBlockList::add(uint64_t start, uint64_t end) {
	blocks.push_back(BasicBlock(start, end));
}

// TODO: Use binary search
void BasicBlockList::visit(uint64_t addr) {
	assert(!blocks.empty());
	for (auto &block : blocks) {
		if (addr >= block.start && addr < block.end) {
			block.visited = true;
			return;
		}
	}

	std::cerr << "unknown block at: 0x" << std::hex << addr << std::endl;
}

bool BasicBlockList::visitedAll(void) {
	assert(!blocks.empty());
	for (auto &block : blocks) {
		if (!block.visited) {
			return false;
		}
	}

	return true;
}
