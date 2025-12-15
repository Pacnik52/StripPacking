#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <numeric>
#include <sstream>

namespace binpack {
    using namespace std;


    inline float scale1( float minVal, float maxVal, float val ) {
        return ((val-minVal)/(maxVal-minVal));
    }
    inline float scale2( float minVal, float maxVal, float val ) {
        return -1.0f + ((val-minVal)/(maxVal-minVal))*2;
    }


    inline float scaleTanh(float x) {
        //    static const float shift = 4.0f;
        //    static const float rshift = 0.25f;
        static const float shift = 3.5f;
        static const float rshift = 1.0f / 3.5f;
        if (x >= 0.f) {
            if (x >= shift) return 1.0f;
            float tmp = (x - shift) * rshift;
            return 1.0f - tmp * tmp * tmp * tmp;
        } else if (x >= -shift) {
            float tmp = (x + shift) * rshift;
            return -1.0f + tmp * tmp * tmp * tmp;
        } else {
            return -1.0f;
        }
    }

    inline float scaleTanh2(float x) {
        //    static const float shift = 4.0f;
        //    static const float rshift = 0.25f;
        static const float shift = 3.5f;
        static const float rshift = 1.0f / 3.5f;
        if (x >= 0.f) {
            if (x >= shift) return 1.0f + (x-shift)*0.01;
            float tmp = (x - shift) * rshift;
            return 1.0f - tmp * tmp * tmp * tmp;
        } else if (x >= -shift) {
            float tmp = (x + shift) * rshift;
            return -1.0f + tmp * tmp * tmp * tmp;
        } else {
            return -1.0f - (shift - x)*0.01;
        }
    }



    // inline float scaleTanh(float x) {
    //     return tanhf(x);
    // }

    inline float scaleSigm(float x) {
        //    static const float shift = 4.0f;
        //    static const float rshift = 0.25f;
        static const float shift = 3.5f;
        static const float rshift = 1.0f / 3.5f;
        if (x >= 0.f) {
            if (x >= shift) return 1.0f;
            float tmp = (x - shift) * rshift;
            return 1.0f - tmp * tmp * tmp * tmp / 2;
        } else if (x >= -shift) {
            float tmp = (x + shift) * rshift;
            return -1.0f + tmp * tmp * tmp * tmp / 2;
        } else {
            return 0.0f;
        }
    }

    inline float scaleZet(float x) {
        return max(min(1.0f, x), -1.0f);
    }


    inline float scaleReLU(float x) {
        //    static const float shift = 4.0f;
        //    static const float rshift = 0.25f;
        return max(0.f, x);
    }


    inline float scaleGauss( float x ) {
        float th = scaleTanh(x);
        return 1.f-th*th;
    }

    inline void unquote(string &str) {
        if (str.size() > 1) {
            if (str.front() == '"' && str.back() == '"') {
                if (str.size() == 2) {
                    str.erase();
                } else {
                    str.erase(str.begin());
                    str.erase(str.end() - 1);
                }
            }
        }

    }

    void importCSV(string fn, vector<vector<string> > &Table);


    inline std::string to_string(int n, int width)
    {
        std::ostringstream oss;
        oss.width(width);
        oss.fill('0');
        oss << n;
        return oss.str();
    }

    template <typename T>
    std::string to_string_with_precision(const T a_value, const int n = 6)
    {
        std::ostringstream out;
        out.precision(n);
        out << std::fixed << a_value;
        return std::move(out).str();
    }
}