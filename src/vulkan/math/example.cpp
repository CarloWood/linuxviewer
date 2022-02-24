#include <Eigen/Core>
#include <Eigen/Geometry>
#include <iostream>
#include <algorithm>
#include <vector>

int main()
{
  using namespace Eigen;

#if 0
  std::vector<float> cubeVertices = {
    -1.0, -1.0, -1.0,
    1.0, -1.0, -1.0,
    1.0, 1.0, -1.0,
    -1.0, 1.0, -1.0,
    -1.0, -1.0, 1.0,
    1.0, -1.0, 1.0,
    1.0, 1.0, 1.0,
    -1.0, 1.0, 1.0
  };

  MatrixXf mVertices = Map<Matrix<float, 3, 8>>(cubeVertices.data());

  std::cout << "mVertices =\n" << mVertices << '\n';

  Transform<float, 3, Affine> t = Transform<float, 3, Affine>::Identity();

  std::cout << "transform matrix =\n" << t.matrix() << '\n';

  t.scale(0.8f);

  std::cout << "After scale(0.8); transform matrix =\n" << t.matrix() << '\n';

  t.rotate(AngleAxisf(0.25f * M_PI, Vector3f::UnitX()));

  std::cout << "After rotation of PI/4 around X; transform matrix =\n" << t.matrix() << '\n';

  t.translate(Vector3f(1.5, 10.2, -5.1));

  std::cout << "After translation of (1.5, 10.2, -5.1); transform matrix =\n" << t.matrix() << '\n';

  std::cout << "mVertices.colwise().homogenous() =\n" << mVertices.colwise().homogeneous() << '\n';

  std::cout << "Transformed cube:\n";
  std::cout << t * mVertices.colwise().homogeneous() << std::endl;
#endif

  static constexpr float pos_scale = 0.2f;
  static constexpr std::array<float, 2> pos_translation = {-0.1, -0.1};

  Transform<float, 2, Affine> const t = Transform<float, 2, Affine>{Transform<float, 2, Affine>::Identity()}.translate(Vector2f{pos_translation.data()}).scale(pos_scale);

  Vector2i xy(2, 3);
  Vector2f pos = t * xy.cast<float>();

  Vector3f p3;
  p3 << pos, 0.0f;
  Vector4f v = p3.homogeneous();

  std::cout << v << '\n';
}
