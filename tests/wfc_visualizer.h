#include "../wfc.h"

#include "stdlib.h"
#include "stdio.h"
#include "math.h"

static void draw_border(
    int *img, int img_w, int img_h,
    int x0, int y0, int w, int h,
    int thickness)
{
    int x, y, idx;

    for (y = y0; y < y0 + h; ++y)
    {
        for (x = x0; x < x0 + w; ++x)
        {
            if (y < 0 || y >= img_h || x < 0 || x >= img_w)
            {
                continue;
            }

            if (x < x0 + thickness || x >= x0 + w - thickness || y < y0 + thickness || y >= y0 + h - thickness)
            {
                idx = (y * img_w + x) * 3;
                img[idx + 0] = 255;
                img[idx + 1] = 0;
                img[idx + 2] = 0;
            }
        }
    }
}

static void draw_line(int *buffer, int img_w, int img_h,
                      int x0, int y0, int x1, int y1,
                      int r, int g, int b)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1)
    {
        if (x0 >= 0 && y0 >= 0 && x0 < img_w && y0 < img_h)
        {
            int idx = (y0 * img_w + x0) * 3;

            buffer[idx + 0] = r;
            buffer[idx + 1] = g;
            buffer[idx + 2] = b;
        }

        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

static void wfc_export_ppm(
    wfc_grid *grid, wfc_tiles *tiles,
    char **tile_chars,
    int tile_chars_size,
    char *filename,
    int scale,
    int highlight_tiles,
    int highlight_grid,
    int tile_w, int tile_h)
{
    int grid_img_w, grid_img_h;
    int img_w, img_h;
    int r, c;
    int *buffer;
    FILE *fp;

    if (scale < 1)
    {
        scale = 1;
    }

    grid_img_w = (int)grid->cols * tile_w * scale;
    grid_img_h = (int)grid->rows * tile_h * scale;

    img_w = grid_img_w;
    img_h = grid_img_h;

    buffer = (int *)malloc((size_t)(img_w * img_h * 3 * (int)sizeof(int)));

    if (!buffer)
    {
        return;
    }

    /* Render grid */
    for (r = 0; r < grid_img_h; ++r)
    {
        for (c = 0; c < grid_img_w; ++c)
        {
            int cell_col = c / (tile_w * scale);
            int cell_row = r / (tile_h * scale);
            int idx_grid, tile_id;
            char *art;
            int local_x, local_y;
            int idx_pix;

            if (cell_col >= (int)grid->cols || cell_row >= (int)grid->rows)
            {
                continue;
            }

            idx_grid = cell_row * (int)grid->cols + cell_col;
            local_x = (c % (tile_w * scale)) / scale;
            local_y = (r % (tile_h * scale)) / scale;

            idx_pix = (r * img_w + c) * 3;

            if (!grid->cell_collapsed[idx_grid])
            {
                buffer[idx_pix + 0] = 255;
                buffer[idx_pix + 1] = 0;
                buffer[idx_pix + 2] = 0;
            }
            else
            {
                tile_id = grid->cell_entropies[idx_grid * (int)tiles->tile_count + 0];

                art = tile_chars[tile_id];
                if (art[local_y * tile_w + local_x] == '#')
                {
                    /* Fake 3D elevation */
                    int cell_x = (c % (tile_w * scale));
                    int cell_y = (r % (tile_h * scale));

                    /* Base color for filled pixel */
                    buffer[idx_pix + 0] = 6;
                    buffer[idx_pix + 1] = 50;
                    buffer[idx_pix + 2] = 49;

                    /* Highlight top-left for light effect */
                    if (cell_x < 2 || cell_y < 2)
                    {
                        buffer[idx_pix + 0] += 20;

                        if (buffer[idx_pix + 0] > 255)
                        {
                            buffer[idx_pix + 0] = 255;
                        }

                        buffer[idx_pix + 1] += 20;

                        if (buffer[idx_pix + 1] > 255)
                        {
                            buffer[idx_pix + 1] = 255;
                        }

                        buffer[idx_pix + 2] += 20;

                        if (buffer[idx_pix + 2] > 255)
                        {
                            buffer[idx_pix + 2] = 255;
                        }
                    }

                    /* Shadow bottom-right */
                    if (cell_x > tile_w * scale - 3 || cell_y > tile_h * scale - 3)
                    {
                        buffer[idx_pix + 0] /= 2;
                        buffer[idx_pix + 1] /= 2;
                        buffer[idx_pix + 2] /= 2;
                    }
                }
                else
                {
                    buffer[idx_pix + 0] = 63;
                    buffer[idx_pix + 1] = 132;
                    buffer[idx_pix + 2] = 52;
                }
            }
        }
    }

    /* Highlight one random cell for each tile_id */
    if (highlight_tiles)
    {
        int t;

        for (t = 0; t < tile_chars_size; ++t)
        {
            int tries = 100; /* safety */

            while (tries--)
            {
                int idx = (int)(wfc_randi()) % (int)(grid->rows * grid->cols);

                if (grid->cell_collapsed[idx])
                {
                    int tile_id = grid->cell_entropies[idx * (int)tiles->tile_count + 0];
                    if (tile_id == t)
                    {
                        int x0 = (idx % (int)grid->cols) * tile_w * scale;
                        int y0 = (idx / (int)grid->cols) * tile_h * scale;

                        draw_border(buffer, img_w, img_h, x0, y0, tile_w * scale, tile_h * scale, 2);

                        break;
                    }
                }
            }
        }
    }

    /* After rendering tiles, add: */
    if (highlight_grid) /* draw grid lines */
    {
        int row, col;

        for (row = 0; row <= (int)grid->rows; ++row)
        {
            int y = row * tile_h * scale;

            if (y >= img_h)
            {
                continue;
            }

            draw_line(buffer, img_w, img_h, 0, y, img_w - 1, y, 6, 40, 39);
        }
        
        for (col = 0; col <= (int)grid->cols; ++col)
        {
            int x = col * tile_w * scale;

            if (x >= img_w)
            {
                continue;
            }

            draw_line(buffer, img_w, img_h, x, 0, x, img_h - 1, 6, 40, 39);
        }
    }

    /* Write PPM */
    fp = fopen(filename, "w");

    if (!fp)
    {
        free(buffer);
        return;
    }

    fprintf(fp, "P3\n%d %d\n255\n", img_w, img_h);

    for (r = 0; r < img_h; ++r)
    {
        for (c = 0; c < img_w; ++c)
        {
            int idx = (r * img_w + c) * 3;

            fprintf(fp, "%d %d %d ", buffer[idx], buffer[idx + 1], buffer[idx + 2]);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    free(buffer);

    printf("Exported grid with highlights to %s (%dx%d)\n", filename, img_w, img_h);
}
