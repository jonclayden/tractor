#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "RNifti.h"
#include <array>

// Location conventions: voxel-indexed, scaled for voxel dimensions only (as
// with a diagonal xform), or world coordinates fully respecting the xform
enum struct PointType { Voxel, Scaled, World };

// Rounding strategies: none, standard for nearest-neighbour, or probabilistic
// for stochastic nearest neighbour (probabilities proportional to distance)
enum struct RoundingType { None, Conventional, Probabilistic };

class ImageSpace
{
public:
    typedef RNifti::NiftiImage::Xform::Element Element;
    typedef RNifti::NiftiImage::Xform::Vector3 Point;
    typedef RNifti::NiftiImage::Xform::Vector3 Vector;
    typedef RNifti::NiftiImage::Xform::Matrix Transform;
    
    typedef std::array<RNifti::NiftiImage::dim_t,3> DimVector;
    typedef std::array<RNifti::NiftiImage::pixdim_t,3> PixdimVector;
    
    DimVector dim;
    PixdimVector pixdim;
    Transform transform;
    
    static Vector zeroVector ()
    {
        return Vector(0.0);
    }
    
    static Element norm (const Vector &vector)
    {
        const Element squaredNorm = vector[0]*vector[0] + vector[1]*vector[1] + vector[2]*vector[2];
        return sqrt(squaredNorm);
    }
    
    static Element dot (const Vector &first, const Vector &second)
    {
        const Element product = first[0]*second[0] + first[1]*second[1] + first[2]*second[2];
        return product;
    }
    
    static Vector step (const Point &from, const Point &to)
    {
        Vector result;
        for (int i=0; i<3; i++)
            result[i] = to[i] - from[i];
        return result;
    }
    
    static Vector sphericalToCartesian (const Vector &spherical)
    {
        Vector cartesian;
        cartesian[0] = spherical[0] * sin(spherical[1]) * cos(spherical[2]);
        cartesian[1] = spherical[0] * sin(spherical[1]) * sin(spherical[2]);
        cartesian[2] = spherical[0] * cos(spherical[1]);
        return cartesian;
    }
    
    ImageSpace (const DimVector &dim, const PixdimVector &pixdim, const Transform &transform)
        : dim(dim), pixdim(pixdim), transform(transform) {}
    
    ImageSpace (const DimVector &dim, const PixdimVector &pixdim)
        : dim(dim), pixdim(pixdim)
    {
        this->transform = Transform::eye();
        for (int i=0; i<3; i++)
            this->transform(i,i) = pixdim[i];
    }
    
    ImageSpace (const DimVector &dim)
        : ImageSpace(dim, {1,1,1}, Transform::eye()) {}
    
    ImageSpace ()
        : ImageSpace({0,0,0}, {1,1,1}, Transform::eye()) {}
    
    ImageSpace (const RNifti::NiftiImage &source)
    {
        std::vector<RNifti::NiftiImage::dim_t> vdim = source.dim();
        std::vector<RNifti::NiftiImage::pixdim_t> vpixdim = source.pixdim();
        
        dim = { 1, 1, 1 };
        pixdim = { 1.0, 1.0, 1.0 };
        
        for (unsigned i=0; i<std::min<size_t>(3,vdim.size()); i++)
        {
            dim[i] = vdim[i];
            pixdim[i] = vpixdim[i];
        }
        
        transform = source.xform().matrix();
    }
    
    std::string orientation () const { return RNifti::NiftiImage::Xform(transform).orientation(); }
    
    Point toVoxel (const Point &point, const PointType type, const RoundingType round = RoundingType::Conventional) const;
};

class ImageSpaceEmbedded
{
protected:
    ImageSpace *space = nullptr;
    bool sharedSpace = false;
    
public:
    virtual ~ImageSpaceEmbedded ()
    {
        if (!sharedSpace)
            delete space;
    }
    
    virtual ImageSpace & imageSpace () const
    {
        if (space == nullptr)
            throw std::runtime_error("No image space information is available");
        else
            return *space;
    }
    
    bool hasImageSpace () const { return (space != nullptr); }
    
    void setImageSpace (ImageSpace * const space, const bool shared = false)
    {
        this->space = space;
        sharedSpace = shared;
    }
};

template <class ElementType, int Dimensionality>
class Image : public ImageSpaceEmbedded
{
public:
    typedef ElementType Element;
    typedef std::array<size_t,Dimensionality> ArrayIndex;
    
protected:
    std::vector<Element> data_;
    ArrayIndex dims, strides;
    size_t size_;
    
    void calculateStrides ()
    {
        // The first index always moves fastest (as in R)
        strides[0] = size_ = 1;
        for (size_t i=1; i<Dimensionality; i++)
        {
            strides[i] = strides[i-1] * dims[i-1];
            size_ *= dims[i-1];
        }
        size_ *= dims[Dimensionality - 1];
    }
    
    template <class TargetType>
    void import (const RNifti::NiftiImage &source, std::vector<TargetType> &target)
    {
        const RNifti::NiftiImageData sourceData = source.data();
        std::copy(sourceData.begin(), sourceData.end(), target.begin());
    };
    
    template <>
    void import (const RNifti::NiftiImage &source, std::vector<ImageSpace::Vector> &target)
    {
        // We are currently assuming that vector images are 4D with fourth dimension 3 (FSL-style)
        // This is overly restrictive, and in particular doesn't handle NIFTI_INTENT_VECTOR
        if (source.nDims() != 4 || source->nt != 3)
            throw std::runtime_error("NiftiImage source does not seem to be vector-valued");
        
        const RNifti::NiftiImageData sourceData = source.data();
        const size_t volumeSize = source->nx * source->ny * source->nz;
        for (size_t i=0; i<volumeSize; i++)
        {
            ImageSpace::Element elements[3] { sourceData[i], sourceData[i+volumeSize], sourceData[i+2*volumeSize] };
            target[i] = ImageSpace::Vector(elements);
        }
    }
    
    template <int N>
    struct Indexer
    {
        Indexer<N-1> child;
        
        size_t flatten (const ArrayIndex &loc, const ArrayIndex &strides) const
        {
            return strides[N-1] * loc[N-1] + child.flatten(loc, strides);
        }
    };
    
    template<>
    struct Indexer<1>
    {
        size_t flatten (const ArrayIndex &loc, const ArrayIndex &strides) const { return loc[0]; }
    };
    
    Indexer<Dimensionality> indexer;
    
public:
    Image () { dims.fill(0); strides.fill(0); }
    
    Image (const ArrayIndex &dims, const Element value)
        : dims(dims)
    {
        calculateStrides();
        data_ = std::vector<Element>(size_, value);
    }
    
    Image (const ImageSpace::DimVector &dims, const Element value)
    {
        std::copy(dims.begin(), dims.end(), this->dims.begin());
        calculateStrides();
        data_ = std::vector<Element>(size_, value);
    }
    
    Image (const ArrayIndex &dims, const std::vector<Element> &data)
        : dims(dims)
    {
        calculateStrides();
        if (size_ == data.size())
            this->data_ = data;
        else
            throw std::runtime_error("Data size does not match the specified dimensions");
    }
    
    Image (const RNifti::NiftiImage &source)
    {
        if (source.isNull())
            throw std::runtime_error("NiftiImage source is empty");
        if (source->data == nullptr)
            throw std::runtime_error("NiftiImage source contains no voxel data");
        
        auto sourceDims = source.dim();
        std::copy(sourceDims.begin(), sourceDims.begin() + Dimensionality, dims.begin());
        calculateStrides();
        
        space = new ImageSpace(source);
        
        data_.resize(size_);
        import(source, data_);
    }
    
    operator SEXP () const
    {
        Rcpp::RObject object = Rcpp::wrap(data_);
        object.attr("dim") = dims;
        return object;
    }
    
    const std::vector<Element> & data () const { return data_; }
    const ArrayIndex & dim () const { return dims; }
    size_t size () const { return data_.size(); }
    void fill (const Element &value) { data_.assign(data_.size(), value); }
    
    typename std::vector<Element>::iterator begin () { return data_.begin(); }
    typename std::vector<Element>::iterator end () { return data_.end(); }
    typename std::vector<Element>::const_iterator begin () const { return data_.begin(); }
    typename std::vector<Element>::const_iterator end () const { return data_.end(); }
    
    typename std::vector<Element>::reference operator[] (const size_t n) { return data_[n]; }
    typename std::vector<Element>::reference operator[] (const ArrayIndex &loc) { return data_[indexer.flatten(loc, strides)]; }
    
    typename std::vector<Element>::const_reference operator[] (const size_t n) const { return data_[n]; }
    typename std::vector<Element>::const_reference operator[] (const ArrayIndex &loc) const { return data_[indexer.flatten(loc, strides)]; }
    
    typename std::vector<Element>::reference at (const size_t n) { return data_.at(n); }
    typename std::vector<Element>::reference at (const ArrayIndex &loc)
    {
        for (int i=0; i<3; i++)
        {
            if (loc[i] >= dims[i])
                throw std::out_of_range("Array index is out of range");
        }
        return data_[indexer.flatten(loc, strides)];
    }
    typename std::vector<Element>::reference at (const ImageSpace::Point &point, const PointType type = PointType::Voxel, const RoundingType round = RoundingType::Conventional)
    {
        if (space == nullptr)
            throw std::runtime_error("No space is associated with the image");
        
        const ImageSpace::Point resolvedPoint = space->toVoxel(point, type, round);
        
        ArrayIndex loc;
        for (int i=0; i<3; i++)
        {
            loc[i] = static_cast<size_t>(resolvedPoint[i]);
            if (loc[i] >= dims[i])
                throw std::out_of_range("Array index is out of range");
        }
        return data_[indexer.flatten(loc, strides)];
    }
    
    void flattenIndex (const ArrayIndex &loc, size_t &result) const { result = indexer.flatten(loc, strides); }
};

#endif
