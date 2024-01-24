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

#include "pipeline.cpp"


// uniform buffer

template<typename T>
struct uniform
{
  T host_data;

  descriptor_set dset;
  dbuffer<T> buffer;

  // --

  uniform();

  uniform(uniform& oth)            = delete;
  uniform& operator=(uniform& oth) = delete;

  uniform(uniform&& oth)            noexcept;
  uniform& operator=(uniform&& oth) noexcept;

  uniform(descriptor_set_layout* dset_layout);

  // --

  void map();
};


// entity

struct entity
{
  dbuffer<vertex> vertices;
  dbuffer<u32> indices;
  uniform<object_transform> ubo;

  // ---

  entity();

  entity(const entity& oth)      = delete;
  entity& operator=(entity& oth) = delete;

  entity(entity&& oth)            noexcept;
  entity& operator=(entity&& oth) noexcept;

  entity(descriptor_set_layout* uniform_layout, vertex* vertex_data,
         u64 vertex_len, u32* index_data, u64 index_len);

  // ---

  void update_uniform();
};

/* IMPLEMENTATION ----------------------------------------------------------- */


/* ------- */
/* uniform ------------------------------------------------------------------ */
/* ------- */

template<typename T>
void uniform<T>::map()
{
  memcpy_htod(buffer, &host_data);

  dset.update(buffer, 0);
}

template<typename T>
uniform<T>::uniform(descriptor_set_layout* dset_layout) :
host_data{},
dset{dset_layout},
buffer(1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
{
  dmalloc(buffer);
}

template<typename T>
uniform<T>::uniform() :
host_data{},
dset{},
buffer{0}
{}

template<typename T>
uniform<T>::uniform(uniform&& oth) noexcept :
host_data(std::move(oth.host_data)),
dset(std::move(oth.dset)),
buffer(std::move(oth.buffer))
{}

template<typename T>
uniform<T>& uniform<T>::operator=(uniform&& oth) noexcept
{
  host_data = std::move(oth.host_data);
  dset      = std::move(oth.dset);
  buffer    = std::move(oth.buffer);

  return *this;
}

/* ------ */
/* entity ------------------------------------------------------------------- */
/* ------ */

entity::entity(descriptor_set_layout* uniform_layout, vertex* vertex_data,
               u64 vertex_len, u32* index_data, u64 index_len)
{
  vertices = dbuffer<vertex>(vertex_len,
  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

  indices = dbuffer<u32>(index_len,
  VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

  ubo = uniform<object_transform>{uniform_layout};

  dmalloc(vertices);
  dmalloc(indices);

  memcpy_htod(vertices, vertex_data);
  memcpy_htod(indices, index_data);
}

void entity::update_uniform()
{
  ubo.map();
}

entity::entity() : vertices{}, indices{}, ubo{}
{}

entity::entity(entity&& oth) noexcept :
vertices(std::move(oth.vertices)),
indices(std::move(oth.indices)),
ubo(std::move(oth.ubo))
{}

entity& entity::operator=(entity&& oth) noexcept
{
  vertices = std::move(oth.vertices);
  indices  = std::move(oth.indices);
  ubo      = std::move(oth.ubo);

  return *this;
}
