#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

struct Face {
    std::vector<int> vertexIndices;
};

bool loadOBJ(const std::string& path, std::vector<glm::vec3>& out_vertices, std::vector<Face>& out_faces) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            // Vertex
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            out_vertices.push_back(vertex);
        }
        else if (prefix == "f") {
            // Face
            Face face;
            std::string vertexStr;
            while (iss >> vertexStr) {
                std::istringstream viss(vertexStr);
                int vertexIndex;
                char slash;
                
                // Leer el primer número (índice del vértice)
                viss >> vertexIndex;
                face.vertexIndices.push_back(vertexIndex - 1); // OBJ usa índices base 1
                
                // Ignorar texture/normal indices si existen
            }
            out_faces.push_back(face);
        }
    }

    file.close();
    std::cout << "Modelo cargado: " << out_vertices.size() << " vertices, " 
              << out_faces.size() << " faces" << std::endl;
    return true;
}