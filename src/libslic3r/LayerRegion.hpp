#ifndef slic3r_LayerRegion_hpp_
#define slic3r_LayerRegion_hpp_

#include "BoundingBox.hpp"
#include "ExtrusionEntityCollection.hpp"
#include "SurfaceCollection.hpp"
#include "IndexRange.hpp"
#include "libslic3r/Algorithm/RegionExpansion.hpp"


namespace Slic3r {

class Layer;
using LayerPtrs = std::vector<Layer*>;
class PrintRegion;

using ExtrusionRange = IndexRange<uint32_t>;
using ExPolygonRange = IndexRange<uint32_t>;

class LayerRegion
{
public:
    [[nodiscard]] Layer*                            layer()         { return m_layer; }
    [[nodiscard]] const Layer*                      layer() const   { return m_layer; }
    [[nodiscard]] const PrintRegion&                region() const  { return *m_region; }

    // collection of surfaces generated by slicing the original geometry
    // divided by type top/bottom/internal
    [[nodiscard]] const SurfaceCollection&          slices() const { return m_slices; }

    // Unspecified fill polygons, used for overhang detection ("ensure vertical wall thickness feature")
    // and for re-starting of infills.
    [[nodiscard]] const ExPolygons&                 fill_expolygons() const { return m_fill_expolygons; }
    // and their bounding boxes
    [[nodiscard]] const BoundingBoxes&              fill_expolygons_bboxes() const { return m_fill_expolygons_bboxes; }
    // Storage for fill regions produced for a single LayerIsland, of which infill splits into multiple islands.
    // Not used for a plain single material print with no infill modifiers.
    [[nodiscard]] const ExPolygons&                 fill_expolygons_composite() const { return m_fill_expolygons_composite; }
    // and their bounding boxes
    [[nodiscard]] const BoundingBoxes&              fill_expolygons_composite_bboxes() const { return m_fill_expolygons_composite_bboxes; }

    // collection of surfaces generated by slicing the original geometry
    // divided by type top/bottom/internal
    [[nodiscard]] const SurfaceCollection&          fill_surfaces() const { return m_fill_surfaces; }

    // collection of extrusion paths/loops filling gaps
    // These fills are generated by the perimeter generator.
    // They are not printed on their own, but they are copied to this->fills during infill generation.
    [[nodiscard]] const ExtrusionEntityCollection&  thin_fills() const { return m_thin_fills; }

    // collection of polylines representing the unsupported bridge edges
    [[nodiscard]] const Polylines&                  unsupported_bridge_edges() const { return m_unsupported_bridge_edges; }

    // ordered collection of extrusion paths/loops to build all perimeters
    // (this collection contains only ExtrusionEntityCollection objects)
    [[nodiscard]] const ExtrusionEntityCollection&  perimeters() const { return m_perimeters; }

    // ordered collection of extrusion paths to fill surfaces
    // (this collection contains only ExtrusionEntityCollection objects)
    [[nodiscard]] const ExtrusionEntityCollection&  fills() const { return m_fills; }

    Flow    flow(FlowRole role) const;
    Flow    flow(FlowRole role, double layer_height) const;
    Flow    bridging_flow(FlowRole role, bool force_thick_bridges = false) const;

    void    slices_to_fill_surfaces_clipped();
    void    prepare_fill_surfaces();
    // Produce perimeter extrusions, gap fill extrusions and fill polygons for input slices.
    void    make_perimeters(
        // Input slices for which the perimeters, gap fills and fill expolygons are to be generated.
        const SurfaceCollection                                &slices,
        // Ranges of perimeter extrusions and gap fill extrusions per suface, referencing
        // newly created extrusions stored at this LayerRegion.
        std::vector<std::pair<ExtrusionRange, ExtrusionRange>> &perimeter_and_gapfill_ranges,
        // All fill areas produced for all input slices above.
        ExPolygons                                             &fill_expolygons,
        // Ranges of fill areas above per input slice.
        std::vector<ExPolygonRange>                            &fill_expolygons_ranges);
    void    process_external_surfaces(const Layer *lower_layer, const Polygons *lower_layer_covered);
    double  infill_area_threshold() const;
    // Trim surfaces by trimming polygons. Used by the elephant foot compensation at the 1st layer.
    void    trim_surfaces(const Polygons &trimming_polygons);
    // Single elephant foot compensation step, used by the elephant foor compensation at the 1st layer.
    // Trim surfaces by trimming polygons (shrunk by an elephant foot compensation step), but don't shrink narrow parts so much that no perimeter would fit.
    void    elephant_foot_compensation_step(const float elephant_foot_compensation_perimeter_step, const Polygons &trimming_polygons);

    void    export_region_slices_to_svg(const char *path) const;
    void    export_region_fill_surfaces_to_svg(const char *path) const;
    // Export to "out/LayerRegion-name-%d.svg" with an increasing index with every export.
    void    export_region_slices_to_svg_debug(const char *name) const;
    void    export_region_fill_surfaces_to_svg_debug(const char *name) const;

    // Is there any valid extrusion assigned to this LayerRegion?
    bool    has_extrusions() const { return ! this->perimeters().empty() || ! this->fills().empty(); }

protected:
    friend class Layer;
    friend class PrintObject;

    LayerRegion(Layer *layer, const PrintRegion *region) : m_layer(layer), m_region(region) {}
    ~LayerRegion() = default;

private:
    // Modifying m_slices
    friend std::string fix_slicing_errors(LayerPtrs&, const std::function<void()>&);
    template<typename ThrowOnCancel>
    friend void apply_mm_segmentation(PrintObject& print_object, ThrowOnCancel throw_on_cancel);

    Layer                      *m_layer;
    const PrintRegion          *m_region;

    // Backed up slices before they are split into top/bottom/internal.
    // Only backed up for multi-region layers or layers with elephant foot compensation.
    //FIXME Review whether not to simplify the code by keeping the raw_slices all the time.
    ExPolygons                  m_raw_slices;

//FIXME make m_slices public for unit tests
public:
    // collection of surfaces generated by slicing the original geometry
    // divided by type top/bottom/internal
    SurfaceCollection           m_slices;

private:
    // Unspecified fill polygons, used for overhang detection ("ensure vertical wall thickness feature")
    // and for re-starting of infills.
    ExPolygons                  m_fill_expolygons;
    // and their bounding boxes
    BoundingBoxes               m_fill_expolygons_bboxes;
    // Storage for fill regions produced for a single LayerIsland, of which infill splits into multiple islands.
    // Not used for a plain single material print with no infill modifiers.
    ExPolygons                  m_fill_expolygons_composite;
    // and their bounding boxes
    BoundingBoxes               m_fill_expolygons_composite_bboxes;

    // Collection of surfaces for infill generation, created by splitting m_slices by m_fill_expolygons.
    SurfaceCollection           m_fill_surfaces;

    // Collection of extrusion paths/loops filling gaps
    // These fills are generated by the perimeter generator.
    // They are not printed on their own, but they are copied to this->fills during infill generation.
    ExtrusionEntityCollection   m_thin_fills;

    // collection of polylines representing the unsupported bridge edges
    Polylines                   m_unsupported_bridge_edges;

    // ordered collection of extrusion paths/loops to build all perimeters
    // (this collection contains only ExtrusionEntityCollection objects)
    ExtrusionEntityCollection   m_perimeters;

    // ordered collection of extrusion paths to fill surfaces
    // (this collection contains only ExtrusionEntityCollection objects)
    ExtrusionEntityCollection   m_fills;

    // collection of expolygons representing the bridged areas (thus not
    // needing support material)
//  Polygons                    bridged;
};

struct ExpansionZone {
    ExPolygons expolygons;
    Algorithm::RegionExpansionParameters parameters;
    bool expanded_into = false;
};

/**
* Extract bridging surfaces from "surfaces", expand them into "shells" using expansion_params,
* detect bridges.
* Trim "shells" by the expanded bridges.
*/
Surfaces expand_bridges_detect_orientations(
    Surfaces &surfaces,
    std::vector<ExpansionZone>& expansion_zones,
    const float closing_radius
);

}

#endif // slic3r_LayerRegion_hpp_
