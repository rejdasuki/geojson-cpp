#pragma once

#include <mapbox/geometry.hpp>
#include <mapbox/variant.hpp>
#include <rapidjson/document.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace mapbox {
namespace geojson {

struct empty {};

using point = mapbox::geometry::point<double>;
using multi_point = mapbox::geometry::multi_point<double>;
using line_string = mapbox::geometry::line_string<double>;
using linear_ring = mapbox::geometry::linear_ring<double>;
using multi_line_string = mapbox::geometry::multi_line_string<double>;
using polygon = mapbox::geometry::polygon<double>;
using multi_polygon = mapbox::geometry::multi_polygon<double>;
using geometry = mapbox::geometry::geometry<double>;
using feature = mapbox::geometry::feature<double>;
using feature_collection = mapbox::geometry::feature_collection<double>;

using geojson = mapbox::util::variant<empty, geometry, feature, feature_collection>;

using json_value = rapidjson::Value;
using error = std::runtime_error;

template <typename T>
T convert(const json_value &json);

template<>
point convert<point>(const json_value &json) {
    if (json.Size() < 2) throw error("coordinates array must have at least 2 numbers");
    return point{ json[0].GetDouble(), json[1].GetDouble() };
}

template <typename Cont>
Cont convert(const json_value &json) {
    Cont points;
    auto size = json.Size();
    points.reserve(size);

    for (rapidjson::SizeType i = 0; i < size; ++i) {
        const auto &p = convert<typename Cont::value_type>(json[i]);
        points.push_back(p);
    }
    return points;
}

template<>
geometry convert<geometry>(const json_value &json) {
    if (!json.IsObject()) throw error("Geometry must be an object");
    if (!json.HasMember("type")) throw error("Geometry must have a type property");

    const auto &type = json["type"];

    if (type == "GeometryCollection") {
        if (!json.HasMember("geometries"))
            throw error("GeometryCollection must have a geometries property");

        const auto &json_geometries = json["geometries"];

        if (!json_geometries.IsArray())
            throw error("GeometryCollection geometries property must be an array");

        throw error("GeometryCollection not yet implemented");
    }

    if (!json.HasMember("coordinates"))
        throw std::runtime_error("GeoJSON geometry must have a coordinates property");

    const auto &json_coords = json["coordinates"];
    if (!json_coords.IsArray()) throw error("coordinates property must be an array");

    if (type == "Point") {
        return geometry{ convert<point>(json_coords) };
    }
    if (type == "MultiPoint") {
        return geometry{ convert<multi_point>(json_coords) };
    }
    if (type == "LineString") {
        return geometry{ convert<line_string>(json_coords) };
    }
    if (type == "MultiLineString") {
        return geometry{ convert<multi_line_string>(json_coords) };
    }
    if (type == "Polygon") {
        return geometry{ convert<polygon>(json_coords) };
    }
    if (type == "MultiPolygon") {
        return geometry{ convert<multi_polygon>(json_coords) };
    }

    throw error(std::string(type.GetString()) + " not yet implemented");
}

template<>
feature convert<feature>(const json_value &json) {
    if (!json.IsObject()) throw error("Feature must be an object");
    if (!json.HasMember("type")) throw error("Feature must have a type property");
    if (json["type"] != "Feature") throw error("Feature type must be Feature");
    if (!json.HasMember("geometry")) throw error("Feature must have a geometry property");

    const auto &json_geometry = json["geometry"];

    if (!json_geometry.IsObject()) throw error("Feature geometry must be an object");

    const auto &geom = convert<geometry>(json_geometry);

    feature feature{ geom };
    // TODO feature properties

    return feature;
}

template<>
geojson convert<geojson>(const json_value &json) {
    if (!json.IsObject()) throw error("GeoJSON must be an object");
    if (!json.HasMember("type")) throw error("GeoJSON must have a type property");

    const auto &type = json["type"];

    if (type == "FeatureCollection") {
        if (!json.HasMember("features"))
            throw error("FeatureCollection must have features property");

        const auto &json_features = json["features"];

        if (!json_features.IsArray())
            throw error("FeatureCollection features property must be an array");

        feature_collection collection;

        const auto &size = json_features.Size();
        collection.reserve(size);

        for (rapidjson::SizeType i = 0; i < size; ++i) {
            const auto &f = convert<feature>(json_features[i]);
            collection.push_back(f);
        }

        return geojson{ collection };
    }

    throw error(std::string(type.GetString()) + " not yet implemented");
}

geojson convert(const json_value &json) {
    return convert<geojson>(json);
}

} // namespace geojson
} // namespace mapbox
