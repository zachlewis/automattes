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

class VRAY_Imager;
class VRAY_SampleBuffer;
class IMG_DeepShadow;
class IMG_DeepPixelWriter;

typedef  std::map<uint32_t, float>     IdSamples;
typedef  std::map<uint32_t, float>     HashSamples;
typedef  std::vector<std::vector<IdSamples> > IdImage;

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
    /// -z 1 sort by Pz
    /// -o 1 sort by opacity
    /// -n 8 size of stored sampels per pixel. 
 
    virtual void setArgs(int argc, const char *const argv[]);

    /// getFilterWidth is called after setArgs when Mantra needs to know
    /// how far to expand the render region.
    virtual void getFilterWidth(float &x, float &y) const;

    /// addNeededSpecialChannels is called after setArgs so that this filter
    /// can indicate that it depends on having special channels like z-depths
    /// or Op IDs.
    virtual void addNeededSpecialChannels(VRAY_Imager &imager);

    /// prepFilter is called after setArgs so that this filter can
    /// precompute data structures or values for use in filtering that
    /// depend on the number of samples per pixel in the x or y directions.
    virtual void prepFilter(int samplesperpixelx, int samplesperpixely);

    /// filter is called for each destination tile region with a source
    /// that is at least as large as is needed by this filter, based on
    /// the filter widths returned by getFilterWidth.
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
    /// These must be saved in prepFilter.
    /// Each pixel has mySamplesPerPixelX*mySamplesPerPixelY samples.
    /// @{
    int mySamplesPerPixelX;
    int mySamplesPerPixelY;
    /// @}

    /// true if generate mask using Operator ID
    bool myUseOpID;

    /// true if sorting by Pz is enabled
    bool mySortByPz;

    /// true if sorting by Opacity is enabled
    bool mySortByOpacity;

    /// How many object we will store.
    int myMaskNumber;

    /// Filter width (default 2)
    float myFilterWidth;

    float myOpacitySumX2;
    float myOpacitySumY2;

    int myOpacitySamplesHalfX;
    int myOpacitySamplesHalfY;

    /// Gaussians
    float myGaussianExp;
    float myGaussianAlpha;

    IMG_DeepShadow        *myDsm;
    IMG_File              *myImage;
    UT_Array<PXL_Raster*> myRasters;

    uint myXRes;
    uint myYRes;

    const char       *myDeepImagePath;
    const char       *myImagePath;
    AutomatteSamples *mySamples;
};


} // End HA_HDK namespace

#endif
