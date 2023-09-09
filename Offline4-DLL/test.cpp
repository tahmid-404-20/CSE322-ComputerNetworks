#include <iostream>
#include <random>

int main() {
  // Create a random number generator engine
  std::random_device rd;
  std::mt19937 gen(rd());

  // Create a uniform distribution between 0.0 and 1.0
  std::uniform_real_distribution<double> dis(0.0, 1.0);
  // Generate random numbers from the normal distribution
  for (int i = 0; i < 10; ++i) {
    double random_number = dis(gen);
    std::cout << "Random number from normal distribution: " << random_number
              << std::endl;
  }

  return 0;
}
