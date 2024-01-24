/* ELM                                                                        */
/* Copyright (C) 2024  Miles McGruder                                         */
/*                                                                            */
/* This program is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by       */
/* the Free Software Foundation, either version 3 of the License, or          */
/* (at your option) any later version.                                        */
/*                                                                            */
/* This program is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/* GNU General Public License for more details.                               */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program.  If not, see <https://www.gnu.org/licenses/>.     */

#pragma once

#include <cstdio>
#include <cstring>
#include <cinttypes>
#include <string>
#include <unordered_map>

#include "colormaps.cpp"
#include "basic_types.cpp"

enum optype
{
  ot_float,
  ot_double,
  ot_u32,
  ot_u64,
  ot_s32,
  ot_s64,
  ot_cstring,
  ot_stdstring,
  ot_bool
};

// Argument parser takes an array of this struct, one entry for each argument.
// To be stored in a single list all options must be the same type, so the type
// an the data to be modified are divorced in this struct. The type information
// is used to case the void* correctly in the parser.
struct option
{
  const char* name;
  const char* description;
  optype      type;
  void*       data;
};

// Parses options where option names are marked with a single dash, all other
// types of option are ignored. Automatically assigns parsed values to the
// appropriate data using the separate type information in the "option" struct.
int optparse(int argc, char** argv, usize optc, const option* opts, bool& help)
{
  int head = 1;  // "head" always points to the next entry in argv to be read

  /* print help and exit if requested */

  if (argc > 1 && strcmp(argv[head], "-h") == 0)
  {
    printf("Options:\n");
    for (usize oi = 0; oi < optc; ++oi)
    {
      printf("  %10s: %s\n", opts[oi].name, opts[oi].description);
    }

    help = true;
    return 0;
  }

  /* argument parsing */

  while (head < argc)
  {
    if (argv[head][0] == '-')  // determine if option is argument
    {
      usize oi;
      for (oi = 0; oi < optc; ++oi)  // linear search for argument
      {
        if (strcmp(opts[oi].name, argv[head] + 1) == 0)  // parse if found
        {
          optype type = opts[oi].type;

          if ((head + 1 >= argc) && type != ot_bool)
          {
            printf("no argument provided for \"%s\" at end of argument list\n",
                   argv[head] + 1);
            ++head;
            break;
          }

          switch (type)
          {
            case ot_float:
            {
              *((float*)opts[oi].data) = strtof(argv[head + 1], nullptr);
            }
            break;
            case ot_double:
            {
              *((double*)opts[oi].data) = strtod(argv[head + 1], nullptr);
            }
            break;
            case ot_u32:
            {
              *((u32*)opts[oi].data) =
              (u32)strtoumax(argv[head + 1], nullptr, 10);
            }
            break;
            case ot_u64:
            {
              *((u64*)opts[oi].data) =
              (u64)strtoumax(argv[head + 1], nullptr, 10);
            }
            break;
            case ot_s32:
            {
              *((u32*)opts[oi].data) =
              (s32)strtoimax(argv[head + 1], nullptr, 10);
            }
            break;
            case ot_s64:
            {
              *((u64*)opts[oi].data) =
              (s64)strtoimax(argv[head + 1], nullptr, 10);
            }
            break;
            case ot_cstring:
            {
              strcpy((char*)opts[oi].data, argv[head + 1]);
            }
            break;
            case ot_stdstring:
            {
              *((std::string*)opts[oi].data) = std::string(argv[head + 1]);
            }
            break;
            case ot_bool:
            {
              *((bool*)opts[oi].data) = true;
              --head;
            }
            break;
          }
          head += 2;
          break;
        }
      }

      if (oi == optc)  // if search failed, print message and move on
      {
        printf("failed to find option \"%s\"\n", argv[head] + 1);
        ++head;
      }
    }
    else  // simpy move on if not
    {
      printf("ignoring non-option argument %s\n", argv[head]);
      ++head;
    }
  }
  return 0;
}

// Provides functions to create various option types.
// The type of data in the option is deduced automatically by function
// overloading and the appropriate option type information is assigned to each
// option.

option mkopt_prefix(const char* name, const char* description, void* data)
{
  option opt{};
  opt.name        = name;
  opt.description = description;
  opt.data        = data;
  return opt;
}

option mkopt(const char* name, const char* description, u32* data)
{
  option opt = mkopt_prefix(name, description, data);
  opt.type   = ot_u32;
  return opt;
}

option mkopt(const char* name, const char* description, u64* data)
{
  option opt = mkopt_prefix(name, description, data);
  opt.type   = ot_u64;
  return opt;
}

option mkopt(const char* name, const char* description, s32* data)
{
  option opt = mkopt_prefix(name, description, data);
  opt.type   = ot_s32;
  return opt;
}

option mkopt(const char* name, const char* description, s64* data)
{
  option opt = mkopt_prefix(name, description, data);
  opt.type   = ot_s64;
  return opt;
}

option mkopt(const char* name, const char* description, float* data)
{
  option opt = mkopt_prefix(name, description, data);
  opt.type   = ot_float;
  return opt;
}

option mkopt(const char* name, const char* description, double* data)
{
  option opt = mkopt_prefix(name, description, data);
  opt.type   = ot_double;
  return opt;
}

option mkopt(const char* name, const char* description, char* data)
{
  option opt = mkopt_prefix(name, description, data);
  opt.type   = ot_cstring;
  return opt;
}

option mkopt(const char* name, const char* description, std::string* data)
{
  option opt = mkopt_prefix(name, description, data);
  opt.type   = ot_stdstring;
  return opt;
}

option mkopt(const char* name, const char* description, bool* data)
{
  option opt = mkopt_prefix(name, description, data);
  opt.type   = ot_bool;
  return opt;
}


// Application specific mappings


std::unordered_map<std::string, output_type> output_map;
auto OT_DUMMY0 = output_map.emplace("mach", output_type::mach);
auto OT_DUMMY1 = output_map.emplace("rho",  output_type::rho);
auto OT_DUMMY2 = output_map.emplace("u",    output_type::u);
auto OT_DUMMY3 = output_map.emplace("v",    output_type::v);
auto OT_DUMMY4 = output_map.emplace("w",    output_type::w);
auto OT_DUMMY5 = output_map.emplace("rhoE", output_type::rhoE);

std::unordered_map<std::string, float*> cmap_map;
auto CM_DUMMY0 = cmap_map.emplace("cividis",  colormap_cividis);
auto CM_DUMMY1 = cmap_map.emplace("jet",      colormap_jet);
auto CM_DUMMY2 = cmap_map.emplace("coolwarm", colormap_coolwarm);
auto CM_DUMMY3 = cmap_map.emplace("viridis",  colormap_viridis);
auto CM_DUMMY4 = cmap_map.emplace("plasma",   colormap_plasma);
