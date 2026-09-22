#include "helper.cpp"
#include "../bullet.hpp"


static void analyze(const std::string &path)
{
    ImBMP img(path);
    if (!img.ok())
    {
        printf("Could not open %s\n", path.c_str());
        return;
    }

    FD *fd = img.getFrequencyDomain();

    int margin = 4;
    int window = 3;
    double minRatio = 3.0;

    auto peaks = findPeaks(fd, margin, window, minRatio);
    auto peaksDeduped = dedupe(peaks, 6);

    printf("== %s ==\n", path.c_str());
    printf("Top spike candidates:\n");
    int shown = 0;
    for (const auto &p : peaksDeduped)
    {
        if (shown >= 15)
            break;
        printf("  (%+4d, %+4d)  ratio=%.2f\n", p.x, p.y, p.ratio);
        shown++;
    }
    printf("\n");
}

bullet(analyze_run)
{
    analyze("./");

    return 0;
}
