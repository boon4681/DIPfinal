#pragma once
#include <vector>
#include <string>

struct Bullet
{
    const char *name;
    int (*fn)(const std::string &bullet_name);
};

inline std::vector<Bullet> &magazine()
{
    static std::vector<Bullet> v;
    return v;
}
struct Reg
{
    Reg(const char *name, int (*fn)(const std::string &)) { magazine().push_back({name, fn}); }
};

// bullet
#define bullet(name)                          \
    int name(const std::string &bullet_name); \
    static Reg reg_##name(#name, name);       \
    int name(const std::string &bullet_name)

// silent the bullet
#define $bullet(name)                         \
    int name(const std::string &bullet_name); \
    int name(const std::string &bullet_name)
