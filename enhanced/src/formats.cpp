#include "opensc4/formats.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace opensc4 {

// ============================================================================
// 1. Exemplar & Cohort Parser (EQZB1### / EQZT1### / CQZB1### / CQZT1###)
// ============================================================================
bool cGZExemplar::Parse(const Uint8* data, std::size_t len) {
    properties.clear();
    name.clear();
    if (!data || len < 24) return false;

    if (std::memcmp(data, "EQZB1###", 8) == 0 || std::memcmp(data, "CQZB1###", 8) == 0) {
        isCohort = (data[0] == 'C');
        isTextFormat = false;

        cIGZIStream s(data + 8, len - 8);
        s >> parentCohort.type >> parentCohort.group >> parentCohort.instance;
        Uint32 propCount = 0;
        s >> propCount;
        if (!s.Good() || propCount > 10000) return false;

        for (Uint32 i = 0; i < propCount && s.Good(); ++i) {
            ExemplarProperty prop{};
            Uint8 unused = 0;
            s >> prop.id >> prop.valueType >> prop.keyType >> unused;
            if (!s.Good()) break;

            Uint32 count = 1;
            if (prop.keyType == 0x0080u) {
                s >> count;
            }
            if (!s.Good() || count > 65536u) break;

            switch (static_cast<ExemplarValueType>(prop.valueType)) {
                case ExemplarValueType::Uint8:
                    for (Uint32 k = 0; k < count; ++k) {
                        Uint8 v = 0; s >> v;
                        prop.intValues.push_back(v);
                    }
                    break;
                case ExemplarValueType::Bool:
                    for (Uint32 k = 0; k < count; ++k) {
                        Uint8 v = 0; s >> v;
                        prop.intValues.push_back(v != 0 ? 1 : 0);
                    }
                    break;
                case ExemplarValueType::Uint16:
                    for (Uint32 k = 0; k < count; ++k) {
                        Uint16 v = 0; s >> v;
                        prop.intValues.push_back(v);
                    }
                    break;
                case ExemplarValueType::Sint8:
                    for (Uint32 k = 0; k < count; ++k) {
                        Sint8 v = 0; s >> v;
                        prop.intValues.push_back(v);
                    }
                    break;
                case ExemplarValueType::Sint16:
                    for (Uint32 k = 0; k < count; ++k) {
                        Sint16 v = 0; s >> v;
                        prop.intValues.push_back(v);
                    }
                    break;
                case ExemplarValueType::Uint32:
                    for (Uint32 k = 0; k < count; ++k) {
                        Uint32 v = 0; s >> v;
                        prop.intValues.push_back(v);
                    }
                    break;
                case ExemplarValueType::Sint32:
                    for (Uint32 k = 0; k < count; ++k) {
                        Sint32 v = 0; s >> v;
                        prop.intValues.push_back(v);
                    }
                    break;
                case ExemplarValueType::Sint64:
                    for (Uint32 k = 0; k < count; ++k) {
                        Sint64 v = 0; s >> v;
                        prop.intValues.push_back(v);
                    }
                    break;
                case ExemplarValueType::Float32:
                    for (Uint32 k = 0; k < count; ++k) {
                        Float32 v = 0.0f; s >> v;
                        prop.floatValues.push_back(v);
                    }
                    break;
                case ExemplarValueType::String: {
                    prop.stringValue.resize(count);
                    if (count > 0) {
                        s.ReadBytes(prop.stringValue.data(), count);
                    }
                    break;
                }
                default:
                    return !properties.empty();
            }

            if (prop.id == kPropExemplarName && !prop.stringValue.empty()) {
                name = prop.stringValue;
            }
            properties[prop.id] = std::move(prop);
        }
        return true;
    }

    if (std::memcmp(data, "EQZT1###", 8) == 0 || std::memcmp(data, "CQZT1###", 8) == 0) {
        isCohort = (data[0] == 'C');
        isTextFormat = true;
        std::string txt(reinterpret_cast<const char*>(data + 8), len - 8);
        std::istringstream iss(txt);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("ParentCohort") != std::string::npos) {
                auto b1 = line.find('{');
                auto b2 = line.find('}', b1 == std::string::npos ? 0 : b1);
                if (b1 != std::string::npos && b2 != std::string::npos) {
                    std::string inner = line.substr(b1 + 1, b2 - b1 - 1);
                    std::replace(inner.begin(), inner.end(), ',', ' ');
                    std::istringstream ks(inner);
                    std::string tStr, gStr, iStr;
                    if (ks >> tStr >> gStr >> iStr) {
                        parentCohort.type = static_cast<Uint32>(std::strtoul(tStr.c_str(), nullptr, 0));
                        parentCohort.group = static_cast<Uint32>(std::strtoul(gStr.c_str(), nullptr, 0));
                        parentCohort.instance = static_cast<Uint32>(std::strtoul(iStr.c_str(), nullptr, 0));
                    }
                }
                continue;
            }
            auto eqPos = line.find('=');
            auto pos = line.find("0x");
            if (pos == std::string::npos || (eqPos != std::string::npos && pos > eqPos)) continue;
            Uint32 propId = static_cast<Uint32>(std::strtoul(line.c_str() + pos, nullptr, 16));
            ExemplarProperty prop{};
            prop.id = propId;
            if (eqPos != std::string::npos) {
                auto colon1 = line.find(':', eqPos + 1);
                std::string typeName;
                if (colon1 != std::string::npos) {
                    typeName = line.substr(eqPos + 1, colon1 - (eqPos + 1));
                }
                auto b1 = line.find('{', eqPos + 1);
                auto b2 = line.rfind('}');
                if (b1 != std::string::npos && b2 != std::string::npos && b2 > b1) {
                    std::string inner = line.substr(b1 + 1, b2 - b1 - 1);
                    if (typeName == "String") {
                        prop.valueType = static_cast<Uint16>(ExemplarValueType::String);
                        auto sq1 = inner.find('"');
                        auto sq2 = inner.rfind('"');
                        if (sq1 != std::string::npos && sq2 != std::string::npos && sq2 > sq1) {
                            prop.stringValue = inner.substr(sq1 + 1, sq2 - sq1 - 1);
                        } else {
                            prop.stringValue = inner;
                        }
                        if (propId == kPropExemplarName) name = prop.stringValue;
                    } else if (typeName == "Float32") {
                        prop.valueType = static_cast<Uint16>(ExemplarValueType::Float32);
                        std::replace(inner.begin(), inner.end(), ',', ' ');
                        std::istringstream vs(inner);
                        std::string tok;
                        while (vs >> tok) {
                            prop.floatValues.push_back(std::strtof(tok.c_str(), nullptr));
                        }
                    } else {
                        if (typeName == "Uint8") prop.valueType = static_cast<Uint16>(ExemplarValueType::Uint8);
                        else if (typeName == "Uint16") prop.valueType = static_cast<Uint16>(ExemplarValueType::Uint16);
                        else if (typeName == "Uint32") prop.valueType = static_cast<Uint16>(ExemplarValueType::Uint32);
                        else if (typeName == "Sint8") prop.valueType = static_cast<Uint16>(ExemplarValueType::Sint8);
                        else if (typeName == "Sint16") prop.valueType = static_cast<Uint16>(ExemplarValueType::Sint16);
                        else if (typeName == "Sint32") prop.valueType = static_cast<Uint16>(ExemplarValueType::Sint32);
                        else if (typeName == "Sint64") prop.valueType = static_cast<Uint16>(ExemplarValueType::Sint64);
                        else if (typeName == "Bool") prop.valueType = static_cast<Uint16>(ExemplarValueType::Bool);
                        std::replace(inner.begin(), inner.end(), ',', ' ');
                        std::istringstream vs(inner);
                        std::string tok;
                        while (vs >> tok) {
                            if (tok == "True" || tok == "true") {
                                prop.intValues.push_back(1);
                            } else if (tok == "False" || tok == "false") {
                                prop.intValues.push_back(0);
                            } else if (tok.size() > 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X')) {
                                prop.intValues.push_back(static_cast<Sint64>(std::strtoull(tok.c_str(), nullptr, 16)));
                            } else {
                                prop.intValues.push_back(static_cast<Sint64>(std::strtoll(tok.c_str(), nullptr, 10)));
                            }
                        }
                    }
                }
            }
            properties[propId] = std::move(prop);
        }
        return true;
    }

    return false;
}

std::string cGZExemplar::GetString(Uint32 propId, const std::string& fallback) const {
    auto it = properties.find(propId);
    if (it != properties.end() && !it->second.stringValue.empty()) return it->second.stringValue;
    return fallback;
}

Sint64 cGZExemplar::GetInt(Uint32 propId, Sint64 fallback) const {
    auto it = properties.find(propId);
    if (it != properties.end() && !it->second.intValues.empty()) return it->second.intValues[0];
    return fallback;
}

Float32 cGZExemplar::GetFloat(Uint32 propId, Float32 fallback) const {
    auto it = properties.find(propId);
    if (it != properties.end() && !it->second.floatValues.empty()) return it->second.floatValues[0];
    return fallback;
}

// ============================================================================
// 2. S3D 3D Model Parser (3DMD -> HEAD, VERT, INDX, PRIM, MATS)
// ============================================================================
namespace {

struct S3DVertexGroup {
    Uint32 firstVertex = 0;
    Uint32 count = 0;
};

struct S3DIndexGroup {
    std::vector<Uint16> indices;
};

struct S3DPrim {
    Uint32 type = 0;
    Uint32 first = 0;
    Uint32 count = 0;
};

struct S3DMeshBlocks {
    Uint16 vert = 0, indx = 0, prim = 0, mats = 0;
};

// High-bit formats encode the layout (bit 1: color, bits 14-15: UV sets);
// legacy formats do not, so their stride comes from the chunk length.
std::size_t S3DVertexStride(Uint32 format) noexcept {
    if ((format & 0x80000000u) == 0) return 0;
    return 12u + ((format & 2u) ? 4u : 0u) + ((format >> 14) & 3u) * 8u;
}

// MATS texture entries gained mag/min filter bytes in 1.5. Some shipped 1.5
// files write them but size the chunk as if they did not, so the 1.5 layout
// may run exactly 2 bytes per texture past the declared end; `consumed`
// reports the true length.
bool ParseS3DMaterials(const Uint8* p, std::size_t avail, bool withFilters,
                       std::vector<S3DMaterial>& out, std::size_t& consumed,
                       std::size_t& textureCount) {
    out.clear();
    textureCount = 0;
    cIGZIStream s(p, avail);
    Uint32 count = 0;
    s >> count;
    if (!s.Good() || count > 4096) return false;
    for (Uint32 m = 0; m < count; ++m) {
        S3DMaterial mat{};
        Uint8 depthFunc = 0, reserved = 0, texCount = 0;
        Uint32 matClass = 0;
        s >> mat.flags >> mat.alphaFunc >> depthFunc >> mat.srcBlend >> mat.dstBlend
          >> mat.alphaThreshold >> matClass >> reserved >> texCount;
        textureCount += texCount;
        for (Uint8 t = 0; t < texCount && s.Good(); ++t) {
            Uint32 textureId = 0;
            s >> textureId;
            if (t == 0) mat.textureId = textureId;
            s.Skip(withFilters ? 8 : 6); // wrapS, wrapT, [mag, min], animRate, animMode
            Uint8 nameLen = 0;
            s >> nameLen;
            s.Skip(nameLen);
        }
        if (!s.Good()) return false;
        out.push_back(mat);
    }
    consumed = s.Position();
    return s.Good();
}

// Static models (frame rate 0) omit the per-frame block table; their mesh i
// uses group i of every chunk.
bool ParseS3DAnim(const Uint8* p, std::size_t len, bool withBlocks,
                  std::vector<S3DMeshBlocks>& out) {
    out.clear();
    cIGZIStream s(p, len);
    Uint16 numFrames = 0, frameRate = 0, animMode = 0, numMeshes = 0;
    Uint32 flags = 0;
    Float32 displacement = 0.0f;
    s >> numFrames >> frameRate >> animMode >> flags >> displacement >> numMeshes;
    if (!s.Good() || numMeshes > 4096) return false;
    for (Uint16 m = 0; m < numMeshes; ++m) {
        Uint8 nameLen = 0, meshFlags = 0;
        s >> nameLen >> meshFlags;
        s.Skip(nameLen);
        S3DMeshBlocks blocks{m, m, m, m};
        if (withBlocks) {
            for (Uint16 f = 0; f < numFrames && s.Good(); ++f) {
                S3DMeshBlocks frame{};
                s >> frame.vert >> frame.indx >> frame.prim >> frame.mats;
                if (f == 0) blocks = frame;
            }
        }
        if (!s.Good()) return false;
        out.push_back(blocks);
    }
    return s.Good() && s.Remaining() == 0;
}

} // namespace

bool cS3DModel::Parse(const Uint8* data, std::size_t len) {
    vertices.clear();
    indices.clear();
    materials.clear();
    meshes.clear();
    if (!data || len < 20 || std::memcmp(data, "3DMD", 4) != 0) return false;

    std::vector<S3DVertexGroup> vertGroups;
    std::vector<S3DIndexGroup> indexGroups;
    std::vector<std::vector<S3DPrim>> primGroups;
    std::vector<S3DMeshBlocks> animMeshes;
    bool haveAnim = false;

    std::size_t pos = 8;
    while (pos + 8 <= len) {
        char tag[5] = {0, 0, 0, 0, 0};
        std::memcpy(tag, data + pos, 4);
        Uint32 chunkSize = 0;
        std::memcpy(&chunkSize, data + pos + 4, 4);
        if (chunkSize < 8 || pos + chunkSize > len) break;

        const Uint8* body = data + pos + 8;
        const std::size_t bodyLen = chunkSize - 8;
        cIGZIStream cs(body, bodyLen);
        if (std::memcmp(tag, "HEAD", 4) == 0) {
            cs >> majorVersion >> minorVersion;
        } else if (std::memcmp(tag, "VERT", 4) == 0) {
            Uint32 numGroups = 0;
            cs >> numGroups;
            if (!cs.Good() || numGroups > 4096) break;
            // First pass: headers, to size legacy-format strides from the chunk length.
            std::vector<std::pair<Uint16, Uint32>> headers;
            std::size_t knownBytes = 4, unknownVerts = 0;
            {
                cIGZIStream scan(body, bodyLen);
                scan.Skip(4);
                for (Uint32 g = 0; g < numGroups && scan.Good(); ++g) {
                    Uint16 flags = 0, count = 0;
                    Uint32 format = 0;
                    scan >> flags >> count >> format;
                    headers.emplace_back(count, format);
                    knownBytes += 8;
                    const std::size_t stride = S3DVertexStride(format);
                    if (stride == 0) unknownVerts += count;
                    knownBytes += stride * count;
                    scan.Skip(stride * count);
                }
            }
            const std::size_t legacyStride =
                (unknownVerts > 0 && bodyLen > knownBytes) ? (bodyLen - knownBytes) / unknownVerts : 20;
            for (Uint32 g = 0; g < numGroups && cs.Good(); ++g) {
                Uint16 flags = 0, count = 0;
                Uint32 format = 0;
                cs >> flags >> count >> format;
                std::size_t stride = S3DVertexStride(format);
                if (stride == 0) stride = legacyStride;
                const bool hasColor = (format & 0x80000000u) ? (format & 2u) != 0 : stride == 24;
                vertGroups.push_back({static_cast<Uint32>(vertices.size()), count});
                for (Uint16 v = 0; v < count && cs.Remaining() >= stride; ++v) {
                    S3DVertex vert{};
                    std::size_t used = 12;
                    cs >> vert.x >> vert.y >> vert.z;
                    if (hasColor && stride >= 16) { cs >> vert.color; used += 4; }
                    if (stride >= used + 8) { cs >> vert.u >> vert.v; used += 8; }
                    if (stride > used) cs.Skip(stride - used);
                    vertices.push_back(vert);
                }
            }
        } else if (std::memcmp(tag, "INDX", 4) == 0) {
            Uint32 numGroups = 0;
            cs >> numGroups;
            for (Uint32 g = 0; g < numGroups && cs.Good() && g < 4096; ++g) {
                Uint16 flags = 0, stride = 2, count = 0;
                cs >> flags >> stride >> count;
                S3DIndexGroup group;
                group.indices.reserve(count);
                for (Uint16 i = 0; i < count && cs.Good(); ++i) {
                    Uint16 idx = 0;
                    cs >> idx;
                    if (stride > 2) cs.Skip(stride - 2u);
                    group.indices.push_back(idx);
                }
                indexGroups.push_back(std::move(group));
            }
        } else if (std::memcmp(tag, "PRIM", 4) == 0) {
            Uint32 numGroups = 0;
            cs >> numGroups;
            for (Uint32 g = 0; g < numGroups && cs.Good() && g < 4096; ++g) {
                Uint16 numPrims = 0;
                cs >> numPrims;
                std::vector<S3DPrim> prims;
                for (Uint16 k = 0; k < numPrims && cs.Good(); ++k) {
                    S3DPrim prim{};
                    cs >> prim.type >> prim.first >> prim.count;
                    prims.push_back(prim);
                }
                primGroups.push_back(std::move(prims));
            }
        } else if (std::memcmp(tag, "MATS", 4) == 0) {
            const std::size_t avail = len - (pos + 8);
            std::size_t consumed = 0, textures = 0;
            const bool filters = minorVersion >= 5;
            bool ok = ParseS3DMaterials(body, avail, filters, materials, consumed, textures) &&
                      (consumed == bodyLen || (filters && consumed == bodyLen + 2u * textures));
            if (!ok) {
                ok = ParseS3DMaterials(body, avail, !filters, materials, consumed, textures) &&
                     consumed == bodyLen;
            }
            if (ok) pos += consumed - bodyLen;
            else materials.clear();
        } else if (std::memcmp(tag, "ANIM", 4) == 0) {
            haveAnim = ParseS3DAnim(body, bodyLen, true, animMeshes) ||
                       ParseS3DAnim(body, bodyLen, false, animMeshes);
        }

        pos += chunkSize;
    }

    if (!haveAnim) {
        animMeshes.clear();
        const std::size_t n = std::min(vertGroups.size(), indexGroups.size());
        for (std::size_t g = 0; g < n; ++g) {
            const Uint16 id = static_cast<Uint16>(g);
            animMeshes.push_back({id, id, id, id});
        }
    }

    for (const S3DMeshBlocks& blocks : animMeshes) {
        if (blocks.vert >= vertGroups.size() || blocks.indx >= indexGroups.size()) continue;
        const S3DVertexGroup& vg = vertGroups[blocks.vert];
        const std::vector<Uint16>& group = indexGroups[blocks.indx].indices;
        std::vector<S3DPrim> ranges;
        if (blocks.prim < primGroups.size()) ranges = primGroups[blocks.prim];
        else ranges.push_back({0u, 0u, static_cast<Uint32>(group.size())});

        S3DMesh mesh{};
        mesh.firstIndex = static_cast<Uint32>(indices.size());
        mesh.materialIndex = blocks.mats;
        for (const S3DPrim& prim : ranges) {
            if (prim.type != 0) continue; // Only triangle lists appear in shipped models
            const std::size_t end = std::min<std::size_t>(group.size(),
                                                          static_cast<std::size_t>(prim.first) + prim.count);
            for (std::size_t i = prim.first; i + 3 <= end; i += 3) {
                if (group[i] >= vg.count || group[i + 1] >= vg.count || group[i + 2] >= vg.count) continue;
                indices.push_back(vg.firstVertex + group[i]);
                indices.push_back(vg.firstVertex + group[i + 1]);
                indices.push_back(vg.firstVertex + group[i + 2]);
            }
        }
        mesh.indexCount = static_cast<Uint32>(indices.size()) - mesh.firstIndex;
        if (mesh.indexCount > 0) meshes.push_back(mesh);
    }

    return !vertices.empty();
}

// ============================================================================
// 3. FSH Texture Decoder (SHPI -> DXT1 0x60, DXT3 0x61, ARGB8888 0x7D, RGB888 0x7F)
// ============================================================================
static void DecodeRGB565(Uint16 c, Uint8& r, Uint8& g, Uint8& b) noexcept {
    r = static_cast<Uint8>(((c >> 11) & 0x1Fu) * 255u / 31u);
    g = static_cast<Uint8>(((c >> 5)  & 0x3Fu) * 255u / 63u);
    b = static_cast<Uint8>((c         & 0x1Fu) * 255u / 31u);
}

bool cFSHImage::Parse(const Uint8* data, std::size_t len) {
    rgbaPixels.clear();
    width = 0;
    height = 0;
    if (!data || len < 32 || std::memcmp(data, "SHPI", 4) != 0) return false;

    cIGZIStream s(data, len);
    s.Skip(8); // "SHPI" + fileSize
    Uint32 numEntries = 0;
    s >> numEntries;
    s.Skip(4); // directory ID ("G354", etc.)
    if (numEntries == 0 || !s.Good()) return false;

    char entryTag[5] = {0, 0, 0, 0, 0};
    s.ReadBytes(entryTag, 4);
    tag = entryTag;
    Uint32 entryOffset = 0;
    s >> entryOffset;
    if (entryOffset + 16 > len) return false;

    cIGZIStream es(data + entryOffset, len - entryOffset);
    es >> recordCode;
    es.Skip(3); // 24-bit block size
    es >> width >> height;
    es.Skip(8); // centerX, centerY, leftX, topY

    if (width == 0 || height == 0 || width > 4096 || height > 4096) return false;
    rgbaPixels.assign(static_cast<std::size_t>(width) * height * 4, 255);

    const Uint8* src = es.CurrentPtr();
    std::size_t rem = es.Remaining();

    if (recordCode == 0x60u || recordCode == 0x61u) {
        // DXT1 (0x60, 8 bytes per 4x4 block) or DXT3 (0x61, 16 bytes per 4x4 block)
        const bool isDXT3 = (recordCode == 0x61u);
        const std::size_t blockBytes = isDXT3 ? 16 : 8;
        const int blocksX = (width + 3) / 4;
        const int blocksY = (height + 3) / 4;

        std::size_t bpos = 0;
        for (int by = 0; by < blocksY; ++by) {
            for (int bx = 0; bx < blocksX; ++bx) {
                if (bpos + blockBytes > rem) break;
                const Uint8* blk = src + bpos;
                bpos += blockBytes;

                Uint64 alphaBits = 0xFFFFFFFFFFFFFFFFull;
                if (isDXT3) {
                    std::memcpy(&alphaBits, blk, 8);
                    blk += 8;
                }

                Uint16 c0 = 0, c1 = 0;
                Uint32 lut = 0;
                std::memcpy(&c0, blk, 2);
                std::memcpy(&c1, blk + 2, 2);
                std::memcpy(&lut, blk + 4, 4);

                Uint8 r[4], g[4], b[4], a[4] = {255, 255, 255, 255};
                DecodeRGB565(c0, r[0], g[0], b[0]);
                DecodeRGB565(c1, r[1], g[1], b[1]);

                if (c0 > c1 || isDXT3) {
                    r[2] = static_cast<Uint8>((2u * r[0] + r[1]) / 3u);
                    g[2] = static_cast<Uint8>((2u * g[0] + g[1]) / 3u);
                    b[2] = static_cast<Uint8>((2u * b[0] + b[1]) / 3u);
                    r[3] = static_cast<Uint8>((r[0] + 2u * r[1]) / 3u);
                    g[3] = static_cast<Uint8>((g[0] + 2u * g[1]) / 3u);
                    b[3] = static_cast<Uint8>((b[0] + 2u * b[1]) / 3u);
                } else {
                    r[2] = static_cast<Uint8>((r[0] + r[1]) / 2u);
                    g[2] = static_cast<Uint8>((g[0] + g[1]) / 2u);
                    b[2] = static_cast<Uint8>((b[0] + b[1]) / 2u);
                    r[3] = 0; g[3] = 0; b[3] = 0; a[3] = 0;
                }

                for (int py = 0; py < 4; ++py) {
                    int y = by * 4 + py;
                    if (y >= height) continue;
                    for (int px = 0; px < 4; ++px) {
                        int x = bx * 4 + px;
                        if (x >= width) continue;
                        int idx = py * 4 + px;
                        Uint32 ci = (lut >> (idx * 2)) & 0x03u;
                        std::size_t dstOff = (static_cast<std::size_t>(y) * width + x) * 4;
                        rgbaPixels[dstOff + 0] = r[ci];
                        rgbaPixels[dstOff + 1] = g[ci];
                        rgbaPixels[dstOff + 2] = b[ci];
                        if (isDXT3) {
                            Uint8 a4 = static_cast<Uint8>((alphaBits >> (idx * 4)) & 0x0Fu);
                            rgbaPixels[dstOff + 3] = static_cast<Uint8>(a4 * 17u);
                        } else {
                            rgbaPixels[dstOff + 3] = a[ci];
                        }
                    }
                }
            }
        }
        return true;
    } else if (recordCode == 0x7Du) {
        // 32-bit BGRA8888
        std::size_t count = std::min<std::size_t>(static_cast<std::size_t>(width) * height, rem / 4);
        for (std::size_t i = 0; i < count; ++i) {
            rgbaPixels[i * 4 + 0] = src[i * 4 + 2];
            rgbaPixels[i * 4 + 1] = src[i * 4 + 1];
            rgbaPixels[i * 4 + 2] = src[i * 4 + 0];
            rgbaPixels[i * 4 + 3] = src[i * 4 + 3];
        }
        return true;
    } else if (recordCode == 0x7Fu) {
        // 24-bit BGR888
        std::size_t count = std::min<std::size_t>(static_cast<std::size_t>(width) * height, rem / 3);
        for (std::size_t i = 0; i < count; ++i) {
            rgbaPixels[i * 4 + 0] = src[i * 3 + 2];
            rgbaPixels[i * 4 + 1] = src[i * 3 + 1];
            rgbaPixels[i * 4 + 2] = src[i * 3 + 0];
            rgbaPixels[i * 4 + 3] = 255;
        }
        return true;
    }

    return true;
}

// ============================================================================
// 4. LTEXT Decoder (0x2026960b)
// ============================================================================
std::string DecodeLTEXT(const Uint8* data, std::size_t len) {
    if (!data || len < 4) return {};
    Uint32 charCount = static_cast<Uint32>(data[0]) |
                       (static_cast<Uint32>(data[1]) << 8) |
                       (static_cast<Uint32>(data[2]) << 16);
    Uint8 control = data[3];
    std::string out;
    if (control == 0x10u) {
        // UTF-16LE
        std::size_t maxChars = std::min<std::size_t>(charCount, (len - 4) / 2);
        out.reserve(maxChars);
        for (std::size_t i = 0; i < maxChars; ++i) {
            Uint16 ch = static_cast<Uint16>(data[4 + i * 2]) |
                        (static_cast<Uint16>(data[4 + i * 2 + 1]) << 8);
            if (ch == 0) break;
            if (ch < 128) out.push_back(static_cast<char>(ch));
            else if (ch < 0x800) {
                out.push_back(static_cast<char>(0xC0 | (ch >> 6)));
                out.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
            }
        }
    } else {
        std::size_t maxChars = std::min<std::size_t>(charCount, len - 4);
        out.assign(reinterpret_cast<const char*>(data + 4), maxChars);
    }
    return out;
}

static Uint32 ComputeStandardCRC32(const Uint8* data, std::size_t len) noexcept {
    static Uint32 table[256];
    static bool init = false;
    if (!init) {
        for (Uint32 i = 0; i < 256; ++i) {
            Uint32 c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        init = true;
    }
    Uint32 c = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < len; ++i) c = table[(c ^ data[i]) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static void AppendBE32(std::vector<Uint8>& buf, Uint32 v) {
    buf.push_back(static_cast<Uint8>((v >> 24) & 0xFFu));
    buf.push_back(static_cast<Uint8>((v >> 16) & 0xFFu));
    buf.push_back(static_cast<Uint8>((v >> 8)  & 0xFFu));
    buf.push_back(static_cast<Uint8>(v & 0xFFu));
}

static void AppendPNGChunk(std::vector<Uint8>& png, const char type[4], const Uint8* data, std::size_t len) {
    AppendBE32(png, static_cast<Uint32>(len));
    std::size_t crcStart = png.size();
    png.insert(png.end(), reinterpret_cast<const Uint8*>(type), reinterpret_cast<const Uint8*>(type) + 4);
    if (len > 0 && data) png.insert(png.end(), data, data + len);
    Uint32 crc = ComputeStandardCRC32(png.data() + crcStart, 4 + len);
    AppendBE32(png, crc);
}

std::vector<Uint8> EncodeRGBA8ToPNG(Uint16 width, Uint16 height, const Uint8* rgbaPixels) {
    std::vector<Uint8> png;
    if (width == 0 || height == 0 || !rgbaPixels) return png;

    const Uint8 sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    png.insert(png.end(), sig, sig + 8);

    Uint8 ihdr[13] = {0};
    ihdr[0] = static_cast<Uint8>((width >> 24) & 0xFFu);
    ihdr[1] = static_cast<Uint8>((width >> 16) & 0xFFu);
    ihdr[2] = static_cast<Uint8>((width >> 8)  & 0xFFu);
    ihdr[3] = static_cast<Uint8>(width & 0xFFu);
    ihdr[4] = static_cast<Uint8>((height >> 24) & 0xFFu);
    ihdr[5] = static_cast<Uint8>((height >> 16) & 0xFFu);
    ihdr[6] = static_cast<Uint8>((height >> 8)  & 0xFFu);
    ihdr[7] = static_cast<Uint8>(height & 0xFFu);
    ihdr[8] = 8; // bit depth 8
    ihdr[9] = 6; // color type 6 = RGBA
    AppendPNGChunk(png, "IHDR", ihdr, 13);

    // Build raw scanlines (1 filter byte 0x00 + width*4 bytes per row)
    std::size_t rowBytes = 1 + static_cast<std::size_t>(width) * 4;
    std::vector<Uint8> raw(rowBytes * height, 0);
    for (Uint16 y = 0; y < height; ++y) {
        raw[y * rowBytes] = 0; // Filter: None
        std::memcpy(&raw[y * rowBytes + 1], rgbaPixels + static_cast<std::size_t>(y) * width * 4, static_cast<std::size_t>(width) * 4);
    }

    // Wrap in RFC 1950 zlib stream with uncompressed DEFLATE blocks (BTYPE=00)
    std::vector<Uint8> zstream;
    zstream.reserve(raw.size() + (raw.size() / 65535 + 2) * 5 + 6);
    zstream.push_back(0x78); // CMF
    zstream.push_back(0x01); // FLG

    std::size_t pos = 0;
    Uint32 s1 = 1, s2 = 0;
    while (pos < raw.size()) {
        std::size_t blkLen = std::min<std::size_t>(65535, raw.size() - pos);
        bool isLast = (pos + blkLen == raw.size());
        zstream.push_back(isLast ? 0x01 : 0x00);
        Uint16 len16 = static_cast<Uint16>(blkLen);
        Uint16 nlen16 = ~len16;
        zstream.push_back(static_cast<Uint8>(len16 & 0xFFu));
        zstream.push_back(static_cast<Uint8>((len16 >> 8) & 0xFFu));
        zstream.push_back(static_cast<Uint8>(nlen16 & 0xFFu));
        zstream.push_back(static_cast<Uint8>((nlen16 >> 8) & 0xFFu));
        zstream.insert(zstream.end(), raw.begin() + pos, raw.begin() + pos + blkLen);
        for (std::size_t i = 0; i < blkLen; ++i) {
            s1 = (s1 + raw[pos + i]) % 65521u;
            s2 = (s2 + s1) % 65521u;
        }
        pos += blkLen;
    }
    Uint32 adler = (s2 << 16) | s1;
    AppendBE32(zstream, adler);

    AppendPNGChunk(png, "IDAT", zstream.data(), zstream.size());
    AppendPNGChunk(png, "IEND", nullptr, 0);
    return png;
}

// ============================================================================
// 5. Mach-O 64-bit Symbol & GZCOM CLSID Inspector
// ============================================================================
MachOBinarySummary InspectSC4MachOBinary(const std::string& machOPath) {
    MachOBinarySummary summary{};
    std::ifstream f(machOPath, std::ios::binary | std::ios::ate);
    if (!f) return summary;

    std::size_t fileSize = static_cast<std::size_t>(f.tellg());
    if (fileSize < 64) return summary;
    f.seekg(0, std::ios::beg);

    std::vector<Uint8> bin(fileSize);
    f.read(reinterpret_cast<char*>(bin.data()), fileSize);
    if (!f) return summary;

    // Mach-O 64-bit magic: MH_MAGIC_64 = 0xFEEDFACF
    Uint32 magic = 0;
    std::memcpy(&magic, bin.data(), 4);
    if (magic != 0xFEEDFACFu) return summary;

    Uint32 ncmds = 0;
    std::memcpy(&ncmds, bin.data() + 16, 4);

    std::size_t cmdOffset = 32; // sizeof(mach_header_64)
    Uint32 symoff = 0, nsyms = 0, stroff = 0, strsize = 0;

    for (Uint32 i = 0; i < ncmds && cmdOffset + 8 <= fileSize; ++i) {
        Uint32 cmd = 0, cmdsize = 0;
        std::memcpy(&cmd, bin.data() + cmdOffset, 4);
        std::memcpy(&cmdsize, bin.data() + cmdOffset + 4, 4);
        if (cmdsize < 8 || cmdOffset + cmdsize > fileSize) break;

        // LC_SYMTAB = 0x2
        if (cmd == 0x2u && cmdsize >= 24) {
            std::memcpy(&symoff, bin.data() + cmdOffset + 8, 4);
            std::memcpy(&nsyms, bin.data() + cmdOffset + 12, 4);
            std::memcpy(&stroff, bin.data() + cmdOffset + 16, 4);
            std::memcpy(&strsize, bin.data() + cmdOffset + 20, 4);
            break;
        }
        cmdOffset += cmdsize;
    }

    if (nsyms == 0 || symoff + static_cast<std::size_t>(nsyms) * 16 > fileSize ||
        stroff + strsize > fileSize) {
        return summary;
    }

    summary.valid = true;
    summary.totalSymbols = nsyms;

    // Each nlist_64 is 16 bytes: [uint32_t n_strx, uint8_t n_type, uint8_t n_sect, int16_t n_desc, uint64_t n_value]
    Uint32 nonStabSyms = 0;
    for (Uint32 i = 0; i < nsyms; ++i) {
        const Uint8* nl = bin.data() + symoff + static_cast<std::size_t>(i) * 16;
        Uint32 strx = 0;
        Uint8 n_type = nl[4];
        Uint64 n_value = 0;
        std::memcpy(&strx, nl, 4);
        std::memcpy(&n_value, nl + 8, 8);

        // Skip DWARF STAB debug entries (N_STAB = 0xE0)
        if ((n_type & 0xE0u) != 0) continue;
        ++nonStabSyms;

        if (strx >= strsize) continue;
        const char* rawName = reinterpret_cast<const char*>(bin.data() + stroff + strx);

        if (std::strstr(rawName, "GZCOM_Create") != nullptr) {
            summary.gzcomFactoryCount++;
        }

        // Check for GZCOM_*CLSID or GetGZCLSID methods and extract the immediate 32-bit CLSID from machine code!
        if (n_value >= 0x100000000ull &&
            (std::strstr(rawName, "CLSID") != nullptr)) {
            std::size_t codeOff = static_cast<std::size_t>(n_value - 0x100000000ull);
            if (codeOff + 24 <= fileSize) {
                for (std::size_t k = 0; k < 18; ++k) {
                    if (bin[codeOff + k] == 0xB8u) { // mov eax, IMM32
                        Uint32 clsid = 0;
                        std::memcpy(&clsid, bin.data() + codeOff + k + 1, 4);
                        if (summary.extractedCLSIDs.find(clsid) == summary.extractedCLSIDs.end()) {
                            summary.extractedCLSIDs[clsid] = rawName;
                        }
                        break;
                    }
                }
            }
        }
    }
    summary.totalSymbols = nonStabSyms;

    return summary;
}

} // namespace opensc4
