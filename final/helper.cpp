#ifndef HELPER
#define HELPER
#include "../lib/im.hpp"
#include "../lib/fd.hpp"
#include "../bullet.hpp"
#include <vector>
#include <algorithm>

struct Peak
{
    int x, y;
    double ratio;
};

static std::vector<Peak> findPeaks(FD *fd, int margin, int window, double minRatio)
{
    int centerX = fd->width / 2;
    int centerY = fd->height / 2;
    std::vector<Peak> peaks;

    for (int y = margin; y < fd->height - margin; y++)
    {
        for (int x = margin; x < fd->width - margin; x++)
        {
            if (std::abs(x - centerX) < window && std::abs(y - centerY) < window)
                continue;

            double v = fd->magnitudeAt(x, y);

            double neighborSum = 0;
            int neighborCount = 0;
            for (int dy = -window; dy <= window; dy++)
            {
                for (int dx = -window; dx <= window; dx++)
                {
                    if (dx == 0 && dy == 0)
                        continue;
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || nx >= fd->width || ny < 0 || ny >= fd->height)
                        continue;
                    neighborSum += fd->magnitudeAt(nx, ny);
                    neighborCount++;
                }
            }
            double neighborAvg = neighborSum / neighborCount;
            double ratio = v / (neighborAvg + 1e-6);

            if (ratio >= minRatio && v > 1.0)
            {
                peaks.push_back({x - centerX, y - centerY, ratio});
            }
        }
    }

    std::sort(peaks.begin(), peaks.end(), [](const Peak &a, const Peak &b)
              { return a.ratio > b.ratio; });
    return peaks;
}

// Collapses nearby peaks (same blob) down to their strongest point.
static std::vector<Peak> dedupe(const std::vector<Peak> &peaks, int minDist)
{
    std::vector<Peak> kept;
    for (const auto &p : peaks)
    {
        bool tooClose = false;
        for (const auto &k : kept)
        {
            int dx = p.x - k.x, dy = p.y - k.y;
            if (dx * dx + dy * dy < minDist * minDist)
            {
                tooClose = true;
                break;
            }
        }
        if (!tooClose)
            kept.push_back(p);
    }
    return kept;
}
#endif