#pragma once

#include <vector>
#include "Vertex.h"

std::vector<Vertex> getQuadVertices()
{
    return {
        Vertex({-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, glm::vec3(0.0f, 0.0f, 0.0f)),
        Vertex({ 0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, glm::vec3(0.0f, 0.0f, 0.0f)),
        Vertex({-0.5f,  0.5f,  0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, glm::vec3(0.0f, 0.0f, 0.0f)),
        Vertex({ 0.5f, -0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, glm::vec3(0.0f, 0.0f, 0.0f)),
    };
}

std::vector<uint32_t> getQuadIndices()
{
    return {
        1, 0, 2, 3, 0, 1
    };
}