#include <mesh_io.hpp>

#include <cstring>
#include <iostream>
#include <stdexcept>


MeshWriter::MeshWriter(const std::string& fileName) :
        _fs(fileName, std::ios::out | std::ios::binary) {
    if (!_fs) {
        throw std::runtime_error("Cannot write file: " + fileName);
    }
}

MeshReader::MeshReader(const std::string& fileName) :
        _fs(fileName, std::ios::in | std::ios::binary) {
    if (!_fs) {
        throw std::runtime_error("Cannot read file: " + fileName);
    }
}

// methods for translating mesh attribute enums and strings
// these are temporary since I think the mesh system will switch to just using attribute string
// internally, for greater flexiblity in allowing the user to define new attributes

static std::string getAttributeName(MeshAttribute attrib) {
    switch (attrib) {
    case MeshAttribute::POSITION:
        return "position";
    case MeshAttribute::NORMAL:
        return "normal";
    case MeshAttribute::TEXCOORD:
        return "texCoord";
    case MeshAttribute::COLOR:
        return "color";
    case MeshAttribute::BONE_INDS:
        return "boneInds";
    case MeshAttribute::BONE_WEIGHTS:
        return "boneWeights";
    default:
        return "unknown";
    }
}

static MeshAttribute getAttributeFromName(const std::string& name) {
    // string comparison ordering of accepted tokens:
    // "boneInds"
    // "boneWeights"
    // "color"
    // "normal"
    // "position"
    // "texCoord"
    // do a sort of binary search for these strings
    int cmp = name.compare("normal");
    if (cmp == 0) return MeshAttribute::NORMAL;
    if (cmp < 0) {
        cmp = name.compare("boneWeights");
        if (cmp == 0) return MeshAttribute::BONE_WEIGHTS;
        if (cmp < 0) {
            cmp = name.compare("boneInds");
            if (cmp == 0) return MeshAttribute::BONE_INDS;
        } else {
            cmp = name.compare("color");
            if (cmp == 0) return MeshAttribute::COLOR;
        }
    } else {
        cmp = name.compare("texCoord");
        if (cmp == 0) return MeshAttribute::TEXCOORD;
        if (cmp < 0) {
            cmp = name.compare("position");
            if (cmp == 0) return MeshAttribute::POSITION;
        } else {

        }
    }
    throw std::runtime_error("No conversion defined for attribute name: " + name);
}

struct HeaderData {
    char fileID[8];
    uint8_t attribCount;
    uint64_t vertexCount;
    uint64_t indexCount;
};

struct AttribData {
    uint8_t componentType;
    uint8_t numComponents;
    uint64_t vertexBufferOffset;
    uint64_t vertexBufferStride;
};

static constexpr size_t HEADER_SIZE = 25;
static constexpr size_t ATTRIB_DATA_SIZE = 18;

static void packFileHeader(char* buffer, const HeaderData& header) {
    memcpy(buffer, header.fileID, sizeof(HeaderData::fileID));
    size_t offset = sizeof(HeaderData::fileID);
    memcpy(buffer + offset, &header.attribCount, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(buffer + offset, &header.vertexCount, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    memcpy(buffer + offset, &header.indexCount, sizeof(uint64_t));
}

static HeaderData unpackFileHeader(const char* buffer) {
    HeaderData header;
    memcpy(header.fileID, buffer, sizeof(HeaderData::fileID));
    size_t offset = sizeof(HeaderData::fileID);
    memcpy(&header.attribCount, buffer + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&header.vertexCount, buffer + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    memcpy(&header.indexCount, buffer + offset, sizeof(uint64_t));
    return header;
}

static void packAttribData(char* buffer, const AttribData& data) {
    memcpy(buffer, &data.componentType, sizeof(uint8_t));
    size_t offset = sizeof(uint8_t);
    memcpy(buffer + offset, &data.numComponents, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(buffer + offset, &data.vertexBufferOffset, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    memcpy(buffer + offset, &data.vertexBufferStride, sizeof(uint64_t));
}

static AttribData unpackAttribData(const char* buffer) {
    AttribData data;
    data.componentType = *(reinterpret_cast<const uint8_t*>(buffer));
    size_t offset = sizeof(uint8_t);
    data.numComponents = *(reinterpret_cast<const uint8_t*>(buffer + offset));
    offset += sizeof(uint8_t);
    data.vertexBufferOffset = *(reinterpret_cast<const uint64_t*>(buffer + offset));
    offset += sizeof(uint64_t);
    data.vertexBufferStride = *(reinterpret_cast<const uint64_t*>(buffer + offset));
    return data;
}

static void writeMeshAttributesInterleaved(std::ofstream& fs, const Mesh& mesh, const std::vector<std::string>& attribNames) {
    
    std::cout << "Writing attribute descriptions" << std::endl;

    // compute vertex size
    uint64_t vertexSize = 0;
    for (auto i = 0u; i < mesh.numAttributes(); ++i) {
        vertexSize += mesh.getAttributeBuffer(i).elementSize();
    }
    
    // write attrib descriptions
    uint64_t offset = 0;
    for (auto i = 0u; i < mesh.numAttributes(); ++i) {
        fs.write(attribNames[i].c_str(), attribNames[i].length() + 1);
        
        const auto& attribBuffer = mesh.getAttributeBuffer(i);

        char dataBuffer[ATTRIB_DATA_SIZE];
        AttribData data;
        data.componentType = static_cast<uint8_t>(attribBuffer.componentType());
        data.numComponents = attribBuffer.numComponents();
        data.vertexBufferOffset = offset;
        data.vertexBufferStride = vertexSize;
        packAttribData(dataBuffer, data);

        fs.write(dataBuffer, ATTRIB_DATA_SIZE);

        offset += attribBuffer.elementSize();
    }

    std::cout << "Writing vertex buffer" << std::endl;

    for (size_t i = 0u; i < mesh.numVertices(); ++i) {
        int progress = 100 * (float (i) / (float) mesh.numVertices());
        std::cout << "\rWriting vertex " << (i+1) << " / " << mesh.numVertices() << " [" << progress << "%]           " << std::flush;
        
        std::vector<char> vdata(vertexSize);

        offset = 0;
        for (auto j = 0u; j < mesh.numAttributes(); ++j) {
            const auto& attribBuffer = mesh.getAttributeBuffer(j);

            memcpy(&vdata[offset], attribBuffer.elementPtr(i), attribBuffer.elementSize());
            
            offset += attribBuffer.elementSize();
        }

        fs.write(vdata.data(), vertexSize);
    }

    std::cout << std::endl;
    std::cout << "Finished writing vertex buffer" << std::endl;
}

static void writeMeshAttributesNonInterleaved(std::ofstream& fs, const Mesh& mesh, const std::vector<std::string>& attribNames) {
    
    std::cout << "Writing attribute descriptions" << std::endl;
    
    uint64_t offset = 0;
    for (auto i = 0u; i < mesh.numAttributes(); ++i) {
        fs.write(attribNames[i].c_str(), attribNames[i].length() + 1);
        
        const auto& attribBuffer = mesh.getAttributeBuffer(i);

        char dataBuffer[ATTRIB_DATA_SIZE];
        AttribData data;
        data.componentType = static_cast<uint8_t>(attribBuffer.componentType());
        data.numComponents = attribBuffer.numComponents();
        data.vertexBufferOffset = offset;
        data.vertexBufferStride = attribBuffer.elementSize();

        fs.write(dataBuffer, ATTRIB_DATA_SIZE);

        offset += mesh.numVertices() * attribBuffer.elementSize();
    }

    std::cout << "Writing vertex buffer" << std::endl;

    for (auto i = 0u; i < mesh.numAttributes(); ++i) {
        int progress = 100 * ((float) i / (float) mesh.numAttributes());
        std::cout << "\rWriting attribute buffer " << (i+1) << " / " << mesh.numAttributes() << " [" << progress << "%]              " << std::flush;
        
        const auto& attribBuffer = mesh.getAttributeBuffer(i);
        
        fs.write(static_cast<const char*>(attribBuffer.data()), attribBuffer.elementSize() * mesh.numVertices());
    }

    std::cout << "Finished writing vertex buffer" << std::endl;
}



void MeshWriter::writeMesh(const Mesh& mesh, MeshWriter::AttributeWriteScheme scheme) {
    if (!_fs) {
        throw std::runtime_error("Write error.");
    }

    std::cout << "Writing mesh file..." << std::endl;

    std::cout << "Writing header" << std::endl;

    HeaderData header;
    strncpy(header.fileID, "meshfile", 8);
    header.attribCount = mesh.numAttributes();
    header.vertexCount = mesh.numVertices();
    header.indexCount = mesh.indices().size();

    // write header
    char headerBuffer[HEADER_SIZE];
    packFileHeader(headerBuffer, header);
    _fs.write(headerBuffer, HEADER_SIZE);

    // get name of all attributes in mesh
    std::vector<std::string> attribNames(mesh.numAttributes());
    
    size_t combinedNameLength = 0;
    for (auto i = 0u; i < mesh.numAttributes(); ++i) {
        attribNames[i] = getAttributeName(mesh.getAttributeBuffer(i).getAttribute());
    }
    

    std::cout << "Writing vertex attributes" << std::endl;

    // write attribute descriptions and vertex buffer based on scheme
    switch (scheme) {
    case AttributeWriteScheme::INTERLEAVED:
        writeMeshAttributesInterleaved(_fs, mesh, attribNames);
        break;
    case AttributeWriteScheme::NON_INTERLEAVED:
        writeMeshAttributesNonInterleaved(_fs, mesh, attribNames);
        break;
    default:
        throw std::runtime_error("Unimplemented attribute write scheme.");
    }

    std::cout << "Writing index buffer" << std::endl;

    // write index buffer
    if (mesh.hasIndices()) {
        _fs.write(reinterpret_cast<const char*>(mesh.indices().data()), mesh.indices().size() * sizeof(Mesh::index_t));
    }

    std::cout << "Finished writing mesh." << std::endl;
}

void createMeshAttributeBuffer(Mesh& mesh, MeshAttribute attribute, MeshAttributeComponentType componentType, uint8_t numComponents) {
    switch (numComponents) {
    case 1: {
        switch (componentType) {
        case MeshAttributeComponentType::FLOAT:
            mesh.createAttributeBuffer<float>(attribute);
            break;
        case MeshAttributeComponentType::INT:
            mesh.createAttributeBuffer<int>(attribute);
            break;
        case MeshAttributeComponentType::UINT:
            mesh.createAttributeBuffer<unsigned int>(attribute);
            break;
        default:
            throw std::runtime_error("Unknown attribute component type");
        }
        break;
    }
    case 2: {
        switch (componentType) {
        case MeshAttributeComponentType::FLOAT:
            mesh.createAttributeBuffer<vecmath::vector<float, 2>>(attribute);
            break;
        case MeshAttributeComponentType::INT:
            mesh.createAttributeBuffer<vecmath::vector<int, 2>>(attribute);
            break;
        case MeshAttributeComponentType::UINT:
            mesh.createAttributeBuffer<vecmath::vector<unsigned int, 2>>(attribute);
            break;
        default:
            throw std::runtime_error("Unknown attribute component type");
        }
        break;
    }
    case 3: {
        switch (componentType) {
        case MeshAttributeComponentType::FLOAT:
            mesh.createAttributeBuffer<vecmath::vector<float, 3>>(attribute);
            break;
        case MeshAttributeComponentType::INT:
            mesh.createAttributeBuffer<vecmath::vector<int, 3>>(attribute);
            break;
        case MeshAttributeComponentType::UINT:
            mesh.createAttributeBuffer<vecmath::vector<unsigned int, 3>>(attribute);
            break;
        default:
            throw std::runtime_error("Unknown attribute component type");
        }
        break;
    }
    case 4: {
        switch (componentType) {
        case MeshAttributeComponentType::FLOAT:
            mesh.createAttributeBuffer<vecmath::vector<float, 4>>(attribute);
            break;
        case MeshAttributeComponentType::INT:
            mesh.createAttributeBuffer<vecmath::vector<int, 4>>(attribute);
            break;
        case MeshAttributeComponentType::UINT:
            mesh.createAttributeBuffer<vecmath::vector<unsigned int, 4>>(attribute);
            break;
        default:
            throw std::runtime_error("Unknown attribute component type");
        }
        break;
    }
    default:
        throw std::runtime_error("Mesh attribute component count must be 1-4, given: " + std::to_string(numComponents));
    }
}

Mesh MeshReader::readMesh() {
    if (!_fs) {
        throw std::runtime_error("Read error.");
    }

    std::cout << "Reading mesh file..." << std::endl;

    std::cout << "Reading header" << std::endl;

    char headerBuffer[HEADER_SIZE];
    _fs.read(headerBuffer, HEADER_SIZE);
    HeaderData header = unpackFileHeader(headerBuffer);

    std::cout << "Header data:" << std::endl;

    std::cout << "\tFile ID: " << std::string(header.fileID, 8) << std::endl;
    std::cout << "\tAttribute count: " << (int) header.attribCount << std::endl;
    std::cout << "\tVertex count: " << header.vertexCount << std::endl;
    std::cout << "\tIndex count: " << header.indexCount << std::endl;

    if (std::string("meshfile").compare(std::string(header.fileID, 8)) != 0) {
        throw std::runtime_error("File does not have a valid mesh file ID.");
    }

    std::cout << "Initializing mesh" << std::endl;

    Mesh mesh;
    mesh.setNumVertices(header.vertexCount);
    if (header.indexCount > 0) {
        mesh.indices().resize(header.indexCount);
    }

    std::cout << "Reading vertex attribute descriptions" << std::endl;

    std::vector<uint64_t> attribVertexBufferOffsets(header.attribCount);
    std::vector<uint64_t> attribVertexBufferStrides(header.attribCount);
    
    size_t vertexSize = 0;
    for (uint8_t i = 0u; i < header.attribCount; ++i) {
        std::cout << "Reading attribute " << (i+1) << " / " << (int) header.attribCount << std::endl;
        
        char attribNameChars[32];
        for (auto j = 0u; j < 31; ++j) {
            attribNameChars[j] = _fs.get();
            if (attribNameChars[j] == '\0') break;
        }
        attribNameChars[31] = '\0';

        char dataBuffer[ATTRIB_DATA_SIZE];
        _fs.read(dataBuffer, ATTRIB_DATA_SIZE);
        AttribData data = unpackAttribData(dataBuffer);

        std::cout << "Attribute " << (i+1) << ":" << std::endl;
        std::cout << "\tName: " << attribNameChars << std::endl;
        std::cout << "\tComponent type: " << (int) data.componentType << std::endl;
        std::cout << "\tComponent count: " << (int) data.numComponents << std::endl;
        std::cout << "\tVertex data offset: " << data.vertexBufferOffset << std::endl;
        std::cout << "\tVertex data stride: " << data.vertexBufferStride << std::endl;

        attribVertexBufferOffsets[i] = data.vertexBufferOffset;
        attribVertexBufferStrides[i] = data.vertexBufferStride;

        MeshAttribute attribute = getAttributeFromName(attribNameChars);
        MeshAttributeComponentType componentType = static_cast<MeshAttributeComponentType>(data.componentType);

        createMeshAttributeBuffer(mesh, attribute, componentType, data.numComponents);
        
        vertexSize += componentSize(componentType) * data.numComponents;
    }

    std::cout << "Reading vertex buffer" << std::endl;

    std::vector<char> vertexBufferBytes(vertexSize * header.vertexCount);
    _fs.read(vertexBufferBytes.data(), vertexBufferBytes.size());

    for (uint8_t i = 0u; i < header.attribCount; ++i) {
        int progress = 100 * ((float) i / (float) header.attribCount);
        std::cout << "\rFilling attribute buffer " << (i+1) << " / " << (int) header.attribCount << " [" << progress << "%]      " << std::flush;
        
        auto& attribBuffer = mesh.getAttributeBuffer(i);

        auto offset = attribVertexBufferOffsets[i];
        auto stride = attribVertexBufferStrides[i];
        auto elementSize = attribBuffer.elementSize();
        
        if (stride == elementSize) {
            memcpy(attribBuffer.data(), &vertexBufferBytes[offset], header.vertexCount * stride);
        } else {
            for (size_t j = 0; j < header.vertexCount; ++j) {
                memcpy(attribBuffer.elementPtr(j), &vertexBufferBytes[offset + j * stride], elementSize);
            }
        }
    }

    std::cout << std::endl;
    std::cout << "Finished filling attribute buffers" << std::endl;

    vertexBufferBytes.clear();

    std::cout << "Reading index buffer" << std::endl;

    _fs.read(reinterpret_cast<char*>(mesh.indices().data()), header.indexCount * sizeof(Mesh::index_t));

    std::cout << "Finished reading mesh." << std::endl;

    return mesh;
}