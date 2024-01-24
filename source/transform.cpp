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
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>


struct scene_transform
{
  glm::mat4 view = glm::mat4(1.f);
  glm::mat4 proj = glm::mat4(1.f);

  glm::mat4 slice_model = glm::mat4(1.f);

  scene_transform()
  {
    glm::mat4 cam_model = glm::mat4(1.f);

    const float pi_2      = glm::pi<float>() / 2.f;
    const glm::vec3 yaxis = glm::vec3(0.f, 1.f, 0.f);

    cam_model = glm::rotate(cam_model, pi_2, yaxis);
    cam_model = glm::translate(cam_model, glm::vec3(0.f, 0.f, cam_dist));
    view      = glm::inverse(cam_model);
  }
};

struct object_transform
{
  glm::mat4 model = glm::mat4(1.f);
  glm::mat4 view  = glm::mat4(1.f);
};

// Using the current and last mouse positions in screen space (from global
// state), finds the equivalent translation vector in model space assuming the z
// position of the screen in the frustum intersects the origin. This rule
// automatically generates roughly correct mouse sensitivity at the model.
glm::vec4 model_space_translation(const scene_transform& mvp)
{
  // useful constants
  glm::mat4 iproj  = glm::inverse(mvp.proj);
  glm::mat4 iview  = glm::inverse(mvp.view);
  int img_width_i, img_height_i;
  glfwGetWindowSize(window, &img_width_i, &img_height_i);
  float img_width  = (float)img_width_i;
  float img_height = (float)img_height_i;
  glm::vec2 o(img_width / 2.f, img_height / 2.f);    // screen origin (pixels)
  glm::vec2 lf(last_cursor_xpos, last_cursor_ypos);  // last framebuffer pos
  glm::vec2 cf(cursor_xpos, cursor_ypos);            // curr framebuffer pos

  // find z position of origin in NDC
  glm::vec4 v_origin(0.f, 0.f, -cam_dist, 1.f);
  glm::vec4 c_origin = mvp.proj * v_origin;
  glm::vec4 n_origin = c_origin / c_origin.w;

  // last and current mouse position in normalized device coordinates
  glm::vec4 n_last((lf.x - o.x) * (2.f / img_width),
                   (lf.y - o.y) * (2.f / img_height),
                   n_origin.z, 1.f);
  glm::vec4 n_curr((cf.x - o.x) * (2.f / img_width),
                   (cf.y - o.y) * (2.f / img_height),
                   n_origin.z, 1.f);

  // convert positions back to model space
  glm::vec4 v_last = iproj * n_last;
  glm::vec4 v_curr = iproj * n_curr;
  v_last          /= v_last.w;
  v_curr          /= v_curr.w;
  glm::vec4 m_last = iview * v_last;
  glm::vec4 m_curr = iview * v_curr;

  // translate by (negative) vector between positions in view space
  glm::vec4 m_translation = glm::vec4(m_curr - m_last);

  return m_translation;
}

// Using the current and last mouse positions in screen space (from global
// state), finds a rotation angle and vector in model space assuming the mouse
// is rotating a ball of given radius. If the ball radius is approximately the
// model radius, rotational sensitivity is automatically set to track the mouse.
void model_space_rotation(const scene_transform& mvp, const glm::mat4& cam_model,
                          aabb& domain_bbox, float& ang, glm::vec4& m_rot)
{
  // useful constants
  glm::mat4 iproj  = glm::inverse(mvp.proj);
  glm::mat4 iview  = glm::inverse(mvp.view);
  int img_width_i, img_height_i;
  glfwGetWindowSize(window, &img_width_i, &img_height_i);
  float img_width  = (float)img_width_i;
  float img_height = (float)img_height_i;
  glm::vec2 o(img_width / 2.f, img_height / 2.f);    // screen origin (pixels)
  glm::vec2 lf(last_cursor_xpos, last_cursor_ypos);  // last framebuffer pos
  glm::vec2 cf(cursor_xpos, cursor_ypos);            // curr framebuffer pos

  // find distance in "look direction" to origin
  glm::vec3 lengths(domain_bbox.h - domain_bbox.l);
  float radius = (lengths.x + lengths.y + lengths.z) / 3.f;
  // float radius       = 0.5f;
  float ang_negation = 1.f;

  float z_dist = glm::dot(
    glm::vec3(0.f, 0.f, -1.f),
    glm::vec3(mvp.view * glm::vec4(glm::vec3(cam_model[3]), 0.f))
  );

  if (glm::abs(z_dist) < radius) { ang_negation = -1.f; }

  // find z location of sphere surface in NDC
  glm::vec4 v_surf(0.f, 0.f, z_dist + radius, 1.f);
  glm::vec4 c_surf = mvp.proj * v_surf;
  glm::vec4 n_surf = c_surf / c_surf.w;

  // last and current mouse position in normalized device coordinates
  glm::vec4 n_last((lf.x - o.x) * (2.f / img_width),
                   (lf.y - o.y) * (2.f / img_height),
                   n_surf.z, 1.f);
  glm::vec4 n_curr((cf.x - o.x) * (2.f / img_width),
                   (cf.y - o.y) * (2.f / img_height),
                   n_surf.z, 1.f);

  // convert mouse vector positions back to model space
  glm::vec4 v_last = iproj * n_last;
  glm::vec4 v_curr = iproj * n_curr;
  v_last          /= v_last.w;
  v_curr          /= v_curr.w;
  glm::vec4 m_last = iview * v_last;
  glm::vec4 m_curr = iview * v_curr;

  // find rotation vector and angle in model space
  glm::vec3 m_last3(m_last);
  glm::vec3 m_curr3(m_curr);

  ang = ang_negation * glm::acos(glm::clamp(
  glm::dot(m_last3, m_curr3) / (glm::length(m_last3) * glm::length(m_curr3)),
  -1.f, 1.f));

  m_rot = -glm::vec4(glm::cross(m_last3, m_curr3), 0.f);
}

void update_scene_transform(scene_transform& mvp, aabb& domain_bbox)
{
  /* projection matrix */

  float img_width  = (float)swap_chain_extent.width;
  float img_height = (float)swap_chain_extent.height;
  float img_aspect = img_width / img_height;
  mvp.proj = glm::perspective(glm::radians(45.f), img_aspect, 0.1f, 100.f);
  mvp.proj[1][1] *= -1.;  // convert to right-handed

  /* camera manipulation */

  glm::mat4 cam_model = glm::inverse(mvp.view);

  // zoom : update cam distance maintaining offset direction

  glm::vec3 offset(cam_model[3]);  // current translation
  offset  = glm::normalize(offset);
  offset *= cam_dist;

  cam_model[3][0] = offset[0];
  cam_model[3][1] = offset[1];
  cam_model[3][2] = offset[2];

  // translate : find mouse vector at origin, move camera opposite

  if (mouse_right_pressed)
  {
    glm::vec4 m_trans = model_space_translation(mvp);

    if (RAYCAST_MODE == raycast_mode::slice && modify_slice)
    {
      glm::mat4 smodinv = glm::inverse(mvp.slice_model);
      mvp.slice_model   = glm::translate(mvp.slice_model,
                                         glm::vec3(smodinv * m_trans));
    }
    else
    {
      glm::vec4 v_trans = mvp.view * m_trans;
      cam_model = glm::translate(cam_model, -glm::vec3(v_trans));
    }
  }

  // rotate : rotate the conceptual ball in model space based on mouse vector

  if (mouse_left_pressed)
  {
    float ang; glm::vec4 m_rot;
    model_space_rotation(mvp, cam_model, domain_bbox, ang, m_rot);

    if (glm::abs(ang) > 1e-3f)
    {
      if (RAYCAST_MODE == raycast_mode::slice && modify_slice)
      {
        glm::mat4 smodinv = glm::inverse(mvp.slice_model);
        mvp.slice_model   = glm::rotate(mvp.slice_model,
                                        -ang, glm::vec3(smodinv * m_rot));
      }
      else
      {
        glm::vec4 v_rot = mvp.view * m_rot;

        glm::vec3 m_trans(cam_model[3]);
        glm::vec3 v_trans(mvp.view * glm::vec4(m_trans, 0.f));

        cam_model = glm::translate(cam_model, -v_trans);
        cam_model = glm::rotate(cam_model, ang, glm::vec3(v_rot));
        cam_model = glm::translate(cam_model, v_trans);
      }
    }
  }

  // snapping to fixed views

  const float pi_2  = glm::pi<float>() / 2.f;
  glm::vec3 origin(0.f, 0.f, 0.f);
  float origin_dist = glm::length(glm::vec3(cam_model[3]));

  if (view_axis_x_set)
  {
    if (RAYCAST_MODE == raycast_mode::slice && modify_slice)
    {
      mvp.slice_model =
        glm::rotate(glm::mat4(1.f), pi_2, glm::vec3(0.f, 1.f, 0.f));
    }
    else
    {
      cam_model = glm::inverse(glm::lookAt(
        glm::vec3(origin_dist, 0.f, 0.f), origin, glm::vec3(0.f, 1.f, 0.f)
      ));
    }
  }

  if (view_axis_y_set)
  {
    if (RAYCAST_MODE == raycast_mode::slice && modify_slice)
    {
      mvp.slice_model =
        glm::rotate(glm::mat4(1.f), pi_2, glm::vec3(1.f, 0.f, 0.f));
    }
    else
    {
      cam_model = glm::inverse(glm::lookAt(
        glm::vec3(0.f, origin_dist, 0.f), origin, glm::vec3(1.f, 0.f, 0.f)
      ));
    }
  }

  if (view_axis_z_set)
  {
    if (RAYCAST_MODE == raycast_mode::slice && modify_slice)
    {
      mvp.slice_model = glm::mat4(1.f);
    }
    else
    {
      cam_model = glm::inverse(glm::lookAt(
        glm::vec3(0.f, 0.f, origin_dist), origin, glm::vec3(0.f, 1.f, 0.f)
      ));
    }
  }

  mvp.view = glm::inverse(cam_model);
}

void update_axis_transform(const scene_transform& scene_ubo,
                           object_transform& axis)
{
  axis.view = glm::lookAt(
    glm::vec3(0.f, 0.f, 8.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)
  );

  axis.model       = scene_ubo.view;
  axis.model[3][0] = 0.f;
  axis.model[3][1] = 0.f;
  axis.model[3][2] = 0.f;
}

void reset_ui()
{
  mouse_dx = 0.;
  mouse_dy = 0.;

  last_cursor_xpos = cursor_xpos;
  last_cursor_ypos = cursor_ypos;

  view_axis_x_set = false;
  view_axis_y_set = false;
  view_axis_z_set = false;
}
