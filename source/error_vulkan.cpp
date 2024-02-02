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


#include "error.cpp"
#include "state.cpp"


void exit_clean_vulkan(const char* file, int line, ...)
{
  // print a nice messsage
  char message[ERR_MSG_SIZE];
  va_list args;
  va_start(args, line);
  vsnprintf(message, ERR_MSG_SIZE, va_arg(args, char*), args);
  va_end(args);
  printf("error! (line %d of %s):\n", line, file);
  printf("  %s\n\n", message);
  // kill the fancy objects
  clean_global_resources();
  // bail out
  exit(1);
}


#define VKTERMINATE(...) exit_clean_vulkan(__FILE__, __LINE__, __VA_ARGS__)


#define VK_CHECK(func, ...)                     \
  {                                             \
    VkResult fail_result;                       \
    if ((fail_result = (func)) != VK_SUCCESS)   \
    {                                           \
      printf("result code: %d\n", fail_result); \
      VKTERMINATE(__VA_ARGS__);                 \
    }                                           \
  }


#define VK_CHECK_SUBOPTIMAL(func, ...)             \
  {                                                \
    VkResult fail_result = (func);                 \
    if (fail_result != VK_SUCCESS &&               \
        fail_result != VK_ERROR_OUT_OF_DATE_KHR && \
        fail_result != VK_SUBOPTIMAL_KHR)          \
    {                                              \
      printf("result code: %d\n", fail_result);    \
      VKTERMINATE(__VA_ARGS__);                    \
    }                                              \
  }
