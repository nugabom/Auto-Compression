// Copyright 2020 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <random>
#include <vector>

#include <xnnpack.h>


class DepthToSpaceOperatorTester {
 public:
  inline DepthToSpaceOperatorTester& input_size(size_t input_height, size_t input_width) {
    assert(input_height >= 1);
    assert(input_width >= 1);
    this->input_height_ = input_height;
    this->input_width_ = input_width;
    return *this;
  }

  inline DepthToSpaceOperatorTester& input_height(size_t input_height) {
    assert(input_height >= 1);
    this->input_height_ = input_height;
    return *this;
  }

  inline size_t input_height() const {
    return this->input_height_;
  }

  inline DepthToSpaceOperatorTester& input_width(size_t input_width) {
    assert(input_width >= 1);
    this->input_width_ = input_width;
    return *this;
  }

  inline size_t input_width() const {
    return this->input_width_;
  }

  inline size_t output_height() const {
    return input_height() * block_size();
  }

  inline size_t output_width() const {
    return input_width() * block_size();
  }

  inline DepthToSpaceOperatorTester& block_size(size_t block_size) {
    assert(block_size >= 2);
    this->block_size_ = block_size;
    return *this;
  }

  inline size_t block_size() const {
    return this->block_size_;
  }

  inline size_t input_channels() const {
    return output_channels() * block_size() * block_size();
  }

  inline DepthToSpaceOperatorTester& output_channels(size_t output_channels) {
    assert(output_channels != 0);
    this->output_channels_ = output_channels;
    return *this;
  }

  inline size_t output_channels() const {
    return this->output_channels_;
  }

  inline DepthToSpaceOperatorTester& batch_size(size_t batch_size) {
    assert(batch_size != 0);
    this->batch_size_ = batch_size;
    return *this;
  }

  inline size_t batch_size() const {
    return this->batch_size_;
  }

  inline DepthToSpaceOperatorTester& input_channels_stride(size_t input_channels_stride) {
    assert(input_channels_stride >= 1);
    this->input_channels_stride_ = input_channels_stride;
    return *this;
  }

  inline size_t input_channels_stride() const {
    if (this->input_channels_stride_ == 0) {
      return input_channels();
    } else {
      assert(this->input_channels_stride_ >= input_channels());
      return this->input_channels_stride_;
    }
  }

  inline DepthToSpaceOperatorTester& output_channels_stride(size_t output_channels_stride) {
    assert(output_channels_stride >= 1);
    this->output_channels_stride_ = output_channels_stride;
    return *this;
  }

  inline size_t output_channels_stride() const {
    if (this->output_channels_stride_ == 0) {
      return output_channels();
    } else {
      assert(this->output_channels_stride_ >= output_channels());
      return this->output_channels_stride_;
    }
  }

  inline DepthToSpaceOperatorTester& iterations(size_t iterations) {
    this->iterations_ = iterations;
    return *this;
  }

  inline size_t iterations() const {
    return this->iterations_;
  }

#ifndef XNN_NO_X32_OPERATORS
  void TestNHWCxX32() const {
    std::random_device random_device;
    auto rng = std::mt19937(random_device());
    auto i32rng = std::bind(std::uniform_int_distribution<int32_t>(), rng);

    std::vector<int32_t> input(
      (batch_size() * input_height() * input_width() - 1) * input_channels_stride() + input_channels());
    std::vector<int32_t> output(
      (batch_size() * output_height() * output_width() - 1) * output_channels_stride() + output_channels());
    for (size_t iteration = 0; iteration < iterations(); iteration++) {
      std::generate(input.begin(), input.end(), std::ref(i32rng));
      std::fill(output.begin(), output.end(), INT32_C(0xDEADBEAF));

      // Create, setup, run, and destroy Depth To Space operator.
      ASSERT_EQ(xnn_status_success, xnn_initialize(nullptr /* allocator */));
      xnn_operator_t depth_to_space_op = nullptr;

      ASSERT_EQ(xnn_status_success,
                xnn_create_depth_to_space_nhwc_x32(
                    output_channels(), input_channels_stride(), output_channels_stride(),
                    block_size(), 0, &depth_to_space_op));
      ASSERT_NE(nullptr, depth_to_space_op);

      // Smart pointer to automatically delete depth_to_space_op.
      std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)> auto_depth_to_space_op(depth_to_space_op, xnn_delete_operator);

      ASSERT_EQ(xnn_status_success,
                xnn_setup_depth_to_space_nhwc_x32(
                    depth_to_space_op,
                    batch_size(), input_height(), input_width(),
                    input.data(), output.data(), nullptr /* thread pool */));

      ASSERT_EQ(xnn_status_success,
        xnn_run_operator(depth_to_space_op, nullptr /* thread pool */));

      // Verify results.
      for (size_t i = 0; i < batch_size(); i++) {
        for (size_t iy = 0; iy < input_height(); iy++) {
          for (size_t by = 0; by < block_size(); by++) {
            for (size_t ix = 0; ix < input_width(); ix++) {
              for (size_t bx = 0; bx < block_size(); bx++) {
                for (size_t oc = 0; oc < output_channels(); oc++) {
                  const size_t input_index =
                    ((i * input_height() + iy) * input_width() + ix) * input_channels_stride() +
                      (by * block_size() + bx) * output_channels() + oc;
                  const size_t output_index =
                    ((i * output_height() + iy * block_size() + by) * output_width() + ix * block_size() + bx) *
                      output_channels_stride() + oc;
                  ASSERT_EQ(output[output_index], input[input_index])
                    << "batch: " << i << " / " << batch_size()
                    << ", input x: " << ix << " / " << input_width()
                    << ", input y: " << iy << " / " << input_height()
                    << ", block x: " << bx << " / " << block_size()
                    << ", block y: " << by << " / " << block_size()
                    << ", output channel: " << oc << " / " << output_channels()
                    << ", input stride: " << input_channels_stride()
                    << ", output stride: " << output_channels_stride();
                }
              }
            }
          }
        }
      }
    }
  }

  void TestNCHW2NHWCxX32() const {
    std::random_device random_device;
    auto rng = std::mt19937(random_device());
    auto i32rng = std::bind(std::uniform_int_distribution<int32_t>(), rng);

    std::vector<int32_t> input(XNN_EXTRA_BYTES / sizeof(uint32_t) +
      ((batch_size() - 1) * input_channels_stride() + input_channels()) * input_height() * input_width());
    std::vector<int32_t> output(
      (batch_size() * output_height() * output_width() - 1) * output_channels_stride() + output_channels());
    for (size_t iteration = 0; iteration < iterations(); iteration++) {
      std::generate(input.begin(), input.end(), std::ref(i32rng));
      std::fill(output.begin(), output.end(), INT32_C(0xDEADBEAF));

      // Create, setup, run, and destroy Depth To Space operator.
      ASSERT_EQ(xnn_status_success, xnn_initialize(nullptr /* allocator */));
      xnn_operator_t depth_to_space_op = nullptr;

      ASSERT_EQ(xnn_status_success,
                xnn_create_depth_to_space_nchw2nhwc_x32(
                    output_channels(), input_channels_stride(), output_channels_stride(),
                    block_size(), 0, &depth_to_space_op));
      ASSERT_NE(nullptr, depth_to_space_op);

      // Smart pointer to automatically delete depth_to_space_op.
      std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)> auto_depth_to_space_op(depth_to_space_op, xnn_delete_operator);

      ASSERT_EQ(xnn_status_success,
                xnn_setup_depth_to_space_nchw2nhwc_x32(
                    depth_to_space_op,
                    batch_size(), input_height(), input_width(),
                    input.data(), output.data(), nullptr /* thread pool */));

      ASSERT_EQ(xnn_status_success,
        xnn_run_operator(depth_to_space_op, nullptr /* thread pool */));

      // Verify results.
      for (size_t i = 0; i < batch_size(); i++) {
        for (size_t iy = 0; iy < input_height(); iy++) {
          for (size_t by = 0; by < block_size(); by++) {
            for (size_t ix = 0; ix < input_width(); ix++) {
              for (size_t bx = 0; bx < block_size(); bx++) {
                for (size_t oc = 0; oc < output_channels(); oc++) {
                  const size_t input_index =
                    i * input_channels_stride() * input_height() * input_width() +
                    (((by * block_size() + bx) * output_channels() + oc) * input_height() + iy) * input_width() + ix;
                  const size_t output_index =
                    ((i * output_height() + iy * block_size() + by) * output_width() + ix * block_size() + bx) *
                      output_channels_stride() + oc;
                  ASSERT_EQ(output[output_index], input[input_index])
                    << "batch: " << i << " / " << batch_size()
                    << ", input x: " << ix << " / " << input_width()
                    << ", input y: " << iy << " / " << input_height()
                    << ", block x: " << bx << " / " << block_size()
                    << ", block y: " << by << " / " << block_size()
                    << ", output channel: " << oc << " / " << output_channels()
                    << ", input stride: " << input_channels_stride()
                    << ", output stride: " << output_channels_stride();
                }
              }
            }
          }
        }
      }
    }
  }
#endif

 private:
  size_t input_height_{1};
  size_t input_width_{1};
  size_t output_channels_{1};
  size_t block_size_{2};
  size_t batch_size_{1};
  size_t input_channels_stride_{0};
  size_t output_channels_stride_{0};
  size_t iterations_{1};
};
