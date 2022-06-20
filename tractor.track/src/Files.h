#ifndef _FILES_H_
#define _FILES_H_

#include "DataSource.h"
#include "Streamline.h"

class SourceFileAdapter
{
protected:
    BinaryInputStream inputStream;
    
    size_t count;
    std::vector<std::string> properties;
    ImageSpace *space = nullptr;
    
public:
    SourceFileAdapter (const std::string &path)
    {
        inputStream.attach(path);
    }
    
    virtual ~SourceFileAdapter ()
    {
        delete space;
    }
    
    size_t nStreamlines () { return count; }
    size_t nProperties () { return properties.size(); }
    std::vector<std::string> propertyNames () { return properties; }
    ImageSpace * imageSpace () { return space; }
    
    virtual void open () {}
    virtual size_t dataOffset () { return 0; }
    virtual void seek (const size_t offset)
    {
        inputStream->seekg(offset);
        if (!inputStream->good())
            throw std::runtime_error("Failed to seek to offset " + std::to_string(offset));
    }
    virtual void read (Streamline &data) {}
    virtual void skip (const size_t n = 1)
    {
        // Default implementation: read each streamline as normal, but ignore it
        Streamline ignored;
        for (size_t i=0; i<n; i++)
            read(ignored);
    }
    virtual void close () {}
};

class StreamlineFileSource : public DataSource<Streamline>
{
private:
    // Prevent initialisation without a path
    StreamlineFileSource () {}
    
protected:
    size_t currentStreamline = 0, totalStreamlines = 0;
    SourceFileAdapter *source = nullptr;
    
    bool haveLabels = false;
    std::vector<std::set<int>> labels;
    std::vector<size_t> offsets;
    
    bool fileExists (const std::string &path) const
    {
        return std::ifstream(path).good();
    }
    
    void readLabels (const std::string &path);
    
public:
    StreamlineFileSource (const std::string &fileStem, const bool readLabels = true)
    {
        if (fileExists(fileStem + ".trk"))
            source = new TrackvisFileSource(fileStem + ".trk");
        else if (fileExists(fileStem + ".tck"))
            source = new MrtrixFileSource(fileStem + ".tck");
        else
            throw std::runtime_error("Specified streamline source file does not exist");
        
        source->open();
        totalStreamlines = source->nStreamlines();
        
        if (readLabels && fileExists(fileStem + ".trkl"))
            readLabels(fileStem + ".trkl");
    }
    
    bool more () { return currentStreamline < totalStreamlines; }
    void get (Streamline &data)
    {
        source->read(data);
        if (haveLabels && labels.size() > currentStreamline)
            data.setLabels(labels[currentStreamline]);
        currentStreamline++;
    }
    void seek (const int n);
    bool seekable () { return true; }
    void done () { source->close(); }
};

class SinkFileAdapter
{
protected:
    BinaryOutputStream outputStream;
    
    size_t count;
    
public:
    SinkFileAdapter (const std::string &path)
    {
        outputStream.attach(path);
    }
    
    virtual ~SinkFileAdapter () {}
    
    void setCount (const size_t count) { this->count = count; }
    
    virtual void open (const bool append) {}
    virtual void write (const Streamline &data) {}
    virtual void close () {}
};

class StreamlineFileSink : public DataSink<Streamline>
{
private:
    // Prevent initialisation without a path
    StreamlineFileSink () {}
    
protected:
    size_t currentStreamline = 0;
    std::string fileStem;
    SinkFileAdapter *sink = nullptr;
    
    bool needLabels = false;
    
    void writeLabels (const std::string &path);
    
public:
    StreamlineFileSink (const std::string &fileStem, const bool writeLabels = true, const bool append = false)
        : fileStem(fileStem), needLabels(writeLabels)
    {
        sink = new TrackvisFileSink(fileStem + ".trk");
        sink->open(append);
    }
    
    void put (const Streamline &data)
    {
        sink->write(data);
        currentStreamline++;
    }
    
    void done ()
    {
        sink->setCount(currentStreamline);
        sink->close();
        writeLabels(fileStem + ".trkl");
    }
};

#endif
