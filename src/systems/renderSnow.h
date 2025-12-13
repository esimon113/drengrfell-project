#pragma once

#include "renderCommon.h"
#include <registry.h>
#include <window.h>
#include <utils/shader.h>
#include <utils/framebuffer.h>
#include <glm/glm.hpp>
#include <vector>

namespace df {

class RenderSnowSystem {
public:
    // Static init function matching your project's pattern
    static RenderSnowSystem init(Window* window, Registry* registry) noexcept;
    
    // Main methods
    void step(float deltaTime) noexcept;
    void render(const glm::mat4& view, const glm::mat4& projection) noexcept;
    void deinit() noexcept;
    void reset();

private:
    Window* window;
    Registry* registry;
    
    // OpenGL buffer objects
    GLuint billboard_vertex_buffer;
    GLuint particles_position_buffer;
    GLuint particles_color_buffer;
    GLuint vao;
    
    // Shader
    Shader particleShader;
    
    // Particle settings
    int maxParticles;
    int particlesCount;
    
    // Particle data (CPU side)
    struct Particle {
        glm::vec3 pos;
        glm::vec3 speed;
        unsigned char r, g, b, a; // Color
        float size;
        float life; // Remaining life
        float cameradistance; // For sorting
        
        bool operator<(const Particle& that) const {
            // Sort in reverse order : far particles drawn first.
            return this->cameradistance > that.cameradistance;
        }
    };
    
    std::vector<Particle> particlesContainer;
    
    // CPU buffers that will be sent to GPU
    std::vector<GLfloat> g_particule_position_size_data;
    std::vector<GLubyte> g_particule_color_data;
    
    // Helper methods
    int findUnusedParticle() noexcept;
    void sortParticles() noexcept;
    void initBuffers() noexcept;
};

}