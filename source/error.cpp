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


#include <cstdarg>
#include <cstdio>
#include <cstdlib>


#define ERR_MSG_SIZE 1024


void exit_no_clean(const char* file, int line, ...)
{
  char message[ERR_MSG_SIZE];
  va_list args;
  va_start(args, line);
  vsnprintf(message, ERR_MSG_SIZE, va_arg(args, char*), args);
  va_end(args);
  printf("error! (line %d of %s):\n", line, file);
  printf("  %s\n\n", message);
  exit(1);
}

#define TERMINATE(...) exit_no_clean(__FILE__, __LINE__, __VA_ARGS__)
