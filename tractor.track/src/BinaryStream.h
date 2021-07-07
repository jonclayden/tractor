#ifndef _BINARY_STREAM_H_
#define _BINARY_STREAM_H_

#include <RcppEigen.h>

class BinaryStream
{
protected:
    bool swapEndian;
    template <typename Type> void swap (Type *value);
    
public:
    BinaryStream ()
        : swapEndian(false) {}
    
    std::string nativeEndianness () const;
    void setEndianness (const std::string &endianness);
};

class BinaryInputStream : public BinaryStream
{
private:
    std::istream *stream;
    
public:
    BinaryInputStream ()
        : stream(NULL) {}
    BinaryInputStream (std::istream *stream)
        : stream(stream) {}
    
    void attach (std::istream *stream) { this->stream = stream; }
    void detach () { this->stream = NULL; }
    
    template <typename SourceType> SourceType readValue ();
    template <typename SourceType, typename FinalType> void readVector (std::vector<FinalType> &values, size_t n = 0);
    template <typename SourceType, typename FinalType, int Rows> void readVector (Eigen::Matrix<FinalType,Rows,1> &values, size_t n = Rows);
    template <typename SourceType, typename FinalType, int Rows> void readVector (Eigen::Array<FinalType,Rows,1> &values, size_t n = Rows);
    template <typename SourceType, typename FinalType, int Rows, int Cols> void readMatrix (Eigen::Matrix<FinalType,Rows,Cols> &values, size_t n = Rows, size_t m = Cols);
    std::string readString (const std::string delim = "\0");
    std::string readString (const size_t n);
};

class BinaryOutputStream : public BinaryStream
{
private:
    std::ostream *stream;
    
public:
    BinaryOutputStream ()
        : stream(NULL) {}
    BinaryOutputStream (std::ostream *stream)
        : stream(stream) {}
    
    void attach (std::ostream *stream) { this->stream = stream; }
    void detach () { this->stream = NULL; }
    
    template <typename TargetType> void writeValue (const TargetType value);
    template <typename TargetType> void writeValues (const TargetType value, size_t n);
    template <typename TargetType> void writeArray (TargetType * const pointer, size_t n);
    template <typename TargetType, typename OriginalType> void writeVector (const std::vector<OriginalType> &values, size_t n = 0);
    template <typename TargetType, typename OriginalType, int Rows> void writeVector (const Eigen::Matrix<OriginalType,Rows,1> &values, size_t n = Rows);
    template <typename TargetType, typename OriginalType, int Rows> void writeVector (const Eigen::Array<OriginalType,Rows,1> &values, size_t n = Rows);
    template <typename TargetType, typename OriginalType, int Rows, int Cols> void writeMatrix (const Eigen::Matrix<OriginalType,Rows,Cols> &values, size_t n = Rows, size_t m = Cols);
    void writeString (const std::string &value);
};

#endif
