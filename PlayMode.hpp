#include "PPU466.hpp"
#include "Mode.hpp"
#include "Level.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//animation timer:
	bool animate = false;
	float animate_timer = 0.0f;

	//player position:
	glm::vec2 player_at = glm::vec2(0.0f);
	glm::vec2 player_velocity = glm::vec2(0.0f);
	float gravity = -5.0f;
	//bool climb = false;
	bool grounded = true; //also use this for the ladder

	//boxes
	std::vector<glm::vec2> box_positions;
	std::vector<glm::vec2> box_velocities;
	
	//levels
	std::vector<Level> levels;
	int level_index = 1;
	Level level; //track current level

	//----- drawing handled by PPU466 -----
	std::vector<int> tile_to_palette_map;
	PPU466 ppu;
};
