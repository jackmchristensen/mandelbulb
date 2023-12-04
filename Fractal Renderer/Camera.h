#pragma once

#include <glm/glm.hpp>

class Camera
{
private:
    glm::vec3 m_position    = { 0.f, 0.f, 2.f };
    glm::vec3 m_front       = { 0.f, 0.f, 0.f };
    glm::vec3 m_up          = { 0.f, 1.f, 0.f };
    glm::vec3 m_right       = { 1.f, 0.f, 0.f };

public:
    // Since all variables are dependent on each other I've decided to 
    // disable the setters until I make sure they don't break things
    //    glm::vec3& position()         { return m_position; }
    //    glm::vec3& front()            { return m_front; }
    //    glm::vec3& up()               { return m_up; }
    //    glm::vec3& right()            { return m_right; }
    
    const glm::vec3& position() const   { return m_position; } 
    const glm::vec3& front() const      { return m_front; }         
    const glm::vec3& up() const         { return m_up; }        
    const glm::vec3& right() const      { return m_right; }
    
    void pan(float dist_x, float dist_y);
    void orbit(float yaw, float pitch);
    void zoom(float dist);
};