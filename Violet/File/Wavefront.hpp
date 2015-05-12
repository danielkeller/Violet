#ifndef WAVEFRONT_HPP
#define WAVEFRONT_HPP

class VertexData;
class Mesh;

VertexData WavefrontVertexData(std::string filename);
Mesh WavefrontMesh(std::string filename);
bool IsWavefront(const std::string filename);

#endif