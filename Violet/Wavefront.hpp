#include "Rendering/VAO.hpp"
#include "Rendering/Shader.hpp"
#include <tuple>

std::tuple<VAO::Ref, ShaderProgram::Ref> LoadWavefront(std::string filename);