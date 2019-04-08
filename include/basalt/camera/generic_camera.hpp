/**
BSD 3-Clause License

Copyright (c) 2019, Vladyslav Usenko and Nikolaus Demmel.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <basalt/camera/double_sphere_camera.hpp>
#include <basalt/camera/extended_camera.hpp>
#include <basalt/camera/fov_camera.hpp>
#include <basalt/camera/kannala_brandt_camera4.hpp>
#include <basalt/camera/pinhole_camera.hpp>
#include <basalt/camera/unified_camera.hpp>

#include <variant>

namespace basalt {

template <typename Scalar>
class GenericCamera {
  using Vec2 = Eigen::Matrix<Scalar, 2, 1>;
  using Vec3 = Eigen::Matrix<Scalar, 3, 1>;
  using Vec4 = Eigen::Matrix<Scalar, 4, 1>;
  using VecX = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>;

  using Mat24 = Eigen::Matrix<Scalar, 2, 4>;
  using Mat42 = Eigen::Matrix<Scalar, 4, 2>;
  using Mat4 = Eigen::Matrix<Scalar, 4, 4>;

  using VariantT =
      std::variant<ExtendedUnifiedCamera<Scalar>, DoubleSphereCamera<Scalar>,
                   KannalaBrandtCamera4<Scalar>, UnifiedCamera<Scalar>,
                   PinholeCamera<Scalar>>;

 public:
  template <typename Scalar2>
  inline GenericCamera<Scalar2> cast() const {
    GenericCamera<Scalar2> res;
    std::visit([&](const auto& v) { res.variant = v.template cast<Scalar2>(); },
               variant);
    return res;
  }

  inline int getN() const {
    int res;
    std::visit([&](const auto& v) { res = v.N; }, variant);
    return res;
  }

  inline std::string getName() const {
    std::string res;
    std::visit([&](const auto& v) { res = v.getName(); }, variant);
    return res;
  }

  inline void setFromInit(const Vec4& init) {
    std::visit([&](auto& v) { return v.setFromInit(init); }, variant);
  }

  inline void applyInc(const VecX& inc) {
    std::visit([&](auto& v) { return v += inc; }, variant);
  }

  inline VecX getParam() const {
    VecX res;
    std::visit([&](const auto& cam) { res = cam.getParam(); }, variant);
    return res;
  }

  // SLOW!! functions. Every call requires vtable lookup.
  inline bool unproject(const Vec2& proj, Vec4& p3d,
                        Mat42* d_p3d_d_proj = nullptr) const {
    bool res;
    std::visit(
        [&](const auto& cam) { res = cam.unproject(proj, p3d, d_p3d_d_proj); },
        variant);
    return res;
  }
  // SLOW!! functions. Every call requires vtable lookup.

  inline void project(const Eigen::vector<Vec3>& p3d, const Mat4& T_c_w,
                      Eigen::vector<Vec2>& proj,
                      std::vector<bool>& proj_success) const {
    std::visit(
        [&](const auto& cam) {
          proj.resize(p3d.size());
          proj_success.resize(p3d.size());
          for (size_t i = 0; i < p3d.size(); i++) {
            proj_success[i] =
                cam.project(T_c_w * p3d[i].homogeneous(), proj[i]);
          }
        },
        variant);
  }

  inline void project(const Eigen::vector<Vec4>& p3d, const Mat4& T_c_w,
                      Eigen::vector<Vec2>& proj,
                      std::vector<bool>& proj_success) const {
    std::visit(
        [&](const auto& cam) {
          proj.resize(p3d.size());
          proj_success.resize(p3d.size());
          for (size_t i = 0; i < p3d.size(); i++) {
            proj_success[i] = cam.project(T_c_w * p3d[i], proj[i]);
          }
        },
        variant);
  }

  inline void unproject(const Eigen::vector<Vec2>& proj,
                        Eigen::vector<Vec4>& p3d,
                        std::vector<bool>& unproj_success) const {
    std::visit(
        [&](const auto& cam) {
          p3d.resize(proj.size());
          unproj_success.resize(proj.size());
          for (size_t i = 0; i < p3d.size(); i++) {
            unproj_success[i] = cam.unproject(proj[i], p3d[i]);
          }
        },
        variant);
  }

  static GenericCamera<Scalar> fromString(const std::string& name) {
    GenericCamera<Scalar> res;

    constexpr size_t variant_size = std::variant_size<VariantT>::value;
    visitAllTypes<variant_size - 1>(res, name);

    return res;
  }

  VariantT variant;

 private:
  template <int I>
  static void visitAllTypes(GenericCamera<Scalar>& res,
                            const std::string& name) {
    if constexpr (I >= 0) {
      using cam_t = typename std::variant_alternative<I, VariantT>::type;
      if (cam_t::getName() == name) {
        cam_t val;
        res.variant = val;
      }
      visitAllTypes<I - 1>(res, name);
    }
  }
};
}  // namespace basalt