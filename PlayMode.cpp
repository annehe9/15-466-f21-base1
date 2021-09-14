#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

#include "data_path.hpp"
#include "read_write_chunk.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>

PlayMode::PlayMode() {

	std::vector<PPU466::Palette> palette_table;
	std::vector<PPU466::Tile> tile_table;
	std::vector<Level> levels;

	//read chunks from binary
	std::ifstream in(data_path("../tilebin"), std::ios::binary);
	read_chunk(in, "tile", &tile_table);
	read_chunk(in, "pale", &palette_table);
	read_chunk(in, "tmap", &tile_to_palette_map);
	read_chunk(in, "lvls", &levels);

	assert(tile_table.size() <= 256);
	assert(palette_table.size() <= 8);

	//a light blue
	ppu.background_color = glm::u8vec3(0xbc, 0xe7, 0xfd);
	
	//reset tables
	for (uint32_t i = 0; i < 16 * 16; i++) {
		ppu.tile_table[i].bit0.fill(0);
		ppu.tile_table[i].bit1.fill(0);
	}
	for (uint32_t i = 0; i < 8; i++) {
		ppu.palette_table[i].fill(glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	}

	//read tables
	for (uint32_t i = 0; i < tile_table.size(); ++i) {
		ppu.tile_table[i] = tile_table[i];
	}
	for (uint32_t i = 0; i < palette_table.size(); ++i) {
		ppu.palette_table[i] = palette_table[i];
	}

	//reset bg
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			ppu.background[x + PPU466::BackgroundWidth * y] = (8 << 8) | 256;
		}
	}

	//init level
	level_index = 0;
	level = levels[level_index];
	player_at = level.starting_pos;
	for (uint32_t x = 0; x < 16; ++x) {
		for (uint32_t y = 0; y < 15; ++y) {
			if (level.boxes[x][y]) {
				box_positions.push_back(glm::vec2(x * 16, y * 16));
				box_velocities.push_back(glm::vec2(0.0f));
			}
		}
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_r) {
			level = levels[level_index];
			player_at = level.starting_pos;
			for (uint32_t x = 0; x < 16; ++x) {
				for (uint32_t y = 0; y < 15; ++y) {
					if (level.boxes[x][y]) {
						box_positions.push_back(glm::vec2(x * 16, y * 16));
						box_velocities.push_back(glm::vec2(0.0f));
					}
				}
			}
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	animate_timer += elapsed;
	if (animate_timer > 0.5f) {
		animate = !animate;
		animate_timer = 0.0f;
	}

	constexpr float PlayerSpeed = 30.0f;
	if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	if (up.pressed) player_at.y += PlayerSpeed * elapsed;
	//
	//if (up.pressed && grounded) player_velocity.y += 10;
	//player_at += player_velocity * elapsed;

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	//I would put the code here for character and scene collision and whatnot but I spent too much time trying to get the PPU to display my sprites and also other life things
	//but essentially I would check if object[character_x][character_y] +- some buffer, do something like make character grounded.
	/*
	if (!grounded) {
		player_velocity.y += gravity;
	}
	*/
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//fill
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			ppu.background[x + PPU466::BackgroundWidth * y] = (7 << 8) | 255;
		}
	}

	int hazardCount = 0;
	for (uint32_t x = 0; x < 16; ++x) {
		for (uint32_t y = 0; y < 15; ++y) {
			if (level.topblocks[x][y]) {
				for (uint8_t xCount = 0; xCount < 2; ++xCount) {
					for (uint8_t yCount = 0; yCount < 2; ++yCount) {
						uint8_t offset = xCount + yCount * 2;
						int xCoord = x * 16 + (xCount * 8);
						int yCoord = y * 16 + (yCount * 8);
						uint8_t what = offset + 8;
						ppu.background[xCoord + PPU466::BackgroundWidth * yCoord] = (tile_to_palette_map[8 + offset] << 8) | what;
					}
				}
			}
			else if (level.blocks[x][y]) {
				//std::cout << "a block exists at " << x << " and " << y << "\n";
				for (uint8_t xCount = 0; xCount < 2; ++xCount) {
					for (uint8_t yCount = 0; yCount < 2; ++yCount) {
						uint8_t offset = xCount + yCount * 2;
						int xCoord = x * 16 + (xCount * 8);
						int yCoord = y * 16 + (yCount * 8);
						uint8_t what = offset + 12;
						ppu.background[xCoord + PPU466::BackgroundWidth * yCoord] = (tile_to_palette_map[12 + offset] << 8) | what;
					}
				}
			}
			else if (level.ladders[x][y]) {
				for (uint8_t xCount = 0; xCount < 2; ++xCount) {
					for (uint8_t yCount = 0; yCount < 2; ++yCount) {
						uint8_t offset = xCount + yCount * 2;
						int xCoord = x * 16 + (xCount * 8);
						int yCoord = y * 16 + (yCount * 8);
						uint8_t what = offset + 28;
						//for some reason this doesn't work and I suspect it's also why the blocks are drawing vertically
						//ppu.background[xCoord + PPU466::BackgroundWidth * yCoord] = (tile_to_palette_map[28 + offset] << 8) | what;
					}
				}
			}
			else if (level.hazards[x][y]) {
				for (uint8_t xCount = 0; xCount < 2; ++xCount) {
					for (uint8_t yCount = 0; yCount < 2; ++yCount) {
						uint8_t offset = xCount + yCount * 2;
						int xCoord = x * 16 + (xCount * 8);
						int yCoord = y * 16 + (yCount * 8);
						uint8_t what = offset + 28 + ((hazardCount%3) * 4);
						ppu.background[xCoord + PPU466::BackgroundWidth * yCoord] = (tile_to_palette_map[32 + offset + ((hazardCount % 3) * 4)] << 8) | what;
					}
				}
				hazardCount++;
			}
		}
	}

	//player sprite:
	if (animate) {
		for (uint8_t xCount = 0; xCount < 2; ++xCount) {
			for (uint8_t yCount = 0; yCount < 2; ++yCount) {
				uint8_t ind = xCount + yCount * 2;
				ppu.sprites[ind].x = int32_t(player_at.x + xCount) * 8;
				ppu.sprites[ind].y = int32_t(player_at.y + yCount) * 8;
				ppu.sprites[ind].index = ind;
				ppu.sprites[ind].attributes = tile_to_palette_map[ind];

				ppu.sprites[ind+4].x = int32_t(0);
				ppu.sprites[ind+4].y = int32_t(241);
				ppu.sprites[ind+4].index = ind+4;
				ppu.sprites[ind + 4].attributes = tile_to_palette_map[ind + 4];
			}
		}
	}
	else {
		for (uint8_t xCount = 0; xCount < 2; ++xCount) {
			for (uint8_t yCount = 0; yCount < 2; ++yCount) {
				uint8_t ind = xCount + yCount * 2;
				ppu.sprites[ind].x = int32_t(0);
				ppu.sprites[ind].y = int32_t(241);
				ppu.sprites[ind].index = ind;
				ppu.sprites[ind].attributes = tile_to_palette_map[ind];

				ppu.sprites[ind + 4].x = int32_t(player_at.x + xCount) * 8;
				ppu.sprites[ind + 4].y = int32_t(player_at.y + yCount) * 8 ;
				ppu.sprites[ind + 4].index = ind + 4;
				ppu.sprites[ind + 4].attributes = tile_to_palette_map[ind + 4];
			}
		}
	}
	int sprite_index = 8;
	//draw boxes
	int box_index = 0;
	//std::cout << "drawing " << box_positions.size() << "boxes";
	for (uint32_t i = 0; i < box_positions.size(); ++i) {
		for (uint8_t xCount = 0; xCount < 2; ++xCount) {
			for (uint8_t yCount = 0; yCount < 2; ++yCount) {
				uint8_t offset = xCount + yCount * 2;
				ppu.sprites[sprite_index].x = (uint8_t)box_positions[i].x + (xCount * 8);
				ppu.sprites[sprite_index].y = (uint8_t)box_positions[i].y + (yCount * 8);
				ppu.sprites[sprite_index].index = (box_index % 3) * 4 + 16 + offset;
				ppu.sprites[sprite_index].attributes = tile_to_palette_map[(box_index % 3) * 4 + 16 + offset];
				box_index++;
			}
		}
	}

	//fill in rest
	while (sprite_index < 64) {
		ppu.sprites[sprite_index].y = 242;
		sprite_index++;
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
