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

#define USE_FASTLZ 0

#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <stack>
#include <iostream>
#include "tinyxml.h"

#if USE_FASTLZ
#include "fastlz.h"
#endif

using namespace std;


/* type defines */
#define DATA_NULL				0
#define DATA_CHAR               1
#define DATA_UCHAR              2
#define DATA_INT				3
#define DATA_UINT               4
#define DATA_LONG				5
#define DATA_ULONG              6
#define DATA_LONGLONG           7
#define DATA_FLOAT				8
#define DATA_DOUBLE				9
#define DATA_LONGDOUBLE         10
#define DATA_STRING				11
#define DATA_STR_VECTOR		    12
#define DATA_CHAR_VECTOR        13
#define DATA_UCHAR_VECTOR       14
#define DATA_INT_VECTOR		    15
#define DATA_UINT_VECTOR        16
#define DATA_LONG_VECTOR		17
#define DATA_ULONG_VECTOR       18
#define DATA_LONGLONG_VECTOR    19
#define DATA_FLOAT_VECTOR		20
#define DATA_DOUBLE_VECTOR		21
#define DATA_LONGDOUBLE_VECTOR  22
#define DATA_VOID               23


/* map comparison function */
struct string_less : public std::binary_function<std::string,std::string,bool>
{
 bool operator()(const std::string& a,const std::string& b) const
 {
  return a.compare(b) < 0;
 }
};

/* int comparison function */
struct int_less : public std::binary_function<int,int,bool>
{
 bool operator()(int a,int b) const
 {
        return a < b;
 }
};


/* Data Exceptions */
class DataException
{
private:
    string reason;
    
public:
    DataException(const char *why) : reason(why) {}
    string what() { return reason; } 
    operator string() { return reason; }
};


class DataTypeMismatchException : public DataException
{
public:
    DataTypeMismatchException(const char *why) : DataException(why) { }
};


class DataInvalidChildException : public DataException
{
public:
    DataInvalidChildException(const char *why) : DataException(why) { }
};


class DataElement
{
private:
    int data_type;
    size_t data_size;
    unsigned int unit_size;

    char *data_val;
    
    void data_init(size_t data_size_in);
    
public:
    DataElement();
    ~DataElement();
    
    int getDataType();
    char *getDataPointer();
    size_t getDataSize();
    unsigned int getUnitSize();
    
    /* set overloads */		
    void set(const char &char_in);
    void set(const unsigned char &uchar_in);
    void set(const int &int_in);
    void set(const unsigned int &uint_in);
    void set(const long &long_in);
    void set(const unsigned long &ulong_in);
    void set(const long long &llong_in);
    void set(const float &float_in);
    void set(const double &double_in);
    void set(const long double &ldouble_in);
    
    void set(const char *data_in, long size_in); /* voids, file chunks anyone? */
    void set(const char *data_in);	/* strings, stops at NULL, returns as string */
    
    void set(const string &str_in);
    
    void set(vector<string> &strvect_in);
    void set(std::set<string> &strset_in);
    void set(vector<char> &charvect_in);
    void set(vector<unsigned char> &ucharvect_in);
    void set(vector<int> &intvect_in);
    void set(vector<unsigned int> &uintvect_in);
    void set(vector<long> &longvect_in);
    void set(vector<unsigned long> &ulongvect_in);
    void set(vector<long long> &llongvect_in);
    void set(vector<float> &floatvect_in);
    void set(vector<double> &doublevect_in);
    void set(vector<long double> &ldoublevect_in);
    
    
    /* get overloads */
    void get(char &char_in);
	void get(unsigned char &uchar_in);
    void get(int &int_in);
    void get(unsigned int &uint_in);
    void get(long &long_in);
    void get(unsigned long &ulong_in);
    void get(long long &long_in);
    void get(float &float_in);
    void get(double &double_in);
    void get(long double &ldouble_in);
    
    void get(char **data_in); /* getting a void or string */
    void get(string &str_in); 
    void get(std::set<string> &strset_in);
    
    void get(vector<string> &strvect_in);
    void get(vector<char> &charvect_in);
    void get(vector<unsigned char> &ucharvect_in);
    void get(vector<int> &intvect_in);
    void get(vector<unsigned int> &uintvect_in);
    void get(vector<long> &longvect_in);
    void get(vector<unsigned long> &ulongvect_in);
    void get(vector<long long> &llongvect_in);
    void get(vector<float> &floatvect_in);
    void get(vector<double> &doublevect_in);
    void get(vector<long double> &ldoublevect_in);
    
    
    /* special get functions, saves creating unnecessary vars */
    int getChar() { char i_get; get(i_get); return i_get; };
    unsigned int getUChar() { unsigned char i_get; get(i_get); return i_get; };
    int getInt() { int i_get; get(i_get); return i_get; };
    unsigned int getUInt() { unsigned int i_get; get(i_get); return i_get; };
    long getLong()  { long l_get; get(l_get); return l_get; };
    unsigned long getULong()  { unsigned long l_get; get(l_get); return l_get; };
    long getLongLong()  { long long l_get; get(l_get); return l_get; };
    float getFloat()  { float f_get; get(f_get); return f_get; };
    double getDouble()  { double d_get; get(d_get); return d_get; };
    long double getLongDouble()  { long double d_get; get(d_get); return d_get; };
    
    std::string toString();
    
    /* serialize functions */
    long getSerializedSize();
    long getSerialized(char **ser_str);
    
    void setSerialized(char *ser_str);
};


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
    DataNode(const char *name_in);
    
    ~DataNode();		
    
    void setName(const char *name_in);
    string &getName() { return node_name; }

    DataNode *getParentNode() { return parentNode; };
    void setParentNode(DataNode &parentNode_in) { parentNode = &parentNode_in; };

    int numChildren();	/* Number of children */
    int numChildren(const char *name_in); /* Number of children named 'name_in' */
    
    DataElement *element(); /* DataElement at this node */
    
    DataNode *newChild(const char *name_in);
    DataNode *child(const char *name_in, int index = 0);
    DataNode *child(int index);
    
    
    bool hasAnother(const char *name_in);	/* useful for while() loops in conjunction with getNext() */
    bool hasAnother();
    DataNode *getNext(const char *name_in); /* get next of specified name */
    DataNode *getNext();	/* get next child */
    void rewind(const char *name_in);	/* rewind specific */
    void rewind();	/* rewind generic */
        
    void findAll(const char *name_in, vector<DataNode *> &node_list_out);
    
//    operator string () { string s; element()->get(s); return s; }
    operator const char * () { if (element()->getDataType() == DATA_STRING) return element()->getDataPointer(); else return NULL; }
    operator char () { char v; element()->get(v); return v; }
    operator unsigned char () { unsigned char v; element()->get(v); return v; }
    operator int () { int v; element()->get(v); return v; }
    operator unsigned int () { unsigned int v; element()->get(v); return v; }
    operator long () { long v; element()->get(v); return v; }
    operator unsigned long () { unsigned long v; element()->get(v); return v; }
    operator long long () { long long v; element()->get(v); return v; }
    operator float () { float v; element()->get(v); return v; }
    operator double () { double v; element()->get(v); return v; }
    operator long double () { long double v; element()->get(v); return v; }

    operator vector<char> () { vector<char> v; element()->get(v);  return v; }
    operator vector<unsigned char> () { vector<unsigned char> v; element()->get(v);  return v; }
    operator vector<int> () { vector<int> v; element()->get(v);  return v; }
    operator vector<unsigned int> () { vector<unsigned int> v; element()->get(v);  return v; }
    operator vector<long> () { vector<long> v; element()->get(v);  return v; }
    operator vector<unsigned long> () { vector<unsigned long> v; element()->get(v);  return v; }
    operator vector<float> () { vector<float> v; element()->get(v);  return v; }
    operator vector<double> () { vector<double> v; element()->get(v);  return v; }
    operator vector<long double> () { vector<long double> v; element()->get(v);  return v; }
    
    const string &operator= (const string &s) { element()->set(s); return s; }

    char operator= (char i) { element()->set(i); return i; }
    unsigned char operator= (unsigned char i) { element()->set(i); return i; }
    int operator= (int i) { element()->set(i); return i; }
    unsigned int operator= (unsigned int i) { element()->set(i); return i; }
    long operator= (long i) { element()->set(i); return i; }
    unsigned long operator= (unsigned long i) { element()->set(i); return i; }
    long long operator= (long long i) { element()->set(i); return i; }
    float operator= (float i) { element()->set(i); return i; }
    double operator= (double i) { element()->set(i); return i; }
    long double operator= (long double i) { element()->set(i); return i; }
    
    vector<char> &operator= (vector<char> &v) { element()->set(v); return v; }
    vector<unsigned char> &operator= (vector<unsigned char> &v) { element()->set(v); return v; }
    vector<int> &operator= (vector<int> &v) { element()->set(v); return v; }
    vector<unsigned int> &operator= (vector<unsigned int> &v) { element()->set(v); return v; }
    vector<long> &operator= (vector<long> &v) { element()->set(v); return v; }
    vector<unsigned long> &operator= (vector<unsigned long> &v) { element()->set(v); return v; }
    vector<float> &operator= (vector<float> &v) { element()->set(v); return v; }
    vector<double> &operator= (vector<double> &v) { element()->set(v); return v; }
    vector<long double> &operator= (vector<long double> &v) { element()->set(v); return v; }

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
    
public:
    DataTree(const char *name_in);
    DataTree();
    ~DataTree();
    
    DataNode *rootNode();
    
    void nodeToXML(DataNode *elem, TiXmlElement *elxml);
    void setFromXML(DataNode *elem, TiXmlNode *elxml, bool root_node=true, DT_FloatingPointPolicy fpp=USE_FLOAT);
    void decodeXMLText(DataNode *elem, const char *in_text, DT_FloatingPointPolicy fpp);
    
    void printXML();	/* print datatree as XML */
    long getSerializedSize(DataElement &de_node_names, bool debug=false);	/* get serialized size + return node names header */
    long getSerialized(char **ser_str, bool debug=false);	
    void setSerialized(char *ser_str, bool debug=false);
    
    bool LoadFromFileXML(const std::string& filename, DT_FloatingPointPolicy fpp=USE_FLOAT);
    bool SaveToFileXML(const std::string& filename);
    
//    bool SaveToFile(const std::string& filename);
//    bool LoadFromFile(const std::string& filename);

    bool SaveToFile(const std::string& filename, bool compress = true, int compress_level = 2);
    bool LoadFromFile(const std::string& filename);
};

