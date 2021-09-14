#pragma once

#include <glm/glm.hpp>

#include <vector>

struct Level {
	bool topblocks[16][15] = { 0 };
	bool blocks[16][15] = { 0 };
	bool ladders[16][15] = { 0 };
	bool hazards[16][15] = { 0 };
	bool boxes[16][15] = { 0 }; //their starting positions
	glm::vec2 starting_pos = glm::vec2(0.0f, 4.0f);
};