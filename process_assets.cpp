/**
 * Code referenced:
 * https://github.com/lassyla/game1/blob/master/pack_tiles.cpp, 
 * https://github.com/xinyis991105/15-466-f20-base1/blob/master/asset_generation.cpp, 
 * https://github.com/15-466/15-466-f19-base1/blob/master/pack-sprites.cpp
 * My asset processing was inspired by the method used in the above examples.
 * I will load the sprite pngs, divide them into 8x8 blocks, get the palette, and format the tile, for each sprite.
 * I have designed the sprites to have no more than 4 colors, and the image dimensions are divisible by 8.
 * The levels will be processed from a 32x30 png labeled as a level. Different colored pixels represent different tiles.
 * This makes it easier to design a level.
*/

// places sprite info and level info into binary file
#include "load_save_png.hpp"
#include "read_write_chunk.hpp"
#include "PPU466.hpp"
#include "data_path.hpp"
#include "Level.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <fstream>
#include <cstring>

int main(int argc, char** argv) {
    std::cout << "processing assets...\n";

    const uint32_t num_levels = 4;
    const uint32_t num_sprites = 5;
    std::string tile_files[num_sprites] = { "../tiles/Boxes.png", "../tiles/Cats.png", "../tiles/GreenBlocks.png", "../tiles/Ladder.png", "../tiles/Spikes.png"};
    std::string level_files = "../levels/";
    //png sizes
    glm::uvec2 sizes[5] = { glm::uvec2(48,16), glm::uvec2(16,32), glm::uvec2(16,32), glm::uvec2(16,16), glm::uvec2(48,16) };

    std::vector<PPU466::Palette> palette_table;
    std::vector<PPU466::Tile> tile_table;
    std::vector<int> tile_to_palette_map;
    std::vector<Level> levels;
    

    //loop through all sprite files
    for (uint32_t i = 0; i < num_sprites; ++i) {

        std::cout << "loading " << tile_files[i] << "\n";
        std::vector<glm::u8vec4> data;
        load_png(data_path(tile_files[i]), &sizes[i], &data, UpperLeftOrigin);

        assert(sizes[i].x % 8 == 0);
        assert((sizes[i].y) % 8 == 0);
        assert(sizes[i].x * sizes[i].y <= data.size());

        //loop through tiles
        PPU466::Palette palette;
        PPU466::Tile tile;
        for (uint32_t tileX = 0; tileX < sizes[i].x; tileX+=8) {
            for (uint32_t tileY = 0; tileY < sizes[i].y; tileY+=8) {

                std::cout << "getting palette of (" << tileX << ", " << tileY << ")" << "\n";
                //get palette of tile
                memset(&palette, 0, sizeof(glm::u8vec4) * 4);
                uint32_t num_colors = 0;
                for (uint8_t pixelY = 0; pixelY < 8; ++pixelY) {
                    for (uint8_t pixelX = 0; pixelX < 8; ++pixelX) {
                        glm::u8vec4 curr_color = data[(tileX + pixelX) + sizes[i].x * (tileY + 8 - pixelY)];
                        if (curr_color[3] != 0xff || (curr_color[0] == 0xff && curr_color[1] == 0xff && curr_color[2] == 0xff)) continue;
                        bool found = false;
                        for (uint8_t col = 0; col < 4; ++col) {
                            if (curr_color == palette[col]) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            palette[num_colors] = curr_color;
                            std::cout << "adding color (" << (int)curr_color[0] << ", " << (int)curr_color[1] << ", " << (int)curr_color[2] << ", " << (int)curr_color[3] << ")\n";
                            ++num_colors;
                        }
                    }
                }

                std::cout << "got " << num_colors << " colors. \n";
                assert(num_colors <= 4);
                
                //check if palette exists in table
                int palette_index = -1;
                for (uint32_t p = 0; p < palette_table.size(); ++p) {
                    PPU466::Palette compare_palette = palette_table[p];
                    bool all_found = true;
                    for (uint32_t a = 0; a < num_colors; ++a) {
                        glm::u8vec4 looking_for_color = palette[a];
                        bool found = false;
                        for (uint32_t b = 0; b < 4; ++b) {
                            if (looking_for_color == compare_palette[b]) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            all_found = false;
                            break;
                        }
                    }
                    if (all_found) {
                        palette_index = p;
                        break;
                    }
                }
                if (palette_index == -1) {
                    palette_index = (int)palette_table.size();
                    palette_table.push_back(palette);
                }
                
                assert(palette_index != -1);
                tile_to_palette_map.push_back(palette_index); //since each tile gets pushed, palette is same index

                //get tile information
                palette = palette_table[palette_index];
                memset(&tile.bit0, 0, 8);
                memset(&tile.bit1, 0, 8);
                for (uint32_t pixelY = 0; pixelY < 8; ++pixelY) {
                    for (uint32_t pixelX = 0; pixelX < 8; ++pixelX) {
                        for (uint32_t p = 0; p < 4; p++) {
                            if (data[(tileX + pixelX) + sizes[i].x * (tileY + 8 + pixelY)] == palette[p]) {
                                tile.bit0[pixelY] = tile.bit0[pixelY] | ((p & 1) << pixelX);
                                tile.bit1[pixelY] = tile.bit1[pixelY] | (((p & 2) << (pixelX)) >> 1);
                            }
                        }
                    }
                }
                tile_table.push_back(tile);
            }
        }
    }

    glm::uvec2 level_size = glm::uvec2(16, 15); //based on 16x16 sprites
    //load all level pngs
    for (int i = 1; i <= num_levels; ++i) {
        Level level;
        std::vector<glm::u8vec4> data;

        std::cout << "loading " << (level_files + std::to_string(i) + ".png") << "\n";
        load_png(data_path(level_files + std::to_string(i) + ".png"), &level_size, &data, LowerLeftOrigin);

        for (uint32_t x = 0; x < 16; ++x) {
            for (uint32_t y = 0; y < 15; ++y) {
                glm::u8vec4 color = data[x + y * 16];
                if (color[3] == 0x00) continue;
                if (color[0] == 0x00 && color[1] == 0x00 && color[2] == 0x00) { //black
                    level.starting_pos = glm::ivec2(x*2, y*2);
                }
                else if (color[0] == 0x00 && color[1] == 0xff && color[2] == 0x00) { //green
                    level.topblocks[x][y] = true;
                }
                else if (color[0] == 0xff && color[1] == 0xff && color[2] == 0x00) { //yellow
                    level.blocks[x][y] = true;
                }
                else if (color[0] == 0x00 && color[1] == 0x00 && color[2] == 0xff) { //blue
                    level.ladders[x][y] = true;
                }
                else if (color[0] == 0xff && color[1] == 0x00 && color[2] == 0x00) { //red
                    level.hazards[x][y] = true;
                }
                else if (color[0] == 0xff && color[1] == 0x00 && color[2] == 0xff) { //purple
                    level.boxes[x][y] = true;
                }
            }
        }
        levels.push_back(level);
    }

    std::ofstream out(data_path("../tiles.bin"), std::ios::binary);
    write_chunk("tile", tile_table, &out);
    write_chunk("pale", palette_table, &out);
    write_chunk("tmap", tile_to_palette_map, &out);
    write_chunk("lvls", levels, &out);

    std::cout << "done! created " << tile_table.size() << " tiles, " << palette_table.size() << " palettes, and " << levels.size() << " levels.";

    return 0;
}