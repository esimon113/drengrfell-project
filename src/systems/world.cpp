#include "world.h"

constexpr float EAGLE_ASPECT_RATIO = 870.f/1000.f; //< Aspect ratio of the eagle texture
constexpr float EAGLE_IN_GAME_WIDTH = 0.2f; //< In-Game width of the eagles
constexpr float EAGLE_DELAY = 6.f; //< Max delay between eagle spawns in seconds

constexpr float BUG_ASPECT_RATIO = 199.f/200.f; //< Aspect ratio of the bug texture
constexpr float BUG_IN_GAME_WIDTH = 0.1f; //< In-Game width of the bugs
constexpr float BUG_DELAY = 15.f; //< Max delay between bug spawns in seconds

void createEgg(Registry* registry, const glm::vec2 position, const float radius, const float brightness) noexcept;
void createEagle(Registry* registry, const glm::vec2 position) noexcept;
void createBug(Registry* registry, const glm::vec2 position) noexcept;

WorldSystem WorldSystem::init(Window* window, Registry *registry, AudioSystem *audio_engine) noexcept {
	WorldSystem self;

	self.m_window = window;
	self.m_registry = registry;
	self.m_audio_engine = audio_engine;

	self.m_random_engine = std::default_random_engine(std::random_device()());

	self.m_reset = true;

	return self;
}

void WorldSystem::deinit() noexcept {}

void WorldSystem::reset() noexcept {
	m_score = 0;
	m_eagle_timer = 1.f;
	m_bug_timer = 1.f;
	m_reset = false;
}

void WorldSystem::step(const float delta) noexcept {
	std::string title = fmt::format("Score: {} - FPS: {:.2f} ({:.2f} ms)", m_score, 1/delta, 1000 * delta);
	m_window->setTitle(title.c_str());

	// remove entites that leave the screen on the bottom side
	for (Entity e : m_registry->m_velocities.entities) {
		glm::vec2 position = m_registry->m_positions.get(e);
		glm::vec2 scale = glm::vec2(0);
		if (m_registry->m_scales.has(e))
			scale = m_registry->m_scales.get(e);
		else if (m_registry->m_collision_radius.has(e))
			scale = glm::vec2(m_registry->m_collision_radius.get(e));
		if (!m_registry->m_players.has(e) && position.y + fabs(scale.x/2) < -1) {
			m_registry->clear(e);
		}
	}

	if (m_registry->m_eagles.entities.size() <= MAX_EAGLES
			&& (m_eagle_timer -= delta) <= 0) {
		createEagle(m_registry, {1.f - m_uniform_distribution(m_random_engine), 1.99f});
		m_eagle_timer = (EAGLE_DELAY/2) + m_uniform_distribution(m_random_engine) * (EAGLE_DELAY/2);
	}

	if (m_registry->m_bugs.entities.size() <= MAX_BUGS
			&& (m_bug_timer -= delta) <= 0) {
		// TODO: (A2) Create a new bug using `createBug(m_registry, {0.5, 1})`.
		m_bug_timer = BUG_DELAY;
	}

	if (m_registry->m_death_timers.has(m_registry->player())) {
		float& timer = m_registry->m_death_timers.get(m_registry->player());
		timer -= delta;
		if (timer < 0) {
			m_reset = true;
		}
	}

	// TODO: (A2) Update the `LightUp` timer and remove it if it drops below 0.
	//            See the death timer above for an example. 
}

void WorldSystem::onKeyCallback(GLFWwindow* /* window */, int key, int /* scancode */, int action, int /* mods */) noexcept {
	switch (action) {
		case GLFW_PRESS:
			switch (key) {
				case GLFW_KEY_R: // pressing the 'r' key triggers a reset of the game
					m_reset = true;
					break;
				// TODO: (A2) Handle player movement here
				default:
					break;
			}
			break;
		case GLFW_RELEASE:
			{} break;
		case GLFW_REPEAT:
		default:
			break;
	}
}

void createEgg(Registry* registry, const glm::vec2 position, const float radius, const float brightness) noexcept {
	Entity egg;

	fmt::println("Created Egg @ ({}, {})", position.x, position.y);

	registry->m_positions.emplace(egg, position);
	registry->m_velocities.emplace(egg, glm::vec2(0.f));
	registry->m_collision_radius.emplace(egg, radius);
	registry->m_colors.emplace(egg, glm::vec3(brightness));

	registry->m_eggs.emplace(egg);
}

void createEagle(Registry* registry, const glm::vec2 position) noexcept {
	Entity eagle;

	fmt::println("Created Eagle @ ({}, {})", position.x, position.y);

	registry->m_positions.emplace(eagle, position);
	registry->m_velocities.emplace(eagle, 0.f, -0.25f);
	registry->m_scales.emplace(eagle, -EAGLE_IN_GAME_WIDTH, EAGLE_IN_GAME_WIDTH * EAGLE_ASPECT_RATIO);
	registry->m_angles.emplace(eagle, 0.f);
	registry->m_collision_radius.emplace(eagle, EAGLE_IN_GAME_WIDTH);

	registry->m_eagles.emplace(eagle);
}

void createBug(Registry* registry, const glm::vec2 position) noexcept {
	Entity bug;

	fmt::println("Created Bug @ ({}, {})", position.x, position.y);

	registry->m_positions.emplace(bug, position);
	registry->m_velocities.emplace(bug, 0.f, -0.125f);
	registry->m_scales.emplace(bug, -BUG_IN_GAME_WIDTH, BUG_IN_GAME_WIDTH * BUG_ASPECT_RATIO);
	registry->m_angles.emplace(bug, 0.f);
	registry->m_collision_radius.emplace(bug, BUG_IN_GAME_WIDTH);

	registry->m_bugs.emplace(bug);
}
