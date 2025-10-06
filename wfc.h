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

WFC_API WFC_INLINE unsigned int wfc_randi_range(unsigned int min, unsigned int max)
{
  unsigned int r = wfc_randi();
  unsigned int range = max - min;
  unsigned int val = (r >> 16) % (range ? range : 1);

  return min + val;
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
WFC_API WFC_INLINE wfc_socket_8x07 wfc_socket_reverse(wfc_socket_8x07 s, unsigned int socket_count)
{
  wfc_socket_8x07 r = 0;
  unsigned int i;

  if (socket_count < 1 || socket_count > WFC_SOCKETS_MAX_VALUES)
  {
    return s; /* invalid count, return unchanged */
  }

  for (i = 0; i < socket_count; ++i)
  {
    unsigned int v = wfc_socket_unpack(s, (int)i);
    r = wfc_socket_pack(r, ((int)socket_count - 1) - (int)i, v);
  }

  return r;
}

/* #############################################################################
 * # Tile initialization and setup
 * #############################################################################
 */
/* Data-oriented SoA tiles struct */
typedef struct wfc_tiles
{
  /* Initialization Flags */
  unsigned int tiles_initialized;
  unsigned int tiles_compatible_tiles_computed;
  unsigned int tile_direction_compatible_masks_words; /* = (tile_count + 31) / 32 */

  /* Configuration */
  unsigned int tile_capacity;               /* Maximum number of tiles that can be stored */
  unsigned int tile_count;                  /* Current number of tiles */
  unsigned int tile_direction_count;        /* Number of directions per tile */
  unsigned int tile_direction_socket_count; /* Number of socket values per direction */

  /* Data arrays per tile */
  unsigned int *tile_asset_ids; /* Size: tile_count. User data: Tile ids. These are not used by the actual algorithm  */
  unsigned int *tile_rotations; /* Size: tile_count. For each tile id what rotation did we apply (0=no rotation, 1=1 rotation, ...) */

  /* Data arrays per tile and tile_direction_count */
  wfc_socket_8x07 *tile_direction_sockets; /* Size: tile_count * tile_direction_count. The sockets (e.g flags) for each direction */

  /* Dynamic bitmask: each tile-direction has mask_words words to represent all compatible tiles */
  unsigned int *tile_direction_compatible_masks;

} wfc_tiles;

/* Tile memory size calculation
   Computes the total bytes required for all tile arrays.

   Usage:
      unsigned int size = WFC_TILES_MEMORY_SIZE(tile_capacity, tile_direction_count);

*/
#define WFC_TILES_MEMORY_SIZE(tile_capacity, tile_direction_count)                                         \
  ((unsigned int)(sizeof(unsigned int) * ((tile_capacity) * 2                        /* ids + rotations */ \
                                          + (tile_capacity) * (tile_direction_count) /* sockets */         \
                                          + (tile_capacity) * (tile_direction_count) * ((tile_capacity + 31) / 32) /* mask words */)))

#define WFC_TILE_DIRECTION_COMPATIBLE_TILES_INDEX_AT(tiles_ptr, tile_index, dir_index) \
  (((tile_index) * (tiles_ptr)->tile_direction_count + (dir_index)) * (tiles_ptr)->tile_count)

WFC_API WFC_INLINE int wfc_tiles_initialize(wfc_tiles *tiles, unsigned char *tiles_memory, unsigned int tiles_memory_size)
{
  unsigned char *ptr = tiles_memory;

  if (!tiles || !tiles_memory || tiles_memory_size < WFC_TILES_MEMORY_SIZE(tiles->tile_capacity, tiles->tile_direction_count))
  {
    return 0;
  }

  tiles->tile_asset_ids = (unsigned int *)ptr;
  ptr += sizeof(unsigned int) * tiles->tile_capacity;

  tiles->tile_rotations = (unsigned int *)ptr;
  ptr += sizeof(unsigned int) * tiles->tile_capacity;

  tiles->tile_direction_sockets = (wfc_socket_8x07 *)ptr;
  ptr += sizeof(wfc_socket_8x07) * (tiles->tile_capacity * tiles->tile_direction_count);

  tiles->tile_direction_compatible_masks = (unsigned int *)ptr;

  tiles->tiles_initialized = 1;

  return 1;
}

WFC_API WFC_INLINE int wfc_tiles_add_tile(
    wfc_tiles *tiles,
    unsigned int tile_id,
    wfc_socket_8x07 *tile_direction_sockets,
    unsigned int tile_rotations)
{
  unsigned int i;
  unsigned int tile_direction_count;

  /* Invalid arguments */
  if (!tiles || !tiles->tiles_initialized || tiles->tile_capacity == 0 || !tile_direction_sockets)
  {
    return 0;
  }

  /* not enough space in buffer */
  if (tiles->tile_count + 1 + tile_rotations > tiles->tile_capacity)
  {
    return 0;
  }

  tile_direction_count = tiles->tile_direction_count;

  /* Add the tile */
  tiles->tile_asset_ids[tiles->tile_count] = tile_id;
  tiles->tile_rotations[tiles->tile_count] = 0;

  /* Add the sockets for each direction of the tile */
  for (i = 0; i < tile_direction_count; ++i)
  {
    tiles->tile_direction_sockets[(tiles->tile_count * tile_direction_count) + i] = tile_direction_sockets[i];
  }

  tiles->tile_count++;

  if (tile_rotations > 0)
  {
    unsigned int rotation;

    /* Clamp tile_rotations to tile_direction_count -1 */
    if (tile_rotations > tile_direction_count - 1)
    {
      tile_rotations = tile_direction_count - 1;
    }

    /* Rotate the sockets and add new tiles */
    for (rotation = 0; rotation < tile_rotations; ++rotation)
    {
      tiles->tile_asset_ids[tiles->tile_count] = tile_id;
      tiles->tile_rotations[tiles->tile_count] = rotation + 1;

      /* Rotate the socket values clockwise */
      {
        unsigned int src_base = (tiles->tile_count - 1) * tile_direction_count;
        unsigned int dst_base = tiles->tile_count * tile_direction_count;
        unsigned int j;

        for (j = 0; j < tile_direction_count; ++j)
        {
          unsigned int src_index = (j + (tile_direction_count - 1)) % tile_direction_count;

          tiles->tile_direction_sockets[dst_base + j] = tiles->tile_direction_sockets[src_base + src_index];
        }
      }

      tiles->tile_count++;
    }
  }

  return 1;
}

WFC_API WFC_INLINE int wfc_tiles_compute_compatible_tiles(wfc_tiles *tiles)
{
  unsigned int tile_count;
  unsigned int dir_count;
  unsigned int mask_words;

  unsigned int a, b, d;

  if (!tiles || !tiles->tiles_initialized || !tiles->tile_direction_compatible_masks)
  {
    return 0;
  }

  tile_count = tiles->tile_count;
  dir_count = tiles->tile_direction_count;

  tiles->tile_direction_compatible_masks_words = (tile_count + 31) / 32;
  mask_words = tiles->tile_direction_compatible_masks_words;

  /* Clear masks */
  for (a = 0; a < tile_count * dir_count * mask_words; ++a)
  {
    tiles->tile_direction_compatible_masks[a] = 0;
  }

  for (a = 0; a < tile_count; ++a)
  {

    for (d = 0; d < dir_count; ++d)
    {

      unsigned int base = (a * dir_count + d) * mask_words;
      unsigned int opp_dir = (d + dir_count / 2) % dir_count;
      wfc_socket_8x07 socket_a = tiles->tile_direction_sockets[a * dir_count + d];

      for (b = 0; b < tile_count; ++b)
      {
        wfc_socket_8x07 socket_b = tiles->tile_direction_sockets[b * dir_count + opp_dir];

        if (wfc_socket_reverse(socket_b, tiles->tile_direction_socket_count) == socket_a)
        {
          tiles->tile_direction_compatible_masks[base + (b / 32)] |= 1u << (b % 32);
        }
      }
    }
  }

  tiles->tiles_compatible_tiles_computed = 1;

  return 1;
}

/* #############################################################################
 * # Grid initialization and setup
 * #############################################################################
 */
/* Data-oriented SoA grid struct */
typedef struct wfc_grid
{
  /* Configuration */
  unsigned int rows; /* Number of grid rows    */
  unsigned int cols; /* Number of grid columns */

  /* Runtime information */
  unsigned int cells_processed;    /* The number of cells already processed */
  unsigned int cell_index_current; /* The current processed cell */

  /* Data arrays */
  unsigned char *cell_collapsed;     /* Is the current cell collapsed? Size = rows * cols */
  unsigned char *cell_entropy_count; /* How many entropy/options does the cell have? Size = rows * cols */
  unsigned char *cell_entropies;     /* The entropy array per cell & tile. Size = rows * cols * tile_count */

} wfc_grid;

/* Grid memory size calculation
   Computes the total bytes required for the grid arrays.

   Usage:
      unsigned int size = WFC_GRID_MEMORY_SIZE(rows, cols, tile_count);
*/
#define WFC_GRID_MEMORY_SIZE(rows, cols, tile_count)                                                       \
  ((unsigned int)(sizeof(unsigned char) * (((rows) * (cols)) * 2 /* cell_collapsed + cell_entropy_count */ \
                                           + ((rows) * (cols)) * (tile_count) /* cell_entropies */)))

WFC_API WFC_INLINE int wfc_grid_initialize(wfc_grid *grid, wfc_tiles *tiles, unsigned char *grid_memory, unsigned int grid_memory_size)
{
  unsigned char *ptr = grid_memory;

  unsigned int grid_size;
  unsigned int tile_count;
  unsigned int i;

  if (!grid || !tiles || !grid_memory || grid_memory_size < WFC_GRID_MEMORY_SIZE(grid->rows, grid->cols, tiles->tile_count))
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
    unsigned char tile_index;

    for (tile_index = 0; tile_index < tile_count; ++tile_index)
    {
      /* Set the tile index for the entropy */
      grid->cell_entropies[base_index + tile_index] = tile_index;
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

/* Collapse the current cell and set the first entropies entry to the desired tile_index entropies entry */
WFC_API WFC_INLINE void wfc_grid_collapse_current_cell(wfc_grid *grid, wfc_tiles *tiles, unsigned int tile_index)
{
  grid->cell_collapsed[grid->cell_index_current] = 1;     /* Mark the cell as collapsed */
  grid->cell_entropy_count[grid->cell_index_current] = 1; /* A collapsed cell has to have only 1 entropy left (0 = no tiles found, invalid )*/
  grid->cell_entropies[grid->cell_index_current * tiles->tile_count + 0] = grid->cell_entropies[grid->cell_index_current * tiles->tile_count + tile_index];
  grid->cells_processed++;
}

WFC_API WFC_INLINE int wfc_grid_neighbour_index(wfc_grid *grid, int index, unsigned int dir, unsigned int dir_count)
{
  int x, y;

  wfc_grid_coords_at(index, (int)grid->cols, &x, &y);

  (void)dir_count;

  switch (dir)
  {
  case 0:
    y -= 1;
    break; /* up */
  case 1:
    x += 1;
    break; /* right */
  case 2:
    y += 1;
    break; /* down */
  case 3:
    x -= 1;
    break; /* left */
  default:
    return -1; /* not handled (for >4 dirs extend this) */
  }

  if (x < 0 || y < 0 || x >= (int)grid->cols || y >= (int)grid->rows)
  {
    return -1;
  }

  return wfc_grid_index_at(x, y, (int)grid->cols);
}

/* #############################################################################
 * # Wave Function Collapse Algorithm
 * #############################################################################
 */
WFC_API WFC_INLINE void wfc_update_neighbour_entropies(wfc_grid *grid, wfc_tiles *tiles, unsigned int collapsed_index)
{
  unsigned int dir_count = tiles->tile_direction_count;
  unsigned int tile_count = tiles->tile_count;
  unsigned int mask_words = tiles->tile_direction_compatible_masks_words;
  unsigned int d;

  unsigned int collapsed_tile = grid->cell_entropies[collapsed_index * tile_count + 0];

  for (d = 0; d < dir_count; ++d)
  {
    int neighbour = wfc_grid_neighbour_index(grid, (int)collapsed_index, d, dir_count);

    unsigned int combined_mask[32]; /* Max 1024 tiles with 32-bit words */
    unsigned int *mask_ptr;
    unsigned char new_count = 0;
    unsigned int w, k;

    if (neighbour < 0 || grid->cell_collapsed[neighbour])
    {
      continue;
    }

    for (w = 0; w < mask_words; ++w)
    {
      combined_mask[w] = 0;
    }

    /* Combine masks from all tiles in collapsed cell (here only 1 tile) */
    mask_ptr = &tiles->tile_direction_compatible_masks[(collapsed_tile * dir_count + d) * mask_words];

    for (w = 0; w < mask_words; ++w)
    {
      combined_mask[w] |= mask_ptr[w];
    }

    /* Filter neighbour entropies */
    for (k = 0; k < grid->cell_entropy_count[neighbour]; ++k)
    {
      unsigned int tile = grid->cell_entropies[neighbour * (int)tile_count + (int)k];

      if (combined_mask[tile / 32] & (1u << (tile % 32)))
      {
        grid->cell_entropies[neighbour * (int)tile_count + new_count++] = (unsigned char)tile;
      }
    }

    grid->cell_entropy_count[neighbour] = new_count;
  }
}

WFC_API WFC_INLINE int wfc(wfc_grid *grid, wfc_tiles *tiles)
{
  unsigned int total_cells;
  unsigned int iteration;

  if (!grid || !tiles || !tiles->tiles_initialized || grid->rows < 1 || grid->cols < 1 || tiles->tile_count < 1)
  {
    return 0;
  }

  if (!tiles->tiles_compatible_tiles_computed)
  {
    wfc_tiles_compute_compatible_tiles(tiles);
  }

  total_cells = grid->rows * grid->cols;
  grid->cells_processed = 0;

  /* Repeat until all cells are collapsed */
  for (iteration = 0; iteration < total_cells; ++iteration)
  {
    unsigned int lowest_entropy = 255;
    unsigned int lowest_cell = 0;
    unsigned int i;

    /* 1. Find the non-collapsed cell with the lowest entropy */
    for (i = 0; i < total_cells; ++i)
    {
      unsigned char count = grid->cell_entropy_count[i];

      if (!grid->cell_collapsed[i])
      {
        if (count == 0)
        {
          return 0; /* This cell has no valid tiles → unsolvable */
        }

        if (count < lowest_entropy)
        {
          lowest_entropy = count;
          lowest_cell = i;
        }
      }
    }

    /* no cell found (finished or stuck) */
    if (lowest_entropy == 255)
    {
      break;
    }

    /* 2. Randomly choose one tile from available entropies */
    {
      unsigned int choice_index;

      choice_index = wfc_randi_range(0, lowest_entropy);

      if (choice_index >= lowest_entropy)
      {
        choice_index = lowest_entropy - 1;
      }

      grid->cell_index_current = lowest_cell;
      wfc_grid_collapse_current_cell(grid, tiles, choice_index);
    }

    /* 3. Propagate constraints */
    wfc_update_neighbour_entropies(grid, tiles, grid->cell_index_current);
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
