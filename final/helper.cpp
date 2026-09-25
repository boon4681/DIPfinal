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

bool loadSpectrumBMP(const std::string &path, ImBMP *out, FD *fd)
{
    if (!out->read(path))
    {
        return false;
    }
    if (out->width != fd->width || out->height != fd->height)
    {
        printf("Spectrum %s is %dx%d, expected %dx%d\n", path.c_str(), out->width, out->height, fd->width, fd->height);
        return false;
    }
    return true;
}

void spectrumLogRange(FD *fd, double *outMin, double *outMax)
{
    double max = DBL_MIN, min = DBL_MAX;
    for (int y = 0; y < fd->height; y++)
    {
        for (int x = 0; x < fd->width; x++)
        {
            double spectrum = fd->magnitudeAt(x, y);
            if (spectrum > max)
                max = spectrum;
            if (spectrum < min)
                min = spectrum;
        }
    }
    *outMin = min < 1.0 ? 0.0 : log10(min);
    *outMax = max < 1.0 ? 0.0 : log10(max);
}

bool overrideSpectrum(FD *fd, const std::string &path)
{
    ImBMP spec;
    if (!loadSpectrumBMP(path, &spec, fd))
    {
        return false;
    }
    double logMin, logMax;
    spectrumLogRange(fd, &logMin, &logMax);
    if (logMax <= logMin)
    {
        printf("Degenerate spectrum range\n");
        return false;
    }
    for (int y = 0; y < fd->height; y++)
    {
        for (int x = 0; x < fd->width; x++)
        {
            int gray = spec.getRGB(x, y) & 0xff;
            Complex bin = fd->binAt(x, y);
            double phase = std::arg(bin);
            double magnitude = 0.0;
            if (gray > 0)
            {
                magnitude = pow(10.0, gray * (logMax - logMin) / 255.0 + logMin);
            }
            fd->setBin(x, y, std::polar(magnitude, phase));
        }
    }
    return true;
}


ImBMP Graph(int width, int height = 300)
{
    ImBMP graph("./resource/FinalDIP69.bmp");
    graph.width = width;
    graph.height = height;
    graph.data.resize((size_t)width * height * (24 / 8));
    std::fill(graph.data.begin(), graph.data.end(), 0);
    return graph;
}

std::vector<double> SMA(std::vector<double> v, int window = 5)
{
    int n = v.size();
    std::vector<double> result(n, 0.0);

    if (n == 0 || window <= 0)
        return result;

    int offset = (window - 1) / 2;

    for (int i = 0; i < n; ++i)
    {
        double sum = 0.0;
        int count = 0;

        int start = i - offset;
        int end = start + window;

        for (int j = start; j < end; ++j)
        {
            if (j >= 0 && j < n)
            {
                sum += v[j];
                count++;
            }
        }
        result[i] = sum / count;
    }

    return result;
}

void fft(Complex *x, int size, bool invert)
{
    if (size <= 1)
        return;
    Complex *even = new Complex[size / 2];
    Complex *odd = new Complex[size / 2];
    for (int i = 0; i < size / 2; i++)
    {
        even[i] = x[2 * i];
        odd[i] = x[2 * i + 1];
    }
    fft(even, size / 2, invert);
    fft(odd, size / 2, invert);

    double angle = 2 * M_PI / size * (invert ? -1 : 1);
    Complex w(1);
    Complex wn(cos(angle), sin(angle));
    for (int i = 0; i < size / 2; i++)
    {
        x[i] = even[i] + w * odd[i];
        x[i + size / 2] = even[i] - w * odd[i];
        if (invert)
        {
            x[i] /= 2;
            x[i + size / 2] /= 2;
        }
        w *= wn;
    }
    delete[] even;
    delete[] odd;
}

#endif
