// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/output/compositor_frame.h"
#include "cc/output/delegated_frame_data.h"
#include "cc/quads/render_pass.h"
#include "cc/quads/render_pass_draw_quad.h"
#include "cc/quads/solid_color_draw_quad.h"
#include "cc/quads/surface_draw_quad.h"
#include "cc/quads/texture_draw_quad.h"
#include "cc/resources/shared_bitmap_manager.h"
#include "cc/surfaces/surface.h"
#include "cc/surfaces/surface_aggregator.h"
#include "cc/surfaces/surface_aggregator_test_helpers.h"
#include "cc/surfaces/surface_factory.h"
#include "cc/surfaces/surface_factory_client.h"
#include "cc/surfaces/surface_id_allocator.h"
#include "cc/surfaces/surface_manager.h"
#include "cc/test/fake_output_surface.h"
#include "cc/test/fake_output_surface_client.h"
#include "cc/test/render_pass_test_common.h"
#include "cc/test/render_pass_test_utils.h"
#include "cc/test/test_shared_bitmap_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

namespace cc {
namespace {

SurfaceId InvalidSurfaceId() {
  static SurfaceId invalid;
  invalid.id = static_cast<uint64_t>(-1);
  return invalid;
}

gfx::Size SurfaceSize() {
  static gfx::Size size(5, 5);
  return size;
}

class EmptySurfaceFactoryClient : public SurfaceFactoryClient {
 public:
  virtual void ReturnResources(
      const ReturnedResourceArray& resources) OVERRIDE {}
};

class SurfaceAggregatorTest : public testing::Test {
 public:
  SurfaceAggregatorTest()
      : factory_(&manager_, &empty_client_), aggregator_(&manager_, NULL) {}

 protected:
  SurfaceManager manager_;
  EmptySurfaceFactoryClient empty_client_;
  SurfaceFactory factory_;
  SurfaceAggregator aggregator_;
};

TEST_F(SurfaceAggregatorTest, ValidSurfaceNoFrame) {
  SurfaceId one_id(7);
  factory_.Create(one_id, SurfaceSize());
  std::set<SurfaceId> surface_set;
  scoped_ptr<CompositorFrame> frame =
      aggregator_.Aggregate(one_id, &surface_set);
  EXPECT_FALSE(frame);
  factory_.Destroy(one_id);
}

class SurfaceAggregatorValidSurfaceTest : public SurfaceAggregatorTest {
 public:
  SurfaceAggregatorValidSurfaceTest() : allocator_(1u) {}

  virtual void SetUp() {
    SurfaceAggregatorTest::SetUp();
    root_surface_id_ = allocator_.GenerateId();
    factory_.Create(root_surface_id_, SurfaceSize());
  }

  virtual void TearDown() {
    factory_.Destroy(root_surface_id_);
    SurfaceAggregatorTest::TearDown();
  }

  void AggregateAndVerify(test::Pass* expected_passes,
                          size_t expected_pass_count,
                          SurfaceId* surface_ids,
                          size_t expected_surface_count) {
    std::set<SurfaceId> surface_set;
    scoped_ptr<CompositorFrame> aggregated_frame =
        aggregator_.Aggregate(root_surface_id_, &surface_set);

    ASSERT_TRUE(aggregated_frame);
    ASSERT_TRUE(aggregated_frame->delegated_frame_data);

    DelegatedFrameData* frame_data =
        aggregated_frame->delegated_frame_data.get();

    TestPassesMatchExpectations(
        expected_passes, expected_pass_count, &frame_data->render_pass_list);

    EXPECT_EQ(expected_surface_count, surface_set.size());
    for (size_t i = 0; i < expected_surface_count; i++) {
      EXPECT_TRUE(surface_set.find(surface_ids[i]) != surface_set.end());
    }
  }

  void SubmitFrame(test::Pass* passes,
                   size_t pass_count,
                   SurfaceId surface_id) {
    RenderPassList pass_list;
    AddPasses(&pass_list, gfx::Rect(SurfaceSize()), passes, pass_count);

    scoped_ptr<DelegatedFrameData> frame_data(new DelegatedFrameData);
    pass_list.swap(frame_data->render_pass_list);

    scoped_ptr<CompositorFrame> frame(new CompositorFrame);
    frame->delegated_frame_data = frame_data.Pass();

    factory_.SubmitFrame(surface_id, frame.Pass());
  }

  void QueuePassAsFrame(scoped_ptr<RenderPass> pass, SurfaceId surface_id) {
    scoped_ptr<DelegatedFrameData> delegated_frame_data(new DelegatedFrameData);
    delegated_frame_data->render_pass_list.push_back(pass.Pass());

    scoped_ptr<CompositorFrame> child_frame(new CompositorFrame);
    child_frame->delegated_frame_data = delegated_frame_data.Pass();

    factory_.SubmitFrame(surface_id, child_frame.Pass());
  }

 protected:
  SurfaceId root_surface_id_;
  SurfaceIdAllocator allocator_;
};

// Tests that a very simple frame containing only two solid color quads makes it
// through the aggregator correctly.
TEST_F(SurfaceAggregatorValidSurfaceTest, SimpleFrame) {
  test::Quad quads[] = {test::Quad::SolidColorQuad(SK_ColorRED),
                        test::Quad::SolidColorQuad(SK_ColorBLUE)};
  test::Pass passes[] = {test::Pass(quads, arraysize(quads))};

  SubmitFrame(passes, arraysize(passes), root_surface_id_);

  SurfaceId ids[] = {root_surface_id_};
  AggregateAndVerify(passes, arraysize(passes), ids, arraysize(ids));
}

TEST_F(SurfaceAggregatorValidSurfaceTest, MultiPassSimpleFrame) {
  test::Quad quads[][2] = {{test::Quad::SolidColorQuad(SK_ColorWHITE),
                            test::Quad::SolidColorQuad(SK_ColorLTGRAY)},
                           {test::Quad::SolidColorQuad(SK_ColorGRAY),
                            test::Quad::SolidColorQuad(SK_ColorDKGRAY)}};
  test::Pass passes[] = {test::Pass(quads[0], arraysize(quads[0])),
                         test::Pass(quads[1], arraysize(quads[1]))};

  SubmitFrame(passes, arraysize(passes), root_surface_id_);

  SurfaceId ids[] = {root_surface_id_};
  AggregateAndVerify(passes, arraysize(passes), ids, arraysize(ids));
}

// This tests very simple embedding. root_surface has a frame containing a few
// solid color quads and a surface quad referencing embedded_surface.
// embedded_surface has a frame containing only a solid color quad. The solid
// color quad should be aggregated into the final frame.
TEST_F(SurfaceAggregatorValidSurfaceTest, SimpleSurfaceReference) {
  SurfaceId embedded_surface_id = allocator_.GenerateId();
  factory_.Create(embedded_surface_id, SurfaceSize());

  test::Quad embedded_quads[] = {test::Quad::SolidColorQuad(SK_ColorGREEN)};
  test::Pass embedded_passes[] = {
      test::Pass(embedded_quads, arraysize(embedded_quads))};

  SubmitFrame(embedded_passes, arraysize(embedded_passes), embedded_surface_id);

  test::Quad root_quads[] = {test::Quad::SolidColorQuad(SK_ColorWHITE),
                             test::Quad::SurfaceQuad(embedded_surface_id),
                             test::Quad::SolidColorQuad(SK_ColorBLACK)};
  test::Pass root_passes[] = {test::Pass(root_quads, arraysize(root_quads))};

  SubmitFrame(root_passes, arraysize(root_passes), root_surface_id_);

  test::Quad expected_quads[] = {test::Quad::SolidColorQuad(SK_ColorWHITE),
                                 test::Quad::SolidColorQuad(SK_ColorGREEN),
                                 test::Quad::SolidColorQuad(SK_ColorBLACK)};
  test::Pass expected_passes[] = {
      test::Pass(expected_quads, arraysize(expected_quads))};
  SurfaceId ids[] = {root_surface_id_, embedded_surface_id};
  AggregateAndVerify(
      expected_passes, arraysize(expected_passes), ids, arraysize(ids));

  factory_.Destroy(embedded_surface_id);
}

// This tests referencing a surface that has multiple render passes.
TEST_F(SurfaceAggregatorValidSurfaceTest, MultiPassSurfaceReference) {
  SurfaceId embedded_surface_id = allocator_.GenerateId();
  factory_.Create(embedded_surface_id, SurfaceSize());

  RenderPassId pass_ids[] = {RenderPassId(1, 1), RenderPassId(1, 2),
                             RenderPassId(1, 3)};

  test::Quad embedded_quads[][2] = {
      {test::Quad::SolidColorQuad(1), test::Quad::SolidColorQuad(2)},
      {test::Quad::SolidColorQuad(3), test::Quad::RenderPassQuad(pass_ids[0])},
      {test::Quad::SolidColorQuad(4), test::Quad::RenderPassQuad(pass_ids[1])}};
  test::Pass embedded_passes[] = {
      test::Pass(embedded_quads[0], arraysize(embedded_quads[0]), pass_ids[0]),
      test::Pass(embedded_quads[1], arraysize(embedded_quads[1]), pass_ids[1]),
      test::Pass(embedded_quads[2], arraysize(embedded_quads[2]), pass_ids[2])};

  SubmitFrame(embedded_passes, arraysize(embedded_passes), embedded_surface_id);

  test::Quad root_quads[][2] = {
      {test::Quad::SolidColorQuad(5), test::Quad::SolidColorQuad(6)},
      {test::Quad::SurfaceQuad(embedded_surface_id),
       test::Quad::RenderPassQuad(pass_ids[0])},
      {test::Quad::SolidColorQuad(7), test::Quad::RenderPassQuad(pass_ids[1])}};
  test::Pass root_passes[] = {
      test::Pass(root_quads[0], arraysize(root_quads[0]), pass_ids[0]),
      test::Pass(root_quads[1], arraysize(root_quads[1]), pass_ids[1]),
      test::Pass(root_quads[2], arraysize(root_quads[2]), pass_ids[2])};

  SubmitFrame(root_passes, arraysize(root_passes), root_surface_id_);

  std::set<SurfaceId> surface_set;
  scoped_ptr<CompositorFrame> aggregated_frame =
      aggregator_.Aggregate(root_surface_id_, &surface_set);

  ASSERT_TRUE(aggregated_frame);
  ASSERT_TRUE(aggregated_frame->delegated_frame_data);

  DelegatedFrameData* frame_data = aggregated_frame->delegated_frame_data.get();

  const RenderPassList& aggregated_pass_list = frame_data->render_pass_list;

  ASSERT_EQ(5u, aggregated_pass_list.size());
  RenderPassId actual_pass_ids[] = {
      aggregated_pass_list[0]->id, aggregated_pass_list[1]->id,
      aggregated_pass_list[2]->id, aggregated_pass_list[3]->id,
      aggregated_pass_list[4]->id};
  for (size_t i = 0; i < 5; ++i) {
    for (size_t j = 0; j < i; ++j) {
      EXPECT_NE(actual_pass_ids[i], actual_pass_ids[j]);
    }
  }

  {
    SCOPED_TRACE("First pass");
    // The first pass will just be the first pass from the root surfaces quad
    // with no render pass quads to remap.
    TestPassMatchesExpectations(root_passes[0], aggregated_pass_list[0]);
  }

  {
    SCOPED_TRACE("Second pass");
    // The next two passes will be from the embedded surface since we have to
    // draw those passes before they are referenced from the render pass draw
    // quad embedded into the root surface's second pass.
    // First, there's the first embedded pass which doesn't reference anything
    // else.
    TestPassMatchesExpectations(embedded_passes[0], aggregated_pass_list[1]);
  }

  {
    SCOPED_TRACE("Third pass");
    const QuadList& third_pass_quad_list = aggregated_pass_list[2]->quad_list;
    ASSERT_EQ(2u, third_pass_quad_list.size());
    TestQuadMatchesExpectations(embedded_quads[1][0],
                                third_pass_quad_list.at(0u));

    // This render pass pass quad will reference the first pass from the
    // embedded surface, which is the second pass in the aggregated frame.
    ASSERT_EQ(DrawQuad::RENDER_PASS, third_pass_quad_list.at(1u)->material);
    const RenderPassDrawQuad* third_pass_render_pass_draw_quad =
        RenderPassDrawQuad::MaterialCast(third_pass_quad_list.at(1u));
    EXPECT_EQ(actual_pass_ids[1],
              third_pass_render_pass_draw_quad->render_pass_id);
  }

  {
    SCOPED_TRACE("Fourth pass");
    // The fourth pass will have aggregated quads from the root surface's second
    // pass and the embedded surface's first pass.
    const QuadList& fourth_pass_quad_list = aggregated_pass_list[3]->quad_list;
    ASSERT_EQ(3u, fourth_pass_quad_list.size());

    // The first quad will be the yellow quad from the embedded surface's last
    // pass.
    TestQuadMatchesExpectations(embedded_quads[2][0],
                                fourth_pass_quad_list.at(0u));

    // The next quad will be a render pass quad referencing the second pass from
    // the embedded surface, which is the third pass in the aggregated frame.
    ASSERT_EQ(DrawQuad::RENDER_PASS, fourth_pass_quad_list.at(1u)->material);
    const RenderPassDrawQuad* fourth_pass_first_render_pass_draw_quad =
        RenderPassDrawQuad::MaterialCast(fourth_pass_quad_list.at(1u));
    EXPECT_EQ(actual_pass_ids[2],
              fourth_pass_first_render_pass_draw_quad->render_pass_id);

    // The last quad will be a render pass quad referencing the first pass from
    // the root surface, which is the first pass overall.
    ASSERT_EQ(DrawQuad::RENDER_PASS, fourth_pass_quad_list.at(2u)->material);
    const RenderPassDrawQuad* fourth_pass_second_render_pass_draw_quad =
        RenderPassDrawQuad::MaterialCast(fourth_pass_quad_list.at(2u));
    EXPECT_EQ(actual_pass_ids[0],
              fourth_pass_second_render_pass_draw_quad->render_pass_id);
  }

  {
    SCOPED_TRACE("Fifth pass");
    const QuadList& fifth_pass_quad_list = aggregated_pass_list[4]->quad_list;
    ASSERT_EQ(2u, fifth_pass_quad_list.size());

    TestQuadMatchesExpectations(root_quads[2][0], fifth_pass_quad_list.at(0));

    // The last quad in the last pass will reference the second pass from the
    // root surface, which after aggregating is the fourth pass in the overall
    // list.
    ASSERT_EQ(DrawQuad::RENDER_PASS, fifth_pass_quad_list.at(1u)->material);
    const RenderPassDrawQuad* fifth_pass_render_pass_draw_quad =
        RenderPassDrawQuad::MaterialCast(fifth_pass_quad_list.at(1u));
    EXPECT_EQ(actual_pass_ids[3],
              fifth_pass_render_pass_draw_quad->render_pass_id);
  }
  factory_.Destroy(embedded_surface_id);
}

// Tests an invalid surface reference in a frame. The surface quad should just
// be dropped.
TEST_F(SurfaceAggregatorValidSurfaceTest, InvalidSurfaceReference) {
  test::Quad quads[] = {test::Quad::SolidColorQuad(SK_ColorGREEN),
                        test::Quad::SurfaceQuad(InvalidSurfaceId()),
                        test::Quad::SolidColorQuad(SK_ColorBLUE)};
  test::Pass passes[] = {test::Pass(quads, arraysize(quads))};

  SubmitFrame(passes, arraysize(passes), root_surface_id_);

  test::Quad expected_quads[] = {test::Quad::SolidColorQuad(SK_ColorGREEN),
                                 test::Quad::SolidColorQuad(SK_ColorBLUE)};
  test::Pass expected_passes[] = {
      test::Pass(expected_quads, arraysize(expected_quads))};
  SurfaceId ids[] = {root_surface_id_, InvalidSurfaceId()};
  AggregateAndVerify(
      expected_passes, arraysize(expected_passes), ids, arraysize(ids));
}

// Tests a reference to a valid surface with no submitted frame. This quad
// should also just be dropped.
TEST_F(SurfaceAggregatorValidSurfaceTest, ValidSurfaceReferenceWithNoFrame) {
  SurfaceId surface_with_no_frame_id = allocator_.GenerateId();
  factory_.Create(surface_with_no_frame_id, gfx::Size(5, 5));
  test::Quad quads[] = {test::Quad::SolidColorQuad(SK_ColorGREEN),
                        test::Quad::SurfaceQuad(surface_with_no_frame_id),
                        test::Quad::SolidColorQuad(SK_ColorBLUE)};
  test::Pass passes[] = {test::Pass(quads, arraysize(quads))};

  SubmitFrame(passes, arraysize(passes), root_surface_id_);

  test::Quad expected_quads[] = {test::Quad::SolidColorQuad(SK_ColorGREEN),
                                 test::Quad::SolidColorQuad(SK_ColorBLUE)};
  test::Pass expected_passes[] = {
      test::Pass(expected_quads, arraysize(expected_quads))};
  SurfaceId ids[] = {root_surface_id_, surface_with_no_frame_id};
  AggregateAndVerify(
      expected_passes, arraysize(expected_passes), ids, arraysize(ids));
  factory_.Destroy(surface_with_no_frame_id);
}

// Tests a surface quad referencing itself, generating a trivial cycle.
// The quad creating the cycle should be dropped from the final frame.
TEST_F(SurfaceAggregatorValidSurfaceTest, SimpleCyclicalReference) {
  test::Quad quads[] = {test::Quad::SurfaceQuad(root_surface_id_),
                        test::Quad::SolidColorQuad(SK_ColorYELLOW)};
  test::Pass passes[] = {test::Pass(quads, arraysize(quads))};

  SubmitFrame(passes, arraysize(passes), root_surface_id_);

  test::Quad expected_quads[] = {test::Quad::SolidColorQuad(SK_ColorYELLOW)};
  test::Pass expected_passes[] = {
      test::Pass(expected_quads, arraysize(expected_quads))};
  SurfaceId ids[] = {root_surface_id_};
  AggregateAndVerify(
      expected_passes, arraysize(expected_passes), ids, arraysize(ids));
}

// Tests a more complex cycle with one intermediate surface.
TEST_F(SurfaceAggregatorValidSurfaceTest, TwoSurfaceCyclicalReference) {
  SurfaceId child_surface_id = allocator_.GenerateId();
  factory_.Create(child_surface_id, SurfaceSize());

  test::Quad parent_quads[] = {test::Quad::SolidColorQuad(SK_ColorBLUE),
                               test::Quad::SurfaceQuad(child_surface_id),
                               test::Quad::SolidColorQuad(SK_ColorCYAN)};
  test::Pass parent_passes[] = {
      test::Pass(parent_quads, arraysize(parent_quads))};

  SubmitFrame(parent_passes, arraysize(parent_passes), root_surface_id_);

  test::Quad child_quads[] = {test::Quad::SolidColorQuad(SK_ColorGREEN),
                              test::Quad::SurfaceQuad(root_surface_id_),
                              test::Quad::SolidColorQuad(SK_ColorMAGENTA)};
  test::Pass child_passes[] = {test::Pass(child_quads, arraysize(child_quads))};

  SubmitFrame(child_passes, arraysize(child_passes), child_surface_id);

  // The child surface's reference to the root_surface_ will be dropped, so
  // we'll end up with:
  //   SK_ColorBLUE from the parent
  //   SK_ColorGREEN from the child
  //   SK_ColorMAGENTA from the child
  //   SK_ColorCYAN from the parent
  test::Quad expected_quads[] = {test::Quad::SolidColorQuad(SK_ColorBLUE),
                                 test::Quad::SolidColorQuad(SK_ColorGREEN),
                                 test::Quad::SolidColorQuad(SK_ColorMAGENTA),
                                 test::Quad::SolidColorQuad(SK_ColorCYAN)};
  test::Pass expected_passes[] = {
      test::Pass(expected_quads, arraysize(expected_quads))};
  SurfaceId ids[] = {root_surface_id_, child_surface_id};
  AggregateAndVerify(
      expected_passes, arraysize(expected_passes), ids, arraysize(ids));
  factory_.Destroy(child_surface_id);
}

// Tests that we map render pass IDs from different surfaces into a unified
// namespace and update RenderPassDrawQuad's id references to match.
TEST_F(SurfaceAggregatorValidSurfaceTest, RenderPassIdMapping) {
  SurfaceId child_surface_id = allocator_.GenerateId();
  factory_.Create(child_surface_id, SurfaceSize());

  RenderPassId child_pass_id[] = {RenderPassId(1, 1), RenderPassId(1, 2)};
  test::Quad child_quad[][1] = {{test::Quad::SolidColorQuad(SK_ColorGREEN)},
                                {test::Quad::RenderPassQuad(child_pass_id[0])}};
  test::Pass surface_passes[] = {
      test::Pass(child_quad[0], arraysize(child_quad[0]), child_pass_id[0]),
      test::Pass(child_quad[1], arraysize(child_quad[1]), child_pass_id[1])};

  SubmitFrame(surface_passes, arraysize(surface_passes), child_surface_id);

  // Pass IDs from the parent surface may collide with ones from the child.
  RenderPassId parent_pass_id[] = {RenderPassId(2, 1), RenderPassId(1, 2)};
  test::Quad parent_quad[][1] = {
      {test::Quad::SurfaceQuad(child_surface_id)},
      {test::Quad::RenderPassQuad(parent_pass_id[0])}};
  test::Pass parent_passes[] = {
      test::Pass(parent_quad[0], arraysize(parent_quad[0]), parent_pass_id[0]),
      test::Pass(parent_quad[1], arraysize(parent_quad[1]), parent_pass_id[1])};

  SubmitFrame(parent_passes, arraysize(parent_passes), root_surface_id_);
  std::set<SurfaceId> surface_set;
  scoped_ptr<CompositorFrame> aggregated_frame =
      aggregator_.Aggregate(root_surface_id_, &surface_set);

  ASSERT_TRUE(aggregated_frame);
  ASSERT_TRUE(aggregated_frame->delegated_frame_data);

  DelegatedFrameData* frame_data = aggregated_frame->delegated_frame_data.get();

  const RenderPassList& aggregated_pass_list = frame_data->render_pass_list;

  ASSERT_EQ(3u, aggregated_pass_list.size());
  RenderPassId actual_pass_ids[] = {aggregated_pass_list[0]->id,
                                    aggregated_pass_list[1]->id,
                                    aggregated_pass_list[2]->id};
  // Make sure the aggregated frame's pass IDs are all unique.
  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < i; ++j) {
      EXPECT_NE(actual_pass_ids[j], actual_pass_ids[i]) << "pass ids " << i
                                                        << " and " << j;
    }
  }

  // Make sure the render pass quads reference the remapped pass IDs.
  DrawQuad* render_pass_quads[] = {aggregated_pass_list[1]->quad_list[0],
                                   aggregated_pass_list[2]->quad_list[0]};
  ASSERT_EQ(render_pass_quads[0]->material, DrawQuad::RENDER_PASS);
  EXPECT_EQ(
      actual_pass_ids[0],
      RenderPassDrawQuad::MaterialCast(render_pass_quads[0])->render_pass_id);

  ASSERT_EQ(render_pass_quads[1]->material, DrawQuad::RENDER_PASS);
  EXPECT_EQ(
      actual_pass_ids[1],
      RenderPassDrawQuad::MaterialCast(render_pass_quads[1])->render_pass_id);
  factory_.Destroy(child_surface_id);
}

void AddSolidColorQuadWithBlendMode(const gfx::Size& size,
                                    RenderPass* pass,
                                    const SkXfermode::Mode blend_mode) {
  const gfx::Transform content_to_target_transform;
  const gfx::Size content_bounds(size);
  const gfx::Rect visible_content_rect(size);
  const gfx::Rect clip_rect(size);

  bool is_clipped = false;
  float opacity = 1.f;

  bool force_anti_aliasing_off = false;
  SharedQuadState* sqs = pass->CreateAndAppendSharedQuadState();
  sqs->SetAll(content_to_target_transform,
              content_bounds,
              visible_content_rect,
              clip_rect,
              is_clipped,
              opacity,
              blend_mode,
              0);

  SolidColorDrawQuad* color_quad =
      pass->CreateAndAppendDrawQuad<SolidColorDrawQuad>();
  color_quad->SetNew(pass->shared_quad_state_list.back(),
                     visible_content_rect,
                     visible_content_rect,
                     SK_ColorGREEN,
                     force_anti_aliasing_off);
}

// This tests that we update shared quad state pointers correctly within
// aggregated passes.  The shared quad state list on the aggregated pass will
// include the shared quad states from each pass in one list so the quads will
// end up pointed to shared quad state objects at different offsets. This test
// uses the blend_mode value stored on the shared quad state to track the shared
// quad state, but anything saved on the shared quad state would work.
//
// This test has 4 surfaces in the following structure:
// root_surface -> quad with kClear_Mode,
//                 [child_one_surface],
//                 quad with kDstOver_Mode,
//                 [child_two_surface],
//                 quad with kDstIn_Mode
// child_one_surface -> quad with kSrc_Mode,
//                      [grandchild_surface],
//                      quad with kSrcOver_Mode
// child_two_surface -> quad with kSrcIn_Mode
// grandchild_surface -> quad with kDst_Mode
//
// Resulting in the following aggregated pass:
//  quad_root_0       - blend_mode kClear_Mode
//  quad_child_one_0  - blend_mode kSrc_Mode
//  quad_grandchild_0 - blend_mode kDst_Mode
//  quad_child_one_1  - blend_mode kSrcOver_Mode
//  quad_root_1       - blend_mode kDstOver_Mode
//  quad_child_two_0  - blend_mode kSrcIn_Mode
//  quad_root_2       - blend_mode kDstIn_Mode
TEST_F(SurfaceAggregatorValidSurfaceTest, AggregateSharedQuadStateProperties) {
  const SkXfermode::Mode blend_modes[] = {SkXfermode::kClear_Mode,    // 0
                                          SkXfermode::kSrc_Mode,      // 1
                                          SkXfermode::kDst_Mode,      // 2
                                          SkXfermode::kSrcOver_Mode,  // 3
                                          SkXfermode::kDstOver_Mode,  // 4
                                          SkXfermode::kSrcIn_Mode,    // 5
                                          SkXfermode::kDstIn_Mode,    // 6
  };

  RenderPassId pass_id(1, 1);
  SurfaceId grandchild_surface_id = allocator_.GenerateId();
  factory_.Create(grandchild_surface_id, SurfaceSize());
  scoped_ptr<RenderPass> grandchild_pass = RenderPass::Create();
  gfx::Rect output_rect(SurfaceSize());
  gfx::Rect damage_rect(SurfaceSize());
  gfx::Transform transform_to_root_target;
  grandchild_pass->SetNew(
      pass_id, output_rect, damage_rect, transform_to_root_target);
  AddSolidColorQuadWithBlendMode(
      SurfaceSize(), grandchild_pass.get(), blend_modes[2]);
  QueuePassAsFrame(grandchild_pass.Pass(), grandchild_surface_id);

  SurfaceId child_one_surface_id = allocator_.GenerateId();
  factory_.Create(child_one_surface_id, SurfaceSize());

  scoped_ptr<RenderPass> child_one_pass = RenderPass::Create();
  child_one_pass->SetNew(
      pass_id, output_rect, damage_rect, transform_to_root_target);
  AddSolidColorQuadWithBlendMode(
      SurfaceSize(), child_one_pass.get(), blend_modes[1]);
  SurfaceDrawQuad* grandchild_surface_quad =
      child_one_pass->CreateAndAppendDrawQuad<SurfaceDrawQuad>();
  grandchild_surface_quad->SetNew(child_one_pass->shared_quad_state_list.back(),
                                  gfx::Rect(SurfaceSize()),
                                  gfx::Rect(SurfaceSize()),
                                  grandchild_surface_id);
  AddSolidColorQuadWithBlendMode(
      SurfaceSize(), child_one_pass.get(), blend_modes[3]);
  QueuePassAsFrame(child_one_pass.Pass(), child_one_surface_id);

  SurfaceId child_two_surface_id = allocator_.GenerateId();
  factory_.Create(child_two_surface_id, SurfaceSize());

  scoped_ptr<RenderPass> child_two_pass = RenderPass::Create();
  child_two_pass->SetNew(
      pass_id, output_rect, damage_rect, transform_to_root_target);
  AddSolidColorQuadWithBlendMode(
      SurfaceSize(), child_two_pass.get(), blend_modes[5]);
  QueuePassAsFrame(child_two_pass.Pass(), child_two_surface_id);

  scoped_ptr<RenderPass> root_pass = RenderPass::Create();
  root_pass->SetNew(
      pass_id, output_rect, damage_rect, transform_to_root_target);

  AddSolidColorQuadWithBlendMode(
      SurfaceSize(), root_pass.get(), blend_modes[0]);
  SurfaceDrawQuad* child_one_surface_quad =
      root_pass->CreateAndAppendDrawQuad<SurfaceDrawQuad>();
  child_one_surface_quad->SetNew(root_pass->shared_quad_state_list.back(),
                                 gfx::Rect(SurfaceSize()),
                                 gfx::Rect(SurfaceSize()),
                                 child_one_surface_id);
  AddSolidColorQuadWithBlendMode(
      SurfaceSize(), root_pass.get(), blend_modes[4]);
  SurfaceDrawQuad* child_two_surface_quad =
      root_pass->CreateAndAppendDrawQuad<SurfaceDrawQuad>();
  child_two_surface_quad->SetNew(root_pass->shared_quad_state_list.back(),
                                 gfx::Rect(SurfaceSize()),
                                 gfx::Rect(SurfaceSize()),
                                 child_two_surface_id);
  AddSolidColorQuadWithBlendMode(
      SurfaceSize(), root_pass.get(), blend_modes[6]);

  QueuePassAsFrame(root_pass.Pass(), root_surface_id_);

  std::set<SurfaceId> surface_set;
  scoped_ptr<CompositorFrame> aggregated_frame =
      aggregator_.Aggregate(root_surface_id_, &surface_set);

  ASSERT_TRUE(aggregated_frame);
  ASSERT_TRUE(aggregated_frame->delegated_frame_data);

  DelegatedFrameData* frame_data = aggregated_frame->delegated_frame_data.get();

  const RenderPassList& aggregated_pass_list = frame_data->render_pass_list;

  ASSERT_EQ(1u, aggregated_pass_list.size());

  const QuadList& aggregated_quad_list = aggregated_pass_list[0]->quad_list;

  ASSERT_EQ(7u, aggregated_quad_list.size());

  for (size_t i = 0; i < aggregated_quad_list.size(); ++i) {
    DrawQuad* quad = aggregated_quad_list[i];
    EXPECT_EQ(blend_modes[i], quad->shared_quad_state->blend_mode) << i;
  }
  factory_.Destroy(child_one_surface_id);
  factory_.Destroy(child_two_surface_id);
  factory_.Destroy(grandchild_surface_id);
}

// This tests that when aggregating a frame with multiple render passes that we
// map the transforms for the root pass but do not modify the transform on child
// passes.
//
// The root surface has one pass with a surface quad transformed by +10 in the y
// direction.
//
// The child surface has two passes. The first pass has a quad with a transform
// of +5 in the x direction. The second pass has a reference to the first pass'
// pass id and a transform of +8 in the x direction.
//
// After aggregation, the child surface's root pass quad should have both
// transforms concatenated for a total transform of +8 x, +10 y. The
// contributing render pass' transform in the aggregate frame should not be
// affected.
TEST_F(SurfaceAggregatorValidSurfaceTest, AggregateMultiplePassWithTransform) {
  SurfaceId child_surface_id = allocator_.GenerateId();
  factory_.Create(child_surface_id, SurfaceSize());
  RenderPassId child_pass_id[] = {RenderPassId(1, 1), RenderPassId(1, 2)};
  test::Quad child_quads[][1] = {
      {test::Quad::SolidColorQuad(SK_ColorGREEN)},
      {test::Quad::RenderPassQuad(child_pass_id[0])}};
  test::Pass child_passes[] = {
      test::Pass(child_quads[0], arraysize(child_quads[0]), child_pass_id[0]),
      test::Pass(child_quads[1], arraysize(child_quads[1]), child_pass_id[1])};

  RenderPassList child_pass_list;
  AddPasses(&child_pass_list,
            gfx::Rect(SurfaceSize()),
            child_passes,
            arraysize(child_passes));

  RenderPass* child_nonroot_pass = child_pass_list.at(0u);
  child_nonroot_pass->transform_to_root_target.Translate(8, 0);
  SharedQuadState* child_nonroot_pass_sqs =
      child_nonroot_pass->shared_quad_state_list[0];
  child_nonroot_pass_sqs->content_to_target_transform.Translate(5, 0);

  RenderPass* child_root_pass = child_pass_list.at(1u);
  SharedQuadState* child_root_pass_sqs =
      child_root_pass->shared_quad_state_list[0];
  child_root_pass_sqs->content_to_target_transform.Translate(8, 0);
  child_root_pass_sqs->is_clipped = true;
  child_root_pass_sqs->clip_rect = gfx::Rect(0, 0, 5, 5);

  scoped_ptr<DelegatedFrameData> child_frame_data(new DelegatedFrameData);
  child_pass_list.swap(child_frame_data->render_pass_list);

  scoped_ptr<CompositorFrame> child_frame(new CompositorFrame);
  child_frame->delegated_frame_data = child_frame_data.Pass();

  factory_.SubmitFrame(child_surface_id, child_frame.Pass());

  test::Quad root_quads[] = {test::Quad::SolidColorQuad(1),
                             test::Quad::SurfaceQuad(child_surface_id)};
  test::Pass root_passes[] = {test::Pass(root_quads, arraysize(root_quads))};

  RenderPassList root_pass_list;
  AddPasses(&root_pass_list,
            gfx::Rect(SurfaceSize()),
            root_passes,
            arraysize(root_passes));

  root_pass_list.at(0)
      ->shared_quad_state_list[0]
      ->content_to_target_transform.Translate(0, 7);
  root_pass_list.at(0)
      ->shared_quad_state_list[1]
      ->content_to_target_transform.Translate(0, 10);

  scoped_ptr<DelegatedFrameData> root_frame_data(new DelegatedFrameData);
  root_pass_list.swap(root_frame_data->render_pass_list);

  scoped_ptr<CompositorFrame> root_frame(new CompositorFrame);
  root_frame->delegated_frame_data = root_frame_data.Pass();

  factory_.SubmitFrame(root_surface_id_, root_frame.Pass());

  std::set<SurfaceId> surface_set;
  scoped_ptr<CompositorFrame> aggregated_frame =
      aggregator_.Aggregate(root_surface_id_, &surface_set);

  ASSERT_TRUE(aggregated_frame);
  ASSERT_TRUE(aggregated_frame->delegated_frame_data);

  DelegatedFrameData* frame_data = aggregated_frame->delegated_frame_data.get();

  const RenderPassList& aggregated_pass_list = frame_data->render_pass_list;

  ASSERT_EQ(2u, aggregated_pass_list.size());

  ASSERT_EQ(1u, aggregated_pass_list[0]->shared_quad_state_list.size());

  // The first pass should have one shared quad state for the one solid color
  // quad.
  EXPECT_EQ(1u, aggregated_pass_list[0]->shared_quad_state_list.size());
  // The second (root) pass should have just two shared quad states. We'll
  // verify the properties through the quads.
  EXPECT_EQ(2u, aggregated_pass_list[1]->shared_quad_state_list.size());

  SharedQuadState* aggregated_first_pass_sqs =
      aggregated_pass_list[0]->shared_quad_state_list.front();

  // The first pass's transform should be unaffected by the embedding and still
  // be a translation by +5 in the x direction.
  gfx::Transform expected_aggregated_first_pass_sqs_transform;
  expected_aggregated_first_pass_sqs_transform.Translate(5, 0);
  EXPECT_EQ(expected_aggregated_first_pass_sqs_transform.ToString(),
            aggregated_first_pass_sqs->content_to_target_transform.ToString());

  // The first pass's transform to the root target should include the aggregated
  // transform.
  gfx::Transform expected_first_pass_transform_to_root_target;
  expected_first_pass_transform_to_root_target.Translate(8, 10);
  EXPECT_EQ(expected_first_pass_transform_to_root_target.ToString(),
            aggregated_pass_list[0]->transform_to_root_target.ToString());

  ASSERT_EQ(2u, aggregated_pass_list[1]->quad_list.size());

  gfx::Transform expected_root_pass_quad_transforms[2];
  // The first quad in the root pass is the solid color quad from the original
  // root surface. Its transform should be unaffected by the aggregation and
  // still be +7 in the y direction.
  expected_root_pass_quad_transforms[0].Translate(0, 7);
  // The second quad in the root pass is aggregated from the child surface so
  // its transform should be the combination of its original translation (0, 10)
  // and the child surface draw quad's translation (8, 0).
  expected_root_pass_quad_transforms[1].Translate(8, 10);

  for (size_t i = 0; i < 2; ++i) {
    DrawQuad* quad = aggregated_pass_list[1]->quad_list.at(i);
    EXPECT_EQ(expected_root_pass_quad_transforms[i].ToString(),
              quad->quadTransform().ToString())
        << i;
  }

  EXPECT_EQ(true,
            aggregated_pass_list[1]->shared_quad_state_list[1]->is_clipped);

  // The second quad in the root pass is aggregated from the child, so its
  // clip rect must be transformed by the child's translation.
  EXPECT_EQ(
      gfx::Rect(0, 10, 5, 5).ToString(),
      aggregated_pass_list[1]->shared_quad_state_list[1]->clip_rect.ToString());

  factory_.Destroy(child_surface_id);
}

class SurfaceAggregatorWithResourcesTest : public testing::Test {
 public:
  virtual void SetUp() {
    output_surface_ = FakeOutputSurface::CreateSoftware(
        make_scoped_ptr(new SoftwareOutputDevice));
    output_surface_->BindToClient(&output_surface_client_);
    shared_bitmap_manager_.reset(new TestSharedBitmapManager);

    resource_provider_ = ResourceProvider::Create(output_surface_.get(),
                                                  shared_bitmap_manager_.get(),
                                                  0,
                                                  false,
                                                  1,
                                                  false);

    aggregator_.reset(
        new SurfaceAggregator(&manager_, resource_provider_.get()));
  }

 protected:
  SurfaceManager manager_;
  FakeOutputSurfaceClient output_surface_client_;
  scoped_ptr<OutputSurface> output_surface_;
  scoped_ptr<SharedBitmapManager> shared_bitmap_manager_;
  scoped_ptr<ResourceProvider> resource_provider_;
  scoped_ptr<SurfaceAggregator> aggregator_;
};

class ResourceTrackingSurfaceFactoryClient : public SurfaceFactoryClient {
 public:
  ResourceTrackingSurfaceFactoryClient() {}
  virtual ~ResourceTrackingSurfaceFactoryClient() {}

  virtual void ReturnResources(
      const ReturnedResourceArray& resources) OVERRIDE {
    returned_resources_ = resources;
  }

  ReturnedResourceArray returned_resources() const {
    return returned_resources_;
  }

 private:
  ReturnedResourceArray returned_resources_;

  DISALLOW_COPY_AND_ASSIGN(ResourceTrackingSurfaceFactoryClient);
};

void SubmitFrameWithResources(ResourceProvider::ResourceId* resource_ids,
                              size_t num_resource_ids,
                              SurfaceFactory* factory,
                              SurfaceId surface_id) {
  scoped_ptr<DelegatedFrameData> frame_data(new DelegatedFrameData);
  scoped_ptr<RenderPass> pass = RenderPass::Create();
  pass->id = RenderPassId(1, 1);
  SharedQuadState* sqs = pass->CreateAndAppendSharedQuadState();
  for (size_t i = 0u; i < num_resource_ids; ++i) {
    TransferableResource resource;
    resource.id = resource_ids[i];
    resource.is_software = true;
    frame_data->resource_list.push_back(resource);
    TextureDrawQuad* quad = pass->CreateAndAppendDrawQuad<TextureDrawQuad>();
    const gfx::Rect rect;
    const gfx::Rect opaque_rect;
    const gfx::Rect visible_rect;
    bool needs_blending = false;
    bool premultiplied_alpha = false;
    const gfx::PointF uv_top_left;
    const gfx::PointF uv_bottom_right;
    SkColor background_color = SK_ColorGREEN;
    const float vertex_opacity[4] = {0.f, 0.f, 1.f, 1.f};
    bool flipped = false;
    quad->SetAll(sqs,
                 rect,
                 opaque_rect,
                 visible_rect,
                 needs_blending,
                 resource_ids[i],
                 premultiplied_alpha,
                 uv_top_left,
                 uv_bottom_right,
                 background_color,
                 vertex_opacity,
                 flipped);

    quad->shared_quad_state = sqs;
  }
  frame_data->render_pass_list.push_back(pass.Pass());
  scoped_ptr<CompositorFrame> frame(new CompositorFrame);
  frame->delegated_frame_data = frame_data.Pass();
  factory->SubmitFrame(surface_id, frame.Pass());
}

TEST_F(SurfaceAggregatorWithResourcesTest, TakeResourcesOneSurface) {
  ResourceTrackingSurfaceFactoryClient client;
  SurfaceFactory factory(&manager_, &client);
  SurfaceId surface_id(7u);
  factory.Create(surface_id, SurfaceSize());

  ResourceProvider::ResourceId ids[] = {11, 12, 13};
  SubmitFrameWithResources(ids, arraysize(ids), &factory, surface_id);

  std::set<SurfaceId> surface_set;
  scoped_ptr<CompositorFrame> frame =
      aggregator_->Aggregate(surface_id, &surface_set);

  // Nothing should be available to be returned yet.
  EXPECT_TRUE(client.returned_resources().empty());

  SubmitFrameWithResources(NULL, 0u, &factory, surface_id);

  surface_set.clear();
  frame = aggregator_->Aggregate(surface_id, &surface_set);

  ASSERT_EQ(3u, client.returned_resources().size());
  ResourceProvider::ResourceId returned_ids[3];
  for (size_t i = 0; i < 3; ++i) {
    returned_ids[i] = client.returned_resources()[i].id;
  }
  EXPECT_THAT(returned_ids,
              testing::WhenSorted(testing::ElementsAreArray(ids)));
  factory.Destroy(surface_id);
}

}  // namespace
}  // namespace cc

