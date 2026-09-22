#ifndef FD_MANAGER
#define FD_MANAGER
#define _USE_MATH_DEFINES
#include <complex>
#include <cmath>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <fstream>
#include <float.h>
#include <algorithm>
#include "im.hpp"
typedef std::complex<double> Complex;

class FD
{
public:
    int width;
    int height;
    int imgWidth;
    int imgHeight;

    FD(ImBMP *im)
    {
        this->im = im;
        this->imgWidth = im->width;
        this->imgHeight = im->height;
        this->width = nextPowerOf2(imgWidth);
        this->height = nextPowerOf2(imgHeight);
        img = new Complex[this->height * this->width];
        original = new Complex[this->height * this->width];
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int color = im->getRGB(x, y);
                int gray = color & 0xff;
                img[y * width + x] = Complex(gray, 0);
            }
        }
        fft2d(false);
        shifting();
        for (int i = 0; i < (height * width); i++)
        {
            original[i] = img[i];
        }
    }

    void shifting()
    {
        int halfWidth = width / 2;
        int halfHeight = height / 2;
        for (int y = 0; y < halfHeight; y++)
        {
            for (int x = 0; x < width; x++)
            {
                Complex temp = Complex(img[y * width + x].real(), img[y * width + x].imag());
                img[y * width + x] = Complex(
                    img[(y + halfHeight) * width + x].real(),
                    img[(y + halfHeight) * width + x].imag());
                img[(y + halfHeight) * width + x] = Complex(temp.real(), temp.imag());
            }
        }
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < halfWidth; x++)
            {
                Complex temp = Complex(img[y * width + x].real(), img[y * width + x].imag());

                img[y * width + x] = Complex(img[y * width + x + halfWidth].real(), img[y * width + x + halfWidth].imag());
                img[y * width + x + halfWidth] = Complex(temp.real(), temp.imag());
            }
        }
    }

    int nextPowerOf2(int a)
    {
        int b = 1;
        while (b < a)
        {
            b = b << 1;
        }
        return b;
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

    void fft2d(bool invert)
    {
        // Rows
        for (int y = 0; y < height; y++)
        {
            fft(&img[y * width], width, invert);
        }
        // Columns
        Complex *column = new Complex[imgHeight];
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                column[y] = img[y * width + x];
            }
            fft(column, height, invert);
            for (int y = 0; y < height; y++)
            {
                img[y * width + x] = column[y];
            }
        }
        delete[] column;
    }

    bool writeSpectrumLogScaled(const std::string &filename)
    {
        unsigned char *buf = new unsigned char[(height * width * (im->bitDepth / BYTE))];
        double max = DBL_MIN, min = DBL_MAX;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                double spectrum = abs(img[y * width + x]);
                if (spectrum > max)
                    max = spectrum;
                if (spectrum < min)
                    min = spectrum;
            }
        }
        min = min < 1.0f ? 0.0f : log10(min);
        max = max < 1.0f ? 0.0f : log10(max);

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                double spectrum = abs(img[y * width + x]);
                spectrum = spectrum < 1.0f ? 0.0f : log10(spectrum);
                spectrum = ((spectrum - min) * 255 / (max - min));
                int color = ((int)spectrum << 16) | ((int)spectrum << 8) | (int)spectrum;
                int i = y * width * (im->bitDepth / BYTE) + x * (im->bitDepth / BYTE);
                buf[i] = color;
                buf[i + 1] = color;
                buf[i + 2] = color;
            }
        }
        FILE *fo = fopen(filename.c_str(), "wb");
        if (fo == (FILE *)0)
        {
            std::cout << "Unable to create file" << std::endl;
            return false;
        }
        fwrite(im->header, sizeof(unsigned char), BMP_HEADER_SIZE, fo);
        if (im->bitDepth <= 8)
        {
            fwrite(im->colorTable, sizeof(unsigned char), BMP_COLOR_TABLE_SIZE, fo);
        }
        for (int i = 0; i < height; i++)
        {
            fwrite(buf + (i * width * (im->bitDepth / BYTE)), sizeof(unsigned char), (width * (im->bitDepth / BYTE)), fo);
            if ((width * (im->bitDepth / BYTE)) % 4 != 0)
            {
                fwrite(buf, sizeof(unsigned char), (4 - (width * (im->bitDepth / BYTE)) % 4), fo);
            }
        }
        std::cout << "Image " << filename << " has been written!" << std::endl;
        fclose(fo);
        delete[] buf;
        return true;
    }

    double magnitudeAt(int x, int y)
    {
        return abs(img[y * width + x]);
    }

    void getInverse()
    {
        shifting();
        fft2d(true);
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int gray = (int)img[y * width + x].real();
                gray = gray > 255 ? 255 : gray;
                gray = gray < 0 ? 0 : gray;
                int color = (gray << 16) | (gray << 8) | gray;
                im->setRGB(x, y, color);
            }
        }
    }

    void ILPF(double radius)
    {
        if (radius <= 0 || radius > std::min(width / 2, height / 2))
        {
            std::cout << "INVALID Radius!" << std::endl;
            return;
        }
        int centerX = width / 2;
        int centerY = height / 2;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                if ((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY) > radius * radius)
                {
                    img[y * width + x] = Complex(0, 0);
                }
            }
        }
    }

    void GLPF(double radius)
    {
        if (radius <= 0 || radius > std::min(width / 2, height / 2))
        {
            std::cout << "INVALID Radius!" << std::endl;
            return;
        }
        int centerX = width / 2;
        int centerY = height / 2;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                double distanceSquared = (x - centerX) * (x - centerX) + (y - centerY) * (y - centerY);
                double h = exp(-distanceSquared / (2 * radius * radius));
                img[y * width + x] = img[y * width + x] * h;
            }
        }
    }

    void IHPF(double radius)
    {
        if (radius <= 0 || radius > std::min(width / 2, height / 2))
        {
            std::cout << "INVALID Radius!" << std::endl;
            return;
        }
        int centerX = width / 2;
        int centerY = height / 2;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {

                if ((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY) <= radius * radius)
                {
                    img[y * width + x] = Complex(0, 0);
                }
            }
        }
    }

    void GHPF(double radius)
    {
        if (radius <= 0 || radius > std::min(width / 2, height / 2))
        {
            std::cout << "INVALID Radius!" << std::endl;
            return;
        }
        int centerX = width / 2;
        int centerY = height / 2;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                double distanceSquared = (x - centerX) * (x - centerX) + (y - centerY) * (y - centerY);
                double h = 1 - exp(-distanceSquared / (2 * radius * radius));
                img[y * width + x] = img[y * width + x] * h;
            }
        }
    }

    void BLPF(double radius, int order)
    {
        if (radius <= 0 || radius > std::min(width / 2, height / 2))
        {
            std::cout << "INVALID Radius!" << std::endl;
            return;
        }
        int centerX = width / 2;
        int centerY = height / 2;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                double distance = std::sqrt((double)((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY)));
                double h = 1.0 / (1.0 + pow(distance / radius, 2 * order));
                img[y * width + x] = img[y * width + x] * h;
            }
        }
    }

    void BHPF(double radius, int order)
    {
        if (radius <= 0 || radius > std::min(width / 2, height / 2))
        {
            std::cout << "INVALID Radius!" << std::endl;
            return;
        }
        int centerX = width / 2;
        int centerY = height / 2;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                if (x == centerX && y == centerY)
                {
                    img[y * width + x] = Complex(0, 0);
                    continue;
                }
                double distance = std::sqrt((double)((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY)));
                double h = 1.0 / (1.0 + pow(radius / distance, 2 * order));
                img[y * width + x] = img[y * width + x] * h;
            }
        }
    }

    void wienerGaussian(double sigma, double K)
    {
        int centerX = width / 2;
        int centerY = height / 2;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                double distanceSquared = (x - centerX) * (x - centerX) + (y - centerY) * (y - centerY);
                double h = exp(-distanceSquared / (2 * sigma * sigma));
                double hMagSquared = h * h;
                double gain = (hMagSquared / (hMagSquared + K)) / (h == 0 ? 1e-6 : h);
                img[y * width + x] = img[y * width + x] * gain;
            }
        }
    }

    void notch(int cx, int cy, int rx, int ry)
    {
        int centerX = width / 2;
        int centerY = height / 2;
        int sx = 2 * centerX - cx;
        int sy = 2 * centerY - cy;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                double d1 = ((x - cx) * (x - cx)) / (double)(rx * rx) + ((y - cy) * (y - cy)) / (double)(ry * ry);
                double d2 = ((x - sx) * (x - sx)) / (double)(rx * rx) + ((y - sy) * (y - sy)) / (double)(ry * ry);
                if (d1 <= 1.0 || d2 <= 1.0)
                {
                    img[y * width + x] = Complex(0, 0);
                }
            }
        }
    }

    void gaussianNotch(int cx, int cy, double rx, double ry)
    {
        int centerX = width / 2;
        int centerY = height / 2;
        int sx = 2 * centerX - cx;
        int sy = 2 * centerY - cy;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                double d1 = ((x - cx) * (x - cx)) / (rx * rx) + ((y - cy) * (y - cy)) / (ry * ry);
                double d2 = ((x - sx) * (x - sx)) / (rx * rx) + ((y - sy) * (y - sy)) / (ry * ry);
                double h1 = 1 - exp(-d1 / 2);
                double h2 = 1 - exp(-d2 / 2);
                img[y * width + x] = img[y * width + x] * (h1 * h2);
            }
        }
    }

    ~FD()
    {
        delete[] img;
        delete[] original;
    }

private:
    Complex *img;
    Complex *original;
    ImBMP *im;
};
#endif