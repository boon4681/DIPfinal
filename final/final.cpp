#include "../lib/im.hpp"
#include "../lib/fd.hpp"
#include "../bullet.hpp"
#include <vector>
#include <algorithm>
#include <numeric>
#include "helper.cpp"
#include <string>
#include <set>
#include <iostream>
#include <fstream>

StructuringElement squareSE()
{
    StructuringElement se(3, 3, {1, 1});
    for (int i = 0; i < 9; i++)
    {
        se.elements[i] = 255;
    }
    return se;
}

StructuringElement crossSE()
{
    StructuringElement se(3, 3, {1, 1});
    int cross[9] = {0, 255, 0, 255, 255, 255, 0, 255, 0};
    for (int i = 0; i < 9; i++)
    {
        se.elements[i] = cross[i];
    }
    return se;
}

StructuringElement horizontalSE()
{
    StructuringElement se(7, 1, {3, 0});
    for (int i = 0; i < 7; i++)
    {
        se.elements[i] = 255;
    }
    return se;
}

StructuringElement verticalSE()
{
    StructuringElement se(1, 7, {0, 3});
    for (int i = 0; i < 7; i++)
    {
        se.elements[i] = 255;
    }
    return se;
}

StructuringElement blockSE()
{
    StructuringElement se(5, 5, {2, 2});
    for (int i = 0; i < 25; i++)
    {
        se.elements[i] = 255;
    }
    return se;
}

StructuringElement sidesSE()
{
    StructuringElement se(9, 9, {4, 4});
    se.elements[4] = se.elements[36] = se.elements[44] = se.elements[76] = 255;
    return se;
}

double median(std::vector<double> v)
{
    std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
    return v[v.size() / 2];
}

std::vector<double> getDiff(std::vector<std::vector<double>> data, int y0, int y1)
{
    int width = data.empty() ? 0 : (int)data[0].size();
    std::vector<double> diff{};
    diff.resize(width);
    std::fill(diff.begin(), diff.end(), 0.0);

    std::vector<std::vector<double>> v(width);
    for (int x = 0; x < width - 1; x++)
    {
        v[x].resize(y1 - y0);
        for (int y = y0; y < y1; y++)
        {
            v[x][y - y0] = data[y][x + 1] - data[y][x];
        }
    }

    for (int x = 0; x < width - 1; x++)
    {
        diff[x] = median(v[x]);
    }
    return diff;
}

std::vector<double> getNoise(std::vector<double> diff, int window)
{
    int width = (int)diff.size();

    std::vector<double> wave{};
    wave.resize(width);
    wave[0] = 0;
    for (int x = 1; x < width; x++)
    {
        wave[x] = wave[x - 1] + diff[x - 1];
    }

    std::vector<double> pure{};
    pure.resize(width);
    std::fill(pure.begin(), pure.end(), 0.0);

    std::vector<double> v{};
    for (int pass = 0; pass < 2; pass++)
    {
        for (int x = 0; x < width; x++)
        {
            v.clear();
            for (int j = x - window / 2; j <= x + window / 2; j++)
            {
                if (j >= 0 && j < width)
                {
                    v.push_back(wave[j] - pure[j]);
                }
            }
            pure[x] = wave[x] - median(v);
        }
    }

    double mean = 0;
    for (int x = 0; x < width; x++)
    {
        mean += pure[x];
    }
    mean /= width;
    for (int x = 0; x < width; x++)
    {
        pure[x] -= mean;
    }
    return pure;
}

bool lineIntersect(Point start1, Point end1, Point start2, Point end2)
{
    double a_dx = end1.x - start1.x;
    double a_dy = end1.y - start1.y;
    double b_dx = end2.x - start2.x;
    double b_dy = end2.y - start2.y;
    double s = (-a_dy * (start1.x - start2.x) + a_dx * (start1.y - start2.y)) / (-b_dx * a_dy + a_dx * b_dy);
    double t = (+b_dx * (start1.y - start2.y) - b_dy * (start1.x - start2.x)) / (-b_dx * a_dy + a_dx * b_dy);
    return (s >= 0 && s <= 1 && t >= 0 && t <= 1);
}

Point intersectPoint(Point start1, Point end1, Point start2, Point end2)
{
    double a_dx = end1.x - start1.x;
    double a_dy = end1.y - start1.y;
    double b_dx = end2.x - start2.x;
    double b_dy = end2.y - start2.y;
    double t = (b_dx * (start1.y - start2.y) - b_dy * (start1.x - start2.x)) / (-b_dx * a_dy + a_dx * b_dy);
    return {(int)round(start1.x + t * a_dx), (int)round(start1.y + t * a_dy)};
}

std::vector<int> boundingBox(std::vector<Point> &corners)
{
    int x0 = corners[0].x, y0 = corners[0].y, x1 = corners[0].x, y1 = corners[0].y;
    for (auto &c : corners)
    {
        x0 = std::min(x0, c.x);
        y0 = std::min(y0, c.y);
        x1 = std::max(x1, c.x);
        y1 = std::max(y1, c.y);
    }
    return {x0, y0, x1, y1};
}

double rectArea(std::vector<int> box)
{
    return (double)(box[2] - box[0]) * (box[3] - box[1]);
}

double boxIoU(std::vector<Point> &a, std::vector<Point> &b)
{
    std::vector<int> boxA = boundingBox(a);
    std::vector<int> boxB = boundingBox(b);
    std::vector<int> overlap = {
        std::max(boxA[0], boxB[0]),
        std::max(boxA[1], boxB[1]),
        std::min(boxA[2], boxB[2]),
        std::min(boxA[3], boxB[3]),
    };
    if (overlap[2] <= overlap[0] || overlap[3] <= overlap[1])
    {
        return 0;
    }
    double inter = rectArea(overlap);
    return inter / (rectArea(boxA) + rectArea(boxB) - inter);
}

std::vector<double> shapeData(ImBMP &img)
{
    img.otsuThreshold();
    std::vector<Point> points;
    for (int y = 0; y < img.height; y++)
    {
        for (int x = 0; x < img.width; x++)
        {
            if ((img.getRGB(x, y) & 0xff) == 0)
            {
                points.push_back({x, y});
            }
        }
    }
    if (points.empty())
    {
        return {};
    }
    std::vector<int> box = boundingBox(points);
    double width = box[2] - box[0] + 1;
    double height = box[3] - box[1] + 1;
    std::vector<double> pattern(8, 0);
    pattern[0] = width / height;
    pattern[1] = points.size() / (width * height);
    for (auto &p : points)
    {
        int x = (p.x - box[0]) * 2 / width;
        int y = (p.y - box[1]) * 3 / height;
        pattern[2 + y * 2 + x] += 6.0 / (width * height);
    }
    return pattern;
}

bullet(destripe)
{
    ImBMP img("./resource/FinalDIP69.bmp");
    if (!img.ok())
    {
        return 1;
    }
    img.convertToGrayscale();
    ImBMP blurImage = img;
    blurImage.medianFilter(3);
    for (int y = 0; y < img.height; y++)
    {
        for (int x = 0; x < img.width; x++)
        {
            int gray = img.getRGB(x, y) & 0xff;
            if (gray == 0 || gray == 255)
            {
                img.setRGB(x, y, blurImage.getRGB(x, y));
            }
        }
    }

    int width = img.width;
    int height = img.height;

    std::vector<std::vector<double>> data{};
    data.resize(height);
    for (int y = 0; y < height; y++)
    {
        data[y].resize(width);
    }

    std::vector<std::vector<double>> columns(width, std::vector<double>(height));
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            columns[x][y] = img.getRGB(x, y) & 0xff;
        }
    }

    int average_win = 9;
    std::vector<std::vector<double>> smooth(width);
    for (int x = 0; x < width; x++)
    {
        smooth[x] = SMA(columns[x], average_win);
    }

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            data[y][x] = smooth[x][y];
        }
    }

    std::vector<double> diff = getDiff(data, 0, height);

    std::vector<double> pure(width, 0.0);
    std::vector<std::vector<double>> slot = data;
    int slotHeight = height;
    int slotWidth = width;
    int size = std::gcd(slotWidth, slotHeight);
    while (((slotHeight + size - 1) / size) % 2 == 0)
    {
        size++;
    }

    slotWidth = (slotWidth + size - 1) / size * size;
    slotHeight = (slotHeight + size - 1) / size * size;
    for (int y = 0; y < height; y++)
    {
        double v = slot[y].back();
        slot[y].resize(slotWidth, v);
    }
    slot.resize(slotHeight);
    for (int y = height; y < slotHeight; y++)
    {
        slot[y] = slot[height - 1];
    }
    std::vector<double> slotDiff = getDiff(slot, 0, slotHeight);
    int window = 51;
    std::vector<double> wave = getNoise(slotDiff, window);

    std::vector<std::vector<double>> waves{};
    int slots = slotHeight / size;
    waves.resize(slots);
    std::vector<std::vector<double>> diffs(slots);
    for (int r = 0; r < slots; r++)
    {
        int y0 = r * size;
        int y1 = y0 + size;
        diffs[r] = getDiff(slot, y0, y1);
    }

    for (int r = 0; r < slots; r++)
    {
        waves[r] = getNoise(diffs[r], window);
    }

    double ratio = 1.5;
    std::vector<double> a{};
    a.resize(slotWidth);
    std::fill(a.begin(), a.end(), 0.0);

    for (int start = 0; start < slotWidth; start += size)
    {
        int end = start + size;
        int n = end - start;

        std::vector<double> mid{};
        mid.resize(n);
        std::vector<double> v{};
        v.resize(slots);
        for (int i = 0; i < n; i++)
        {
            for (int r = 0; r < slots; r++)
            {
                v[r] = waves[r][start + i];
            }
            mid[i] = median(v);
        }

        std::vector<double> dist{};
        dist.resize(slots);
        for (int r = 0; r < slots; r++)
        {
            double sum = 0;
            for (int i = 0; i < n; i++)
            {
                double d = waves[r][start + i] - mid[i];
                sum += d * d;
            }
            dist[r] = sqrt(sum / n);
        }

        double limit = std::max(median(dist), 1e-6);

        std::vector<int> keep{};
        for (int r = 0; r < slots; r++)
        {
            if (dist[r] <= ratio * limit)
            {
                keep.push_back(r);
            }
        }

        for (int i = 0; i < n; i++)
        {
            double sum = 0;
            int count = (int)keep.size();
            for (int j = 0; j < count; j++)
            {
                int r = keep[j];
                sum += waves[r][start + i];
            }
            a[start + i] = count > 0 ? sum / count : 0.0;
        }
    }
    std::vector<std::vector<double>> error(slots, std::vector<double>(slotWidth));
    for (int r = 0; r < slots; r++)
    {
        for (int x = 0; x < slotWidth; x++)
        {
            error[r][x] = std::abs(waves[r][x] - wave[x]);
        }
    }

    int localWindow = 41;
    std::vector<std::vector<double>> avg(slots);
    for (int r = 0; r < slots; r++)
    {
        avg[r] = SMA(error[r], localWindow);
    }

    std::vector<double> errors{};
    for (int r = 0; r < slots; r++)
    {
        for (int x = 0; x < slotWidth; x++)
        {
            errors.push_back(avg[r][x]);
        }
    }
    double limit = std::max(median(errors), 1e-6);

    double localRatio = 3.0;
    std::vector<std::vector<double>> accepted(slotWidth);
    for (int x = 0; x < slotWidth; x++)
    {
        for (int r = 0; r < slots; r++)
        {
            if (avg[r][x] <= localRatio * limit)
            {
                accepted[x].push_back(waves[r][x]);
            }
        }
    }

    std::vector<double> b(slotWidth, 0.0);
    for (int x = 0; x < slotWidth; x++)
    {
        double sum = 0;
        int count = (int)accepted[x].size();
        for (int i = 0; i < count; i++)
        {
            sum += accepted[x][i];
        }
        b[x] = count > 0 ? sum / count : 0.0;
    }

    int c = 0;
    for (int x = 0; x < slotWidth; x++)
    {
        c += slots - (int)accepted[x].size();
    }

    pure.resize(slotWidth);
    std::fill(pure.begin(), pure.end(), 0.0);

    int n = 1;
    while (n < slotWidth)
    {
        n <<= 1;
    }

    std::vector<Complex> f(n, Complex(0, 0));
    std::vector<Complex> f1(n, Complex(0, 0));
    std::vector<Complex> f2(n, Complex(0, 0));
    for (int x = 0; x < slotWidth; x++)
    {
        f[x] = Complex(wave[x], 0);
        f1[x] = Complex(b[x], 0);
        f2[x] = Complex(a[x], 0);
    }

    fft(f.data(), n, false);
    fft(f1.data(), n, false);
    fft(f2.data(), n, false);

    int half = n / 2;
    std::vector<double> scale(half, 0.0);
    for (int k = 1; k < half; k++)
    {
        double mag = std::abs(f[k]);
        double s1 = mag > 1e-9 ? std::abs(f1[k]) / mag : 0.0;
        double s2 = mag > 1e-9 ? std::abs(f2[k]) / mag : 0.0;
        scale[k] = std::min(1.0, std::min(s1, s2));
    }

    double minRatio = 0.3;
    int count = 0;
    std::vector<int> keep{};
    for (int k = 1; k < half; k++)
    {
        if (scale[k] < minRatio)
        {
            count++;
            continue;
        }
        keep.push_back(k);
    }

    std::vector<Complex> filtered(n, Complex(0, 0));
    for (int i = 0; i < keep.size(); i++)
    {
        int k = keep[i];
        filtered[k] = f[k] * scale[k];
        filtered[n - k] = f[n - k] * scale[k];
    }

    fft(filtered.data(), n, true);

    for (int x = 0; x < slotWidth; x++)
    {
        pure[x] = filtered[x].real();
    }

    std::cout << "bands rejected " << count << "/" << (half - 1) << "  slot parts rejected " << c << "\n";
    pure.resize(width);

    int bins = 4;
    double minGray = 15;
    double maxGray = 235;
    std::vector<double> range{};
    range.resize(bins + 1);
    std::vector<double> v{};
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (data[y][x] > minGray && data[y][x] < maxGray)
            {
                v.push_back(data[y][x]);
            }
        }
    }
    std::sort(v.begin(), v.end());
    for (int b = 0; b <= bins; b++)
    {
        size_t i = std::min(v.size() - 1, (size_t)((double)v.size() * b / bins));
        range[b] = v[i];
    }
    range[bins] += 1e-6;

    std::vector<double> level{};
    level.resize(bins);
    std::fill(level.begin(), level.end(), 0.0);
    std::vector<double> total{};
    total.resize(bins);
    std::fill(total.begin(), total.end(), 0.0);
    std::vector<double> num{};
    num.resize(bins);
    std::fill(num.begin(), num.end(), 0.0);
    std::vector<double> den{};
    den.resize(bins);
    std::fill(den.begin(), den.end(), 0.0);

    std::vector<std::vector<double>> values{};
    values.resize(bins);
    for (int x = 0; x < width - 1; x++)
    {
        for (int b = 0; b < bins; b++)
        {
            values[b].clear();
        }
        for (int y = 0; y < height; y++)
        {
            double a = data[y][x];
            double c = data[y][x + 1];
            double mid = (a + c) * 0.5;
            if (mid > minGray && mid < maxGray)
            {
                for (int b = 0; b < bins; b++)
                {
                    if (mid >= range[b] && mid < range[b + 1])
                    {
                        values[b].push_back(c - a);
                        level[b] += mid;
                        total[b] += 1;
                        break;
                    }
                }
            }
        }
        for (int b = 0; b < bins; b++)
        {
            if (values[b].size() >= 30)
            {
                num[b] += median(values[b]) * diff[x];
                den[b] += diff[x] * diff[x];
            }
        }
    }

    for (int b = 0; b < bins; b++)
    {
        level[b] = total[b] > 0 ? level[b] / total[b] : 0.0;
    }

    double maxScale = 3.0;
    double weight = 0.15;
    std::vector<double> scales{};
    scales.resize(bins);
    for (int b = 0; b < bins; b++)
    {
        double scale = den[b] > 1e-9 ? num[b] / den[b] : 1.0;
        scale = 1.0 + weight * (scale - 1.0);
        scales[b] = std::min(maxScale, std::max(0.0, scale));
    }

    ImBMP out = img;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int gray = img.getRGB(x, y) & 0xff;
            double scale = 1.0;
            if (gray <= level[0])
            {
                scale = scales[0] * std::max(0, gray) / std::max(level[0], 1.0);
            }
            else if (gray >= level[bins - 1])
            {
                scale = scales[bins - 1];
            }
            else
            {
                for (int b = 0; b + 1 < bins; b++)
                {
                    if (gray >= level[b] && gray < level[b + 1])
                    {
                        double t = (gray - level[b]) / (level[b + 1] - level[b]);
                        scale = scales[b] * (1.0 - t) + scales[b + 1] * t;
                        break;
                    }
                }
            }
            double gray_overflow = gray - scale * pure[x];
            int q = std::min(255, std::max(0, (int)round(gray_overflow)));
            out.setRGB(x, y, (q << 16) | (q << 8) | q);
        }
    }

    std::cout << "scale";
    for (int b = 0; b < bins; b++)
    {
        std::cout << " " << round(scales[b] * 100) / 100.0 << "@" << (int)level[b];
    }
    std::cout << "\n";

    ImBMP graph = Graph(width);
    for (int x = 0; x < width; x++)
    {
        int y = std::min(299, std::max(0, (int)round(150 + pure[x] * 8)));
        graph.setRGB(x, y, 0xFFFFFF);
    }
    graph.write("./final-profile.bmp");
    // out.kMeansClustering(2);
    ImBMP cleaned = out;
    cleaned.write("./code_cleaned.bmp");

    out.adjustContrast(255);
    out.kMeansClustering(2);
    out.otsuThreshold();
    out.write("./final-otsu.bmp");
    out.erode(squareSE());
    out.dilate(squareSE());
    out.dilate(squareSE());
    out.erode(squareSE());
    ImBMP eroded = out;
    eroded.erode(squareSE());
    out = out - eroded;
    out.write("./final-erode.bmp");
    ImBMP hough = out;
    std::vector<std::tuple<Point, Point, std::tuple<double, double>>> lines = hough.houghTransform(0.4, "./HoughArray-" + bullet_name + ".bmp");
    for (auto &[start, end, angle] : lines)
    {
        auto &[sin, cos] = angle;
        std::cout << "[" << start.x << "," << start.y << "], [" << end.x << "," << end.y << "]" << "[" << sin << "," << cos << "]" << std::endl;
    }

    std::vector<std::tuple<Point, Point, Point, Point>> pairs{};
    for (int i = 0; i < lines.size(); i++)
    {
        for (int j = i + 1; j < lines.size(); j++)
        {
            auto &[start1, end1, _1] = lines[i];
            auto &[start2, end2, _2] = lines[j];
            if (start1 == start2 && end1 == end2)
            {
                continue;
            }
            bool cross = lineIntersect(start1, end1, start2, end2);

            if (cross)
            {
                pairs.push_back({start1, end1, start2, end2});
            }

            // std::cout
            //     << lineIntersect(start1, end1, start2, end2)
            //     << " [" << start1.x << "," << start1.y << "], [" << end1.x << "," << end1.y << "]"
            //     << "\n  [" << start2.x << "," << start2.y << "], [" << end2.x << "," << end2.y << "]"
            //     << "\n";
        }
    }
    std::set<std::vector<std::tuple<int, int, int, int>>> found{};
    std::vector<std::vector<Point>> rects{};
    for (int i = 0; i < pairs.size(); i++)
    {
        for (int j = i + 1; j < pairs.size(); j++)
        {
            auto &[start1, end1, start2, end2] = pairs[i];
            auto &[start3, end3, start4, end4] = pairs[j];
            bool cross1 = lineIntersect(start1, end1, start3, end3);
            bool cross4 = lineIntersect(start2, end2, start4, end4);

            bool cross2 = lineIntersect(start1, end1, start4, end4);
            bool cross3 = lineIntersect(start2, end2, start3, end3);

            if ((cross1 && cross4) || (cross2 && cross3))
            {
                std::vector<std::tuple<int, int, int, int>> rect = {
                    {start1.x, start1.y, end1.x, end1.y}, {start2.x, start2.y, end2.x, end2.y}, {start3.x, start3.y, end3.x, end3.y}, {start4.x, start4.y, end4.x, end4.y}};
                // sort for dedupe
                std::sort(rect.begin(), rect.end());
                if (found.insert(rect).second)
                {
                    std::vector<std::tuple<Point, Point>> order{};
                    if (cross2 && cross3)
                    {
                        order = {{start1, end1}, {start2, end2}, {start3, end3}, {start4, end4}};
                    }
                    else
                    {
                        order = {{start1, end1}, {start2, end2}, {start4, end4}, {start3, end3}};
                    }
                    std::vector<Point> corners{};
                    for (int k = 0; k < 4; k++)
                    {
                        auto &[s1, e1] = order[k];
                        auto &[s2, e2] = order[(k + 1) % 4];
                        corners.push_back(intersectPoint(s1, e1, s2, e2));
                    }
                    rects.push_back(corners);
                }
            }
        }
    }

    for (auto &corners : rects)
    {
        double turn = (corners[1].x - corners[0].x) * (corners[2].y - corners[0].y) - (corners[1].y - corners[0].y) * (corners[2].x - corners[0].x);
        if (turn < 0)
        {
            std::swap(corners[1], corners[3]);
        }
    }

    std::sort(rects.begin(), rects.end(), [](std::vector<Point> &a, std::vector<Point> &b) { return rectArea(boundingBox(a)) > rectArea(boundingBox(b)); });
    double sameArea = 0.8;
    std::vector<std::vector<Point>> kept{};
    for (auto &rect : rects)
    {
        bool found = false;
        for (auto &other : kept)
        {
            if (boxIoU(rect, other) > sameArea)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            kept.push_back(rect);
        }
    }
    rects = kept;

    std::vector<ImBMP> crops{};
    for (int r = 0; r < rects.size(); r++)
    {
        std::vector<Point> corners = rects[r];
        double wa = std::hypot(corners[1].x - corners[0].x, corners[1].y - corners[0].y);
        double wb = std::hypot(corners[2].x - corners[3].x, corners[2].y - corners[3].y);
        double ha = std::hypot(corners[3].x - corners[0].x, corners[3].y - corners[0].y);
        double hb = std::hypot(corners[2].x - corners[1].x, corners[2].y - corners[1].y);
        double w = (int)round(std::max(wa, wb));
        double h = (int)round(std::max(ha, hb));
        if (w < 32 || h < 32)
        {
            continue;
        }

        std::vector<std::vector<double>> src{};
        for (auto &c : corners)
        {
            src.push_back({(double)c.x, (double)(height - 1 - c.y)});
        }
        double outW = 700;
        double outH = 350;
        for (int k = 0; k < 4; k++)
        {
            std::vector<std::vector<double>> dst = {
                {outW - 1, 0},
                {0, 0},
                {0, outH - 1},
                {outW - 1, outH - 1},
            };
            // rotate dst
            std::vector<std::vector<double>> shifted{};
            for (int i = 0; i < 4; i++)
            {
                shifted.push_back(dst[(i + k) % 4]);
            }

            ImBMP crop = cleaned;
            std::vector<double> H = crop.calculateHomography(src, shifted);
            crop.applyHomography(H, outW, outH);
            crops.push_back(crop);
            crop.write("./rect-" + std::to_string(r) + "-" + std::to_string(k) + ".bmp");
        }
    }

    hough.write("./hough-" + bullet_name + ".bmp");
    out.write("./final-result.bmp");

    std::vector<std::string> classNames = {
        "E13B_0",
        "E13B_1",
        "E13B_2",
        "E13B_3",
        "E13B_4",
        "E13B_5",
        "E13B_6",
        "E13B_7",
        "E13B_8",
        "E13B_9",
    };
    std::vector<std::string> classPaths = {
        "./resource/prototypes/E13B_0.bmp",
        "./resource/prototypes/E13B_1.bmp",
        "./resource/prototypes/E13B_2.bmp",
        "./resource/prototypes/E13B_3.bmp",
        "./resource/prototypes/E13B_4.bmp",
        "./resource/prototypes/E13B_5.bmp",
        "./resource/prototypes/E13B_6.bmp",
        "./resource/prototypes/E13B_7.bmp",
        "./resource/prototypes/E13B_8.bmp",
        "./resource/prototypes/E13B_9.bmp",
    };

    std::vector<std::vector<double>> prototypes;
    for (std::size_t i = 0; i < classPaths.size(); i++)
    {
        ImBMP img(classPaths[i]);
        std::vector<double> pattern = shapeData(img);
        prototypes.push_back(pattern);
        printf("%s: aspect=%.4f normArea=%.4f\n", classNames[i].c_str(), pattern[0], pattern[1]);
    }

    ImBMP bestCard;
    ImBMP bestCharacters;
    std::string bestCode;
    double bestQuality = 0;
    StructuringElement horizontal = horizontalSE();
    StructuringElement vertical = verticalSE();
    StructuringElement block = blockSE();
    StructuringElement sides = sidesSE();
    for (auto &crop : crops)
    {
        ImBMP image = crop;
        image.otsuThreshold();
        std::vector<std::tuple<int, ImBMP>> parts{};
        int width = image.width;
        int height = image.height;
        double rowRatio = 0.04;
        double columnRatio = 0.05;
        double minWidthRatio = 0.15;
        double maxWidthRatio = 0.95;
        int minHeight = std::max(12, height / 10);
        int maxHeight = height / 2;

        std::vector<int> rows{};
        rows.resize(height);
        std::fill(rows.begin(), rows.end(), 0);
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int gray = image.getRGB(x, y) & 0xff;
                if (gray == 0)
                {
                    rows[y]++;
                }
            }
        }
        for (int y = 0; y < height; y++)
        {
            if (rows[y] < width * rowRatio)
            {
                continue;
            }
            int y0 = y;
            while (y < height && rows[y] >= width * rowRatio)
            {
                y++;
            }
            int h = y - y0;
            if (h < minHeight || h >= maxHeight)
            {
                continue;
            }
            std::vector<int> columns{};
            columns.resize(width);
            std::fill(columns.begin(), columns.end(), 0);
            for (int x = 0; x < width; x++)
            {
                for (int row = y0; row < y; row++)
                {
                    int gray = image.getRGB(x, row) & 0xff;
                    if (gray == 0)
                    {
                        columns[x]++;
                    }
                }
            }
            for (int x = 0; x < width; x++)
            {
                if (columns[x] < h * columnRatio)
                {
                    continue;
                }
                int x0 = x;
                while (x < width && columns[x] >= h * columnRatio)
                {
                    x++;
                }
                int w = x - x0;
                if (w < h * minWidthRatio || w > h * maxWidthRatio)
                {
                    continue;
                }
                ImBMP region = Graph(w + 4, h + 4);
                std::fill(region.data.begin(), region.data.end(), 255);
                for (int row = 0; row < h; row++)
                {
                    for (int col = 0; col < w; col++)
                    {
                        region.setRGB(col + 2, row + 2, crop.getRGB(x0 + col, y0 + row));
                    }
                }
                region.otsuThreshold();

                ImBMP horizontals = region;
                horizontals.erode(horizontal);
                horizontals.dilate(horizontal);
                ImBMP verticals = region;
                verticals.erode(vertical);
                verticals.dilate(vertical);

                ImBMP dots = region;
                dots.erode(crossSE());
                dots.dilate(crossSE());
                dots = region - dots;
                region.invert();
                ImBMP dotSupport = region;
                dotSupport.erode(sides);
                ImBMP outside = dots - dotSupport;
                dots = dots - outside;
                region.dilate(block);
                region.erode(block);
                region.erode(vertical);
                region.dilate(vertical);
                region.erode(horizontal);
                region.dilate(horizontal);
                region = region - horizontals;
                region = region - verticals;
                region = region - dots;
                region.invert();
                parts.push_back({x0, region});
            }
        }
        std::sort(parts.begin(), parts.end(), [](auto &a, auto &b) {
            auto &[a1, a2] = a;
            auto &[b1, b2] = b;
            return a1 < b1;
        });
        std::string code;
        double quality = 0;
        for (auto &[x, region] : parts)
        {
            std::vector<double> pattern = shapeData(region);
            int label = img.minimumDistanceClassifier(pattern, prototypes);
            double distance = 0;
            for (int j = 0; j < pattern.size(); j++)
            {
                double difference = pattern[j] - prototypes[label][j];
                distance += difference * difference;
            }
            if (distance > 0.2)
            {
                code.clear();
                break;
            }
            code += char('0' + label);
            quality += 1.0 - distance;
        }
        printf("score=%.4f code=%s\n", quality, code.c_str());
        if (code.size() == 6 && quality > bestQuality)
        {
            bestQuality = quality;
            bestCode = code;
            bestCard = crop;
            bestCharacters = Graph(600, 120);
            std::fill(bestCharacters.data.begin(), bestCharacters.data.end(), 255);
            for (int i = 0; i < parts.size(); i++)
            {
                auto &[a, b] = parts[i];
                ImBMP character = b;
                double scale = std::min(90.0 / character.width, 110.0 / character.height);
                character.resizeNearestNeighbour(scale, scale);
                int x0 = i * 100 + (100 - character.width) / 2;
                int y0 = (120 - character.height) / 2;
                for (int y = 0; y < character.height; y++)
                {
                    for (int x = 0; x < character.width; x++)
                    {
                        bestCharacters.setRGB(x0 + x, y0 + y, character.getRGB(x, y));
                    }
                }
            }
        }
    }
    if (bestCode.empty())
    {
        std::cerr << "No six-digit code found.\n";
        return 1;
    }
    bestCard.write("./code_rectified.bmp");
    bestCharacters.write("./code_characters.bmp");
    std::cout << "Detected digits: " << bestCode << "\n";
    std::ofstream decoded("./decoded.txt");
    decoded << "Decoded number: " << bestCode;
    decoded.close();
    return 0;
}
