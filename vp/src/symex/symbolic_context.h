/*
 * Copyright (c) 2020,2021 Group of Computer Architecture, University of Bremen
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

#ifndef RISCV_ISA_SYMBOLIC_CTX_H
#define RISCV_ISA_SYMBOLIC_CTX_H

#include <stdint.h>
#include <stdbool.h>

#include <clover/clover.h>

class SymbolicContext {
public:
	clover::Solver solver;
	clover::Trace trace;
	clover::ExecutionContext ctx;
	void *user_data;

	SymbolicContext(void);
};

extern SymbolicContext symbolic_context;

#endif
