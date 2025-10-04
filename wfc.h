/* wfc.h - v0.2 - public domain data structures - nickscha 2025

A C89 standard compliant, single header, nostdlib (no C Standard Library) Wave Function Collapse (WFC).

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#ifndef WFC_H
#define WFC_H

/* #############################################################################
 * # COMPILER SETTINGS
 * #############################################################################
 */
/* Check if using C99 or later (inline is supported) */
#if __STDC_VERSION__ >= 199901L
#define WFC_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define WFC_INLINE __inline__
#elif defined(_MSC_VER)
#define WFC_INLINE __inline
#else
#define WFC_INLINE
#endif

#define WFC_API static

/* #############################################################################
 * # Math & RNG
 * #############################################################################
 */
/* Linear Congruential Generator (LCG) constants */
#define WFC_LCG_A 1664525U
#define WFC_LCG_C 1013904223U
#define WFC_LCG_M 4294967296.0f /* 2^32 */

/* Seed for the random number generator */
static unsigned int wfc_seed_lcg = 1;

WFC_API WFC_INLINE unsigned int wfc_randi(void)
{
  wfc_seed_lcg = (WFC_LCG_A * wfc_seed_lcg + WFC_LCG_C);
  return wfc_seed_lcg;
}

WFC_API WFC_INLINE int wfc_randi_range(int min, int max)
{
  unsigned int r = wfc_randi();
  unsigned int range = (unsigned int)(max - min);
  unsigned int val = (r >> 16) % (range ? range : 1);

  return min + (int)val;
}

/* #############################################################################
 * # Socket Mask
 * #############################################################################
 */
/* Packs 8 sockets that can have a value from 0 - 7 in a single 32bit type (here we only use 24bits, 8 values * 3bits for 0-7 range)  */
#define WFC_SOCKETS_MAX_VALUES 8
typedef unsigned int wfc_socket_8x07;

/* Set socket[idx] = value (idx = 0..7, value = 0..7) */
WFC_API WFC_INLINE wfc_socket_8x07 wfc_socket_pack(wfc_socket_8x07 s, int idx, unsigned int value)
{
  int shift = idx * 3;
  wfc_socket_8x07 mask = (wfc_socket_8x07)(0x7 << shift);
  s = (s & ~mask) | ((value & 0x7) << shift);
  return s;
}

WFC_API WFC_INLINE wfc_socket_8x07 wfc_socket_pack_4(
    unsigned int socket_value_0,
    unsigned int socket_value_1,
    unsigned int socket_value_2,
    unsigned int socket_value_3)
{
  wfc_socket_8x07 s = 0;
  s = wfc_socket_pack(s, 0, socket_value_0);
  s = wfc_socket_pack(s, 1, socket_value_1);
  s = wfc_socket_pack(s, 2, socket_value_2);
  s = wfc_socket_pack(s, 3, socket_value_3);

  return s;
}

WFC_API WFC_INLINE wfc_socket_8x07 wfc_socket_pack_8(
    unsigned int socket_value_0,
    unsigned int socket_value_1,
    unsigned int socket_value_2,
    unsigned int socket_value_3,
    unsigned int socket_value_4,
    unsigned int socket_value_5,
    unsigned int socket_value_6,
    unsigned int socket_value_7)
{
  wfc_socket_8x07 s = 0;
  s = wfc_socket_pack(s, 0, socket_value_0);
  s = wfc_socket_pack(s, 1, socket_value_1);
  s = wfc_socket_pack(s, 2, socket_value_2);
  s = wfc_socket_pack(s, 3, socket_value_3);
  s = wfc_socket_pack(s, 4, socket_value_4);
  s = wfc_socket_pack(s, 5, socket_value_5);
  s = wfc_socket_pack(s, 6, socket_value_6);
  s = wfc_socket_pack(s, 7, socket_value_7);

  return s;
}

/* Get socket[idx] (idx = 0..7) */
WFC_API WFC_INLINE unsigned int wfc_socket_unpack(wfc_socket_8x07 s, int idx)
{
  int shift = idx * 3;
  return (s >> shift) & 0x7;
}

/* Reverse order of sockets, up to socket_count */
WFC_API WFC_INLINE wfc_socket_8x07 wfc_socket_reverse(wfc_socket_8x07 s, int socket_count)
{
  wfc_socket_8x07 r = 0;
  int i;

  if (socket_count <= 0 || socket_count > WFC_SOCKETS_MAX_VALUES)
  {
    return s; /* invalid count, return unchanged */
  }

  for (i = 0; i < socket_count; ++i)
  {
    unsigned int v = wfc_socket_unpack(s, i);
    r = wfc_socket_pack(r, (socket_count - 1) - i, v);
  }

  return r;
}

/* #############################################################################
 * # Grid & Tile initialization & setup
 * #############################################################################
 */
/* Data-oriented SoA tiles struct */
typedef struct wfc_tiles
{
  unsigned int tile_count;             /* Total number of tiles */
  unsigned int tile_edge_count;        /* Number of edges per tile */
  unsigned int tile_edge_socket_count; /* Number of socket values per edge */
  unsigned int *tile_ids;              /* User data: Tile ids. These are not used by the actual algorithm  */
  wfc_socket_8x07 *tile_edge_sockets;  /* (tile_count * tile_edge_count)*/

} wfc_tiles;

typedef struct wfc_grid
{
  unsigned int rows;                 /* Number of grid rows    */
  unsigned int cols;                 /* Number of grid columns */
  unsigned int cell_current;         /* The current processed cell */
  unsigned char *cell_collapsed;     /* Is the current cell collapsed? Size = rows * cols */
  unsigned char *cell_entropy_count; /* How many entropy/options does the cell have? Size = rows * cols */
  unsigned char *cell_entropies;     /* The entropy array per cell & tile. Size = rows * cols * tile_count */

} wfc_grid;

WFC_API WFC_INLINE int wfc_tiles_memory_size(wfc_tiles *tiles, unsigned int *tiles_memory_size)
{
  unsigned int base_size = (unsigned int)sizeof(unsigned int);

  if (!tiles || !tiles_memory_size || tiles->tile_count == 0 || tiles->tile_edge_count == 0 || tiles->tile_edge_socket_count == 0)
  {
    return 0;
  }

  *tiles_memory_size = base_size * tiles->tile_count +                         /* tile_ids */
                       base_size * tiles->tile_count * tiles->tile_edge_count; /* tile_edge_sockets */

  return 1;
}

WFC_API WFC_INLINE int wfc_grid_memory_size(wfc_grid *grid, wfc_tiles *tiles, unsigned int *grid_memory_size)
{
  unsigned int base_size = (unsigned int)sizeof(unsigned char);
  unsigned int grid_size = 0;

  if (!grid || !tiles || !grid_memory_size || grid->rows == 0 || grid->cols == 0 || tiles->tile_count == 0)
  {
    return 0;
  }

  grid_size = grid->cols * grid->rows;

  *grid_memory_size = base_size * grid_size +                    /* cell_collapsed */
                      base_size * grid_size +                    /* cell_entropy_count */
                      base_size * grid_size * tiles->tile_count; /* cell_entropies */
  return 1;
}

WFC_API WFC_INLINE int wfc_tiles_initialize(wfc_tiles *tiles, unsigned char *tiles_memory, unsigned int tiles_memory_size)
{
  unsigned char *ptr = tiles_memory;
  unsigned int required_memory_size = 0;

  if (!tiles || !tiles_memory || !wfc_tiles_memory_size(tiles, &required_memory_size) || tiles_memory_size < required_memory_size)
  {
    return 0;
  }

  tiles->tile_ids = (unsigned int *)ptr;
  ptr += sizeof(unsigned int) * tiles->tile_count;
  tiles->tile_edge_sockets = (wfc_socket_8x07 *)ptr;

  return 1;
}

WFC_API WFC_INLINE int wfc_grid_initialize(wfc_grid *grid, wfc_tiles *tiles, unsigned char *grid_memory, unsigned int grid_memory_size)
{
  unsigned char *ptr = grid_memory;
  unsigned int required_memory_size = 0;

  unsigned int grid_size;
  unsigned int tile_count;
  unsigned int i;
  unsigned char t;

  if (!grid || !tiles || !grid_memory || !wfc_grid_memory_size(grid, tiles, &required_memory_size) || grid_memory_size < required_memory_size)
  {
    return 0;
  }

  grid_size = grid->cols * grid->rows;
  tile_count = tiles->tile_count;

  grid->cell_collapsed = ptr;
  ptr += sizeof(unsigned char) * grid_size;
  grid->cell_entropy_count = ptr;
  ptr += sizeof(unsigned char) * grid_size;
  grid->cell_entropies = ptr;

  /* For each cell in the grid we set the cell_entropies to all available tiles */
  for (i = 0; i < grid_size; ++i)
  {
    unsigned int base_index = (i * tile_count);

    for (t = 0; t < tile_count; ++t)
    {
      /* Set the tile index for the entropy */
      grid->cell_entropies[base_index + t] = t;
    }

    grid->cell_collapsed[i] = 0;
    grid->cell_entropy_count[i] = (unsigned char)tile_count;
  }

  return 1;
}

WFC_API WFC_INLINE int wfc_grid_index_at(int x, int y, int cols)
{
  return (y * cols + x);
}

WFC_API WFC_INLINE void wfc_grid_coords_at(int index, int cols, int *x, int *y)
{
  if (!x || !y)
  {
    return; /* invalid input, no-op */
  }

  *y = index / cols;
  *x = index % cols;
}

/* #############################################################################
 * # Wave Function Collapse Algorithm
 * #############################################################################
 */

WFC_API WFC_INLINE void wfc_update_neighbor(
    wfc_grid *grid, wfc_tiles *tiles,
    int cell_idx, int neighbor_idx,
    int edge, int opposite_edge)
{
  int t, new_count = 0;
  int tile_count = (int)tiles->tile_count;

  /* collapsed tile of current cell */
  unsigned char collapsed_tile = grid->cell_entropies[cell_idx * tile_count + 0];
  wfc_socket_8x07 collapsed_socket = tiles->tile_edge_sockets[collapsed_tile * tiles->tile_edge_count + (unsigned int)edge];

  /* scan neighbor's entropy list */
  for (t = 0; t < tile_count; ++t)
  {
    unsigned char candidate_tile = grid->cell_entropies[neighbor_idx * tile_count + t];
    wfc_socket_8x07 neighbor_socket;

    if (candidate_tile >= tile_count)
      continue; /* skip invalid */

    neighbor_socket = tiles->tile_edge_sockets[candidate_tile * tiles->tile_edge_count + (unsigned int)opposite_edge];

    /* if sockets match, keep this tile */
    if (collapsed_socket == neighbor_socket)
    {
      grid->cell_entropies[neighbor_idx * tile_count + new_count] = candidate_tile;
      new_count++;
    }
  }

  grid->cell_entropy_count[neighbor_idx] = (unsigned char)new_count;
}

WFC_API WFC_INLINE int wfc(wfc_grid *grid, wfc_tiles *tiles)
{
  int grid_size, tile_count;
  int steps = 0;

  if (!grid || !tiles)
    return 0;

  grid_size = (int)(grid->rows * grid->cols);
  tile_count = (int)tiles->tile_count;

  while (1)
  {
    int min_entropy = 255;
    int candidates[16 * 16];
    int n_candidates = 0;
    int i;
    int chosen_idx;
    int tile_choice;

    /* (1) Find non-collapsed cells with lowest entropy */
    for (i = 0; i < grid_size; ++i)
    {
      unsigned char entropy;

      if (grid->cell_collapsed[i])
        continue;

      entropy = grid->cell_entropy_count[i];
      if (entropy == 0)
        return 0; /* contradiction: no possible tiles */

      if (entropy < min_entropy)
      {
        min_entropy = entropy;
        n_candidates = 0;
        candidates[n_candidates++] = i;
      }
      else if (entropy == min_entropy)
      {
        candidates[n_candidates++] = i;
      }
    }

    if (n_candidates == 0)
      break; /* all cells collapsed */

    /* (2) Pick a random lowest-entropy cell */
    chosen_idx = candidates[wfc_randi_range(0, n_candidates)];

    /* (3) Pick one of its allowed tiles randomly */
    tile_choice = wfc_randi_range(0, grid->cell_entropy_count[chosen_idx]);
    grid->cell_entropies[chosen_idx * tile_count + 0] =
        grid->cell_entropies[chosen_idx * tile_count + tile_choice];
    grid->cell_entropy_count[chosen_idx] = 1;
    grid->cell_collapsed[chosen_idx] = 1;

    /* (4) Propagate constraints to neighbors */
    {
      static const int wfc_dirs[4][3] = {
          {0, -1, 2}, /* top edge → neighbor bottom */
          {1, 0, 3},  /* right edge → neighbor left */
          {0, 1, 0},  /* bottom edge → neighbor top */
          {-1, 0, 1}  /* left edge → neighbor right */
      };

      int x, y, e;
      wfc_grid_coords_at(chosen_idx, (int)grid->cols, &x, &y);

      for (e = 0; e < (int)tiles->tile_edge_count; ++e)
      {
        int nx = x + wfc_dirs[e][0];
        int ny = y + wfc_dirs[e][1];
        int n_idx;

        if (nx < 0 || ny < 0 || nx >= (int)grid->cols || ny >= (int)grid->rows)
          continue;

        n_idx = wfc_grid_index_at(nx, ny, (int)grid->cols);
        if (grid->cell_collapsed[n_idx])
          continue;

        wfc_update_neighbor(grid, tiles, chosen_idx, n_idx, e, wfc_dirs[e][2]);
      }
    }

    steps++;
    if (steps > 1000)
      break; /* safety guard */
  }

  return 1;
}

#endif /* WFC_H */

/*
   ------------------------------------------------------------------------------
   This software is available under 2 licenses -- choose whichever you prefer.
   ------------------------------------------------------------------------------
   ALTERNATIVE A - MIT License
   Copyright (c) 2025 nickscha
   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   ------------------------------------------------------------------------------
   ALTERNATIVE B - Public Domain (www.unlicense.org)
   This is free and unencumbered software released into the public domain.
   Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
   software, either in source code form or as a compiled binary, for any purpose,
   commercial or non-commercial, and by any means.
   In jurisdictions that recognize copyright laws, the author or authors of this
   software dedicate any and all copyright interest in the software to the public
   domain. We make this dedication for the benefit of the public at large and to
   the detriment of our heirs and successors. We intend this dedication to be an
   overt act of relinquishment in perpetuity of all present and future rights to
   this software under copyright law.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   ------------------------------------------------------------------------------
*/
