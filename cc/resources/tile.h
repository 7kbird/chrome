// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_TILE_H_
#define CC_RESOURCES_TILE_H_

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "cc/base/ref_counted_managed.h"
#include "cc/resources/managed_tile_state.h"
#include "cc/resources/picture_pile_impl.h"
#include "cc/resources/raster_mode.h"
#include "cc/resources/tile_priority.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

namespace cc {

class CC_EXPORT Tile : public RefCountedManaged<Tile> {
 public:
  enum TileRasterFlags { USE_PICTURE_ANALYSIS = 1 << 0 };

  typedef uint64 Id;

  Id id() const {
    return id_;
  }

  PicturePileImpl* picture_pile() {
    return picture_pile_.get();
  }

  const PicturePileImpl* picture_pile() const {
    return picture_pile_.get();
  }

  const TilePriority& priority(WhichTree tree) const {
    return priority_[tree];
  }

  TilePriority priority_for_tree_priority(TreePriority tree_priority) const {
    switch (tree_priority) {
      case SMOOTHNESS_TAKES_PRIORITY:
        return priority_[ACTIVE_TREE];
      case NEW_CONTENT_TAKES_PRIORITY:
        return priority_[PENDING_TREE];
      case SAME_PRIORITY_FOR_BOTH_TREES:
        return combined_priority();
      default:
        NOTREACHED();
        return TilePriority();
    }
  }

  TilePriority combined_priority() const {
    return TilePriority(priority_[ACTIVE_TREE],
                        priority_[PENDING_TREE]);
  }

  void SetPriority(WhichTree tree, const TilePriority& priority);

  void set_is_occluded(WhichTree tree, bool is_occluded) {
    is_occluded_[tree] = is_occluded;
  }

  bool is_occluded(WhichTree tree) const { return is_occluded_[tree]; }

  void set_shared(bool is_shared) { is_shared_ = is_shared; }
  bool is_shared() const { return is_shared_; }

  bool is_occluded_for_tree_priority(TreePriority tree_priority) const {
    switch (tree_priority) {
      case SMOOTHNESS_TAKES_PRIORITY:
        return is_occluded_[ACTIVE_TREE];
      case NEW_CONTENT_TAKES_PRIORITY:
        return is_occluded_[PENDING_TREE];
      case SAME_PRIORITY_FOR_BOTH_TREES:
        return is_occluded_[ACTIVE_TREE] && is_occluded_[PENDING_TREE];
      default:
        NOTREACHED();
        return false;
    }
  }

  void MarkRequiredForActivation();

  bool required_for_activation() const {
    return priority_[PENDING_TREE].required_for_activation;
  }

  bool use_picture_analysis() const {
    return !!(flags_ & USE_PICTURE_ANALYSIS);
  }

  bool NeedsRasterForMode(RasterMode mode) const {
    return !managed_state_.tile_versions[mode].IsReadyToDraw();
  }

  bool HasResources() const {
    for (int mode = 0; mode < NUM_RASTER_MODES; ++mode) {
      if (managed_state_.tile_versions[mode].has_resource())
        return true;
    }
    return false;
  }

  void AsValueInto(base::debug::TracedValue* dict) const;

  inline bool IsReadyToDraw() const {
    for (int mode = 0; mode < NUM_RASTER_MODES; ++mode) {
      if (managed_state_.tile_versions[mode].IsReadyToDraw())
        return true;
    }
    return false;
  }

  const ManagedTileState::TileVersion& GetTileVersionForDrawing() const {
    for (int mode = 0; mode < NUM_RASTER_MODES; ++mode) {
      if (managed_state_.tile_versions[mode].IsReadyToDraw())
        return managed_state_.tile_versions[mode];
    }
    return managed_state_.tile_versions[HIGH_QUALITY_RASTER_MODE];
  }

  gfx::Rect opaque_rect() const { return opaque_rect_; }
  float contents_scale() const { return contents_scale_; }
  gfx::Rect content_rect() const { return content_rect_; }

  int layer_id() const { return layer_id_; }

  int source_frame_number() const { return source_frame_number_; }

  void set_picture_pile(scoped_refptr<PicturePileImpl> pile) {
    DCHECK(pile->CanRaster(contents_scale_, content_rect_))
        << gfx::ScaleToEnclosingRect(content_rect_, 1.f / contents_scale_)
               .ToString();
    picture_pile_ = pile;
  }

  size_t GPUMemoryUsageInBytes() const;

  gfx::Size size() const { return size_; }

  RasterMode DetermineRasterModeForTree(WhichTree tree) const;
  RasterMode DetermineOverallRasterMode() const;

  // Functionality used in tests.
  RasterMode GetRasterModeForTesting() const {
    return managed_state().raster_mode;
  }
  ManagedTileState::TileVersion& GetTileVersionForTesting(RasterMode mode) {
    return managed_state_.tile_versions[mode];
  }

 private:
  friend class TileManager;
  friend class PrioritizedTileSet;
  friend class FakeTileManager;
  friend class BinComparator;
  friend class FakePictureLayerImpl;

  // Methods called by by tile manager.
  Tile(TileManager* tile_manager,
       PicturePileImpl* picture_pile,
       const gfx::Size& tile_size,
       const gfx::Rect& content_rect,
       const gfx::Rect& opaque_rect,
       float contents_scale,
       int layer_id,
       int source_frame_number,
       int flags);
  ~Tile();

  ManagedTileState& managed_state() { return managed_state_; }
  const ManagedTileState& managed_state() const { return managed_state_; }
  RasterMode DetermineRasterModeForResolution(TileResolution resolution) const;

  bool HasRasterTask() const;

  TileManager* tile_manager_;
  scoped_refptr<PicturePileImpl> picture_pile_;
  gfx::Size size_;
  gfx::Rect content_rect_;
  float contents_scale_;
  gfx::Rect opaque_rect_;
  bool is_occluded_[NUM_TREES];

  TilePriority priority_[NUM_TREES];
  ManagedTileState managed_state_;
  int layer_id_;
  int source_frame_number_;
  int flags_;
  bool is_shared_;

  Id id_;
  static Id s_next_id_;

  DISALLOW_COPY_AND_ASSIGN(Tile);
};

}  // namespace cc

#endif  // CC_RESOURCES_TILE_H_
