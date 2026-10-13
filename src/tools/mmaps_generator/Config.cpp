/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Config.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <fkYAML/node.hpp>
#include "PathCommon.h"
#include "TerrainBuilder.h"

namespace MMAP
{
    bool NavmeshExclusion::Overlaps(float const* bmin, float const* bmax) const
    {
        for (std::size_t axis = 0; axis < minBounds.size(); ++axis)
            if (maxBounds[axis] < bmin[axis] || minBounds[axis] > bmax[axis])
                return false;

        return true;
    }

    float ComputeBaseUnitDim(int vertexPerMapEdge)
    {
        return GRID_SIZE / static_cast<float>(vertexPerMapEdge);
    }

    std::pair<uint32, uint32> MakeTileKey(uint32 x, uint32 y)
    {
        return {x, y};
    }

    bool isCurrentDirectory(std::string const& pathStr) {
        try {
            const std::filesystem::path givenPath = std::filesystem::canonical(std::filesystem::absolute(pathStr));
            const std::filesystem::path currentPath = std::filesystem::canonical(std::filesystem::current_path());
            return givenPath == currentPath;
        } catch (std::filesystem::filesystem_error const& e) {
            std::cerr << "Filesystem error: " << e.what() << "\n";
            return false;
        }
    }

    MmapTileRecastConfig ResolvedMeshConfig::toMMAPTileRecastConfig() const {
        MmapTileRecastConfig config;
        config.walkableSlopeAngle = walkableSlopeAngle;
        config.walkableHeight = walkableHeight;
        config.walkableClimb = walkableClimb;
        config.walkableRadius = walkableRadius;
        config.maxSimplificationError = maxSimplificationError;
        config.cellSizeHorizontal = cellSizeHorizontal;
        config.cellSizeVertical = cellSizeVertical;
        config.baseUnitDim = baseUnitDim;
        config.vertexPerMapEdge = vertexPerMapEdge;
        config.vertexPerTileEdge = vertexPerTileEdge;
        config.tilesPerMapEdge = tilesPerMapEdge;
        return config;
    }

    std::optional<Config> Config::FromFile(std::string_view configFile) {
        Config config;
        if (!config.LoadConfig(configFile))
            return std::nullopt;

        return config;
    }

    Config::Config()
    {
    }

    ResolvedMeshConfig Config::GetConfigForTile(uint32 mapID, uint32 tileX, uint32 tileY) const
    {
        MapOverride const* mapOverride = nullptr;
        TileOverride const* tileOverride = nullptr;

        // Lookup map and tile overrides
        if (auto mapIt = _maps.find(mapID); mapIt != _maps.end())
        {
            mapOverride = &mapIt->second;

            auto tileIt = mapOverride->tileOverrides.find(MakeTileKey(tileY, tileX));
            if (tileIt != mapOverride->tileOverrides.end())
                tileOverride = &tileIt->second;
        }

        // Helper lambdas to resolve values in order: tile -> map -> global
        auto resolveFloat = [&](auto TileField, auto MapField, float GlobalValue) -> float {
            if (tileOverride && TileField(tileOverride)) return *TileField(tileOverride);
            if (mapOverride && MapField(mapOverride)) return *MapField(mapOverride);
            return GlobalValue;
        };

        auto resolveInt = [&](auto TileField, auto MapField, int GlobalValue) -> int {
            if (tileOverride && TileField(tileOverride)) return *TileField(tileOverride);
            if (mapOverride && MapField(mapOverride)) return *MapField(mapOverride);
            return GlobalValue;
        };

        // Resolve vertex settings
        int vertexPerMap = resolveInt(
            [](TileOverride const*) { return std::optional<int>{}; },
            [](MapOverride const* m) { return m->vertexPerMapEdge; },
            _global.vertexPerMapEdge
        );

        int vertexPerTile = resolveInt(
            [](TileOverride const*) { return std::optional<int>{}; },
            [](MapOverride const* m) { return m->vertexPerTileEdge; },
            _global.vertexPerTileEdge
        );

        ResolvedMeshConfig config;
        config.walkableSlopeAngle = resolveFloat(
            [](TileOverride const* t) { return t->walkableSlopeAngle; },
            [](MapOverride const* m) { return m->walkableSlopeAngle; },
            _global.walkableSlopeAngle
        );

        config.walkableRadius = resolveInt(
            [](TileOverride const* t) { return t->walkableRadius; },
            [](MapOverride const* m) { return m->walkableRadius; },
            _global.walkableRadius
        );

        config.walkableHeight = resolveInt(
            [](TileOverride const* t) { return t->walkableHeight; },
            [](MapOverride const* m) { return m->walkableHeight; },
            _global.walkableHeight
        );

        config.walkableClimb = resolveInt(
            [](TileOverride const* t) { return t->walkableClimb; },
            [](MapOverride const* m) { return m->walkableClimb; },
            _global.walkableClimb
        );

        config.vertexPerMapEdge = vertexPerMap;
        config.vertexPerTileEdge = vertexPerTile;
        config.baseUnitDim = ComputeBaseUnitDim(vertexPerMap);
        config.tilesPerMapEdge = vertexPerMap / vertexPerTile;
        config.maxSimplificationError = _global.maxSimplificationError;
        config.cellSizeHorizontal = config.baseUnitDim;
        config.cellSizeVertical = config.baseUnitDim;

        if (mapOverride && mapOverride->cellSizeHorizontal.has_value())
            config.cellSizeHorizontal = *mapOverride->cellSizeHorizontal;

        if (mapOverride && mapOverride->cellSizeVertical.has_value())
            config.cellSizeVertical = *mapOverride->cellSizeVertical;

        if (mapOverride)
            config.navmeshExclusions = mapOverride->navmeshExclusions;

        return config;
    }

    bool Config::LoadConfig(std::string_view configFile) {
        FILE* f = std::fopen(configFile.data(), "r");
        if (!f)
            return false;

        fkyaml::node root = fkyaml::node::deserialize(f);
        std::fclose(f);

        if (!root.contains("mmapsConfig"))
            return false;

        fkyaml::node mmapsNode = root["mmapsConfig"];

        auto tryFloat = [](fkyaml::node const& n, char const* key, float& out)
        {
            if (n.contains(key)) out = n[key].get_value<float>();
        };
        auto tryInt = [](fkyaml::node const& n, char const* key, int& out)
        {
            if (n.contains(key)) out = n[key].get_value<int>();
        };
        auto tryBoolean = [](fkyaml::node const& n, char const* key, bool& out)
        {
            if (n.contains(key)) out = n[key].get_value<bool>();
        };
        auto tryString = [](fkyaml::node const& n, char const* key, std::string& out)
        {
            if (n.contains(key)) out = n[key].get_value<std::string>();
        };

        tryBoolean(mmapsNode, "skipLiquid", _skipLiquid);
        tryBoolean(mmapsNode, "skipContinents", _skipContinents);
        tryBoolean(mmapsNode, "skipJunkMaps", _skipJunkMaps);
        tryBoolean(mmapsNode, "skipBattlegrounds", _skipBattlegrounds);
        tryBoolean(mmapsNode, "debugOutput", _debugOutput);

        if (mmapsNode.contains("offmeshConnections") && mmapsNode["offmeshConnections"].is_sequence())
        {
            _offmeshConnections = mmapsNode["offmeshConnections"].get_value<std::vector<std::string>>();
        }
        else
        {
            _offmeshConnections.clear();
        }

        std::string dataDirPath;
        tryString(mmapsNode, "dataDir", dataDirPath);
        _dataDir = dataDirPath;

        mmapsNode = mmapsNode["meshSettings"];

        // Global config
        tryFloat(mmapsNode, "walkableSlopeAngle", _global.walkableSlopeAngle);
        tryInt(mmapsNode, "walkableHeight", _global.walkableHeight);
        tryInt(mmapsNode, "walkableClimb", _global.walkableClimb);
        tryInt(mmapsNode, "walkableRadius", _global.walkableRadius);
        tryInt(mmapsNode, "verticesPerMapEdge", _global.vertexPerMapEdge);
        tryInt(mmapsNode, "verticesPerTileEdge", _global.vertexPerTileEdge);
        tryFloat(mmapsNode, "maxSimplificationError", _global.maxSimplificationError);

        // Map overrides
        if (mmapsNode.contains("mapsOverrides"))
        {
            fkyaml::node maps = mmapsNode["mapsOverrides"];
            for (auto const& mapEntry : maps.as_map())
            {
                uint32 mapId = std::stoi(mapEntry.first.as_str());

                MapOverride override;
                fkyaml::node mapNode = mapEntry.second;

                if (mapNode.contains("navmeshExclusions"))
                {
                    auto const& exclusions = mapNode["navmeshExclusions"];
                    if (!exclusions.is_sequence())
                        return false;

                    for (auto const& exclusionNode : exclusions.as_seq())
                    {
                        if (!exclusionNode.is_mapping() || !exclusionNode.contains("minHeight") ||
                            !exclusionNode.contains("maxHeight") ||
                            !exclusionNode.contains("vertices") || !exclusionNode["vertices"].is_sequence())
                            return false;

                        auto isNumber = [](fkyaml::node const& node)
                        {
                            return node.is_integer() || node.is_float_number();
                        };
                        if (!isNumber(exclusionNode["minHeight"]) || !isNumber(exclusionNode["maxHeight"]))
                            return false;

                        float minHeight = exclusionNode["minHeight"].get_value<float>();
                        float maxHeight = exclusionNode["maxHeight"].get_value<float>();
                        auto const& points = exclusionNode["vertices"].as_seq();
                        if (!std::isfinite(minHeight) || !std::isfinite(maxHeight) || minHeight >= maxHeight ||
                            points.size() < 3)
                            return false;

                        NavmeshExclusion exclusion;
                        float limit = std::numeric_limits<float>::max();
                        exclusion.minBounds = {limit, minHeight, limit};
                        exclusion.maxBounds = {-limit, maxHeight, -limit};

                        for (auto const& point : points)
                        {
                            if (!point.is_sequence() || point.size() != 2 || !isNumber(point[0]) || !isNumber(point[1]))
                                return false;

                            float worldX = point[0].get_value<float>();
                            float worldY = point[1].get_value<float>();
                            if (!std::isfinite(worldX) || !std::isfinite(worldY))
                                return false;

                            exclusion.vertices.insert(exclusion.vertices.end(), {worldY, 0.0f, worldX});
                            exclusion.minBounds[0] = std::min(exclusion.minBounds[0], worldY);
                            exclusion.minBounds[2] = std::min(exclusion.minBounds[2], worldX);
                            exclusion.maxBounds[0] = std::max(exclusion.maxBounds[0], worldY);
                            exclusion.maxBounds[2] = std::max(exclusion.maxBounds[2], worldX);
                        }

                        // Every other vertex must be on the same side of each edge: convex, ordered polygon.
                        float winding = 0.0f;
                        for (std::size_t i = 0; i < points.size(); ++i)
                        {
                            std::size_t next = (i + 1) % points.size();
                            float dx = exclusion.vertices[next * 3] - exclusion.vertices[i * 3];
                            float dz = exclusion.vertices[next * 3 + 2] - exclusion.vertices[i * 3 + 2];
                            if (dx == 0.0f && dz == 0.0f)
                                return false;

                            for (std::size_t j = 0; j < points.size(); ++j)
                            {
                                float cross = dx * (exclusion.vertices[j * 3 + 2] - exclusion.vertices[i * 3 + 2]) -
                                    dz * (exclusion.vertices[j * 3] - exclusion.vertices[i * 3]);
                                if (cross == 0.0f)
                                    continue;

                                if (winding != 0.0f && (cross > 0.0f) != (winding > 0.0f))
                                    return false;

                                winding = cross;
                            }
                        }

                        if (winding == 0.0f)
                            return false;

                        override.navmeshExclusions.push_back(std::move(exclusion));
                    }
                }

                if (mapNode.contains("walkableSlopeAngle"))
                    override.walkableSlopeAngle = mapNode["walkableSlopeAngle"].get_value<float>();
                if (mapNode.contains("walkableRadius"))
                    override.walkableRadius = mapNode["walkableRadius"].get_value<int>();
                if (mapNode.contains("walkableHeight"))
                    override.walkableHeight = mapNode["walkableHeight"].get_value<int>();
                if (mapNode.contains("walkableClimb"))
                    override.walkableClimb = mapNode["walkableClimb"].get_value<int>();
                if (mapNode.contains("verticesPerMapEdge"))
                    override.vertexPerMapEdge = mapNode["verticesPerMapEdge"].get_value<int>();
                if (mapNode.contains("verticesPerTileEdge"))
                    override.vertexPerTileEdge = mapNode["verticesPerTileEdge"].get_value<int>();
                if (mapNode.contains("cellSizeHorizontal"))
                    override.cellSizeHorizontal = mapNode["cellSizeHorizontal"].get_value<float>();
                if (mapNode.contains("cellSizeVertical"))
                    override.cellSizeVertical = mapNode["cellSizeVertical"].get_value<float>();

                // Tile overrides
                if (mapNode.contains("tilesOverrides"))
                {
                    fkyaml::node tiles = mapNode["tilesOverrides"];
                    for (auto const& tileEntry : tiles.as_map())
                    {
                        std::string key = tileEntry.first.as_str();
                        fkyaml::node tileNode = tileEntry.second;

                        size_t comma = key.find(',');
                        if (comma == std::string::npos)
                            continue;

                        uint32 tileX = static_cast<uint32>(std::stoi(key.substr(0, comma)));
                        uint32 tileY = static_cast<uint32>(std::stoi(key.substr(comma + 1)));

                        TileOverride tileOverride;
                        if (tileNode.contains("walkableSlopeAngle"))
                            tileOverride.walkableSlopeAngle = tileNode["walkableSlopeAngle"].get_value<float>();
                        if (tileNode.contains("walkableRadius"))
                            tileOverride.walkableRadius = tileNode["walkableRadius"].get_value<int>();
                        if (tileNode.contains("walkableHeight"))
                            tileOverride.walkableHeight = tileNode["walkableHeight"].get_value<int>();
                        if (tileNode.contains("walkableClimb"))
                            tileOverride.walkableClimb = tileNode["walkableClimb"].get_value<int>();

                        override.tileOverrides[{tileX, tileY}] = std::move(tileOverride);
                    }
                }

                _maps[mapId] = std::move(override);
            }
        }

        // Resolve data dir path. Maybe we need to use an executable path instead of the current dir.
        if (isCurrentDirectory(_dataDir.string()) && !std::filesystem::exists(MapsPath()))
            if (auto execPath = std::filesystem::path(executableDirectoryPath()); std::filesystem::exists(execPath/ "maps"))
                _dataDir = execPath;

        return true;
    }
}
