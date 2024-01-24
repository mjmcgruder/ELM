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

#include "state.cpp"

void frame_buffer_resized_callback(GLFWwindow* this_window,
                                   int width, int height)
{
  frame_buffer_resized = true;
}

void scroll_callback(GLFWwindow* win, double xoffset, double yoffset)
{
  double old_cam_dist = cam_dist;
  cam_dist -= 0.1 * yoffset;
  if (cam_dist < 0.)
    cam_dist = old_cam_dist;
}

void cursor_position_callback(GLFWwindow* win, double xpos, double ypos)
{
  cursor_xpos = xpos;
  cursor_ypos = ypos;
  mouse_dx    = cursor_xpos - last_cursor_xpos;
  mouse_dy    = cursor_ypos - last_cursor_ypos;
}

void mouse_button_callback(GLFWwindow* win, int button, int action, int mods)
{
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    mouse_left_pressed = true;
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    mouse_left_pressed = false;
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    mouse_right_pressed = true;
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    mouse_right_pressed = false;
}

void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
  bool shift = mods & GLFW_MOD_SHIFT;

  if (key == GLFW_KEY_S && action == GLFW_PRESS)
    RAYCAST_MODE = raycast_mode::surface;
  if (key == GLFW_KEY_L && action == GLFW_PRESS)
    RAYCAST_MODE = raycast_mode::slice;
  if (key == GLFW_KEY_I && action == GLFW_PRESS)
    RAYCAST_MODE = raycast_mode::isosurface;

  if (key == GLFW_KEY_C && action == GLFW_PRESS)
    modify_slice = true;
  if (key == GLFW_KEY_C && action == GLFW_RELEASE)
    modify_slice = false;

  if (key == GLFW_KEY_X && action == GLFW_PRESS)
    view_axis_x_set = true;
  if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    view_axis_y_set = true;
  if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    view_axis_z_set = true;

  if (key == GLFW_KEY_M && action == GLFW_PRESS)
    mesh_display_toggle_on = !mesh_display_toggle_on;

  if (key == GLFW_KEY_U && action == GLFW_PRESS)
    render_ui = !render_ui;

  if (key == GLFW_KEY_R && (action == GLFW_PRESS || action == GLFW_REPEAT))
  {
    if (shift)
    {
      if (render_image_scale < 32)  // max downscale 32X
      {
        render_image_scale  += 1;
        frame_buffer_resized = true;
      }
    }
    else
    {
      if (render_image_scale > 1)  // max scale 1X
      {
        render_image_scale  -= 1;
        frame_buffer_resized = true;
      }
    }
  }
}
