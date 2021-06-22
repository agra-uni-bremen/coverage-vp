#include "coverage.h"
#include <iostream>

using namespace rv32;

void BasicBlockList::addBlock(uint64_t start, uint64_t end) {
	blocks.push_back(BasicBlock(start, end));
}

// TODO: Use binary search
void BasicBlockList::visit(uint64_t addr) {
	for (auto &block : blocks) {
		if (addr >= block.start && addr < block.end) {
			block.visited = true;
			return;
		}
	}

	std::cerr << "unknown block at: 0x" << std::hex << addr << std::endl;
}

bool BasicBlockList::visitedAll(void) {
	for (auto &block : blocks) {
		if (!block.visited) {
			return false;
		}
	}

	return true;
}
