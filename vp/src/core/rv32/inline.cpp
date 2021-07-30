/* Return source line information for all code parts where code at address in lined or.
   Taken from src/addrline.c in the elfutils-0.185 source code.

   Copyright (C) 2005-2010, 2012, 2013, 2015 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2005.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdexcept>
#include <dwarf.h>

#include "inline.h"

static const char *
get_diename (Dwarf_Die *die)
{
	Dwarf_Attribute attr;
	const char *name;

	name = dwarf_formstring (dwarf_attr_integrate (die, DW_AT_linkage_name, &attr));
	if (name == NULL)
		name = dwarf_diename (die);
	if (name == NULL)
		throw std::runtime_error("get_diename failed");

	return name;
}

static void
get_inlines(std::vector<SourceInfo> &funcs, Dwfl_Module *mod, Dwarf_Addr addr)
{
	Dwarf_Addr bias = 0;
	Dwarf_Die *cudie = dwfl_module_addrdie (mod, addr, &bias);

	Dwarf_Die *scopes = NULL;
	int nscopes = dwarf_getscopes (cudie, addr - bias, &scopes);
	if (nscopes < 0)
		return;

	if (nscopes > 0)
	{
		Dwarf_Die subroutine;
		Dwarf_Off dieoff = dwarf_dieoffset (&scopes[0]);
		dwarf_offdie (dwfl_module_getdwarf (mod, &bias),
				dieoff, &subroutine);
		free (scopes);
		scopes = NULL;

		nscopes = dwarf_getscopes_die (&subroutine, &scopes);
		if (nscopes <= 0)
			return;

		Dwarf_Die cu;
		Dwarf_Files *files;
		if (dwarf_diecu (&scopes[0], &cu, NULL, NULL) != NULL
				&& dwarf_getsrcfiles (cudie, &files, NULL) == 0)
		{
			for (int i = 0; i < nscopes - 1; i++)
			{
				Dwarf_Word val;
				Dwarf_Attribute attr;
				const char *name = NULL;
				Dwarf_Die *die = &scopes[i];
				if (dwarf_tag (die) != DW_TAG_inlined_subroutine)
					continue;

				/* Search for the parent inline or function.  It
				   might not be directly above this inline -- e.g.
				   there could be a lexical_block in between.  */
				for (int j = i + 1; j < nscopes; j++)
				{
					Dwarf_Die *parent = &scopes[j];
					int tag = dwarf_tag (parent);
					if (tag == DW_TAG_inlined_subroutine
							|| tag == DW_TAG_entry_point
							|| tag == DW_TAG_subprogram)
					{
						name = get_diename(parent);
						break;
					}
				}
				if (!name)
					throw std::runtime_error("get_inlines: no symbol name found");

				const char *src = NULL;
				int lineno = 0;
				int linecol = 0;
				if (dwarf_formudata (dwarf_attr (die, DW_AT_call_file, &attr), &val) == 0)
					src = dwarf_filesrc (files, val, NULL, NULL);
				if (dwarf_formudata (dwarf_attr (die, DW_AT_call_line, &attr), &val) == 0)
					lineno = val;
				if (dwarf_formudata (dwarf_attr (die, DW_AT_call_column, &attr), &val) == 0)
					linecol = val;

				if (!src)
					throw std::runtime_error("get_inlines: could not determine source file");

				funcs.push_back(SourceInfo(
					std::string(name),
					std::string(src),
					lineno, linecol));
			}
		}
	}

	free(scopes);
}

std::vector<SourceInfo>
get_sources(Dwfl_Module *mod, Dwarf_Addr addr)
{
	std::vector<SourceInfo> funcs;

	Dwfl_Line *line = dwfl_module_getsrc(mod, addr);
	if (!line)
		return funcs; /* no line number information */

	get_inlines(funcs, mod, addr);
	if (funcs.empty()) { // not inlined
		int lnum, cnum;
		const char *srcfp;
		if (!(srcfp = dwfl_lineinfo(line, NULL, &lnum, &cnum, NULL, NULL)))
			throw std::runtime_error("dwfl_lineinfo failed");

		const char *name;
		if (!(name = dwfl_module_addrname(mod, addr)))
			throw std::runtime_error("dwfl_module_addrname failed");

		funcs.push_back(SourceInfo(
			std::string(name),
			std::string(srcfp),
			lnum, cnum));
	}

	return funcs;
}
