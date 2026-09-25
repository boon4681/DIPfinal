// include script to load as a bullet
#include "./final/inspect.cpp"
#include "./final/final.cpp"
#include "./final/main.cpp"

#include <cstdio>
#include "./bullet.hpp"

int main()
{
    for (auto &b : magazine())
    {
        printf("Running %s\n", b.name);
        int v = b.fn(b.name);
        if (v != 0) {
            return v;
        }
        printf("\n");
    }
    return 0;
}


// 512, 511