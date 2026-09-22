#ifndef IM_HPP
#define IM_HPP

#define _USE_MATH_DEFINES
#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>

class FD;

#define BYTE 8
#define BMP_COLOR_TABLE_SIZE 1024
#define BMP_HEADER_SIZE 54

#include <list>
#include <vector>
struct Point
{
    int x;
    int y;
    bool operator==(const Point &rhs) const
    {
        return x == rhs.x && y == rhs.y;
    }
};
struct StructuringElement
{
    std::vector<int> elements;
    struct Point origin;
    std::list<struct Point> ignoreElements;
    int width;
    int height;
    StructuringElement(int w, int h, struct Point o)
    {
        width = w;
        height = h;
        origin.x = o.x;
        origin.y = o.y;
        elements.resize(width * height);
    }
};

class ImBMP
{
public:
    int width = 0;
    int height = 0;
    int originalWidth;
    int originalHeight;
    int bitDepth = 0;
    unsigned char header[BMP_HEADER_SIZE] = {};
    unsigned char colorTable[BMP_COLOR_TABLE_SIZE] = {};
    std::vector<unsigned char> origin;
    std::vector<unsigned char> data;

    ImBMP() = default;

    explicit ImBMP(const std::string &filename) { read(filename); }

    int c_size()
    {
        return height * width * (bitDepth / BYTE);
    }

    bool ok() const
    {
        return !data.empty();
    }

    bool read(const std::string &filename)
    {
        FILE *fi = fopen(filename.c_str(), "rb");
        if (!fi)
        {
            fprintf(stderr, "Unable to open file: %s\n", filename.c_str());
            return false;
        }

        fread(header, 1, BMP_HEADER_SIZE, fi);
        width = *(int *)&header[18];
        height = *(int *)&header[22];
        bitDepth = *(short *)&header[28];
        originalWidth = width;
        originalHeight = height;
        if (bitDepth <= 8)
        {
            fread(colorTable, 1, BMP_COLOR_TABLE_SIZE, fi);
        }

        data.resize((size_t)width * height * (bitDepth / 8));
        origin.resize((size_t)width * height * (bitDepth / 8));
        fread(origin.data(), 1, origin.size(), fi);
        data = origin;

        fclose(fi);
        return true;
    }

    bool write(const std::string &filename)
    {
        FILE *fo = fopen(filename.c_str(), "wb");
        if (fo == (FILE *)0)
        {
            std::cout << "Unable to create file" << std::endl;
            return false;
        }
        int bytePerPixel = bitDepth / BYTE;
        int rowSize = width * bytePerPixel;
        int paddingSize = (4 - rowSize % 4) % 4;
        int imageSize = (rowSize + paddingSize) * height;
        int dataOffset = *(int *)(&header[10]);
        int fileSize = dataOffset + imageSize;
        header[2] = (unsigned char)(fileSize & 0xff);
        header[3] = (unsigned char)((fileSize >> 8) & 0xff);
        header[4] = (unsigned char)((fileSize >> 16) & 0xff);
        header[5] = (unsigned char)((fileSize >> 24) & 0xff);
        header[18] = (unsigned char)(width & 0xff);
        header[19] = (unsigned char)((width >> 8) & 0xff);
        header[20] = (unsigned char)((width >> 16) & 0xff);
        header[21] = (unsigned char)((width >> 24) & 0xff);
        header[22] = (unsigned char)(height & 0xff);
        header[23] = (unsigned char)((height >> 8) & 0xff);
        header[24] = (unsigned char)((height >> 16) & 0xff);
        header[25] = (unsigned char)((height >> 24) & 0xff);
        header[34] = (unsigned char)(imageSize & 0xff);
        header[35] = (unsigned char)((imageSize >> 8) & 0xff);
        header[36] = (unsigned char)((imageSize >> 16) & 0xff);
        header[37] = (unsigned char)((imageSize >> 24) & 0xff);
        fwrite(header, sizeof(unsigned char), BMP_HEADER_SIZE, fo);
        if (bitDepth <= 8)
        {
            fwrite(colorTable, sizeof(unsigned char), BMP_COLOR_TABLE_SIZE, fo);
        }
        unsigned char pad[3] = {0, 0, 0};
        for (int y = 0; y < height; y++)
        {
            fwrite(data.data() + (size_t)y * rowSize, sizeof(unsigned char), rowSize, fo);
            if (paddingSize > 0)
            {
                fwrite(pad, sizeof(unsigned char), paddingSize, fo);
            }
        }
        std::cout << "Image " << filename << " has been written!" << std::endl;
        fclose(fo);
        return true;
    }

    void resizeNearestNeighbour(double scaleX, double scaleY)
    {
        if (scaleX <= 0 || scaleY <= 0)
        {
            std::cout << "Scale must be greater than zero." << std::endl;
            return;
        }
        int newWidth = (int)round(width * scaleX);
        int newHeight = (int)round(height * scaleY);
        newWidth = newWidth < 1 ? 1 : newWidth;
        newHeight = newHeight < 1 ? 1 : newHeight;
        int *tempBuf = new int[newHeight * newWidth];
        for (int y = 0; y < newHeight; y++)
        {
            for (int x = 0; x < newWidth; x++)
            {
                double oldX = (x + 0.5) / scaleX - 0.5;
                double oldY = (y + 0.5) / scaleY - 0.5;
                int xNearest = (int)round(oldX);
                int yNearest = (int)round(oldY);
                xNearest = xNearest >= width ? width - 1 : xNearest;
                xNearest = xNearest < 0 ? 0 : xNearest;
                yNearest = yNearest >= height ? height - 1 : yNearest;
                yNearest = yNearest < 0 ? 0 : yNearest;
                tempBuf[y * newWidth + x] = getRGB(xNearest, yNearest);
            }
        }
        width = newWidth;
        height = newHeight;
        data.resize((size_t)width * height * (bitDepth / BYTE));
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
    }

    void resizeBilinear(double scaleX, double scaleY)
    {
        if (scaleX <= 0 || scaleY <= 0)
        {
            std::cout << "Scale must be greater than zero." << std::endl;
            return;
        }
        int newWidth = (int)round(width * scaleX);
        int newHeight = (int)round(height * scaleY);
        newWidth = newWidth < 1 ? 1 : newWidth;
        newHeight = newHeight < 1 ? 1 : newHeight;
        int *tempBuf = new int[newHeight * newWidth];
        for (int y = 0; y < newHeight; y++)
        {
            for (int x = 0; x < newWidth; x++)
            {
                double oldX = (x + 0.5) / scaleX - 0.5;
                double oldY = (y + 0.5) / scaleY - 0.5;
                oldX = oldX < 0 ? 0 : oldX;
                oldX = oldX > width - 1 ? width - 1 : oldX;
                oldY = oldY < 0 ? 0 : oldY;
                oldY = oldY > height - 1 ? height - 1 : oldY;
                // get 4 coordinates
                int x1 = (int)std::floor(oldX);
                int y1 = (int)std::floor(oldY);
                int x2 = std::min(x1 + 1, width - 1);
                int y2 = std::min(y1 + 1, height - 1);

                // get colours
                int color11 = getRGB(x1, y1);
                int r11 = (color11 >> 16) & 0xff;
                int g11 = (color11 >> 8) & 0xff;
                int b11 = color11 & 0xff;
                int color12 = getRGB(x2, y1);
                int r12 = (color12 >> 16) & 0xff;
                int g12 = (color12 >> 8) & 0xff;
                int b12 = color12 & 0xff;
                int color21 = getRGB(x1, y2);
                int r21 = (color21 >> 16) & 0xff;
                int g21 = (color21 >> 8) & 0xff;
                int b21 = color21 & 0xff;
                int color22 = getRGB(x2, y2);
                int r22 = (color22 >> 16) & 0xff;
                int g22 = (color22 >> 8) & 0xff;
                int b22 = color22 & 0xff;
                // interpolate x
                double dx = oldX - x1;
                double P1r = (1 - dx) * r11 + dx * r12;
                double P1g = (1 - dx) * g11 + dx * g12;
                double P1b = (1 - dx) * b11 + dx * b12;
                double P2r = (1 - dx) * r21 + dx * r22;
                double P2g = (1 - dx) * g21 + dx * g22;
                double P2b = (1 - dx) * b21 + dx * b22;
                // interpolate y
                double dy = oldY - y1;
                double Pr = (1 - dy) * P1r + dy * P2r;
                double Pg = (1 - dy) * P1g + dy * P2g;
                double Pb = (1 - dy) * P1b + dy * P2b;
                int r = (int)round(Pr);
                int g = (int)round(Pg);
                int b = (int)round(Pb);
                r = r > 255 ? 255 : r;
                r = r < 0 ? 0 : r;
                g = g > 255 ? 255 : g;
                g = g < 0 ? 0 : g;
                b = b > 255 ? 255 : b;
                b = b < 0 ? 0 : b;
                int newColor = (r << 16) | (g << 8) | b;
                tempBuf[y * newWidth + x] = newColor;
            }
        }
        width = newWidth;
        height = newHeight;
        data.resize((size_t)width * height * (bitDepth / BYTE));
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
    }

    int getRGB(int x, int y)
    {
        int i = y * width * (bitDepth / BYTE) + x * (bitDepth / BYTE);
        int b = data[i];
        int g = data[i + 1];
        int r = data[i + 2];
        int color = (r << 16) | (g << 8) | b;
        return color;
    }

    ImBMP operator-(ImBMP &other)
    {
        ImBMP result = *this;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int a = getRGB(x, y) & 0xff;
                int b = other.getRGB(x, y) & 0xff;
                int val = (a == 255 && b == 0) ? 255 : 0;
                result.setRGB(x, y, (val << 16) | (val << 8) | val);
            }
        }
        return result;
    }

    void setRGB(int x, int y, int color)
    {
        int r = (color >> 16) & 0xff;
        int g = (color >> 8) & 0xff;
        int b = color & 0xff;
        int i = y * width * (bitDepth / BYTE) + x * (bitDepth / BYTE);
        data[i] = b;
        data[i + 1] = g;
        data[i + 2] = r;
    }

    void extractRGB(int x, int y, int *r, int *g, int *b)
    {
        int color = getRGB(x, y);
        *r = (color >> 16) & 0xff;
        *g = (color >> 8) & 0xff;
        *b = color & 0xff;
    }

    void convertToRed()
    {
        int r, g, b;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                extractRGB(x, y, &r, &g, &b);
                int color = (r << 16) | (0 << 8) | 0;
                setRGB(x, y, color);
            }
        }
    }

    void convertToGreen()
    {
        int r, g, b;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                extractRGB(x, y, &r, &g, &b);
                int color = (0 << 16) | (g << 8) | 0;
                setRGB(x, y, color);
            }
        }
    }

    void convertToBlue()
    {
        int r, g, b;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                extractRGB(x, y, &r, &g, &b);
                int color = (0 << 16) | (0 << 8) | b;
                setRGB(x, y, color);
            }
        }
    }

    void erode(const StructuringElement &se)
    {
        convertToGrayscale();
        int *tempBuf = new int[height * width];
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                bool isEroded = true;
                for (int seY = 0; seY < se.height; seY++)
                {
                    for (int seX = 0; seX < se.width; seX++)
                    {
                        struct Point checkingPoint;
                        checkingPoint.x = seX;
                        checkingPoint.y = seY;
                        if (std::find(se.ignoreElements.begin(), se.ignoreElements.end(), checkingPoint)

                                != se.ignoreElements.end() ||
                            se.elements[seY * se.width + seX] != 255)

                            continue;
                        int imageX = x + seX - se.origin.x;
                        int imageY = y + seY - se.origin.y;
                        if (imageX < 0 || imageX >= width ||
                            imageY < 0 || imageY >= height)
                        {
                            isEroded = false;
                            goto se_check;
                        }
                        int color = getRGB(imageX, imageY);
                        int gray = color & 0xff;
                        if (gray != 255)
                        {
                            isEroded = false;
                            goto se_check;
                        }
                    }
                }
            se_check:
                int newGray = isEroded ? 255 : 0;
                int newColor = (newGray << 16) | (newGray << 8) | newGray;
                tempBuf[y * width + x] = newColor;
            }
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
        delete[] tempBuf;
    }

    void dilate(const StructuringElement &se)
    {
        convertToGrayscale();
        int *tempBuf = new int[height * width];
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                bool isDilated = false;
                for (int seY = 0; seY < se.height; seY++)
                {
                    for (int seX = 0; seX < se.width; seX++)
                    {
                        struct Point checkingPoint;
                        checkingPoint.x = seX;
                        checkingPoint.y = seY;
                        if (std::find(se.ignoreElements.begin(), se.ignoreElements.end(), checkingPoint)

                                != se.ignoreElements.end() ||
                            se.elements[seY * se.width + seX] != 255)

                            continue;
                        int imageX = x + seX - se.origin.x;
                        int imageY = y + seY - se.origin.y;
                        if (imageX < 0 || imageX >= width ||
                            imageY < 0 || imageY >= height)
                        {
                            continue;
                        }
                        int color = getRGB(imageX, imageY);
                        int gray = color & 0xff;
                        if (gray == 255)
                        {
                            isDilated = true;
                            goto se_check;
                        }
                    }
                }
            se_check:
                int newGray = isDilated ? 255 : 0;
                int newColor = (newGray << 16) | (newGray << 8) | newGray;
                tempBuf[y * width + x] = newColor;
            }
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
        delete[] tempBuf;
    }

    void convertToGrayscale()
    {
        int r, g, b, avg;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                extractRGB(x, y, &r, &g, &b);
                avg = (r + g + b) / 3;
                int color = (avg << 16) | (avg << 8) | avg;
                setRGB(x, y, color);
            }
        }
    }

    void adjustBrightness(int brightness)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int color = getRGB(x, y);
                int r = (color >> 16) & 0xff;
                int g = (color >> 8) & 0xff;
                int b = color & 0xff;

                r = r + brightness;
                r = r > 255 ? 255 : r;
                r = r < 0 ? 0 : r;
                g = g + brightness;
                g = g > 255 ? 255 : g;
                g = g < 0 ? 0 : g;
                b = b + brightness;
                b = b > 255 ? 255 : b;
                b = b < 0 ? 0 : b;
                color = (r << 16) | (g << 8) | b;
                setRGB(x, y, color);
            }
        }
    }

    void invert()
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int color = getRGB(x, y);
                int r = (color >> 16) & 0xff;
                int g = (color >> 8) & 0xff;
                int b = color & 0xff;

                r = 255 - r;
                g = 255 - g;
                b = 255 - b;
                color = (r << 16) | (g << 8) | b;
                setRGB(x, y, color);
            }
        }
    }

    int *getGrayscaleHistogram()
    {
        convertToGrayscale();
        int *histogram = new int[256];
        for (int i = 0; i < 256; i++)
        {
            histogram[i] = 0;
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int color = getRGB(x, y);
                int gray = color & 0xff;
                histogram[gray]++;
            }
        }
        // restore();
        return histogram;
    }

    float getContrast()
    {
        float contrast = 0;
        int *histogram = getGrayscaleHistogram();
        float avgIntensity = 0;
        float pixelNum = width * height;
        for (int i = 0; i < 256; i++)
        {
            avgIntensity += histogram[i] * i;
        }
        avgIntensity /= pixelNum;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int color = getRGB(x, y);
                int value = color & 0xff;
                contrast += pow((value)-avgIntensity, 2);
            }
        }
        contrast = (float)sqrt(contrast / pixelNum);
        return contrast;
    }

    void adjustContrast(int contrast)
    {
        float currentContrast = getContrast();
        int *histogram = getGrayscaleHistogram();
        float avgIntensity = 0;
        float pixelNum = width * height;
        for (int i = 0; i < 256; i++)
        {
            avgIntensity += histogram[i] * i;
        }
        avgIntensity /= pixelNum;
        float min = avgIntensity - currentContrast;
        float max = avgIntensity + currentContrast;
        float newMin = avgIntensity - currentContrast - contrast / 2;
        float newMax = avgIntensity + currentContrast + contrast / 2;
        newMin = newMin < 0 ? 0 : newMin;
        newMax = newMax < 0 ? 0 : newMax;
        newMin = newMin > 255 ? 255 : newMin;
        newMax = newMax > 255 ? 255 : newMax;
        if (newMin > newMax)
        {
            float temp = newMax;
            newMax = newMin;
            newMin = temp;
        }
        float contrastFactor = (newMax - newMin) / (max - min);
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int color = getRGB(x, y);
                int r = (color >> 16) & 0xff;
                int g = (color >> 8) & 0xff;
                int b = color & 0xff;
                r = (int)((r - min) * contrastFactor + newMin);
                r = r > 255 ? 255 : r;
                r = r < 0 ? 0 : r;
                g = (int)((g - min) * contrastFactor + newMin);
                g = g > 255 ? 255 : g;
                g = g < 0 ? 0 : g;
                b = (int)((b - min) * contrastFactor + newMin);
                b = b > 255 ? 255 : b;
                b = b < 0 ? 0 : b;
                color = (r << 16) | (g << 8) | b;
                setRGB(x, y, color);
            }
        }
    }

    void adjustGamma(double gamma)
    {
        double c = 255.0;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int color = getRGB(x, y);
                int r = (color >> 16) & 0xff;
                int g = (color >> 8) & 0xff;
                int b = color & 0xff;

                r = (int)round(c * pow(r / c, gamma));
                r = r > 255 ? 255 : r;
                r = r < 0 ? 0 : r;
                g = (int)round(c * pow(g / c, gamma));
                g = g > 255 ? 255 : g;
                g = g < 0 ? 0 : g;
                b = (int)round(c * pow(b / c, gamma));
                b = b > 255 ? 255 : b;
                b = b < 0 ? 0 : b;
                color = (r << 16) | (g << 8) | b;
                setRGB(x, y, color);
            }
        }
    }

    void averagingFilter(int size)
    {
        if (size % 2 == 0)
        {
            std::cout << "Size Invalid: must be odd number!" << std::endl;
            return;
        }
        int *tempBuf = new int[height * width];
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int sumRed = 0, sumGreen = 0, sumBlue = 0;
                for (int i = y - size / 2; i <= y + size / 2; i++)
                {
                    for (int j = x - size / 2; j <= x + size / 2; j++)
                    {
                        if (i >= 0 && i < height && j >= 0 && j < width)
                        {
                            int color = getRGB(j, i);
                            int r = (color >> 16) & 0xff;
                            int g = (color >> 8) & 0xff;
                            int b = color & 0xff;
                            sumRed += r;
                            sumGreen += g;
                            sumBlue += b;
                        }
                    }
                }
                sumRed /= (size * size);
                sumRed = sumRed > 255 ? 255 : sumRed;
                sumRed = sumRed < 0 ? 0 : sumRed;
                sumGreen /= (size * size);
                sumGreen = sumGreen > 255 ? 255 : sumGreen;
                sumGreen = sumGreen < 0 ? 0 : sumGreen;
                sumBlue /= (size * size);
                sumBlue = sumBlue > 255 ? 255 : sumBlue;
                sumBlue = sumBlue < 0 ? 0 : sumBlue;
                int newColor = (sumRed << 16) | (sumGreen << 8) | sumBlue;
                tempBuf[y * width + x] = newColor;
            }
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
        delete[] tempBuf;
    }

    void unsharpMasking(int size, double k)
    {
        if (size % 2 == 0)
        {
            std::cout << "Size Invalid: must be odd number!" << std::endl;
            return;
        }
        std::vector<unsigned char> original = data;
        averagingFilter(size);
        int bytePerPixel = bitDepth / BYTE;
        for (size_t i = 0; i < data.size(); i += bytePerPixel)
        {
            for (int c = 0; c < bytePerPixel; c++)
            {
                int orig = original[i + c];
                int blurred = data[i + c];
                int mask = orig - blurred;
                int val = orig + (int)round(k * mask);
                val = val > 255 ? 255 : val;
                val = val < 0 ? 0 : val;
                data[i + c] = (unsigned char)val;
            }
        }
    }

    void medianFilter(int size)
    {
        if (size % 2 == 0)
        {
            std::cout << "Size Invalid: must be odd number!" << std::endl;
            return;
        }
        int *tempBuf = new int[height * width];
        std::vector<int> win;
        win.reserve(size * size);

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                win.clear();
                for (int dy = y - size / 2; dy <= y + size / 2; dy++)
                {
                    for (int dx = x - size / 2; dx <= x + size / 2; dx++)
                    {
                        int ny = std::clamp(dy, 0, height - 1);
                        int nx = std::clamp(dx, 0, width - 1);
                        int color = getRGB(nx, ny);
                        win.push_back(color);
                    }
                }

                size_t mid = win.size() / 2;
                std::nth_element(win.begin(), win.begin() + mid, win.end());
                tempBuf[y * width + x] = win[mid];
            }
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
        delete[] tempBuf;
    }

    FD *getFrequencyDomain();

    void addSaltNoise(double percent)
    {
        convertToGrayscale();
        double noOfPX = height * width;
        int noiseAdded = (int)(percent * noOfPX);
        int whiteColor = 255 << 16 | 255 << 8 | 255;
        for (int i = 1; i <= noiseAdded; i++)
        {
            int x = rand() % width;
            int y = rand() % height;
            setRGB(x, y, whiteColor);
        }
    }

    void addPepperNoise(double percent)
    {
        convertToGrayscale();
        double noOfPX = height * width;
        int noiseAdded = (int)(percent * noOfPX);
        int blackColor = 0;
        for (int i = 1; i <= noiseAdded; i++)
        {
            int x = rand() % width;
            int y = rand() % height;
            setRGB(x, y, blackColor);
        }
    }

    void addUniformNoise(double percent, int distribution)
    {
        convertToGrayscale();
        double noOfPX = height * width;
        int noiseAdded = (int)(percent * noOfPX);
        for (int i = 1; i <= noiseAdded; i++)
        {
            int x = rand() % width;
            int y = rand() % height;
            int color = getRGB(x, y);
            int gray = color & 0xff;
            gray += (rand() % (distribution * 2) - distribution);
            gray = gray > 255 ? 255 : gray;
            gray = gray < 0 ? 0 : gray;
            int newColor = gray << 16 | gray << 8 | gray;
            setRGB(x, y, newColor);
        }
    }

    void contraharmonicFilter(int size, double Q)
    {
        if (size % 2 == 0)
        {
            std::cout << "Size Invalid: must be odd number!" << std::endl;
            return;
        }
        int *tempBuf = new int[height * width];
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                double sumRedAbove = 0, sumGreenAbove = 0, sumBlueAbove = 0;
                double sumRedBelow = 0, sumGreenBelow = 0, sumBlueBelow = 0;
                for (int i = y - size / 2; i <= y + size / 2; i++)
                {
                    for (int j = x - size / 2; j <= x + size / 2; j++)
                    {
                        if (i >= 0 && i < height && j >= 0 && j < width)
                        {
                            int color = getRGB(j, i);
                            int r = (color >> 16) & 0xff;
                            int g = (color >> 8) & 0xff;
                            int b = color & 0xff;
                            sumRedAbove += pow(r, Q + 1);
                            sumGreenAbove += pow(g, Q + 1);
                            sumBlueAbove += pow(b, Q + 1);
                            sumRedBelow += pow(r, Q);
                            sumGreenBelow += pow(g, Q);
                            sumBlueBelow += pow(b, Q);
                        }
                    }
                }
                sumRedAbove /= sumRedBelow;
                sumRedAbove = sumRedAbove > 255 ? 255 : sumRedAbove;
                sumRedAbove = sumRedAbove < 0 ? 0 : sumRedAbove;
                sumGreenAbove /= sumGreenBelow;
                sumGreenAbove = sumGreenAbove > 255 ? 255 : sumGreenAbove;
                sumGreenAbove = sumGreenAbove < 0 ? 0 : sumGreenAbove;
                sumBlueAbove /= sumBlueBelow;
                sumBlueAbove = sumBlueAbove > 255 ? 255 : sumBlueAbove;
                sumBlueAbove = sumBlueAbove < 0 ? 0 : sumBlueAbove;

                int newColor = ((int)sumRedAbove << 16) | ((int)sumGreenAbove << 8) | (int)sumBlueAbove;

                tempBuf[y * width + x] = newColor;
            }
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
        delete[] tempBuf;
    }

    void alphaTrimmedFilter(int size, int d)
    {
        if (size % 2 == 0)
        {
            std::cout << "Size Invalid: must be odd number!" << std::endl;
            return;
        }
        int *tempBuf = new int[height * width];
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int *kernelRed = new int[size * size];
                int *kernelGreen = new int[size * size];
                int *kernelBlue = new int[size * size];
                for (int i = y - size / 2; i <= y + size / 2; i++)
                {
                    for (int j = x - size / 2; j <= x + size / 2; j++)
                    {
                        int ni = std::clamp(i, 0, height - 1);
                        int nj = std::clamp(j, 0, width - 1);
                        int color = getRGB(nj, ni);
                        int r = (color >> 16) & 0xff;
                        int g = (color >> 8) & 0xff;
                        int b = color & 0xff;
                        kernelRed[(i - (y - size / 2)) * size + (j - (x - size / 2))] = r;
                        kernelGreen[(i - (y - size / 2)) * size + (j - (x - size / 2))] = g;
                        kernelBlue[(i - (y - size / 2)) * size + (j - (x - size / 2))] = b;
                    }
                }
                for (int i = 0; i < size * size - 1; i++)
                {
                    for (int j = 0; j < size * size - i - 1; j++)
                    {
                        int temp;
                        if (kernelRed[j] > kernelRed[j + 1])
                        {
                            temp = kernelRed[j];
                            kernelRed[j] = kernelRed[j + 1];
                            kernelRed[j + 1] = temp;
                        }
                        if (kernelGreen[j] > kernelGreen[j + 1])
                        {
                            temp = kernelGreen[j];
                            kernelGreen[j] = kernelGreen[j + 1];
                            kernelGreen[j + 1] = temp;
                        }
                        if (kernelBlue[j] > kernelBlue[j + 1])
                        {
                            temp = kernelBlue[j];
                            kernelBlue[j] = kernelBlue[j + 1];
                            kernelBlue[j + 1] = temp;
                        }
                    }
                }
                int remainingPixel = size * size - d;
                int red = 0, green = 0, blue = 0;
                for (int i = 0; i < remainingPixel; i++)
                {
                    red += kernelRed[(d / 2) + i];
                    green += kernelGreen[(d / 2) + i];
                    blue += kernelBlue[(d / 2) + i];
                }

                red /= remainingPixel;
                red = red > 255 ? 255 : red;
                red = red < 0 ? 0 : red;
                green /= remainingPixel;
                green = green > 255 ? 255 : green;
                green = green < 0 ? 0 : green;
                blue /= remainingPixel;
                blue = blue > 255 ? 255 : blue;
                blue = blue < 0 ? 0 : blue;
                int newColor = (red << 16) | (green << 8) | blue;
                tempBuf[y * width + x] = newColor;
            }
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
        delete[] tempBuf;
    }

    void thresholding(int threshold)
    {
        convertToGrayscale();
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int color = getRGB(x, y);
                int gray = color & 0xff;
                gray = gray < threshold ? 0 : 255;
                color = (gray << 16) | (gray << 8) | gray;
                setRGB(x, y, color);
            }
        }
    }

    void otsuThreshold()
    {
        convertToGrayscale();
        int *histogram = new int[256];
        float *histogramNorm = new float[256];
        float *histogramCS = new float[256];
        float *histogramMean = new float[256];
        for (int i = 0; i < 256; i++)
        {
            histogram[i] = 0;
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int color = getRGB(x, y);
                int gray = color & 0xff;
                histogram[gray]++;
            }
        }
        float pixelNum = width * height;
        for (int i = 0; i < 256; i++)
        {
            histogramNorm[i] = histogram[i] / pixelNum;
        }
        for (int i = 0; i < 256; i++)
        {
            if (i == 0)
            {
                histogramCS[i] = histogramNorm[i];
                histogramMean[i] = 0;
            }
            else
            {
                histogramCS[i] = histogramCS[i - 1] + histogramNorm[i];
                histogramMean[i] = histogramMean[i - 1] + histogramNorm[i] * i;
            }
        }
        float globalMean = histogramMean[255];
        float max = 0;
        float maxVariance = -1;
        int countMax = 0;
        for (int i = 0; i < 256; i++)
        {
            if (histogramCS[i] <= 0 || histogramCS[i] >= 1)
            {
                continue;
            }
            float variance = (float)pow(
                                 globalMean * histogramCS[i] - histogramMean[i], 2) /
                             (histogramCS[i] * (1 - histogramCS[i]));
            if (variance > maxVariance)
            {
                maxVariance = variance;
                max = i;
                countMax = 1;
            }
            else if (variance == maxVariance)
            {
                countMax++;
                max = ((max * (countMax - 1)) + i) / countMax;
            }
        }
        int threshold = (int)round(max);
        std::cout << "Otsu threshold = " << threshold << std::endl;
        delete[] histogram;
        delete[] histogramNorm;
        delete[] histogramCS;
        delete[] histogramMean;
        thresholding(threshold);
    }

    void linearSpatialFilter(double *kernel, int size)
    {
        if (size % 2 == 0)
        {
            std::cout << "Size Invalid: must be odd number!" << std::endl;
            return;
        }
        int *tempBuf = new int[height * width];
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                double sumRed = 0, sumGreen = 0, sumBlue = 0;
                for (int i = y - size / 2; i <= y + size / 2; i++)
                {
                    for (int j = x - size / 2; j <= x + size / 2; j++)
                    {
                        if (i >= 0 && i < height && j >= 0 && j < width)
                        {
                            int color = getRGB(j, i);
                            int r = (color >> 16) & 0xff;
                            int g = (color >> 8) & 0xff;
                            int b = color & 0xff;
                            int kernelIndex = (i - (y - size / 2)) * size + (j - (x - size / 2));
                            sumRed += r * kernel[kernelIndex];
                            sumGreen += g * kernel[kernelIndex];
                            sumBlue += b * kernel[kernelIndex];
                        }
                    }
                }
                sumRed = sumRed > 255 ? 255 : sumRed;
                sumRed = sumRed < 0 ? 0 : sumRed;
                sumGreen = sumGreen > 255 ? 255 : sumGreen;
                sumGreen = sumGreen < 0 ? 0 : sumGreen;
                sumBlue = sumBlue > 255 ? 255 : sumBlue;
                sumBlue = sumBlue < 0 ? 0 : sumBlue;
                int newColor = ((int)sumRed << 16) | ((int)sumGreen << 8) | (int)sumBlue;
                tempBuf[y * width + x] = newColor;
            }
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
        delete[] tempBuf;
    }

    void cannyEdgeDetector(int lower, int upper)
    {
        // Step 1 - Apply 5 x 5 Gaussian filter
        double gaussian[25] = {2.0 / 159.0, 4.0 / 159.0, 5.0 / 159.0, 4.0 / 159.0, 2.0 / 159.0,
                               4.0 / 159.0, 9.0 / 159.0, 12.0 / 159.0, 9.0 / 159.0, 4.0 / 159.0,
                               5.0 / 159.0, 12.0 / 159.0, 15.0 / 159.0, 12.0 / 159.0, 5.0 / 159.0,
                               4.0 / 159.0, 9.0 / 159.0, 12.0 / 159.0, 9.0 / 159.0, 4.0 / 159.0,
                               2.0 / 159.0, 4.0 / 159.0, 5.0 / 159.0, 4.0 / 159.0, 2.0 / 159.0};

        linearSpatialFilter(gaussian, 5);
        convertToGrayscale();

        // Step 2 - Find intensity gradient
        double sobelX[9] =
            {1, 0, -1,
             2, 0, -2,
             1, 0, -1};
        double sobelY[9] =
            {1, 2, 1,
             0, 0, 0,
             -1, -2, -1};
        double *magnitude = new double[height * width]();
        double *direction = new double[height * width]();
        for (int y = 1; y < height - 1; y++)
        {
            for (int x = 1; x < width - 1; x++)
            {
                double gx = 0, gy = 0;
                for (int i = y - 1; i <= y + 1; i++)
                {
                    for (int j = x - 1; j <= x + 1; j++)
                    {
                        int color = getRGB(j, i);
                        int gray = color & 0xff;
                        int kernelIndex = (i - (y - 1)) * 3 + (j - (x - 1));
                        gx += gray * sobelX[kernelIndex];
                        gy += gray * sobelY[kernelIndex];
                    }
                }
                magnitude[y * width + x] = sqrt(gx * gx + gy * gy);
                direction[y * width + x] = atan2(gy, gx) * 180 / 3.141592653589793;
            }
        }
        // Step 3 - Nonmaxima Suppression
        double *gn = new double[height * width]();
        double maxMagnitude = 0;
        for (int y = 1; y < height - 1; y++)
        {
            for (int x = 1; x < width - 1; x++)
            {
                int targetX = 0, targetY = 0;
                // find closest direction
                if (direction[y * width + x] <= -157.5)
                {
                    targetX = 1;
                    targetY = 0;
                }
                else if (direction[y * width + x] <= -112.5)
                {
                    targetX = 1;
                    targetY = -1;
                }
                else if (direction[y * width + x] <= -67.5)
                {
                    targetX = 0;
                    targetY = 1;
                }
                else if (direction[y * width + x] <= -22.5)
                {
                    targetX = 1;
                    targetY = 1;
                }
                else if (direction[y * width + x] <= 22.5)
                {
                    targetX = 1;
                    targetY = 0;
                }
                else if (direction[y * width + x] <= 67.5)
                {
                    targetX = 1;
                    targetY = -1;
                }
                else if (direction[y * width + x] <= 112.5)
                {
                    targetX = 0;
                    targetY = 1;
                }
                else if (direction[y * width + x] <= 157.5)
                {
                    targetX = 1;
                    targetY = 1;
                }
                else
                {
                    targetX = 1;
                    targetY = 0;
                }
                double current = magnitude[y * width + x];
                double next = magnitude[(y + targetY) * width + (x + targetX)];
                double previous = magnitude[(y - targetY) * width + (x - targetX)];
                if (current < next || current < previous)
                {
                    gn[y * width + x] = 0;
                }
                else
                {
                    gn[y * width + x] = current;
                }
                if (gn[y * width + x] > maxMagnitude)
                {
                    maxMagnitude = gn[y * width + x];
                }
            }
        }
        // Step 4 - Hysteresis Thresholding
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int newGray = 0;
                if (maxMagnitude > 0)
                {
                    newGray = (int)round(
                        gn[y * width + x] * 255.0 / maxMagnitude);
                }
                int newColor = (newGray << 16) | (newGray << 8) | newGray;
                setRGB(x, y, newColor);
            }
        }
        // upper threshold checking with recursive
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int checking = getRGB(x, y) & 0xff;
                if (checking >= upper)
                {
                    int newColor = (255 << 16) | (255 << 8) | 255;
                    setRGB(x, y, newColor);
                    hystConnect(x, y, lower);
                }
            }
        }
        // clear unwanted values
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int checking = getRGB(x, y) & 0xff;
                if (checking != 255)
                {
                    setRGB(x, y, 0);
                }
            }
        }
        delete[] magnitude;
        delete[] direction;
        delete[] gn;
    }

    void hystConnect(int x, int y, int threshold)
    {
        for (int i = y - 1; i <= y + 1; i++)
        {
            for (int j = x - 1; j <= x + 1; j++)
            {
                if (j < width && i < height && j >= 0 && i >= 0 && !(j == x && i == y))
                {
                    int value = getRGB(j, i) & 0xff;
                    if (value != 255)
                    {
                        if (value >= threshold)
                        {
                            int newColor = (255 << 16) | (255 << 8) | 255;
                            setRGB(j, i, newColor);
                            hystConnect(j, i, threshold);
                        }
                        else
                        {
                            setRGB(j, i, 0);
                        }
                    }
                }
            }
        }
    }

    void houghTransform(double percent, const std::string &houghArrayPath = "")
    {
        if (percent <= 0 || percent > 1)
        {
            std::cout << "Percent must be greater than 0 and not greater than 1." << std::endl;
            return;
        }
        // The image should be converted to a binary edge map first
        // Work out how the Hough space is quantized
        int numOfTheta = 180;
        double thetaStep = M_PI / numOfTheta;
        int highestR = (int)round(std::max(width, height) * sqrt(2));
        int centreX = width / 2;
        int centreY = height / 2;
        std::cout << "Hough array w: " << numOfTheta
                  << " height: " << 2 * highestR << std::endl;
        double *cosTheta = new double[numOfTheta];
        double *sinTheta = new double[numOfTheta];
        for (int i = 0; i < numOfTheta; i++)
        {
            cosTheta[i] = cos(i * thetaStep);
            sinTheta[i] = sin(i * thetaStep);
        }
        // Create the Hough array and initialize to zero
        int **houghArray = new int *[numOfTheta];
        for (int i = 0; i < numOfTheta; i++)
        {
            houghArray[i] = new int[2 * highestR];
            for (int j = 0; j < 2 * highestR; j++)
            {
                houghArray[i][j] = 0;
            }
        }
        // Step 1 - find each white edge pixel
        // Step 2 - apply the line equation and vote in the array
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int pointColor = getRGB(x, y) & 0xff;
                if (pointColor == 255)
                {
                    for (int i = 0; i < numOfTheta; i++)
                    {
                        int r = (int)round(
                            (x - centreX) * cosTheta[i] + (y - centreY) * sinTheta[i]);
                        r = r + highestR;
                        if (r < 0 || r >= 2 * highestR)
                        {
                            continue;
                        }
                        houghArray[i][r]++;
                    }
                }
            }
        }
        // Step 3 - find the maximum vote
        int maxHough = 0;
        for (int i = 0; i < numOfTheta; i++)
        {
            for (int j = 0; j < 2 * highestR; j++)
            {
                if (houghArray[i][j] > maxHough)
                {
                    maxHough = houghArray[i][j];
                }
            }
        }
        if (maxHough == 0)
        {
            std::cout << "No edge pixels found." << std::endl;
            for (int i = 0; i < numOfTheta; i++)
            {
                delete[] houghArray[i];
            }
            delete[] houghArray;
            delete[] cosTheta;
            delete[] sinTheta;
            return;
        }
        // Write the normalized Hough array for demonstration
        int oldWidth = width;
        int oldHeight = height;
        int bytesPerPixel = bitDepth / BYTE;
        int oldSize = oldWidth * oldHeight * bytesPerPixel;
        unsigned char *edgeMap = new unsigned char[oldSize];
        for (int i = 0; i < oldSize; i++)
        {
            edgeMap[i] = data[i];
        }
        data.clear();
        width = numOfTheta;
        height = 2 * highestR;
        data.resize(width * height * bytesPerPixel);
        for (int i = 0; i < width * height * bytesPerPixel; i++)
        {
            data[i] = 0;
        }
        for (int j = 0; j < 2 * highestR; j++)
        {
            for (int i = 0; i < numOfTheta; i++)
            {
                int gray = (int)round(
                    houghArray[i][j] * 255.0 / maxHough);
                int color = (gray << 16) | (gray << 8) | gray;
                setRGB(i, j, color);
            }
        }
        if (!houghArrayPath.empty())
        {
            write(houghArrayPath);
        }
        width = oldWidth;
        height = oldHeight;
        data.assign(edgeMap, edgeMap + oldSize);
        delete[] edgeMap;
        // The threshold is a proportion of the maximum vote
        int threshold = (int)round(percent * maxHough);
        threshold = threshold < 1 ? 1 : threshold;
        std::cout << "Maximum vote = " << maxHough << std::endl;
        std::cout << "Hough threshold = " << threshold << std::endl;
        // Step 4 - search for local peaks and draw complete lines
        for (int i = 0; i < numOfTheta; i++)
        {
            for (int j = 0; j < 2 * highestR; j++)
            {
                if (houghArray[i][j] >= threshold)
                {
                    bool draw = true;
                    int peak = houghArray[i][j];
                    for (int k = -4; k <= 4; k++)
                    {
                        for (int l = -4; l <= 4; l++)
                        {
                            if (k == 0 && l == 0)
                            {
                                continue;
                            }
                            int testTheta = i + k;
                            int testOffset = j + l;
                            if (testTheta < 0 || testTheta >= numOfTheta || testOffset < 0 || testOffset >= 2 * highestR)
                            {
                                continue;
                            }
                            int testPeak =
                                houghArray[testTheta][testOffset];
                            if (testPeak > peak || (testPeak == peak && (testTheta < i || (testTheta == i && testOffset < j))))
                            {
                                draw = false;
                                break;
                            }
                        }
                        if (!draw)
                        {
                            break;
                        }
                    }
                    if (!draw)
                    {
                        continue;
                    }
                    double tsin = sinTheta[i];
                    double tcos = cosTheta[i];
                    if (i <= numOfTheta / 4 || i >= (3 * numOfTheta) / 4)
                    {
                        for (int y = 0; y < height; y++)
                        {
                            int x = (int)round(
                                ((j - highestR) - (y - centreY) * tsin) / tcos + centreX);
                            if (x >= 0 && x < width)
                            {
                                int redColor = (255 << 16);
                                setRGB(x, y, redColor);
                            }
                        }
                    }
                    else
                    {
                        for (int x = 0; x < width; x++)
                        {
                            int y = (int)round(
                                ((j - highestR) - (x - centreX) * tcos) / tsin + centreY);
                            if (y >= 0 && y < height)
                            {
                                int redColor = (255 << 16);
                                setRGB(x, y, redColor);
                            }
                        }
                    }
                }
            }
        }
        for (int i = 0; i < numOfTheta; i++)
        {
            delete[] houghArray[i];
        }
        delete[] houghArray;
        delete[] cosTheta;
        delete[] sinTheta;
    }

    void regionGrowing(int seedX, int seedY, int threshold, bool useEightConnectivity)
    {
        if (seedX < 0 || seedX >= width || seedY < 0 || seedY >= height || threshold < 0)
        {
            std::cout << "Region growing parameters invalid!" << std::endl;
            return;
        }
        convertToGrayscale();
        int pixelNum = width * height;
        int *source = new int[pixelNum];
        bool *visited = new bool[pixelNum];
        Point *queue = new Point[pixelNum];
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int index = y * width + x;
                source[index] = getRGB(x, y) & 0xff;
                visited[index] = false;
                setRGB(x, y, 0);
            }
        }
        int internalSeedY = height - seedY - 1;
        int seedGray = source[internalSeedY * width + seedX];
        int first = 0;
        int last = 0;
        queue[last].x = seedX;
        queue[last].y = internalSeedY;
        last++;
        visited[internalSeedY * width + seedX] = true;
        while (first < last)
        {
            Point point = queue[first];
            first++;
            int white = (255 << 16) | (255 << 8) | 255;
            setRGB(point.x, point.y, white);
            for (int y = -1; y <= 1; y++)
            {
                for (int x = -1; x <= 1; x++)
                {
                    if (x == 0 && y == 0)
                    {
                        continue;
                    }
                    if (!useEightConnectivity && abs(x) + abs(y) != 1)
                    {
                        continue;
                    }
                    int newX = point.x + x;
                    int newY = point.y + y;
                    if (newX < 0 || newX >= width || newY < 0 || newY >= height)
                    {
                        continue;
                    }
                    int index = newY * width + newX;
                    if (visited[index])
                    {
                        continue;
                    }
                    visited[index] = true;
                    if (abs(source[index] - seedGray) <= threshold)
                    {
                        queue[last].x = newX;
                        queue[last].y = newY;
                        last++;
                    }
                }
            }
        }
        delete[] source;
        delete[] visited;
        delete[] queue;
    }
    void kMeansClustering(int k)
    {
        if (k < 2 || k > 256)
        {
            std::cout << "Invalid number of clusters!" << std::endl;
            return;
        }
        convertToGrayscale();
        double *means = new double[k];
        int *clusters = new int[width * height];
        for (int i = 0; i < k; i++)
        {
            means[i] = 255.0 * i / (k - 1);
        }
        bool changed = true;
        while (changed)
        {
            changed = false;
            double *sums = new double[k];
            int *counts = new int[k];
            for (int i = 0; i < k; i++)
            {
                sums[i] = 0;
                counts[i] = 0;
            }
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    int gray = getRGB(x, y) & 0xff;
                    int nearest = 0;
                    double minDistance = fabs(gray - means[0]);
                    for (int i = 1; i < k; i++)
                    {
                        double distance = fabs(gray - means[i]);
                        if (distance < minDistance)
                        {
                            minDistance = distance;
                            nearest = i;
                        }
                    }
                    clusters[y * width + x] = nearest;
                    sums[nearest] += gray;
                    counts[nearest]++;
                }
            }
            for (int i = 0; i < k; i++)
            {
                if (counts[i] > 0)
                {
                    double newMean = sums[i] / counts[i];
                    if (newMean != means[i])
                    {
                        changed = true;
                    }
                    means[i] = newMean;
                }
            }
            delete[] sums;
            delete[] counts;
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int gray = (int)round(means[clusters[y * width + x]]);
                int color = (gray << 16) | (gray << 8) | gray;
                setRGB(x, y, color);
            }
        }
        delete[] means;
        delete[] clusters;
    }
    void ADIAbsolute(std::vector<std::string> sequences, int threshold, int step)
    {
        int *tempBuf = new int[height * width];
        for (int i = 0; i < height * width; i++)
        {
            tempBuf[i] = 0;
        }
        for (std::size_t n = 0; n < sequences.size(); n++)
        {
            unsigned char *otherImage = new unsigned char[height * width * (bitDepth / BYTE)];
            FILE *fi = fopen(sequences[n].c_str(), "rb");
            if (fi == (FILE *)0)
            {
                std::cout << "Unable to open file" << sequences[n] << std::endl;
                delete[] otherImage;
                delete[] tempBuf;
                return;
            }
            for (int i = 0; i < BMP_HEADER_SIZE; i++)
            {
                getc(fi);
            }
            fread(otherImage, sizeof(unsigned char),
                  height * width * (bitDepth / BYTE), fi);
            fclose(fi);
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    int color1 = getRGB(x, y);
                    int r1 = (color1 >> 16) & 0xff;
                    int g1 = (color1 >> 8) & 0xff;
                    int b1 = color1 & 0xff;
                    int i = y * width * (bitDepth / BYTE) + x * (bitDepth / BYTE);
                    int b2 = otherImage[i];
                    int g2 = otherImage[i + 1];
                    int r2 = otherImage[i + 2];
                    int dr = r1 - r2;
                    int dg = g1 - g2;
                    int db = b1 - b2;
                    int dGray = (int)round(
                        0.2126 * dr + 0.7152 * dg + 0.0722 * db);
                    if (abs(dGray) > threshold)
                    {
                        int currentColor = tempBuf[y * width + x] & 0xff;
                        currentColor += step;
                        currentColor = currentColor > 255 ? 255 : currentColor;
                        currentColor = currentColor < 0 ? 0 : currentColor;
                        int newColor = (currentColor << 16) | (currentColor << 8) | currentColor;
                        tempBuf[y * width + x] = newColor;
                    }
                }
            }
            delete[] otherImage;
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
        delete[] tempBuf;
    }
    void ADIPositive(std::vector<std::string> sequences, int threshold, int step)
    {
        int *tempBuf = new int[height * width];
        for (int i = 0; i < height * width; i++)
        {
            tempBuf[i] = 0;
        }
        for (std::size_t n = 0; n < sequences.size(); n++)
        {
            unsigned char *otherImage = new unsigned char[height * width * (bitDepth / BYTE)];
            FILE *fi = fopen(sequences[n].c_str(), "rb");
            if (fi == (FILE *)0)
            {
                std::cout << "Unable to open file" << sequences[n] << std::endl;
                delete[] otherImage;
                delete[] tempBuf;
                return;
            }
            for (int i = 0; i < BMP_HEADER_SIZE; i++)
            {
                getc(fi);
            }
            fread(otherImage, sizeof(unsigned char),
                  height * width * (bitDepth / BYTE), fi);
            fclose(fi);
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    int color1 = getRGB(x, y);
                    int r1 = (color1 >> 16) & 0xff;
                    int g1 = (color1 >> 8) & 0xff;
                    int b1 = color1 & 0xff;
                    int i = y * width * (bitDepth / BYTE) + x * (bitDepth / BYTE);
                    int b2 = otherImage[i];
                    int g2 = otherImage[i + 1];
                    int r2 = otherImage[i + 2];
                    int dr = r1 - r2;
                    int dg = g1 - g2;
                    int db = b1 - b2;
                    int dGray = (int)round(
                        0.2126 * dr + 0.7152 * dg + 0.0722 * db);
                    if (dGray > threshold)
                    {
                        int currentColor = tempBuf[y * width + x] & 0xff;
                        currentColor += step;
                        currentColor = currentColor > 255 ? 255 : currentColor;
                        currentColor = currentColor < 0 ? 0 : currentColor;
                        int newColor = (currentColor << 16) | (currentColor << 8) | currentColor;
                        tempBuf[y * width + x] = newColor;
                    }
                }
            }
            delete[] otherImage;
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
        delete[] tempBuf;
    }
    void ADINegative(std::vector<std::string> sequences, int threshold, int step)
    {
        int *tempBuf = new int[height * width];
        for (int i = 0; i < height * width; i++)
        {
            tempBuf[i] = 0;
        }
        for (std::size_t n = 0; n < sequences.size(); n++)
        {
            unsigned char *otherImage = new unsigned char[height * width * (bitDepth / BYTE)];
            FILE *fi = fopen(sequences[n].c_str(), "rb");
            if (fi == (FILE *)0)
            {
                std::cout << "Unable to open file" << sequences[n] << std::endl;
                delete[] otherImage;
                delete[] tempBuf;
                return;
            }
            for (int i = 0; i < BMP_HEADER_SIZE; i++)
            {
                getc(fi);
            }
            fread(otherImage, sizeof(unsigned char),
                  height * width * (bitDepth / BYTE), fi);
            fclose(fi);
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    int color1 = getRGB(x, y);
                    int r1 = (color1 >> 16) & 0xff;
                    int g1 = (color1 >> 8) & 0xff;
                    int b1 = color1 & 0xff;
                    int i = y * width * (bitDepth / BYTE) + x * (bitDepth / BYTE);
                    int b2 = otherImage[i];
                    int g2 = otherImage[i + 1];
                    int r2 = otherImage[i + 2];
                    int dr = r1 - r2;
                    int dg = g1 - g2;
                    int db = b1 - b2;
                    int dGray = (int)round(
                        0.2126 * dr + 0.7152 * dg + 0.0722 * db);
                    if (dGray < -threshold)
                    {
                        int currentColor = tempBuf[y * width + x] & 0xff;
                        currentColor += step;
                        currentColor = currentColor > 255 ? 255 : currentColor;
                        currentColor = currentColor < 0 ? 0 : currentColor;
                        int newColor = (currentColor << 16) | (currentColor << 8) | currentColor;
                        tempBuf[y * width + x] = newColor;
                    }
                }
            }
            delete[] otherImage;
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
        delete[] tempBuf;
    }

    std::vector<Point> traceBoundary()
    {
        std::vector<Point> boundary;
        Point start;
        bool foundStart = false;
        for (int y = 0; y < height && !foundStart; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int internalY = height - y - 1;
                if ((getRGB(x, internalY) & 0xff) > 0)
                {
                    start.x = x;
                    start.y = y;
                    foundStart = true;
                    break;
                }
            }
        }
        if (!foundStart)
            return boundary;
        int dx[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
        int dy[8] = {0, -1, -1, -1, 0, 1, 1, 1};
        Point current = start;
        Point c;
        c.x = start.x - 1;
        c.y = start.y;
        do
        {
            boundary.push_back(current);
            int startDirection = 0;
            for (int i = 0; i < 8; i++)
            {
                if (current.x + dx[i] == c.x && current.y + dy[i] == c.y)
                {
                    startDirection = i;
                    break;
                }
            }
            Point next;
            Point nextC;
            bool foundNext = false;
            for (int i = 0; i < 8; i++)
            {
                int direction = (startDirection + i) % 8;
                int testX = current.x + dx[direction];
                int testY = current.y + dy[direction];
                if (testX >= 0 && testX < width && testY >= 0 && testY < height)
                {
                    int internalY = height - testY - 1;
                    if ((getRGB(testX, internalY) & 0xff) > 0)
                    {
                        next.x = testX;
                        next.y = testY;
                        int previousDirection = (direction + 7) % 8;
                        nextC.x = current.x + dx[previousDirection];
                        nextC.y = current.y + dy[previousDirection];
                        foundNext = true;
                        break;
                    }
                }
            }
            if (!foundNext)
                break;
            current = next;
            c = nextC;
        } while (!(current == start));
        return boundary;
    }

    std::vector<int> getFreemanChainCode(std::vector<Point> boundary)
    {
        std::vector<int> chainCode;
        if (boundary.size() < 2)
            return chainCode;
        int dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
        int dy[8] = {0, -1, -1, -1, 0, 1, 1, 1};
        for (std::size_t i = 0; i < boundary.size(); i++)
        {
            Point current = boundary[i];
            Point next = boundary[(i + 1) % boundary.size()];
            int moveX = next.x - current.x;
            int moveY = next.y - current.y;
            for (int direction = 0; direction < 8; direction++)
            {
                if (moveX == dx[direction] && moveY == dy[direction])
                {
                    chainCode.push_back(direction);
                    break;
                }
            }
        }
        return chainCode;
    }

    std::vector<double> getRegionDescriptors(std::vector<Point> boundary)
    {
        std::vector<double> descriptors;
        double area = 0;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int internalY = height - y - 1;
                if ((getRGB(x, internalY) & 0xff) > 0)
                {
                    area++;
                }
            }
        }
        double perimeter = 0;
        for (std::size_t i = 0; i < boundary.size(); i++)
        {
            Point current = boundary[i];
            Point next = boundary[(i + 1) % boundary.size()];
            int moveX = abs(next.x - current.x);
            int moveY = abs(next.y - current.y);
            if (moveX + moveY == 1)
            {
                perimeter += 1;
            }
            else if (moveX == 1 && moveY == 1)
            {
                perimeter += sqrt(2.0);
            }
        }
        double compactness = 0;
        double circularity = 0;
        if (area > 0 && perimeter > 0)
        {
            compactness = perimeter * perimeter / area;
            circularity = 4 * M_PI * area / (perimeter * perimeter);
        }
        descriptors.push_back(area);
        descriptors.push_back(perimeter);
        descriptors.push_back(compactness);
        descriptors.push_back(circularity);
        return descriptors;
    }
    void drawBoundary(std::vector<Point> boundary)
    {
        int redColor = (255 << 16) | (0 << 8) | 0;
        for (std::size_t i = 0; i < boundary.size(); i++)
        {
            int internalY = height - boundary[i].y - 1;
            setRGB(boundary[i].x, internalY, redColor);
        }
    }

    std::vector<Point> detectHarrisFeatures(int strongest)
    {
        // Convert to grayscale
        convertToGrayscale();
        double *Ix = new double[height * width]();
        double *Iy = new double[height * width]();
        // Initialize matrices to store products of gradients
        double *Ix2 = new double[height * width]();
        double *Iy2 = new double[height * width]();
        double *Ixy = new double[height * width]();
        // Compute gradients Ix and Iy, drop the border
        for (int y = 1; y < height - 1; y++)
        {
            for (int x = 1; x < width - 1; x++)
            {
                int index = y * width + x;
                int internalY = height - y - 1;
                Ix[index] = ((getRGB(x + 1, internalY) & 0xff) - (getRGB(x - 1, internalY) & 0xff)) / 2.0;
                Iy[index] = ((getRGB(x, internalY - 1) & 0xff) - (getRGB(x, internalY + 1) & 0xff)) / 2.0;
                Ix2[index] = Ix[index] * Ix[index];
                Iy2[index] = Iy[index] * Iy[index];
                Ixy[index] = Ix[index] * Iy[index];
            }
        }
        // Apply 3x3 Gaussian smoothing
        double *Sx2 = new double[height * width]();
        double *Sy2 = new double[height * width]();
        double *Sxy = new double[height * width]();
        double gaussian[9] =
            {
                1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
                2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
                1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0};
        for (int y = 1; y < height - 1; y++)
        {
            for (int x = 1; x < width - 1; x++)
            {
                for (int i = -1; i <= 1; i++)
                {
                    for (int j = -1; j <= 1; j++)
                    {
                        int index = (y + i) * width + (x + j);
                        int gaussianIndex = (i + 1) * 3 + (j + 1);
                        Sx2[y * width + x] += Ix2[index] * gaussian[gaussianIndex];
                        Sy2[y * width + x] += Iy2[index] * gaussian[gaussianIndex];
                        Sxy[y * width + x] += Ixy[index] * gaussian[gaussianIndex];
                    }
                }
            }
        }
        // Compute the corner response function R
        double *corners = new double[height * width]();
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int index = y * width + x;
                double det = Sx2[index] * Sy2[index] - Sxy[index] * Sxy[index];
                double trace = Sx2[index] + Sy2[index];
                corners[index] = det - 0.04 * trace * trace;
            }
        }
        std::vector<Point> cornerPoints;
        std::vector<double> cornerValues;
        // Maxima Suppression
        for (int y = 1; y < height - 1; y++)
        {
            for (int x = 1; x < width - 1; x++)
            {
                int index = y * width + x;
                // if zero or negative, not a corner
                if (corners[index] <= 0)
                    continue;
                double peak = corners[index];
                bool isMaxima = true;
                for (int k = -1; k <= 1 && isMaxima; k++)
                {
                    for (int l = -1; l <= 1; l++)
                    {
                        if (k == 0 && l == 0)
                            continue;
                        int testIndex = (y + k) * width + (x + l);
                        if (corners[testIndex] > peak)
                        {
                            isMaxima = false;
                            break;
                        }
                    }
                }
                if (isMaxima)
                {
                    Point temp;
                    temp.x = x;
                    temp.y = y;
                    std::vector<double>::iterator insertPos = std::lower_bound(
                        cornerValues.begin(), cornerValues.end(), peak, std::greater<double>());
                    int position = std::distance(cornerValues.begin(), insertPos);
                    cornerPoints.insert(cornerPoints.begin() + position, temp);
                    cornerValues.insert(insertPos, peak);
                    if (cornerPoints.size() > (std::size_t)strongest)
                    {
                        cornerPoints.pop_back();
                        cornerValues.pop_back();
                    }
                }
            }
        }
        restore();
        convertToGrayscale();
        // Draw red X on the image at the corner points
        for (std::size_t i = 0; i < cornerPoints.size(); i++)
        {
            Point p = cornerPoints[i];
            int redColor = (255 << 16) | (0 << 8) | 0;
            setRGB(p.x, height - p.y - 1, redColor);
            setRGB(p.x + 1, height - (p.y + 1) - 1, redColor);
            setRGB(p.x + 1, height - (p.y - 1) - 1, redColor);
            setRGB(p.x - 1, height - (p.y + 1) - 1, redColor);
            setRGB(p.x - 1, height - (p.y - 1) - 1, redColor);
        }
        delete[] Ix;
        delete[] Iy;
        delete[] Ix2;
        delete[] Iy2;
        delete[] Ixy;
        delete[] Sx2;
        delete[] Sy2;
        delete[] Sxy;
        delete[] corners;
        return cornerPoints;
    }

    std::vector<double> calculateHomography(
        std::vector<std::vector<double>> &srcPoints,
        std::vector<std::vector<double>> &dstPoints)
    {
        std::vector<std::vector<double>> A(8, std::vector<double>(8, 0));
        std::vector<double> b(8, 0);
        for (int i = 0; i < 4; i++)
        {
            double xSrc = srcPoints[i][0];
            double ySrc = srcPoints[i][1];
            double xDst = dstPoints[i][0];
            double yDst = dstPoints[i][1];
            A[2 * i][0] = xSrc;
            A[2 * i][1] = ySrc;
            A[2 * i][2] = 1;
            A[2 * i][3] = 0;
            A[2 * i][4] = 0;
            A[2 * i][5] = 0;
            A[2 * i][6] = -xSrc * xDst;
            A[2 * i][7] = -ySrc * xDst;
            A[2 * i + 1][0] = 0;
            A[2 * i + 1][1] = 0;
            A[2 * i + 1][2] = 0;
            A[2 * i + 1][3] = xSrc;
            A[2 * i + 1][4] = ySrc;
            A[2 * i + 1][5] = 1;
            A[2 * i + 1][6] = -xSrc * yDst;
            A[2 * i + 1][7] = -ySrc * yDst;
            b[2 * i] = xDst;
            b[2 * i + 1] = yDst;
        }
        return gaussianElimination(A, b);
    }

    std::vector<double> gaussianElimination(
        std::vector<std::vector<double>> &A, std::vector<double> &b)
    {
        int n = b.size();
        for (int i = 0; i < n; i++)
        {
            int max = i;
            for (int j = i + 1; j < n; j++)
            {
                if (fabs(A[j][i]) > fabs(A[max][i]))
                {
                    max = j;
                }
            }
            std::swap(A[i], A[max]);
            std::swap(b[i], b[max]);
            for (int k = i + 1; k < n; k++)
            {
                double factor = A[k][i] / A[i][i];
                b[k] -= factor * b[i];
                for (int j = i; j < n; j++)
                {
                    A[k][j] -= factor * A[i][j];
                }
            }
        }
        std::vector<double> x(n, 0);
        for (int i = n - 1; i >= 0; i--)
        {
            double sum = 0;
            for (int j = i + 1; j < n; j++)
            {
                sum += A[i][j] * x[j];
            }
            x[i] = (b[i] - sum) / A[i][i];
        }
        std::vector<double> H(9, 0);
        for (int i = 0; i < 8; i++)
        {
            H[i] = x[i];
        }
        H[8] = 1;
        return H;
    }

    std::vector<double> invertHomography(std::vector<double> &H)
    {
        std::vector<double> invH(9, 0);
        double det = H[0] * (H[4] * H[8] - H[5] * H[7]) - H[1] * (H[3] * H[8] - H[5] * H[6]) + H[2] * (H[3] * H[7] - H[4] * H[6]);

        if (det == 0)
        {
            throw std::invalid_argument("Matrix is not invertible");
        }
        double invDet = 1.0 / det;
        invH[0] = invDet * (H[4] * H[8] - H[5] * H[7]);
        invH[1] = invDet * (H[2] * H[7] - H[1] * H[8]);
        invH[2] = invDet * (H[1] * H[5] - H[2] * H[4]);
        invH[3] = invDet * (H[5] * H[6] - H[3] * H[8]);
        invH[4] = invDet * (H[0] * H[8] - H[2] * H[6]);
        invH[5] = invDet * (H[2] * H[3] - H[0] * H[5]);
        invH[6] = invDet * (H[3] * H[7] - H[4] * H[6]);
        invH[7] = invDet * (H[1] * H[6] - H[0] * H[7]);
        invH[8] = invDet * (H[0] * H[4] - H[1] * H[3]);
        return invH;
    }

    std::vector<double> applyHomographyToPoint(std::vector<double> &H, double x, double y)
    {
        double xh = H[0] * x + H[1] * y + H[2];
        double yh = H[3] * x + H[4] * y + H[5];
        double w = H[6] * x + H[7] * y + H[8];
        return {xh / w, yh / w};
    }

    void applyHomography(std::vector<double> &H)
    {
        int *tempBuf = new int[height * width];
        std::vector<double> invH = invertHomography(H);
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int destinationY = height - y - 1;
                std::vector<double> sourcePoint =
                    applyHomographyToPoint(invH, x, destinationY);
                int srcX = (int)round(sourcePoint[0]);
                int sourceY = (int)round(sourcePoint[1]);
                int srcY = height - sourceY - 1;
                if (srcX >= 0 && srcX < width && srcY >= 0 && srcY < height)
                {
                    tempBuf[y * width + x] = getRGB(srcX, srcY);
                }
                else
                {
                    tempBuf[y * width + x] = 0;
                }
            }
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                setRGB(x, y, tempBuf[y * width + x]);
            }
        }
        delete[] tempBuf;
    }

    int minimumDistanceClassifier(
        std::vector<double> &pattern,
        std::vector<std::vector<double>> &prototypes)
    {
        int nearest = 0;
        double minDistance = 1.0e100;
        for (int i = 0; i < (int)prototypes.size(); i++)
        {
            double distance = 0;
            for (int j = 0; j < (int)pattern.size(); j++)
            {
                double difference = pattern[j] - prototypes[i][j];
                distance += difference * difference;
            }
            distance = sqrt(distance);
            if (distance < minDistance)
            {
                minDistance = distance;
                nearest = i;
            }
        }
        return nearest;
    }

    void restore()
    {
        width = originalWidth;
        height = originalHeight;
        data = origin;
    }
};

#include "fd.hpp"

inline FD *ImBMP::getFrequencyDomain()
{
    convertToGrayscale();
    FD *fft = new FD(this);
    restore();
    return fft;
}

#endif