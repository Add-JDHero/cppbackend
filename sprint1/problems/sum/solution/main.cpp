#include <iostream>
#include <limits>


int Sum(int x, int y) {
    if (x > 0 && y > 0 && (std::numeric_limits<int>::max() - x) < y) {
        throw std::overflow_error("Positive overflow");
    }
    
    if (x < 0 && y < 0 && (std::numeric_limits<int>::min() - x) > y) {
        throw std::underflow_error("Negative overflow");
    }
    
    return x + y;
}    

int main() {
    int x, y;
    std::cin >> x >> y;
    
    std::cout << Sum(x, y) << std::endl;
}
