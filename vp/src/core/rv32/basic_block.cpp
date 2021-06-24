#include <assert.h>
#include <iostream>

#include "coverage.h"

using namespace rv32;

std::unique_ptr<BasicBlock> BasicBlockList::add(uint64_t start, uint64_t end) {
	return std::make_unique<BasicBlock>(blocks.emplace_back(BasicBlock(start, end)));
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

size_t BasicBlockList::size(void) {
	return blocks.size();
}

size_t BasicBlockList::visited(void) {
	size_t visited = 0;

	for (auto &block : blocks)
		if (block.visited) visited++;

	return visited;
}
