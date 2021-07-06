#pragma once
/*
 * DataElement/DataNode/DataTree -- structured serialization/deserialization system
 * designed for the CoolMule project :)
 *
 Copyright (C) 2003 by Charles J. Cliffe
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <stack>
#include <iostream>
#include "tinyxml.h"

using namespace std;


/* map comparison function */
struct string_less
{
 bool operator()(const std::string& a,const std::string& b) const
 {
  return a.compare(b) < 0;
 }
};



/* Data Exceptions */
class DataException : public exception
{
private:
    string reason;
    
public:
    explicit DataException(const char *why) : reason(why) {}
    const char* what() const noexcept override { return reason.c_str(); }
    explicit operator string() { return reason; }
};


class DataTypeMismatchException : public DataException
{
public:
    explicit DataTypeMismatchException(const char *why) : DataException(why) { }
};


class DataInvalidChildException : public DataException
{
public:
    explicit DataInvalidChildException(const char *why) : DataException(why) { }
};


class DataElement
{
public :

    enum class Type {
        DATA_NULL,
        DATA_CHAR,
        DATA_UCHAR,
        DATA_INT,
        DATA_UINT,
        DATA_LONG,
        DATA_ULONG,
        DATA_LONGLONG,
        DATA_FLOAT,
        DATA_DOUBLE,
        DATA_STRING,
        DATA_STR_VECTOR,
        DATA_CHAR_VECTOR,
        DATA_UCHAR_VECTOR,
        DATA_INT_VECTOR,
        DATA_UINT_VECTOR,
        DATA_LONG_VECTOR,
        DATA_ULONG_VECTOR,
        DATA_LONGLONG_VECTOR,
        DATA_FLOAT_VECTOR,
        DATA_DOUBLE_VECTOR,
        DATA_VOID,
        DATA_WSTRING
    };

    typedef vector<unsigned char> DataElementBuffer;

    typedef vector< DataElementBuffer > DataElementBufferVector;

private:
    Type data_type;

    // raw buffer holding data_type element in bytes form.
    DataElementBuffer data_val;

    //keep the vector of types in a spearate vector of DataElementBuffer.
    DataElementBufferVector data_val_vector;

   //specializations to extract type: (need to be declared/done OUTSIDE of class scope else "Error: explicit specialization is not allowed in the current scope")
   //this is apparently fixed in C++17...
   // so we need to workaround it with a partial specialization using a fake Dummy parameter.

    //if the exact right determineScalarDataType specialization was not used, throw exception at runtime.
    template<typename U, typename Dummy = int >
    Type determineScalarDataType(const U& /* type_in */) { throw DataTypeMismatchException("determineScalarDataType(U) usage with unsupported type !"); }

    template< typename Dummy = int >
   Type determineScalarDataType(const char& /* type_in */) { return DataElement::Type::DATA_CHAR; }

    template< typename Dummy = int >
    Type determineScalarDataType(const unsigned char& /* type_in */) { return DataElement::Type::DATA_UCHAR; }

    template< typename Dummy = int >
    Type determineScalarDataType(const int& /* type_in */) { return DataElement::Type::DATA_INT; }

    template< typename Dummy = int >
    Type determineScalarDataType(const unsigned int& /* type_in */) { return DataElement::Type::DATA_UINT; }

    template< typename Dummy = int >
    Type determineScalarDataType(const long& /* type_in */) { return DataElement::Type::DATA_LONG; }

    template< typename Dummy = int >
    Type determineScalarDataType(const unsigned long& /* type_in */) { return DataElement::Type::DATA_ULONG; }

    template< typename Dummy = int >
    Type determineScalarDataType(const long long& /* type_in */) { return DataElement::Type::DATA_LONGLONG; }

    template< typename Dummy = int >
    Type determineScalarDataType(const float& /* type_in */) { return DataElement::Type::DATA_FLOAT; }

    template< typename Dummy = int >
    Type determineScalarDataType(const double& /* type_in */) { return DataElement::Type::DATA_DOUBLE; }
  
    //vector versions:
    //if the exact right determineVectorDataType specialization was not used, throw exception at runtime.
    template<typename V, typename Dummy = int >
    Type determineVectorDataType(const vector<V>& /* type_in */) { throw DataTypeMismatchException("determineVectorDataType(V) usage with unsupported type !"); }

    template< typename Dummy = int >
    Type determineVectorDataType(const vector<char>& /* type_in */) { return DataElement::Type::DATA_CHAR_VECTOR; }

    template< typename Dummy = int >
   Type determineVectorDataType(const vector<unsigned char>& /* type_in */) { return DataElement::Type::DATA_UCHAR_VECTOR; }

    template< typename Dummy = int >
    Type determineVectorDataType(const vector<int>& /* type_in */) { return DataElement::Type::DATA_INT_VECTOR; }

    template< typename Dummy = int >
    Type determineVectorDataType(const vector<unsigned int>& /* type_in */) { return DataElement::Type::DATA_UINT_VECTOR; }

    template< typename Dummy = int >
    Type determineVectorDataType(const vector<long>& /* type_in */) { return DataElement::Type::DATA_LONG_VECTOR; }

    template< typename Dummy = int >
    Type determineVectorDataType(const vector<unsigned long>& /* type_in */) { return DataElement::Type::DATA_ULONG_VECTOR; }

    template< typename Dummy = int >
    Type determineVectorDataType(const vector<long long>& /* type_in */) { return DataElement::Type::DATA_LONGLONG_VECTOR; }

    template< typename Dummy = int >
    Type determineVectorDataType(const vector<float>& /* type_in */) { return DataElement::Type::DATA_FLOAT_VECTOR; }

    template< typename Dummy = int >
    Type determineVectorDataType(const vector<double>& /* type_in */) { return DataElement::Type::DATA_DOUBLE_VECTOR; }

public: 

    DataElement();
    DataElement(DataElement &cloneFrom);
    ~DataElement();
    
    Type getDataType();
    char *getDataPointer();
    size_t getDataSize();
     
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //set overloads :

    // general templates : for scalars
    template<typename T, typename Dummy = int >
    void set(const T& scalar_in) {

        data_type = determineScalarDataType<T>(scalar_in);

        int unit_size = sizeof(T);
        //copy in a temporary variable (needed ?)
        T local_copy = scalar_in;
        auto* local_copy_ptr = reinterpret_cast<unsigned char*>(&local_copy);

        data_val.assign(local_copy_ptr, local_copy_ptr + unit_size);
    }

    // general templates : for vector of scalars
    template<typename T, typename Dummy = int >
    void set(const vector<T>& scalar_vector_in) {

        data_type = determineVectorDataType<T>(scalar_vector_in);

        int unit_size = sizeof(T);

        data_val_vector.clear();

        DataElementBuffer single_buffer;

        for (auto single_element : scalar_vector_in) {

            //copy in a temporary variable (needed ?)
            T local_copy = single_element;
            auto* local_copy_ptr = reinterpret_cast<unsigned char*>(&local_copy);

            single_buffer.assign(local_copy_ptr, local_copy_ptr + unit_size);

            data_val_vector.push_back(single_buffer);
        }
    }

    //template specialization : for string 
    template< typename Dummy = int >
    void set(const string& str_in) {

        data_type = DataElement::Type::DATA_STRING;

        data_val.assign(str_in.begin(), str_in.end());
    }

    //template specialization : for wstring 
    template< typename Dummy = int >
    void set(const wstring& wstr_in) {

        data_type = DataElement::Type::DATA_WSTRING;

        //wchar_t is tricky, the terminating zero is actually a (wchar_t)0 !
        //wchar_t is typically 16 bits on windows, and 32 bits on Unix, so use sizeof(wchar_t) everywhere.
        size_t maxLenBytes = (wstr_in.length() + 1) * sizeof(wchar_t);

        //be paranoid, zero the buffer
        char *tmp_str = (char *)::calloc(maxLenBytes, sizeof(char));

        //if something awful happens, the last sizeof(wchar_t) is at least zero...
        ::wcstombs(tmp_str, wstr_in.c_str(), maxLenBytes - sizeof(wchar_t));

        data_val.assign(tmp_str, tmp_str + maxLenBytes - sizeof(wchar_t));

        ::free(tmp_str);
    }

    //template specialization : for vector<string> 
    template< typename Dummy = int >
    void set(const vector<string>& vector_str_in) {

        data_type = DataElement::Type::DATA_STR_VECTOR;

        data_val_vector.clear();

        DataElementBuffer single_buffer;

        for (auto single_element : vector_str_in) {

            single_buffer.assign(single_element.begin(), single_element.end());

            data_val_vector.push_back(single_buffer);
        }
    }

  
    ///specific versions
    void set(const std::set<string> &strset_in);
    void set(const char *data_in, long size_in); /* voids, file chunks anyone? */
    void set(const char *data_in);	/* strings, stops at NULL, returns as string */
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /* get overloads */

    template<typename T, typename Dummy = int >
    void get(T& scalar_out) {

        if (getDataSize() == 0) {
            throw DataException("Cannot get() the scalar, DataElement is empty !");
        }

        DataElement::Type storageType = getDataType();

        //TODO: smarter way with templates ?
        if (storageType == DataElement::Type::DATA_CHAR) {
            char* storage_ptr = reinterpret_cast<char*>(&data_val[0]);
            //constructor-like 
            scalar_out = T(*storage_ptr);

        } else if (storageType == DataElement::Type::DATA_UCHAR) {
            auto* storage_ptr = reinterpret_cast<unsigned char*>(&data_val[0]);
            //constructor-like 
            scalar_out = T(*storage_ptr);
        } else if (storageType == DataElement::Type::DATA_INT) {
            int* storage_ptr = reinterpret_cast<int*>(&data_val[0]);
            //constructor-like 
            scalar_out = T(*storage_ptr);
        } else if (storageType == DataElement::Type::DATA_UINT) {
            auto* storage_ptr = reinterpret_cast<unsigned int*>(&data_val[0]);
            //constructor-like 
            scalar_out = T(*storage_ptr);
        } else if (storageType == DataElement::Type::DATA_LONG) {
            long* storage_ptr = reinterpret_cast<long*>(&data_val[0]);
            //constructor-like 
            scalar_out = T(*storage_ptr);
        } else if (storageType == DataElement::Type::DATA_ULONG) {
            auto* storage_ptr = reinterpret_cast<unsigned long*>(&data_val[0]);
            //constructor-like 
            scalar_out = T(*storage_ptr);
        } else if (storageType == DataElement::Type::DATA_LONGLONG) {
            auto* storage_ptr = reinterpret_cast<long long*>(&data_val[0]);
            //constructor-like 
            scalar_out = T(*storage_ptr);
        } else if (storageType == DataElement::Type::DATA_FLOAT) {
            auto* storage_ptr = reinterpret_cast<float*>(&data_val[0]);
            //constructor-like 
            scalar_out = T(*storage_ptr);
        } else if (storageType == DataElement::Type::DATA_DOUBLE) {
            auto* storage_ptr = reinterpret_cast<double*>(&data_val[0]);
            //constructor-like 
            scalar_out = T(*storage_ptr);
        } 
    }

    // general templates : for vector of scalars
    template<typename T, typename Dummy = int >
    void get(vector<T>& scalar_vector_out) {

        scalar_vector_out.clear();

        DataElementBuffer single_buffer;

        Type storageType = getDataType();

        for (auto single_storage_element : data_val_vector) {

            if (single_storage_element.empty()) {
                throw DataException("Cannot get(vector<scalar>) on single element because it is empty!");
            }

            T scalar_out;

            //TODO: smarter way with templates ?
            if (storageType == DataElement::Type::DATA_CHAR_VECTOR) {
                char* storage_ptr = reinterpret_cast<char*>(&single_storage_element[0]);
                //constructor-like 
                scalar_out = T(*storage_ptr);

            } else if (storageType == DataElement::Type::DATA_UCHAR_VECTOR) {
                auto* storage_ptr = reinterpret_cast<unsigned char*>(&single_storage_element[0]);
                //constructor-like 
                scalar_out = T(*storage_ptr);
            } else if (storageType == DataElement::Type::DATA_INT_VECTOR) {
                int* storage_ptr = reinterpret_cast<int*>(&single_storage_element[0]);
                //constructor-like 
                scalar_out = T(*storage_ptr);
            } else if (storageType == DataElement::Type::DATA_UINT_VECTOR) {
                auto* storage_ptr = reinterpret_cast<unsigned int*>(&single_storage_element[0]);
                //constructor-like 
                scalar_out = T(*storage_ptr);
            } else if (storageType == DataElement::Type::DATA_LONG_VECTOR) {
                long* storage_ptr = reinterpret_cast<long*>(&single_storage_element[0]);
                //constructor-like 
                scalar_out = T(*storage_ptr);
            } else if (storageType == DataElement::Type::DATA_ULONG_VECTOR) {
                auto* storage_ptr = reinterpret_cast<unsigned long*>(&single_storage_element[0]);
                //constructor-like 
                scalar_out = T(*storage_ptr);
            } else if (storageType == DataElement::Type::DATA_LONGLONG_VECTOR) {
                auto* storage_ptr = reinterpret_cast<long long*>(&single_storage_element[0]);
                //constructor-like 
                scalar_out = T(*storage_ptr);
            } else if (storageType == DataElement::Type::DATA_FLOAT_VECTOR) {
                auto* storage_ptr = reinterpret_cast<float*>(&single_storage_element[0]);
                //constructor-like 
                scalar_out = T(*storage_ptr);
            } else if (storageType == DataElement::Type::DATA_DOUBLE_VECTOR) {
                auto* storage_ptr = reinterpret_cast<double*>(&single_storage_element[0]);
                //constructor-like 
                scalar_out = T(*storage_ptr);
            } 

            scalar_vector_out.push_back(scalar_out);
        } //end for.
    }

    //template specialization : for string or void* returned as string
    template< typename Dummy = int >
    void get(string& str_out) {

        //reset
        str_out.clear();

        if (data_type == DataElement::Type::DATA_NULL) {
            //it means TinyXML has parsed an empty tag,
            //so return an empty string.
            return;
        }

        if (data_type != DataElement::Type::DATA_STRING && data_type != DataElement::Type::DATA_VOID) {
            throw(DataTypeMismatchException("Type mismatch, neither a STRING nor a VOID*"));
        }

        for (auto single_char : data_val) {
            str_out.push_back((char)single_char);
        }
    }

    //template specialization : for wstring 
    template< typename Dummy = int >
    void get(wstring& wstr_out) {

        //reset
        wstr_out.clear();

        if (data_type == DataElement::Type::DATA_NULL) {
            //it means TinyXML has parsed an empty tag,
            //so return an empty string.
            return;
        }

        if (data_type != DataElement::Type::DATA_WSTRING) {
            throw(DataTypeMismatchException("Type mismatch, not a WSTRING"));
        }

        if (getDataSize() >= sizeof(wchar_t)) {

            //data_val is an array of bytes holding wchar_t characters, plus a terminating (wchar_t)0
           //wchar_t is typically 16 bits on windows, and 32 bits on Unix, so use sizeof(wchar_t) everywhere.
            size_t maxNbWchars = getDataSize() / sizeof(wchar_t);

            //be paranoid, zero the buffer
            auto *tmp_wstr = (wchar_t *)::calloc(maxNbWchars + 1, sizeof(wchar_t));

            //the last wchar_t is actually zero if anything goes wrong...
            ::mbstowcs(tmp_wstr, (const char*)&data_val[0], maxNbWchars);

            wstr_out.assign(tmp_wstr);

            ::free(tmp_wstr);
        }
    }

    //template specialization : for vector<string> 
    template< typename Dummy = int >
    void get(vector<string>& vector_str_out) {

        if (data_type != DataElement::Type::DATA_STR_VECTOR) {
            throw(DataTypeMismatchException("Type mismatch, not a STRING VECTOR"));
        }

        vector_str_out.clear();

        string single_buffer;

        for (auto single_element : data_val_vector) {

            single_buffer.assign(single_element.begin(), single_element.end());

            vector_str_out.push_back(single_buffer);
        }
    }
  
    //special versions:
    void get(DataElementBuffer& data_out); /* getting a void or string */
    void get(std::set<string> &strset_out);
       
    
    /* special get functions, saves creating unnecessary vars */
    int getChar()  { char i_get; get(i_get); return i_get; };
    unsigned int getUChar()  { unsigned char i_get; get(i_get); return i_get; };
    int getInt()  { int i_get; get(i_get); return i_get; };
    unsigned int getUInt()  { unsigned int i_get; get(i_get); return i_get; };
    long getLong()  { long l_get; get(l_get); return l_get; };
    unsigned long getULong() { unsigned long l_get; get(l_get); return l_get; };
    long long getLongLong() { long long l_get; get(l_get); return l_get; };
    float getFloat() { float f_get; get(f_get); return f_get; };
    double getDouble() { double d_get; get(d_get); return d_get; };
    
    std::string toString();
};

///
class DataNode
{
private:
    DataNode *parentNode;
    vector<DataNode *> children;
    map<string, vector<DataNode *>, string_less> childmap;
    map<string, unsigned int, string_less> childmap_ptr;
    
    string node_name;
    DataElement *data_elem;
    unsigned int ptr;
    
    
public:
    DataNode();
    explicit DataNode(const char *name_in);
    DataNode(const char *name_in, DataElement &cloneFrom);
    DataNode(const char *name_in, DataNode &cloneFrom);

    ~DataNode();
    
    void setName(const char *name_in);
    string &getName() { return node_name; }

    DataNode *getParentNode() { return parentNode; };
    void setParentNode(DataNode &parentNode_in) { parentNode = &parentNode_in; };

    size_t numChildren();	/* Number of children */
    size_t numChildren(const char *name_in); /* Number of children named 'name_in' */
    
    DataElement *element(); /* DataElement at this node */
    
    DataNode *newChild(const char *name_in);
    DataNode *newChild(const char *name_in, DataNode *otherNode);
    DataNode *newChildCloneFrom(const char *name_in, DataNode *cloneFrom);
    DataNode *child(const char *name_in, int index = 0);
    DataNode *child(int index);
    
    
    bool hasAnother(const char *name_in);	/* useful for while() loops in conjunction with getNext() */
    bool hasAnother();
    DataNode *getNext(const char *name_in); /* get next of specified name */
    DataNode *getNext();	/* get next child */
    void rewind(const char *name_in);	/* rewind specific */
    void rewind();	/* rewind generic */
    void rewindAll();
    
    void findAll(const char *name_in, vector<DataNode *> &node_list_out);
    
    explicit operator string () { string s; element()->get(s); return s; }
    explicit operator const char * () { if (element()->getDataType() == DataElement::Type::DATA_STRING) { return element()->getDataPointer(); } else { return nullptr; } }
    explicit operator char () { char v=0; element()->get(v); return v; }
    explicit operator unsigned char () { unsigned char v=0; element()->get(v); return v; }
    explicit operator int () { int v=0; element()->get(v); return v; }
    explicit operator unsigned int () { unsigned int v=0; element()->get(v); return v; }
    explicit operator long () { long v=0; element()->get(v); return v; }
    explicit operator unsigned long () { unsigned long v=0; element()->get(v); return v; }
    explicit operator long long () { long long v=0; element()->get(v); return v; }
    explicit operator float () { float v=0; element()->get(v); return v; }
    explicit operator double () { double v=0; element()->get(v); return v; }
    
    explicit operator vector<char> () { vector<char> v; element()->get(v);  return v; }
    explicit operator vector<unsigned char> () { vector<unsigned char> v; element()->get(v);  return v; }
    explicit operator vector<int> () { vector<int> v; element()->get(v);  return v; }
    explicit operator vector<unsigned int> () { vector<unsigned int> v; element()->get(v);  return v; }
    explicit operator vector<long> () { vector<long> v; element()->get(v);  return v; }
    explicit operator vector<unsigned long> () { vector<unsigned long> v; element()->get(v);  return v; }
    explicit operator vector<float> () { vector<float> v; element()->get(v);  return v; }
    explicit operator vector<double> () { vector<double> v; element()->get(v);  return v; }
    
    const string &operator= (const string &s) { element()->set(s); return s; }
    const wstring &operator= (const wstring &s) { element()->set(s); return s; }

    char operator= (char i) { element()->set(i); return i; }
    unsigned char operator= (unsigned char i) { element()->set(i); return i; }
    int operator= (int i) { element()->set(i); return i; }
    unsigned int operator= (unsigned int i) { element()->set(i); return i; }
    long operator= (long i) { element()->set(i); return i; }
    unsigned long operator= (unsigned long i) { element()->set(i); return i; }
    long long operator= (long long i) { element()->set(i); return i; }
    float operator= (float i) { element()->set(i); return i; }
    double operator= (double i) { element()->set(i); return i; }
    
    vector<char> &operator= (vector<char> &v) { element()->set(v); return v; }
    vector<unsigned char> &operator= (vector<unsigned char> &v) { element()->set(v); return v; }
    vector<int> &operator= (vector<int> &v) { element()->set(v); return v; }
    vector<unsigned int> &operator= (vector<unsigned int> &v) { element()->set(v); return v; }
    vector<long> &operator= (vector<long> &v) { element()->set(v); return v; }
    vector<unsigned long> &operator= (vector<unsigned long> &v) { element()->set(v); return v; }
    vector<float> &operator= (vector<float> &v) { element()->set(v); return v; }
    vector<double> &operator= (vector<double> &v) { element()->set(v); return v; }
    
    DataNode *operator[] (const char *name_in) { return getNext(name_in); }
    DataNode *operator[] (int idx) { return child(idx); }

    bool operator() (const char *name_in) { return hasAnother(name_in); }
    bool operator() () { return hasAnother(); }

    DataNode *operator ^(const char *name_in) { return newChild(name_in); }
};


typedef vector<DataNode *> DataNodeList;

enum DT_FloatingPointPolicy {
    USE_FLOAT,
    USE_DOUBLE
};

class DataTree
{
private:
    DataNode dn_root;
    
    string wsEncode(const wstring& wstr);
    wstring wsDecode(const string& str);

public:
    explicit DataTree(const char *name_in);
    DataTree();
    ~DataTree();
    
    DataNode *rootNode();
    
    void nodeToXML(DataNode *elem, TiXmlElement *elxml);
    void setFromXML(DataNode *elem, TiXmlNode *elxml, bool root_node=true, DT_FloatingPointPolicy fpp=USE_FLOAT);
    void decodeXMLText(DataNode *elem, const char *in_text, DT_FloatingPointPolicy fpp);
    
    void printXML();	/* print datatree as XML */
    
    bool LoadFromFileXML(const std::string& filename, DT_FloatingPointPolicy fpp=USE_FLOAT);
    bool SaveToFileXML(const std::string& filename);
   
};
