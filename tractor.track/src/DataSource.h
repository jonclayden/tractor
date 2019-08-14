#ifndef _DATA_SOURCE_H_
#define _DATA_SOURCE_H_

// Data source: responsible for reading or generating data elements
template <class ElementType> class DataSource
{
public:
    virtual bool more () { return false; }
    virtual void get (ElementType &data) {}
    virtual void seek (const int n) {}
    virtual bool seekable () { return false; }
};

// Data sink: responsible for exporting or writing data elements
template <class ElementType> class DataSink
{
protected:
    typedef typename std::list<ElementType>::size_type size_type;
    typedef typename std::list<ElementType>::const_iterator const_iterator;
    
public:
    virtual ~DataSink () {}
    
    virtual void setup (const size_type &count, const_iterator begin, const_iterator end) {}
    virtual void put (const ElementType &data) {}
    virtual void finish () {}
    virtual void done () {}
};

// Data manipulator: responsible for transforming or removing data elements
template <class ElementType> class DataManipulator
{
public:
    virtual ~DataManipulator () {}
    
    // If the return value is false, the element will be removed
    virtual bool process (ElementType &data) { return true; }
};

#endif
