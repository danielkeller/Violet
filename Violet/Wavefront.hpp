#include "Rendering/VAO.hpp"
#include "Rendering/Shader.hpp"
#include <tuple>

std::tuple<VAO, ShaderProgram> LoadWavefront(std::string filename);