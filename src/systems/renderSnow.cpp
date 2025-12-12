#include "renderSnow.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include "../core/camera.h"

namespace df {

// The VBO containing the 4 vertices of the particles (billboard quad)
static const GLfloat g_vertex_buffer_data[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
    -0.5f,  0.5f, 0.0f,
     0.5f,  0.5f, 0.0f,
};

RenderSnowSystem RenderSnowSystem::init(Window* window, Registry* registry) noexcept {
    RenderSnowSystem self;
    
    self.window = window;
    self.registry = registry;
    
    self.maxParticles = 10000;
    self.particlesCount = 0;
    
    self.particlesContainer.resize(self.maxParticles);
    self.g_particule_position_size_data.resize(self.maxParticles * 4);
    self.g_particule_color_data.resize(self.maxParticles * 4);
    
    // Load shader - make sure particle.vert and particle.frag exist in your shaders folder
    self.particleShader = Shader::init(assets::Shader::particle).value();
    
    self.initBuffers();
    
    return self;
}

void RenderSnowSystem::initBuffers() noexcept {
    // Create VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // The VBO containing the 4 vertices of the particles (shared by all particles)
    glGenBuffers(1, &billboard_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    
    // The VBO containing the positions and sizes of the particles
    glGenBuffers(1, &particles_position_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    // Initialize with empty (NULL) buffer : it will be updated later, each frame.
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    
    // The VBO containing the colors of the particles
    glGenBuffers(1, &particles_color_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    // Initialize with empty (NULL) buffer : it will be updated later, each frame.
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
    
    glBindVertexArray(0);
}

void RenderSnowSystem::deinit() noexcept {
    glDeleteBuffers(1, &billboard_vertex_buffer);
    glDeleteBuffers(1, &particles_position_buffer);
    glDeleteBuffers(1, &particles_color_buffer);
    glDeleteVertexArrays(1, &vao);
    
    particleShader.deinit();
}

int RenderSnowSystem::findUnusedParticle() noexcept {
    static int lastUsedParticle = 0;
    for(int i = lastUsedParticle; i < maxParticles; i++){
        if (particlesContainer[i].life < 0){
            lastUsedParticle = i;
            return i;
        }
    }
    
    for(int i = 0; i < lastUsedParticle; i++){
        if (particlesContainer[i].life < 0){
            lastUsedParticle = i;
            return i;
        }
    }
    
    return 0; // All particles are taken, override the first one
}

void RenderSnowSystem::sortParticles() noexcept {
    std::sort(&particlesContainer[0], &particlesContainer[maxParticles]);
}

void RenderSnowSystem::step(float deltaTime) noexcept {
    // Get camera position from registry
    Camera& cam = registry->cameras.get(registry->getCamera());
    glm::vec3 cameraPos = glm::vec3(cam.position.x, cam.position.y, 0.0f);
    
    // Generate new particles (snow spawning)
    int newparticles = (int)(deltaTime * 10000.0); // Adjust spawn rate
    if (newparticles > (int)(0.016f * 10000.0))
        newparticles = (int)(0.016f * 10000.0);
    
    for(int i = 0; i < newparticles; i++){
        int particleIndex = findUnusedParticle();
        Particle& p = particlesContainer[particleIndex];
        
        // Spawn particles in a volume above the camera
        p.life = 5.0f; // Snow lives for 5 seconds
        p.pos = cameraPos + glm::vec3(
            (rand() % 2000 - 1000.0f) / 100.0f,  // x: -10 to 10
            10.0f,                                 // y: above camera
            (rand() % 2000 - 1000.0f) / 100.0f   // z: -10 to 10
        );
        
        // Snow falls down with slight horizontal drift
        p.speed = glm::vec3(
            (rand() % 200 - 100.0f) / 1000.0f,   // slight x drift
            -1.0f,                                 // fall down
            (rand() % 200 - 100.0f) / 1000.0f    // slight z drift
        );
        
        // White color with slight variation
        p.r = 255;
        p.g = 255;
        p.b = 255;
        p.a = (rand() % 256) / 3;
        
        p.size = (rand() % 1000) / 20000.0f + 0.01f;
    }
    
    // Simulate all particles
    particlesCount = 0;
    for(int i = 0; i < maxParticles; i++){
        Particle& p = particlesContainer[i];
        
        if(p.life > 0.0f){
            // Decrease life
            p.life -= deltaTime;
            if (p.life > 0.0f){
                // Simulate simple physics
                p.speed += glm::vec3(0.0f, -0.1f, 0.0f) * deltaTime * 0.5f;
                p.pos += p.speed * deltaTime;
                p.cameradistance = glm::length(p.pos - cameraPos);
                
                // Fill the GPU buffer
                g_particule_position_size_data[4*particlesCount+0] = p.pos.x;
                g_particule_position_size_data[4*particlesCount+1] = p.pos.y;
                g_particule_position_size_data[4*particlesCount+2] = p.pos.z;
                g_particule_position_size_data[4*particlesCount+3] = p.size;
                
                g_particule_color_data[4*particlesCount+0] = p.r;
                g_particule_color_data[4*particlesCount+1] = p.g;
                g_particule_color_data[4*particlesCount+2] = p.b;
                g_particule_color_data[4*particlesCount+3] = p.a;
            } else {
                // Particle is dead, put it at the end for sorting
                p.cameradistance = -1.0f;
            }
            
            particlesCount++;
        }
    }
    
    sortParticles();
}

void RenderSnowSystem::render(const glm::mat4& view, const glm::mat4& projection) noexcept {
    // Update the buffers that OpenGL uses for rendering
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data.data());
    
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particlesCount * sizeof(GLubyte) * 4, g_particule_color_data.data());
    
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use our shader
    particleShader.use();
    
    // Get camera vectors for billboarding
    glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
    
    // Set uniforms using your Shader class methods
    particleShader.setMat4("V", view);
    particleShader.setMat4("P", projection);
    particleShader.setVec3("CameraRight_worldspace", cameraRight);
    particleShader.setVec3("CameraUp_worldspace", cameraUp);
    
    glBindVertexArray(vao);
    
    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    // 2nd attribute buffer : positions of particles' centers
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    // 3rd attribute buffer : particles' colors
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);
    
    // These functions are specific to glDrawArrays*Instanced*.
    glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices
    glVertexAttribDivisor(1, 1); // positions : one per quad (its center)
    glVertexAttribDivisor(2, 1); // color : one per quad
    
    // Draw the particles!
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particlesCount);
    
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void RenderSnowSystem::reset() noexcept{

}

}