/* wfc.h - v0.2 - public domain data structures - nickscha 2025

A C89 standard compliant, single header, nostdlib (no C Standard Library) Wave Function Collapse (WFC).

This Test class defines cases to verify that we don't break the excepted behaviours in the future upon changes.

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#include "../wfc.h"         /* Wave Function Collapse      */
#include "../deps/test.h"   /* Simple Testing framework    */
#include "../deps/perf.h"   /* Simple Performance profiler */
#include "wfc_visualizer.h" /* Export grid as ppm file     */

static void wfc_test_socket(void)
{
  wfc_socket_8x07 socket = wfc_socket_pack_8(0, 1, 2, 3, 4, 5, 6, 7);
  wfc_socket_8x07 reversed;

  assert(wfc_socket_unpack(socket, 0) == 0);
  assert(wfc_socket_unpack(socket, 1) == 1);
  assert(wfc_socket_unpack(socket, 2) == 2);
  assert(wfc_socket_unpack(socket, 3) == 3);
  assert(wfc_socket_unpack(socket, 4) == 4);
  assert(wfc_socket_unpack(socket, 5) == 5);
  assert(wfc_socket_unpack(socket, 6) == 6);
  assert(wfc_socket_unpack(socket, 7) == 7);

  /* Reverse hole socket */
  reversed = wfc_socket_reverse(socket, 8);

  assert(wfc_socket_unpack(reversed, 0) == 7);
  assert(wfc_socket_unpack(reversed, 1) == 6);
  assert(wfc_socket_unpack(reversed, 2) == 5);
  assert(wfc_socket_unpack(reversed, 3) == 4);
  assert(wfc_socket_unpack(reversed, 4) == 3);
  assert(wfc_socket_unpack(reversed, 5) == 2);
  assert(wfc_socket_unpack(reversed, 6) == 1);
  assert(wfc_socket_unpack(reversed, 7) == 0);

  /* Reverse only the first three entries */
  reversed = wfc_socket_reverse(socket, 3);

  assert(wfc_socket_unpack(reversed, 0) == 2);
  assert(wfc_socket_unpack(reversed, 1) == 1);
  assert(wfc_socket_unpack(reversed, 2) == 0);

  socket = wfc_socket_pack(socket, 2, 0);

  assert(wfc_socket_unpack(socket, 2) == 0);
}

static void wfc_test_tile_stack_alloc(void)
{
#define TILES_CAPACTIY 128
#define TILES_EDGE_COUNT 4

  unsigned char tiles_memory[WFC_TILES_MEMORY_SIZE(TILES_CAPACTIY, TILES_EDGE_COUNT)];

  wfc_tiles tiles = {0};
  tiles.tile_capacity = TILES_CAPACTIY;     /* 5 tiles */
  tiles.tile_edge_count = TILES_EDGE_COUNT; /* 4 edges */
  tiles.tile_edge_socket_count = 3;         /* 3 values per edge */

  assert(wfc_tiles_initialize(&tiles, tiles_memory, (unsigned int)(sizeof(tiles_memory) / sizeof(tiles_memory[0]))));
}

static void wfc_test_tile_rotation_even_sockets(void)
{
  unsigned char *tiles_memory;
  unsigned int tiles_memory_size = 0;
  wfc_socket_8x07 socket_buffer[4];

  wfc_tiles tiles = {0};
  tiles.tile_capacity = 5;          /* 5 tiles */
  tiles.tile_edge_count = 4;        /* 4 edges */
  tiles.tile_edge_socket_count = 3; /* 3 values per edge */

  tiles_memory_size = WFC_TILES_MEMORY_SIZE(tiles.tile_capacity, tiles.tile_edge_count);
  tiles_memory = malloc(tiles_memory_size);

  assert(wfc_tiles_initialize(&tiles, tiles_memory, tiles_memory_size));

  /* Setup tile sockets */

  /* Tile 0 (empty, no rotation required):
     "   "
     "   "
     "   "
  */
  socket_buffer[0] = wfc_socket_pack_4(0, 0, 0, 0); /* Top    */
  socket_buffer[1] = wfc_socket_pack_4(0, 0, 0, 0); /* Right  */
  socket_buffer[2] = wfc_socket_pack_4(0, 0, 0, 0); /* Bottom */
  socket_buffer[3] = wfc_socket_pack_4(0, 0, 0, 0); /* Left   */

  /* Add tile without additional rotations */
  wfc_tiles_add_tile(&tiles, 0, socket_buffer, 0);

  /* Tile 1 (cross, rotate 3 times for each edge):
     " # "
     "###"
     "   "
  */
  socket_buffer[0] = wfc_socket_pack_4(0, 1, 0, 0); /* Top    */
  socket_buffer[1] = wfc_socket_pack_4(0, 1, 0, 0); /* Right  */
  socket_buffer[2] = wfc_socket_pack_4(0, 0, 0, 0); /* Bottom */
  socket_buffer[3] = wfc_socket_pack_4(0, 1, 0, 0); /* Left   */

  /* Add tile with three rotations */
  wfc_tiles_add_tile(&tiles, 1, socket_buffer, 3);

  assert(tiles.tile_count == 5);
  assert(tiles.tile_ids[0] == 0);
  assert(tiles.tile_ids[1] == 1);
  assert(tiles.tile_ids[2] == 1);
  assert(tiles.tile_ids[3] == 1);
  assert(tiles.tile_ids[4] == 1);
  assert(tiles.tile_rotations[0] == 0);
  assert(tiles.tile_rotations[1] == 0);
  assert(tiles.tile_rotations[2] == 1);
  assert(tiles.tile_rotations[3] == 2);
  assert(tiles.tile_rotations[4] == 3);

  /* Check original tile 1 sockets
     " # "
     "###"
     "   "
  */
  assert(tiles.tile_edge_sockets[(1 * tiles.tile_edge_count) + 0] == wfc_socket_pack_4(0, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(1 * tiles.tile_edge_count) + 1] == wfc_socket_pack_4(0, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(1 * tiles.tile_edge_count) + 2] == wfc_socket_pack_4(0, 0, 0, 0));
  assert(tiles.tile_edge_sockets[(1 * tiles.tile_edge_count) + 3] == wfc_socket_pack_4(0, 1, 0, 0));

  /* Check first rotated tile 1 sockets
     " # "
     " ##"
     " # "
   */
  assert(tiles.tile_edge_sockets[(2 * tiles.tile_edge_count) + 0] == wfc_socket_pack_4(0, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(2 * tiles.tile_edge_count) + 1] == wfc_socket_pack_4(0, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(2 * tiles.tile_edge_count) + 2] == wfc_socket_pack_4(0, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(2 * tiles.tile_edge_count) + 3] == wfc_socket_pack_4(0, 0, 0, 0));

  /* Check second rotated tile 1 sockets
     "   "
     "###"
     " # "
   */
  assert(tiles.tile_edge_sockets[(3 * tiles.tile_edge_count) + 0] == wfc_socket_pack_4(0, 0, 0, 0));
  assert(tiles.tile_edge_sockets[(3 * tiles.tile_edge_count) + 1] == wfc_socket_pack_4(0, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(3 * tiles.tile_edge_count) + 2] == wfc_socket_pack_4(0, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(3 * tiles.tile_edge_count) + 3] == wfc_socket_pack_4(0, 1, 0, 0));

  /* Check third rotated tile 1 sockets
     " # "
     "## "
     " # "
   */
  assert(tiles.tile_edge_sockets[(4 * tiles.tile_edge_count) + 0] == wfc_socket_pack_4(0, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(4 * tiles.tile_edge_count) + 1] == wfc_socket_pack_4(0, 0, 0, 0));
  assert(tiles.tile_edge_sockets[(4 * tiles.tile_edge_count) + 2] == wfc_socket_pack_4(0, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(4 * tiles.tile_edge_count) + 3] == wfc_socket_pack_4(0, 1, 0, 0));

  free(tiles_memory);
}

static void wfc_test_tile_rotation_uneven_sockets(void)
{
  unsigned char *tiles_memory;
  unsigned int tiles_memory_size = 0;
  wfc_socket_8x07 socket_buffer[4];

  wfc_tiles tiles = {0};
  tiles.tile_capacity = 5;          /* 5 tiles */
  tiles.tile_edge_count = 4;        /* 4 edges */
  tiles.tile_edge_socket_count = 3; /* 3 values per edge */

  tiles_memory_size = WFC_TILES_MEMORY_SIZE(tiles.tile_capacity, tiles.tile_edge_count);
  tiles_memory = malloc(tiles_memory_size);

  assert(wfc_tiles_initialize(&tiles, tiles_memory, tiles_memory_size));

  /* Setup tile sockets */

  /* Tile 0 (empty, no rotation required):
     "   "
     "   "
     "   "
  */
  socket_buffer[0] = wfc_socket_pack_4(0, 0, 0, 0); /* Top    */
  socket_buffer[1] = wfc_socket_pack_4(0, 0, 0, 0); /* Right  */
  socket_buffer[2] = wfc_socket_pack_4(0, 0, 0, 0); /* Bottom */
  socket_buffer[3] = wfc_socket_pack_4(0, 0, 0, 0); /* Left   */

  /* Add tile without additional rotations */
  wfc_tiles_add_tile(&tiles, 0, socket_buffer, 0);

  /* Tile 1 (uneven, rotate 3 times for each edge):
     "## "
     "## "
     "  #"
  */
  socket_buffer[0] = wfc_socket_pack_4(1, 1, 0, 0); /* Top    */
  socket_buffer[1] = wfc_socket_pack_4(0, 0, 1, 0); /* Right  */
  socket_buffer[2] = wfc_socket_pack_4(1, 0, 0, 0); /* Bottom */
  socket_buffer[3] = wfc_socket_pack_4(0, 1, 1, 0); /* Left   */

  /* Add tile with three rotations */
  wfc_tiles_add_tile(&tiles, 1, socket_buffer, 3);

  assert(tiles.tile_count == 5);
  assert(tiles.tile_ids[0] == 0);
  assert(tiles.tile_ids[1] == 1);
  assert(tiles.tile_ids[2] == 1);
  assert(tiles.tile_ids[3] == 1);
  assert(tiles.tile_ids[4] == 1);
  assert(tiles.tile_rotations[0] == 0);
  assert(tiles.tile_rotations[1] == 0);
  assert(tiles.tile_rotations[2] == 1);
  assert(tiles.tile_rotations[3] == 2);
  assert(tiles.tile_rotations[4] == 3);

  /* Check original tile 1 sockets
     "## "
     "## "
     "  #"
  */
  assert(tiles.tile_edge_sockets[(1 * tiles.tile_edge_count) + 0] == wfc_socket_pack_4(1, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(1 * tiles.tile_edge_count) + 1] == wfc_socket_pack_4(0, 0, 1, 0));
  assert(tiles.tile_edge_sockets[(1 * tiles.tile_edge_count) + 2] == wfc_socket_pack_4(1, 0, 0, 0));
  assert(tiles.tile_edge_sockets[(1 * tiles.tile_edge_count) + 3] == wfc_socket_pack_4(0, 1, 1, 0));

  /* Check first rotated tile 1 sockets
     " ##"
     " ##"
     "#  "
   */
  assert(tiles.tile_edge_sockets[(2 * tiles.tile_edge_count) + 0] == wfc_socket_pack_4(0, 1, 1, 0));
  assert(tiles.tile_edge_sockets[(2 * tiles.tile_edge_count) + 1] == wfc_socket_pack_4(1, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(2 * tiles.tile_edge_count) + 2] == wfc_socket_pack_4(0, 0, 1, 0));
  assert(tiles.tile_edge_sockets[(2 * tiles.tile_edge_count) + 3] == wfc_socket_pack_4(1, 0, 0, 0));

  /* Check second rotated tile 1 sockets
     "#  "
     " ##"
     " ##"
   */
  assert(tiles.tile_edge_sockets[(3 * tiles.tile_edge_count) + 0] == wfc_socket_pack_4(1, 0, 0, 0));
  assert(tiles.tile_edge_sockets[(3 * tiles.tile_edge_count) + 1] == wfc_socket_pack_4(0, 1, 1, 0));
  assert(tiles.tile_edge_sockets[(3 * tiles.tile_edge_count) + 2] == wfc_socket_pack_4(1, 1, 0, 0));
  assert(tiles.tile_edge_sockets[(3 * tiles.tile_edge_count) + 3] == wfc_socket_pack_4(0, 0, 1, 0));

  /* Check third rotated tile 1 sockets
     "  #"
     "## "
     "## "
   */
  assert(tiles.tile_edge_sockets[(4 * tiles.tile_edge_count) + 0] == wfc_socket_pack_4(0, 0, 1, 0));
  assert(tiles.tile_edge_sockets[(4 * tiles.tile_edge_count) + 1] == wfc_socket_pack_4(1, 0, 0, 0));
  assert(tiles.tile_edge_sockets[(4 * tiles.tile_edge_count) + 2] == wfc_socket_pack_4(0, 1, 1, 0));
  assert(tiles.tile_edge_sockets[(4 * tiles.tile_edge_count) + 3] == wfc_socket_pack_4(1, 1, 0, 0));

  free(tiles_memory);
}

static void wfc_test_simple_tiles(void)
{
  unsigned char *tiles_memory;
  unsigned int tiles_memory_size = 0;

  wfc_tiles tiles = {0};
  tiles.tile_capacity = 5;          /* 5 tiles */
  tiles.tile_edge_count = 4;        /* 4 edges */
  tiles.tile_edge_socket_count = 3; /* 3 values per edge */

  tiles_memory_size = WFC_TILES_MEMORY_SIZE(tiles.tile_capacity, tiles.tile_edge_count);
  tiles_memory = malloc(tiles_memory_size);

  assert(wfc_tiles_initialize(&tiles, tiles_memory, tiles_memory_size));

  /* Setup tile sockets */
  {
    wfc_socket_8x07 socket_buffer[4];

    /* Tile 0 (empty, no rotation required):
       "   "
       "   "
       "   "
    */
    socket_buffer[0] = wfc_socket_pack_4(0, 0, 0, 0); /* Top    */
    socket_buffer[1] = wfc_socket_pack_4(0, 0, 0, 0); /* Right  */
    socket_buffer[2] = wfc_socket_pack_4(0, 0, 0, 0); /* Bottom */
    socket_buffer[3] = wfc_socket_pack_4(0, 0, 0, 0); /* Left   */

    /* Add tile without additional rotations */
    wfc_tiles_add_tile(&tiles, 0, socket_buffer, 0);

    /* Tile 1 (cross, rotate 3 times for each edge):
       " # "
       "###"
       "   "
    */
    socket_buffer[0] = wfc_socket_pack_4(0, 1, 0, 0); /* Top    */
    socket_buffer[1] = wfc_socket_pack_4(0, 1, 0, 0); /* Right  */
    socket_buffer[2] = wfc_socket_pack_4(0, 0, 0, 0); /* Bottom */
    socket_buffer[3] = wfc_socket_pack_4(0, 1, 0, 0); /* Left   */

    /* Add tile with three rotations */
    wfc_tiles_add_tile(&tiles, 1, socket_buffer, 3);

    assert(tiles.tile_count == 5);
  }

  {
    unsigned int retries = 0;
    unsigned char *grid_memory;
    unsigned int grid_memory_size = 0;

    wfc_grid grid = {0};
    grid.cols = 16;
    grid.rows = 16;

    grid_memory_size = WFC_GRID_MEMORY_SIZE(grid.rows, grid.cols, tiles.tile_count);

    printf("[wfc] tiles_memory_size (mb): %10.6f\n", (double)tiles_memory_size / 1024.0 / 1024.0);
    printf("[wfc]  grid_memory_size (mb): %10.6f\n", (double)grid_memory_size / 1024.0 / 1024.0);
    printf("[wfc]        total_size (mb): %10.6f\n", ((double)tiles_memory_size / 1024.0 / 1024.0) + ((double)grid_memory_size / 1024.0 / 1024.0));

    grid_memory = malloc(grid_memory_size);

    assert(wfc_grid_initialize(&grid, &tiles, grid_memory, grid_memory_size));
    assert(grid.cell_collapsed[0] == 0);
    assert(grid.cell_entropy_count[0] == tiles.tile_count);
    assert(grid.cell_collapsed[grid.rows * grid.cols - 1] == 0);
    assert(grid.cell_entropy_count[grid.rows * grid.cols - 1] == tiles.tile_count);

    /* Run WFC */
    wfc_seed_lcg = 42;

    while (!wfc(&grid, &tiles))
    {
      wfc_grid_initialize(&grid, &tiles, grid_memory, grid_memory_size);
      wfc_seed_lcg += 1; /* increment seed if WFC fails */
      retries++;
    }

    printf("[wfc] solved grid after %d retries\n", retries);

    /* Export WFC to PPM */
    {
      /* These represent the tiles for visualization purposes */
      char *tile_0 =
          "   " /*   */
          "   " /*   */
          "   " /*   */;

      char *tile_1 =
          " # " /*   */
          "###" /*   */
          "   " /*   */;

      char *tile_2 =
          " # " /*   */
          " ##" /*   */
          " # " /*   */;

      char *tile_3 =
          "   " /*   */
          "###" /*   */
          " # " /*   */;

      char *tile_4 =
          " # " /*   */
          "## " /*   */
          " # " /*   */;

      char *tile_chars[5];
      tile_chars[0] = tile_0;
      tile_chars[1] = tile_1;
      tile_chars[2] = tile_2;
      tile_chars[3] = tile_3;
      tile_chars[4] = tile_4;

      wfc_export_ppm(&grid, &tiles, tile_chars, (int)(sizeof(tile_chars) / sizeof(tile_chars[0])), "wfc.ppm", 8, 1, 0, 3, 3);
    }
  }
}

int main(void)
{
  wfc_test_socket();
  wfc_test_tile_stack_alloc();
  wfc_test_tile_rotation_even_sockets();
  wfc_test_tile_rotation_uneven_sockets();
  wfc_test_simple_tiles();

  return 0;
}

/*
   -----------------------------------------------------------------------------
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
