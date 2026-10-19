#include <iostream>

#include "mtt/version.hpp"

int main() {
    std::cout << "mtt tracker " << mtt::version() << '\n';
    return 0;
}
