#pragma once

#include <fstream>
#include <string>

#include "mesh.hpp"


class MeshWriter {

public:

    enum class AttributeWriteScheme {
        INTERLEAVED,     // faster to read directly into vertex buffers
        NON_INTERLEAVED  // faster to read into mesh objects
    };

    explicit MeshWriter(const std::string& fileName);

    void writeMesh(const Mesh& mesh, AttributeWriteScheme scheme = AttributeWriteScheme::INTERLEAVED);

private:

    std::ofstream _fs;

};


class MeshReader {

public:

    explicit MeshReader(const std::string& fileName);

    Mesh readMesh();

private:

    std::ifstream _fs;

};