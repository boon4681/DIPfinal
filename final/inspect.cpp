#include "../lib/im.hpp"
#include "../lib/fd.hpp"
#include "../bullet.hpp"
#include <vector>
#include <algorithm>
#include "helper.cpp"
#include <string>

$bullet(inspect1)
{
    ImBMP img("./resource/FinalDIP69.bmp");
    img.convertToGrayscale();
    img.alphaTrimmedFilter(3, 5);

    for (int n = 0; n < 40; n++)
    {
        ImBMP graph = Graph(img.width);
        for (int i = 0; i < graph.width; i++)
        {
            graph.setRGB(i, img.getRGB(i, n) & 0xff, 0xFFFFFF);
        }
        graph.write("./out/graph-" + std::to_string(n) + ".bmp");
    }

    return 0;
}

bullet(inspect2)
{
    ImBMP img("./resource/FinalDIP69.bmp");
    std::vector<double> wave{};
    wave.resize(img.width);
    std::fill(wave.begin(), wave.end(), 0);

    img.convertToGrayscale();
    img.alphaTrimmedFilter(3, 5);
    ImBMP graph = Graph(img.width);

    {
        for (int y = 0; y < 1; y++)
        {
            for (int x = 0; x < img.width; x++)
            {
                int k = img.getRGB(x, y) & 0xff;
                wave[x] = y == 0 ? k : (int)round(((double)k + wave[x]) * 0.5f);
            }
        }
    }
    {
        // for (int y = 200; y < 240; y++)
        // {
        //     for (int x = 0; x < img.width; x++)
        //     {
        //         int k = img.getRGB(x, y) & 0xff;
        //         wave[x] = y == 0 ? k : (int)round(((double)k + wave[x]) * 0.5f);
        //     }
        // }
    }
    for (int i = 0; i < graph.width; i++)
    {
        graph.setRGB(i, (int)wave[i] & 0xff, 0xFFFFFF);
    }
    graph.write("./graph.bmp");

    int width = img.width;
    std::vector<double> a = SMA(wave, 3);
    // int half = (int)round((double)width / 2);
    // int w = (int)round((double)width / 4);
    // std::vector<int> v{};
    // v.resize(width);
    // int h = (int)round((double)w / 2);

    // for (int i = 0; i < width; i++)
    // {
    //     int start = std::max(0, i - h);
    //     int end = std::min(width, i + h);
    //     double avg = 0;
    //     for (int m = start; m < end; m++)
    //     {
    //         avg += wave[m];
    //     }
    //     avg /= end - start;
    //     v[i] = (int)round(avg);
    // }

    // std::vector<int> a{};
    // a.resize(width);
    // for (int i = 0; i < width; i++)
    // {
    //     a[i] = wave[i] - v[i];
    // }
    // for (int i = 0; i < graph.width; i++)
    // {
    //     std::cout << a[i] << "\n";
    //     graph.setRGB(i, a[i] & 0xff, 0xFFFFFF);
    // }

    // graph.write("./graph0.bmp");

    int c = 0;
    for (int i = 1; i < width; i++)
    {
        if ((a[i] >= 0 && a[i - 1] < 0) || (a[i] < 0 && a[i - 1] >= 0))
        {
            c++;
        }
    }

    std::cout << "c:" << c << "\n";

    for (int i = 0; i < graph.width; i++)
    {
        // std::cout << a[i] << "\n";
        graph.setRGB(i, (int)round(a[i]) & 0xff, 0xFFFFFF);
    }

    graph.write("./graph0-1.bmp");
    return 0;
}

bullet(inspect3)
{
    ImBMP img("./resource/FinalDIP69.bmp");
    std::vector<double> wave{};
    wave.resize(img.width);
    std::fill(wave.begin(), wave.end(), 0);

    img.convertToGrayscale();
    img.alphaTrimmedFilter(3, 5);

    for (int y = 0; y < img.height; y++)
    {
        for (int x = 0; x < img.width; x++)
        {
            int k = img.getRGB(x, y) & 0xff;
            wave[x] = y == 0 ? k : (int)round(((double)k + wave[x]) * 0.5f);
        }
    }

    std::vector<double> a = SMA(wave, 3);

    for (int i = 0; i < a.size(); i++)
    {
        wave[i] = (int)round((double)a[i]);
    }

    int n = 1;
    while (n < img.width)
    {
        n <<= 1;
    }

    double mean = 0;
    for (int i = 0; i < img.width; i++)
    {
        mean += wave[i];
    }
    mean /= img.width;

    std::vector<Complex> f(n, Complex(0, 0));
    for (int i = 0; i < n; i++)
    {
        double v = i < img.width ? wave[i] - mean : 0.0;
        f[i] = Complex(v, 0);
    }

    fft(f.data(), n, false);

    int half = n / 2;
    std::vector<double> mag(half, 0.0);
    double peak = 0;
    for (int k = 0; k < half; k++)
    {
        mag[k] = std::abs(f[k]);
        if (mag[k] > peak)
        {
            peak = mag[k];
        }
    }

    ImBMP fftgraph = Graph(half);
    for (int k = 0; k < half; k++)
    {
        double s = peak > 0 ? log(1 + mag[k]) / log(1 + peak) : 0;
        fftgraph.setRGB(k, (int)(s * 255) & 0xff, 0xFFFFFF);
    }
    fftgraph.write("./fft.bmp");

    double scanPeak = 0;
    for (int k = n / 128; k < half - 1; k++)
    {
        if (mag[k] > scanPeak)
        {
            scanPeak = mag[k];
        }
    }

    int windowSize = 3;
    int minK = n / 128;
    std::vector<Complex> keep(n, Complex(0, 0));
    for (int k = minK; k < half - 1; k++)
    {
        if (mag[k] > mag[k - 1] && mag[k] > mag[k + 1] && mag[k] > scanPeak * 0.85)
        {
            std::cout << "k: " << k
                      << " period: " << (double)n / k
                      << " mag: " << mag[k] << "\n";
            for (int i = -windowSize; i <= windowSize; i++)
            {
                int targetK = k + i;
                if (targetK > 0 && targetK < half)
                {
                    keep[targetK] = f[targetK];
                    keep[n - targetK] = f[n - targetK];
                }
            }
        }
    }

    fft(keep.data(), n, true);
    std::vector<int> pure(img.width);
    for (int i = 0; i < img.width; i++)
    {
        // int v = (int)round(keep[i].real() + 128);
        // pure[i] = std::min(255, std::max(0, v));
        pure[i] = (int)round(keep[i].real());
        // else
        // {
        //     pure[i] = 12-(int)abs(keep[i].real());
        // }
    }

    ImBMP graph = Graph(img.width, img.height);
    for (int y = 0; y < graph.height; y++)
    {
        for (int x = 0; x < graph.width; x++)
        {
            // int gray = pure[x] & 0xff;
            int gray = std::min(255, std::max(0, pure[x] + 128));
            graph.setRGB(x, y, (gray << 16) | (gray << 8) | gray);
        }
    }
    graph.write("./graph0-2.bmp");
    double headroom = 40.0;

    ImBMP out = img;
    for (int y = 0; y < img.height; y++)
    {
        for (int x = 0; x < img.width; x++)
        {
            int gray = img.getRGB(x, y) & 0xff;
            double room = std::min(gray, 255 - gray) / headroom;
            double scale = std::min(1.0, room);
            double gray_overflow = gray - scale * pure[x];
            int q = std::min(255, std::max(0, (int)round(gray_overflow)));
            out.setRGB(x, y, (q << 16) | (q << 8) | q);
        }
    }
    out.write("./out.bmp");
    return 0;
}

bullet(inspect4) {
    return 0;
}