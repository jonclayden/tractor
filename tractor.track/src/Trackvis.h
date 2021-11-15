#ifndef _TRACKVIS_H_
#define _TRACKVIS_H_

#include <RcppEigen.h>

#include "Grid.h"
#include "Streamline.h"
#include "DataSource.h"
#include "BinaryStream.h"

// Base class for Trackvis readers: provides common functionality
class TrackvisDataSource : public Griddable3D, public StreamlineFileSource
{
protected:
    int nScalars, nProperties, seedProperty;
    Grid<3> grid;
    
    void readStreamline (Streamline &data);
    
public:
    void attach (const std::string &fileStem);
    
    bool hasGrid () const { return true; }
    Grid<3> getGrid3D () const { return grid; }
};

// Basic Trackvis reader: read all streamlines, including seed property
class BasicTrackvisDataSource : public TrackvisDataSource
{
public:
    BasicTrackvisDataSource (const std::string &fileStem)
    {
        attach(fileStem);
    }
    
    void get (Streamline &data) { readStreamline(data); }
    void seek (const int n);
    bool seekable () { return true; }
};

// Labelled Trackvis reader: also read auxiliary file containing label info
class LabelledTrackvisDataSource : public TrackvisDataSource
{
protected:
    std::ifstream auxFileStream;
    BinaryInputStream auxBinaryStream;
    bool externalLabelList;
    
public:
    LabelledTrackvisDataSource ()
    {
        auxBinaryStream.attach(&auxFileStream);
    }
    
    LabelledTrackvisDataSource (const std::string &fileStem, StreamlineLabelList *labelList = NULL)
    {
        auxBinaryStream.attach(&auxFileStream);
        labelList = labelList;
        this->externalLabelList = (labelList != NULL);
        attach(fileStem);
    }
    
    ~LabelledTrackvisDataSource ()
    {
        auxBinaryStream.detach();
        if (auxFileStream.is_open())
            auxFileStream.close();
        if (!externalLabelList)
            delete labelList;
    }
    
    void attach (const std::string &fileStem);
    void get (Streamline &data);
    void seek (const int n);
    bool seekable () { return true; }
};

// Median Trackvis reader: construct and return median streamline only
class MedianTrackvisDataSource : public TrackvisDataSource
{
protected:
    bool read;
    double quantile;
    
public:
    MedianTrackvisDataSource ()
        : read(false) {}
    
    MedianTrackvisDataSource (const std::string &fileStem, const double quantile = 0.99)
        : quantile(quantile), read(false)
    {
        attach(fileStem);
    }
    
    bool more () { return (!read); }
    void get (Streamline &data);
};

class TrackvisDataSink : public Griddable3D, public DataSink<Streamline>
{
protected:
    std::fstream fileStream;
    BinaryOutputStream binaryStream;
    size_t totalStreamlines;
    Grid<3> grid;
    bool append;
    
    TrackvisDataSink ()
        : append(false)
    {
        binaryStream.attach(&fileStream);
    }
    
    TrackvisDataSink (const std::string &fileStem, const bool append = false)
        : append(append)
    {
        binaryStream.attach(&fileStream);
        attach(fileStem);
    }
    
    TrackvisDataSink (const std::string &fileStem, const Grid<3> &grid, const bool append = false)
        : grid(grid), append(append)
    {
        binaryStream.attach(&fileStream);
        attach(fileStem);
    }
    
    void writeStreamline (const Streamline &data);
    
public:
    static std::map<int,char> orientationCodeMap;
    
    static std::map<int,char> createOrientationCodeMap ()
    {
        std::map<int,char> map;
        map[NIFTI_L2R] = 'R';
        map[NIFTI_R2L] = 'L';
        map[NIFTI_P2A] = 'A';
        map[NIFTI_A2P] = 'P';
        map[NIFTI_I2S] = 'S';
        map[NIFTI_S2I] = 'I';
        return map;
    }
    
    virtual ~TrackvisDataSink ()
    {
        binaryStream.detach();
        if (fileStream.is_open())
            fileStream.close();
    }
    
    virtual void attach (const std::string &fileStem);
    void setup (const size_type &count, const_iterator begin, const_iterator end);
    void done ();
    Grid<3> getGrid3D () const { return grid; }
};

class BasicTrackvisDataSink : public TrackvisDataSink
{
public:
    BasicTrackvisDataSink (const std::string &fileStem, const bool append = false)
        : TrackvisDataSink(fileStem,append) {}
    
    BasicTrackvisDataSink (const std::string &fileStem, const Grid<3> &grid, const bool append = false)
        : TrackvisDataSink(fileStem,grid,append) {}
    
    void put (const Streamline &data) { writeStreamline(data); }
    void append (const Streamline &data)
    {
        writeStreamline(data);
        totalStreamlines++;
    }
};

class LabelledTrackvisDataSink : public TrackvisDataSink
{
protected:
    std::ofstream auxFileStream;
    BinaryOutputStream auxBinaryStream;
    std::map<int,std::string> labelDictionary;
    
public:
    LabelledTrackvisDataSink ()
    {
        auxBinaryStream.attach(&auxFileStream);
    }
    
    // Don't call base class constructor explicitly here
    LabelledTrackvisDataSink (const std::string &fileStem, const Grid<3> &grid, const std::map<int,std::string> labelDictionary)
        : labelDictionary(labelDictionary)
    {
        this->grid = grid;
        auxBinaryStream.attach(&auxFileStream);
        attach(fileStem);
    }
    
    ~LabelledTrackvisDataSink ()
    {
        auxBinaryStream.detach();
        if (auxFileStream.is_open())
            auxFileStream.close();
    }
    
    void attach (const std::string &fileStem);
    void put (const Streamline &data);
    void done ();
};

class MedianTrackvisDataSink : public TrackvisDataSink
{
protected:
    double quantile;
    Streamline median;
    
public:
    MedianTrackvisDataSink (const std::string &fileStem, const Grid<3> &grid, const double quantile = 0.99)
        : TrackvisDataSink(fileStem,grid), quantile(quantile) {}
    
    void setup (const size_type &count, const_iterator begin, const_iterator end);
    void done ();
};

#endif
