#include "Camera.h"

void Camera::pan(float dist_x, float dist_y) {
    m_front += m_right * dist_x * glm::length(m_position - m_front);
    m_position += m_right * dist_x * glm::length(m_position - m_front);

    m_front += m_up * dist_y * glm::length(m_position - m_front);
    m_position += m_up * dist_y * glm::length(m_position - m_front);
}

void Camera::orbit(float yaw, float pitch) {
    glm::vec3 direction;

    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = -sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    m_right.x = cos(glm::radians(yaw + 90.f));
    m_right.z = sin(glm::radians(yaw + 90.f));
    m_right = glm::normalize(m_right);

    m_position = m_front - glm::normalize(direction) * glm::length(m_position - m_front);
    m_up = glm::cross(m_right, glm::normalize(m_front - m_position));
}

void Camera::zoom(float dist) {
    m_position += (m_position - m_front) * dist;
    if (glm::length(m_position - m_front) > 100.f) {
        m_position = glm::normalize(m_position) * 100.f + m_front;
    }
}