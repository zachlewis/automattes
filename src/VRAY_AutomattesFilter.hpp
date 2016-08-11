/*
 Partaly taken from HDK examples:
 http://www.sidefx.com/docs/hdk15.5/_v_r_a_y_2_v_r_a_y__demo_edge_detect_filter_8h-example.html

 Contains snippets from PsyOp
 [1] - Jonah Friedman, Andrew C. Jones, Fully automatic ID mattes with support for motion blur and transparency.
 */

#pragma once

#ifndef __VRAY_AutomatteFilter__
#define __VRAY_AutomatteFilter__

#include <VRAY/VRAY_PixelFilter.h>
#include <VRAY/VRAY_Procedural.h>

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) do {} while (0)
#endif


typedef  std::map<float, float>  HashMap;


namespace HA_HDK {

static const char* plane_names[] = {"CryptoObject",   "CryptoObject00", 
                                    "CryptoObject01", "CryptoObject02"};

// From Cryptomatte specification[1]
float hash_to_float(uint32_t hash)
{
    uint32_t mantissa = hash & (( 1 << 23) - 1);
    uint32_t exponent = (hash >> 23) & ((1 << 8) - 1);
    exponent = std::max(exponent, (uint32_t) 1);
    exponent = std::min(exponent, (uint32_t) 254);
    exponent = exponent << 23;
    uint32_t sign = (hash >> 31);
    sign = sign << 31;
    uint32_t float_bits = sign | exponent | mantissa;
    float f;
    std::memcpy(&f, &float_bits, 4);
    return f;
}

struct AutomatteSamples
{
    void init(const int xres, const int yres) 
    {
        myImage.resize(0);
        myXRes = xres;
        myYRes = yres;
        for (int y=0; y<yres; ++y) {
            std::vector<IdSamples>  line;
            for (int x=0; x < xres; ++x) {
                IdSamples mask;
                line.push_back(mask);
            }
            myImage.push_back(line);
        }
    }

    void write(const int x, const int y, const float norm, IdSamples &mask) 
    {
        const int mx = SYSmin(x, myXRes-1);
        const int my = SYSmin(y, myYRes-1);
        IdSamples::iterator it(mask.begin());
        for(; it!=mask.end(); ++it)
            it->second = SYSmax(it->second/norm, 0.f);
        myImage.at(my).at(mx) = mask;
    }   

    const IdSamples get(const int x, const int y) const 
    {
        const int mx = SYSmin(x, myXRes-1);
        const int my = SYSmin(y, myYRes-1);
        return  myImage.at(my).at(mx);
    }

private:
    IdImage myImage;
    int myXRes;
    int myYRes;
};


class VRAY_AutomatteFilter : public VRAY_PixelFilter {
public:
    VRAY_AutomatteFilter();
    virtual ~VRAY_AutomatteFilter();

    virtual VRAY_PixelFilter *clone() const;

    /// setArgs is called with the options specified after the pixel filter
    /// name in the Pixel Filter parameter on the Mantra ROP.
    /// -i 1 use OpId istead of 'mask' raster

    virtual void setArgs(int argc, const char *const argv[]);

   
    virtual void getFilterWidth(float &x, float &y) const;

    virtual void addNeededSpecialChannels(VRAY_Imager &imager);

    virtual void prepFilter(int samplesperpixelx, int samplesperpixely);

    virtual void filter(
        float *destination,
        int vectorsize,
        const VRAY_SampleBuffer &source,
        int channel,
        int sourcewidth,
        int sourceheight,
        int destwidth,
        int destheight,
        int destxoffsetinsource,
        int destyoffsetinsource,
        const VRAY_Imager &imager) const;

private:
   
    int mySamplesPerPixelX;
    int mySamplesPerPixelY;

    float myOpacitySumX2;
    float myOpacitySumY2;

    int myOpacitySamplesHalfX;
    int myOpacitySamplesHalfY;

    // 
    int myUseOpID;
    int myRank;

    // Filter width (default 2)
    float myFilterWidth;
    //  Gaussians parms
    float myGaussianExp;
    float myGaussianAlpha;
    const char* myHashChannel;


};


} // End HA_HDK namespace

#endif
