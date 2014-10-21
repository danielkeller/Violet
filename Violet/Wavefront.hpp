#include "Rendering/VAO.hpp"
#include "Rendering/Shader.hpp"
#include <tuple>

std::tuple<VAO, Mesh, ShaderProgram> LoadWavefront(std::string filename);