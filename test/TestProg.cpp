
#include <iostream>

#include "corrugate/FeatureBox.hpp"

int main(int argc, char** argv) {
  std::cout << "hi" << std::endl;
  cg::FeatureBox box;

  glm::dvec2 v = box.GetOrigin();
  std::cout << "origin: " << v.x << ", " << v.y << std::endl;

  return 0;
}
