/*
 * DataElement/DataNode/DataTree -- structured serialization/unserialization system 
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

#include "DataTree.h"
#include <fstream>
#include <math.h>

/* DataElement class */

using namespace std;

#define STRINGIFY(A)  #A

DataElement::DataElement() : data_type(DATA_NULL), data_val(NULL), data_size(0), unit_size(0) {
}

DataElement::~DataElement() {
    if (data_val) {
        delete data_val;
        data_val = NULL;
    }
}

void DataElement::data_init(long data_size_in) {
    if (data_val) {
        delete data_val;
        data_val = NULL;
    }
    data_size = data_size_in;
    if (data_size) {
        data_val = new char[data_size];
    }
}

char * DataElement::getDataPointer() {
    return data_val;
}

int DataElement::getDataType() {
    return data_type;
}

long DataElement::getDataSize() {
    return data_size;
}

int DataElement::getUnitSize() {
    return unit_size;
}

#define DataElementSetNumericDef(enumtype, datatype) void DataElement::set(const datatype& val_in) { \
        data_type = enumtype; \
        unit_size = sizeof(datatype); \
        data_init(unit_size); \
        memcpy(data_val, &val_in, data_size); \
}

DataElementSetNumericDef(DATA_CHAR, char)
DataElementSetNumericDef(DATA_UCHAR, unsigned char)
DataElementSetNumericDef(DATA_INT, int)
DataElementSetNumericDef(DATA_UINT, unsigned int)
DataElementSetNumericDef(DATA_LONG, long)
DataElementSetNumericDef(DATA_ULONG, unsigned long)
DataElementSetNumericDef(DATA_LONGLONG, long long)
DataElementSetNumericDef(DATA_FLOAT, float)
DataElementSetNumericDef(DATA_DOUBLE, double)
DataElementSetNumericDef(DATA_LONGDOUBLE, long double)

void DataElement::set(const char *data_in, long size_in) {
    data_type = DATA_VOID;
    if (!data_size) { return; }
    data_init(size_in);
    memcpy(data_val, data_in, data_size);
}

void DataElement::set(const char *data_in) {
    data_type = DATA_STRING;
    data_init(strlen(data_in) + 1);
    memcpy(data_val, data_in, data_size);
}

void DataElement::set(const string &str_in) {
    data_type = DATA_STRING;
    data_init(str_in.length() + 1);
    memcpy(data_val, str_in.c_str(), data_size);
}

void DataElement::set(vector<string> &strvect_in) {
    vector<string>::iterator i;
    long vectsize;
    long ptr;

    data_type = DATA_STR_VECTOR;

    vectsize = 0;

    for (i = strvect_in.begin(); i != strvect_in.end(); i++) {
        vectsize += (*i).length() + 1;
    }

    data_init(vectsize);

    ptr = 0;

    for (i = strvect_in.begin(); i != strvect_in.end(); i++) {
        int str_length;

        str_length = (*i).length() + 1;

        memcpy(data_val + ptr, (*i).c_str(), str_length);
        ptr += str_length;
    }
}

void DataElement::set(std::set<string> &strset_in) {
    std::set<string>::iterator i;
    vector<string> tmp_vect;

    for (i = strset_in.begin(); i != strset_in.end(); i++) {
        tmp_vect.push_back(*i);
    }

    set(tmp_vect);
}

#define DataElementSetNumericVectorDef(enumtype, datatype) void DataElement::set(vector<datatype>& val_in) { \
        data_type = enumtype; \
        unit_size = sizeof(datatype); \
        data_init(unit_size * val_in.size()); \
        memcpy(data_val, &val_in[0], data_size); \
}

DataElementSetNumericVectorDef(DATA_CHAR_VECTOR, char)
DataElementSetNumericVectorDef(DATA_UCHAR_VECTOR, unsigned char)
DataElementSetNumericVectorDef(DATA_INT_VECTOR, int)
DataElementSetNumericVectorDef(DATA_UINT_VECTOR, unsigned int)
DataElementSetNumericVectorDef(DATA_LONG_VECTOR, long)
DataElementSetNumericVectorDef(DATA_ULONG_VECTOR, unsigned long)
DataElementSetNumericVectorDef(DATA_LONGLONG_VECTOR, long long)
DataElementSetNumericVectorDef(DATA_FLOAT_VECTOR, float)
DataElementSetNumericVectorDef(DATA_DOUBLE_VECTOR, double)
DataElementSetNumericVectorDef(DATA_LONGDOUBLE_VECTOR, long double)


#define DataElementGetNumericDef(enumtype, datatype, ...) void DataElement::get(datatype& val_out) throw (DataTypeMismatchException) { \
if (!data_type) \
return; \
    int _compat[] = {__VA_ARGS__}; \
    if (data_type != enumtype) { \
        bool compat = false; \
        for (int i = 0; i < sizeof(_compat)/sizeof(int); i++) { \
              if (_compat[i] == data_type) { \
                  compat = true; \
                  break; \
              } \
        } \
        if (!compat) { \
            throw(new DataTypeMismatchException("Type mismatch, element type " #enumtype " is not compatible with a " #datatype)); \
        } \
        if (sizeof(datatype) < data_size) { \
            std::cout << "Warning, data type mismatch requested size for '" << #datatype << "(" << sizeof(datatype) << ")' < data size '" << data_size << "'; possible loss of data."; \
        } \
        memset(&val_out, 0, sizeof(datatype)); \
        if (sizeof(datatype) > 4 && data_size <= 4) { \
            int v = 0; memcpy(&v,data_val,data_size); \
            val_out = (datatype)v; \
            return; \
        } else { \
            memcpy(&val_out, data_val, (sizeof(datatype) < data_size) ? sizeof(datatype) : data_size); \
        } \
        return; \
    } \
    memcpy(&val_out, data_val, data_size); \
}

DataElementGetNumericDef(DATA_CHAR, char, DATA_UCHAR, DATA_UINT, DATA_ULONG, DATA_LONGLONG, DATA_LONGDOUBLE, DATA_INT, DATA_LONG)
DataElementGetNumericDef(DATA_UCHAR, unsigned char, DATA_CHAR, DATA_UINT, DATA_ULONG, DATA_LONGLONG, DATA_LONGDOUBLE, DATA_INT, DATA_LONG)
DataElementGetNumericDef(DATA_UINT, unsigned int, DATA_CHAR, DATA_UCHAR, DATA_ULONG, DATA_LONGLONG, DATA_LONGDOUBLE, DATA_INT, DATA_LONG)
DataElementGetNumericDef(DATA_ULONG, unsigned long, DATA_CHAR, DATA_UCHAR, DATA_UINT, DATA_LONGLONG, DATA_LONGDOUBLE, DATA_INT, DATA_LONG)
DataElementGetNumericDef(DATA_LONGLONG, long long, DATA_CHAR, DATA_UCHAR, DATA_UINT, DATA_ULONG, DATA_LONGDOUBLE, DATA_INT, DATA_LONG)
DataElementGetNumericDef(DATA_LONGDOUBLE, long double, DATA_CHAR, DATA_UCHAR, DATA_UINT, DATA_ULONG, DATA_LONGLONG, DATA_INT, DATA_LONG)
DataElementGetNumericDef(DATA_INT, int, DATA_CHAR, DATA_UCHAR, DATA_UINT, DATA_ULONG, DATA_LONGLONG, DATA_LONGDOUBLE, DATA_LONG)
DataElementGetNumericDef(DATA_LONG, long, DATA_CHAR, DATA_UCHAR, DATA_UINT, DATA_ULONG, DATA_LONGLONG, DATA_LONGDOUBLE, DATA_INT)
DataElementGetNumericDef(DATA_FLOAT, float, DATA_DOUBLE, DATA_CHAR, DATA_UCHAR, DATA_UINT, DATA_ULONG, DATA_LONGLONG, DATA_LONGDOUBLE, DATA_INT,
        DATA_LONG)
DataElementGetNumericDef(DATA_DOUBLE, double, DATA_FLOAT, DATA_CHAR, DATA_UCHAR, DATA_UINT, DATA_ULONG, DATA_LONGLONG, DATA_LONGDOUBLE, DATA_INT,
        DATA_LONG)

void DataElement::get(char **data_in) throw (DataTypeMismatchException) {
    if (data_type != DATA_VOID)
        throw(new DataTypeMismatchException("Type mismatch, not a CHAR*"));
    *data_in = new char[data_size];
    memcpy(*data_in, data_val, data_size);
}

void DataElement::get(string &str_in) throw (DataTypeMismatchException) {
    if (!data_type)
        return;

    if (data_type != DATA_STRING)
        throw(new DataTypeMismatchException("Type mismatch, not a STRING"));

    if (!str_in.empty())	// flush the string
    {
        str_in.erase(str_in.begin(), str_in.end());
    }

    if (data_val) {
        str_in.append(data_val);
    }
}

void DataElement::get(vector<string> &strvect_in) throw (DataTypeMismatchException) {
    long ptr;
    if (!data_type)
        return;

    if (data_type != DATA_STR_VECTOR)
        throw(new DataTypeMismatchException("Type mismatch, not a STRING VECTOR"));

    ptr = 0;

    while (ptr != data_size) {
        strvect_in.push_back(string(data_val + ptr));
        ptr += strlen(data_val + ptr) + 1;
    }

}

void DataElement::get(std::set<string> &strset_in) throw (DataTypeMismatchException) {
    if (!data_type)
        return;

    if (data_type != DATA_STR_VECTOR)
        throw(new DataTypeMismatchException("Type mismatch, not a STRING VECTOR/SET"));

    std::vector<string> tmp_vect;
    std::vector<string>::iterator i;

    get(tmp_vect);

    for (i = tmp_vect.begin(); i != tmp_vect.end(); i++) {
        strset_in.insert(*i);
    }
}

#define DataElementGetNumericVectorDef(enumtype, datatype, ...) void DataElement::get(vector<datatype>& val_out) throw (DataTypeMismatchException) { \
if (!data_type || !unit_size) return; \
if (data_type != enumtype) { \
       int _compat[] = {__VA_ARGS__}; \
       bool compat = false; \
       for (int i = 0; i < sizeof(_compat)/sizeof(int); i++) { \
             if (_compat[i] == data_type) { \
                 compat = true; \
                 break; \
             } \
       } \
       if (!compat) { \
           throw(new DataTypeMismatchException("Type mismatch, element type is not compatible with a " #datatype)); \
       } \
       if (sizeof(datatype) < unit_size) { \
           std::cout << "Warning, data type mismatch for vector<" #datatype ">; " #datatype " size " << sizeof(datatype) << " is less than unit size " << unit_size << "; possible loss of data."; \
       } \
       datatype temp_val; \
       long ptr = 0; \
           while (ptr < data_size) { \
               temp_val = 0; \
               memcpy(&temp_val, data_val + ptr, (unit_size > sizeof(datatype))?sizeof(datatype):unit_size); \
               val_out.push_back(temp_val); \
               ptr += unit_size; \
           } \
           return; \
    } \
    val_out.assign(data_val, data_val + (data_size / sizeof(datatype))); \
}

DataElementGetNumericVectorDef(DATA_CHAR_VECTOR, char, DATA_UCHAR_VECTOR, DATA_INT_VECTOR, DATA_UINT_VECTOR, DATA_LONG_VECTOR, DATA_ULONG_VECTOR,
        DATA_LONGLONG_VECTOR);
DataElementGetNumericVectorDef(DATA_UCHAR_VECTOR, unsigned char, DATA_CHAR_VECTOR, DATA_INT_VECTOR, DATA_UINT_VECTOR, DATA_LONG_VECTOR,
        DATA_ULONG_VECTOR, DATA_LONGLONG_VECTOR);
DataElementGetNumericVectorDef(DATA_INT_VECTOR, int, DATA_CHAR_VECTOR, DATA_UCHAR_VECTOR, DATA_UINT_VECTOR, DATA_LONG_VECTOR, DATA_ULONG_VECTOR,
        DATA_LONGLONG_VECTOR);
DataElementGetNumericVectorDef(DATA_UINT_VECTOR, unsigned int, DATA_CHAR_VECTOR, DATA_UCHAR_VECTOR, DATA_INT_VECTOR, DATA_LONG_VECTOR,
        DATA_ULONG_VECTOR, DATA_LONGLONG_VECTOR);
DataElementGetNumericVectorDef(DATA_LONG_VECTOR, long, DATA_CHAR_VECTOR, DATA_UCHAR_VECTOR, DATA_INT_VECTOR, DATA_UINT_VECTOR, DATA_ULONG_VECTOR,
        DATA_LONGLONG_VECTOR);
DataElementGetNumericVectorDef(DATA_ULONG_VECTOR, unsigned long, DATA_CHAR_VECTOR, DATA_UCHAR_VECTOR, DATA_INT_VECTOR, DATA_UINT_VECTOR,
        DATA_LONG_VECTOR, DATA_LONGLONG_VECTOR);
DataElementGetNumericVectorDef(DATA_LONGLONG_VECTOR, long long, DATA_CHAR_VECTOR, DATA_UCHAR_VECTOR, DATA_INT_VECTOR, DATA_UINT_VECTOR,
        DATA_LONG_VECTOR, DATA_ULONG_VECTOR);
DataElementGetNumericVectorDef(DATA_FLOAT_VECTOR, float, DATA_DOUBLE_VECTOR, DATA_LONGDOUBLE_VECTOR);
DataElementGetNumericVectorDef(DATA_DOUBLE_VECTOR, double, DATA_FLOAT_VECTOR, DATA_LONGDOUBLE_VECTOR);
DataElementGetNumericVectorDef(DATA_LONGDOUBLE_VECTOR, long double, DATA_DOUBLE_VECTOR, DATA_FLOAT_VECTOR);


long DataElement::getSerializedSize() {
    return sizeof(int) + sizeof(long) + data_size;
}

long DataElement::getSerialized(char **ser_str) {
    long ser_size = getSerializedSize();

    *ser_str = new char[ser_size];

    char *ser_pointer;

    ser_pointer = *ser_str;

    memcpy(ser_pointer, &data_type, sizeof(int));
    ser_pointer += sizeof(int);
    memcpy(ser_pointer, &data_size, sizeof(long));
    ser_pointer += sizeof(long);
    memcpy(ser_pointer, data_val, data_size);

    return ser_size;
}

void DataElement::setSerialized(char *ser_str) {
    char *ser_pointer = ser_str;

    memcpy(&data_type, ser_pointer, sizeof(unsigned char));
    ser_pointer += sizeof(unsigned char);
    memcpy(&data_size, ser_pointer, sizeof(unsigned int));
    ser_pointer += sizeof(unsigned int);

    data_init(data_size);
    memcpy(data_val, ser_pointer, data_size);
}

/* DataNode class */

DataNode::DataNode(): ptr(0), parentNode(NULL) {
    data_elem = new DataElement();
}

DataNode::DataNode(const char *name_in): ptr(0), parentNode(NULL) {
    node_name = name_in;
    data_elem = new DataElement();
}

DataNode::~DataNode() {
    while (children.size()) {
        DataNode *del = children.back();
        children.pop_back();
        delete del;
    }
    if (data_elem) {
        delete data_elem;
    }
}

void DataNode::setName(const char *name_in) {
    node_name = name_in;
}

DataElement *DataNode::element() {
    return data_elem;
}

DataNode *DataNode::newChild(const char *name_in) {
    children.push_back(new DataNode(name_in));
    childmap[name_in].push_back(children.back());

    children.back()->setParentNode(*this);

    return children.back();
}

DataNode *DataNode::child(const char *name_in, int index) throw (DataInvalidChildException) {
    DataNode *child_ret;

    child_ret = childmap[name_in][index];

    if (!child_ret) {
        stringstream error_str;
        error_str << "no child '" << index << "' in DataNode '" << node_name << "'";
        throw(DataInvalidChildException(error_str.str().c_str()));
    }

    return child_ret;
}

DataNode *DataNode::child(int index) throw (DataInvalidChildException) {

    DataNode *child_ret;

    child_ret = children[index];

    if (!child_ret) {
        stringstream error_str;
        error_str << "no child '" << index << "' in DataNode '" << node_name << "'";
        throw(DataInvalidChildException(error_str.str().c_str()));
    }

    return child_ret;
}

int DataNode::numChildren() {
    return children.size();
}

int DataNode::numChildren(const char *name_in) {
    return childmap[name_in].size();
}

bool DataNode::hasAnother() {
    return children.size() != ptr;
}

bool DataNode::hasAnother(const char *name_in) {
    return childmap[name_in].size() != childmap_ptr[name_in];
}

DataNode *DataNode::getNext() throw (DataInvalidChildException) {
    return child(ptr++);
}

DataNode *DataNode::getNext(const char *name_in) throw (DataInvalidChildException) {
    return child(name_in, childmap_ptr[name_in]++);
}

void DataNode::rewind() {
    ptr = 0;
}

void DataNode::rewind(const char *name_in) {
    childmap_ptr[name_in] = 0;
}

/* DataTree class */

DataTree::DataTree(const char *name_in) {
    dn_root.setName(name_in);
}

DataTree::DataTree() {

}

DataTree::~DataTree() {
}
;

DataNode *DataTree::rootNode() {
    return &dn_root;
}

std::string trim(std::string& s, const std::string& drop = " ") {
    std::string r = s.erase(s.find_last_not_of(drop) + 1);
    return r.erase(0, r.find_first_not_of(drop));
}

void DataTree::decodeXMLText(DataNode *elem, const char *src_text, DT_FloatingPointPolicy fpp) {

    int tmp_char;
    int tmp_int;
    long tmp_long;
    long long tmp_llong;
    double tmp_double;
    float tmp_float;
    string tmp_str;
    string tmp_str2;
    std::stringstream tmp_stream;
    std::stringstream tmp_stream2;

    vector<char> tmp_charvect;
    vector<int> tmp_intvect;
    vector<long> tmp_longvect;
    vector<long> tmp_llongvect;
    vector<long>::iterator tmp_llongvect_i;
    vector<double> tmp_doublevect;
    vector<double>::iterator tmp_doublevect_i;
    vector<float> tmp_floatvect;

    bool vChars = false;
    bool vInts = false;
    bool vLongs = false;

    string in_text = src_text;

    trim(in_text);
    trim(in_text, "\r\n");
    tmp_stream.str("");
    tmp_stream2.str("");

    if (in_text.find_first_not_of("0123456789-") == string::npos) {
        tmp_stream << in_text;
        tmp_stream >> tmp_llong;

        tmp_int = (int)tmp_llong;
        tmp_long = (long)tmp_llong;

        if (tmp_int == tmp_llong) {
            elem->element()->set(tmp_int);
        } else if (tmp_long == tmp_llong) {
            elem->element()->set(tmp_long);
        } else {
            elem->element()->set(tmp_llong);
        }
    } else if (in_text.find_first_not_of("0123456789.e+-") == string::npos) {
        tmp_stream << in_text;

        if (fpp == USE_FLOAT) {
            tmp_stream >> tmp_float;

            elem->element()->set((float) tmp_float);
        } else {
            tmp_stream >> tmp_double;

            elem->element()->set((double) tmp_double);
        }
    } else if (in_text.find_first_not_of("0123456789- ") == string::npos) {
        tmp_stream << in_text;

        vChars = true;
        vInts = true;
        vLongs = true;

        while (!tmp_stream.eof()) {
            tmp_stream >> tmp_llong;
            tmp_char = tmp_llong;
            tmp_int = tmp_llong;
            tmp_long = tmp_llong;
            if (tmp_char != tmp_llong) {
                vChars = false;
            }
            if (tmp_int != tmp_llong) {
                vInts = false;
            }
            if (tmp_long != tmp_llong) {
                vLongs = false;
            }
            tmp_llongvect.push_back((long) tmp_long);
        }

        if (vChars) {
            for (tmp_llongvect_i = tmp_llongvect.begin(); tmp_llongvect_i != tmp_llongvect.end(); tmp_llongvect_i++) {
                tmp_charvect.push_back(*tmp_llongvect_i);
            }
            tmp_llongvect.clear();
            elem->element()->set(tmp_charvect);
            tmp_charvect.clear();

        } else if (vInts) {
            for (tmp_llongvect_i = tmp_llongvect.begin(); tmp_llongvect_i != tmp_llongvect.end(); tmp_llongvect_i++) {
                tmp_intvect.push_back(*tmp_llongvect_i);
            }
            tmp_llongvect.clear();
            elem->element()->set(tmp_intvect);
            tmp_intvect.clear();
        } else if (vLongs) {
            for (tmp_llongvect_i = tmp_llongvect.begin(); tmp_llongvect_i != tmp_llongvect.end(); tmp_llongvect_i++) {
                tmp_longvect.push_back(*tmp_llongvect_i);
            }
            tmp_llongvect.clear();
            elem->element()->set(tmp_longvect);
            tmp_longvect.clear();
        } else {
            elem->element()->set(tmp_llongvect);
        }
    } else if (in_text.find_first_not_of("0123456789.e-+ ") == string::npos) {
        tmp_stream << in_text;

        if (fpp == USE_FLOAT) {
            tmp_floatvect.clear();
        } else {
            tmp_doublevect.clear();
        }

        while (!tmp_stream.eof()) {

            if (fpp == USE_FLOAT) {
                tmp_stream >> tmp_float;
                tmp_floatvect.push_back(tmp_float);
            } else {
                tmp_stream >> tmp_double;
                tmp_doublevect.push_back(tmp_double);
            }
        }

        if (fpp == USE_FLOAT) {
            elem->element()->set(tmp_floatvect);
        } else {
            elem->element()->set(tmp_doublevect);
        }
    } else {
        elem->element()->set(src_text);
        //					printf( "Unhandled DataTree XML Field: [%s]", tmp_str.c_str() );
    }

}

void DataTree::setFromXML(DataNode *elem, TiXmlNode *elxml, bool root_node, DT_FloatingPointPolicy fpp) {
    TiXmlText *pText;
    int t = elxml->Type();
    string tmp_str;

    switch (t) {
    case TiXmlNode::DOCUMENT:
        //				printf( "Document" );
        break;

    case TiXmlNode::ELEMENT:
        if (!root_node)
            elem = elem->newChild(elxml->Value());

        const TiXmlAttribute *attribs;
        attribs = elxml->ToElement()->FirstAttribute();

        while (attribs) {

// following badgerfish xml->json and xml->ruby convention for attributes..
            string attrName("@");
            attrName.append(attribs->Name());

            decodeXMLText(elem->newChild(attrName.c_str()), attribs->Value(), fpp);

            attribs = attribs->Next();
        }

        //				printf( "Element \"%s\"", elxml->Value());
        break;

    case TiXmlNode::COMMENT:
//				printf( "Comment: \"%s\"", elxml->Value());
        break;

    case TiXmlNode::UNKNOWN:
//				printf( "Unknown" );
        break;

    case TiXmlNode::TEXT:
        pText = elxml->ToText();

        decodeXMLText(elem, pText->Value(), fpp);

//				pText = elxml->ToText();
//				printf( "Text: [%s]", pText->Value() );
        break;

    case TiXmlNode::DECLARATION:
//				printf( "Declaration" );
        break;
    default:
        break;
    }

//	printf( "\n" );

    TiXmlNode * pChild;

    if (!elxml->NoChildren()) {
        if (elxml->FirstChild()->Type() == TiXmlNode::ELEMENT) {
            if (elxml->FirstChild()->Value() == TIXML_STRING("str")) {
                std::vector<std::string> tmp_strvect;

                for (pChild = elxml->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) {
                    if (pChild->Value() == TIXML_STRING("str")) {
                        if (!pChild->FirstChild()) {
                            tmp_strvect.push_back("");
                            continue;
                        }

                        pText = pChild->FirstChild()->ToText();

                        if (pText) {
                            tmp_str = pText->Value();
                            tmp_strvect.push_back(tmp_str);
                        }
                    }
                }

                elem->element()->set(tmp_strvect);

                return;
            }
        }
    }

    for (pChild = elxml->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) {
        setFromXML(elem, pChild, false, fpp);
    }

}

void DataTree::nodeToXML(DataNode *elem, TiXmlElement *elxml) {
    DataNode *child;

    elem->rewind();

    while (elem->hasAnother()) {
        child = elem->getNext();

        std::string nodeName = child->getName();

        TiXmlElement *element;

        element = new TiXmlElement(nodeName.length() ? nodeName.c_str() : "node");
        std::string tmp;
        std::stringstream tmp_stream;
        TiXmlText *text;
        std::vector<float> tmp_floatvect;
        std::vector<float>::iterator tmp_floatvect_i;
        std::vector<double> tmp_doublevect;
        std::vector<double>::iterator tmp_doublevect_i;
        std::vector<int> tmp_intvect;
        std::vector<int>::iterator tmp_intvect_i;
        std::vector<char> tmp_charvect;
        std::vector<char>::iterator tmp_charvect_i;
        std::vector<unsigned char> tmp_ucharvect;
        std::vector<unsigned char>::iterator tmp_ucharvect_i;
        std::vector<unsigned int> tmp_uintvect;
        std::vector<unsigned int>::iterator tmp_uintvect_i;
        std::vector<long> tmp_longvect;
        std::vector<long>::iterator tmp_longvect_i;
        std::vector<unsigned long> tmp_ulongvect;
        std::vector<unsigned long>::iterator tmp_ulongvect_i;
        std::vector<long long> tmp_llongvect;
        std::vector<long long>::iterator tmp_llongvect_i;
        std::vector<unsigned long long> tmp_ullongvect;
        std::vector<unsigned long long>::iterator tmp_ullongvect_i;
        std::vector<string> tmp_stringvect;
        std::vector<string>::iterator tmp_stringvect_i;
        TiXmlElement *tmp_node;
        char *tmp_pstr;
        double tmp_double;
        long double tmp_ldouble;
        float tmp_float;
        char tmp_char;
        unsigned char tmp_uchar;
        int tmp_int;
        unsigned int tmp_uint;
        long tmp_long;
        unsigned long tmp_ulong;
        long long tmp_llong;

        switch (child->element()->getDataType()) {
        case DATA_NULL:
            break;
        case DATA_VOID:
            child->element()->get(&tmp_pstr);
// following badgerfish xml->json and xml->ruby convention for attributes..
            if (nodeName.substr(0, 1) == string("@")) {
                elxml->SetAttribute(nodeName.substr(1).c_str(), tmp_pstr);
                delete element;
                element = NULL;
            } else {
                text = new TiXmlText(tmp_pstr);
                element->LinkEndChild(text);
            }
            delete tmp_pstr;
            break;
        case DATA_CHAR:
            child->element()->get(tmp_char);

            tmp_stream.str("");

            tmp_stream << tmp_char;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DATA_UCHAR:
            child->element()->get(tmp_uchar);

            tmp_stream.str("");

            tmp_stream << tmp_uchar;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DATA_INT:
            child->element()->get(tmp_int);

            tmp_stream.str("");

            tmp_stream << tmp_int;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DATA_UINT:
            child->element()->get(tmp_uint);

            tmp_stream.str("");

            tmp_stream << tmp_uint;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DATA_LONG:
            child->element()->get(tmp_long);

            tmp_stream.str("");

            tmp_stream << tmp_long;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DATA_ULONG:
            child->element()->get(tmp_ulong);

            tmp_stream.str("");

            tmp_stream << tmp_ulong;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DATA_LONGLONG:
            child->element()->get(tmp_llong);

            tmp_stream.str("");

            tmp_stream << tmp_llong;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DATA_FLOAT:
            child->element()->get(tmp_float);

            tmp_stream.str("");

            tmp_stream << tmp_float;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DATA_DOUBLE:
            child->element()->get(tmp_double);

            tmp_stream.str("");

            tmp_stream << tmp_double;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DATA_LONGDOUBLE:
            child->element()->get(tmp_ldouble);

            tmp_stream.str("");

            tmp_stream << tmp_ldouble;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DATA_STRING:
            child->element()->get(tmp);
            if (nodeName.substr(0, 1) == string("@")) {
                elxml->SetAttribute(nodeName.substr(1).c_str(), tmp.c_str());
                delete element;
                element = NULL;
            } else {
                text = new TiXmlText(tmp.c_str());
                element->LinkEndChild(text);
            }
            break;

        case DATA_STR_VECTOR:
            child->element()->get(tmp_stringvect);

            tmp_stream.str("");

            for (tmp_stringvect_i = tmp_stringvect.begin(); tmp_stringvect_i != tmp_stringvect.end(); tmp_stringvect_i++) {
                tmp_node = new TiXmlElement("str");
                text = new TiXmlText((*tmp_stringvect_i).c_str());
                tmp_node->LinkEndChild(text);
                element->LinkEndChild(tmp_node);
            }

            tmp_stringvect.clear();
            break;
        case DATA_CHAR_VECTOR:
            child->element()->get(tmp_charvect);

            tmp_stream.str("");

            for (tmp_charvect_i = tmp_charvect.begin(); tmp_charvect_i != tmp_charvect.end(); tmp_charvect_i++) {
                tmp_stream << (*tmp_charvect_i);
                if (tmp_charvect_i != tmp_charvect.end() - 1)
                    tmp_stream << " ";
            }

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            tmp_charvect.clear();
            break;
        case DATA_UCHAR_VECTOR:
            child->element()->get(tmp_ucharvect);

            tmp_stream.str("");

            for (tmp_ucharvect_i = tmp_ucharvect.begin(); tmp_ucharvect_i != tmp_ucharvect.end(); tmp_ucharvect_i++) {
                tmp_stream << (*tmp_ucharvect_i);
                if (tmp_ucharvect_i != tmp_ucharvect.end() - 1)
                    tmp_stream << " ";
            }

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            tmp_ucharvect.clear();
            break;
        case DATA_INT_VECTOR:
            child->element()->get(tmp_intvect);

            tmp_stream.str("");

            for (tmp_intvect_i = tmp_intvect.begin(); tmp_intvect_i != tmp_intvect.end(); tmp_intvect_i++) {
                tmp_stream << (*tmp_intvect_i);
                if (tmp_intvect_i != tmp_intvect.end() - 1)
                    tmp_stream << " ";
            }

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            tmp_intvect.clear();
            break;
        case DATA_UINT_VECTOR:
            child->element()->get(tmp_uintvect);

            tmp_stream.str("");

            for (tmp_uintvect_i = tmp_uintvect.begin(); tmp_uintvect_i != tmp_uintvect.end(); tmp_uintvect_i++) {
                tmp_stream << (*tmp_intvect_i);
                if (tmp_uintvect_i != tmp_uintvect.end() - 1)
                    tmp_stream << " ";
            }

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            tmp_uintvect.clear();
            break;
        case DATA_LONG_VECTOR:
            child->element()->get(tmp_longvect);

            tmp_stream.str("");

            for (tmp_longvect_i = tmp_longvect.begin(); tmp_longvect_i != tmp_longvect.end(); tmp_longvect_i++) {
                tmp_stream << (*tmp_longvect_i);
                if (tmp_longvect_i != tmp_longvect.end() - 1)
                    tmp_stream << " ";
            }

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            tmp_longvect.clear();
            break;
        case DATA_ULONG_VECTOR:
            child->element()->get(tmp_ulongvect);

            tmp_stream.str("");

            for (tmp_ulongvect_i = tmp_ulongvect.begin(); tmp_ulongvect_i != tmp_ulongvect.end(); tmp_ulongvect_i++) {
                tmp_stream << (*tmp_ulongvect_i);
                if (tmp_ulongvect_i != tmp_ulongvect.end() - 1)
                    tmp_stream << " ";
            }

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            tmp_ulongvect.clear();
            break;
        case DATA_LONGLONG_VECTOR:
            child->element()->get(tmp_llongvect);

            tmp_stream.str("");

            for (tmp_llongvect_i = tmp_llongvect.begin(); tmp_llongvect_i != tmp_llongvect.end(); tmp_llongvect_i++) {
                tmp_stream << (*tmp_llongvect_i);
                if (tmp_llongvect_i != tmp_llongvect.end() - 1)
                    tmp_stream << " ";
            }

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            tmp_llongvect.clear();
            break;
        case DATA_FLOAT_VECTOR:
            child->element()->get(tmp_floatvect);

            tmp_stream.str("");

            for (tmp_floatvect_i = tmp_floatvect.begin(); tmp_floatvect_i != tmp_floatvect.end(); tmp_floatvect_i++) {
                tmp_stream << (*tmp_floatvect_i);
                if (tmp_floatvect_i != tmp_floatvect.end() - 1)
                    tmp_stream << " ";
            }

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            tmp_floatvect.clear();
            break;
        case DATA_DOUBLE_VECTOR:
            child->element()->get(tmp_doublevect);

            tmp_stream.str("");

            for (tmp_doublevect_i = tmp_doublevect.begin(); tmp_doublevect_i != tmp_doublevect.end(); tmp_doublevect_i++) {
                tmp_stream << (*tmp_doublevect_i);
                if (tmp_doublevect_i != tmp_doublevect.end() - 1)
                    tmp_stream << " ";
            }

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            tmp_doublevect.clear();
            break;
        }

        if (element) {
            elxml->LinkEndChild(element);

            if (child->numChildren()) {
                nodeToXML(child, element);
            }
        }
    }

    elem->rewind();
}

void DataTree::printXML() /* get serialized size + return node names header */
{
    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration("1.0", "", "");
    doc.LinkEndChild(decl);

    DataNode *root = rootNode();

    string rootName = root->getName();

    TiXmlElement *element = new TiXmlElement(rootName.empty() ? "root" : rootName.c_str());
    doc.LinkEndChild(element);

    if (!root->numChildren())
        doc.Print();

    nodeToXML(root, element);

    root->rewind();

    doc.Print();
}

long DataTree::getSerializedSize(DataElement &de_node_names, bool debug) /* get serialized size + return node names header */
{
    long total_size = 0;

    stack<DataNode *> dn_stack;
    vector<string> node_names;
    map<string, int, string_less> node_name_index_map;

    DataElement de_name_index;	// just used for sizing purposes
    DataElement de_num_children;

    de_name_index.set((int) 0);
    de_num_children.set((int) 0);

    int de_name_index_size = de_name_index.getSerializedSize();
    int de_num_children_size = de_num_children.getSerializedSize();

    dn_stack.push(&dn_root);

    while (!dn_stack.empty()) {
        int name_index;
        //					int num_children;

        /* build the name list */
        if (dn_stack.top()->getName().empty()) {
            name_index = 0; /* empty string */
        } else if (node_name_index_map[dn_stack.top()->getName().c_str()] == 0) {
            node_names.push_back(string(dn_stack.top()->getName()));
            name_index = node_names.size();
            node_name_index_map[dn_stack.top()->getName().c_str()] = name_index;
        } else {
            name_index = node_name_index_map[dn_stack.top()->getName().c_str()];
        }

        /* add on the size of the name index and number of children */
        total_size += de_name_index_size;
        total_size += de_num_children_size;
        total_size += dn_stack.top()->element()->getSerializedSize();

        /* debug output */
        if (debug) {
            for (unsigned int i = 0; i < dn_stack.size() - 1; i++)
                cout << "--";
            cout << (dn_stack.top()->getName().empty() ? "NULL" : dn_stack.top()->getName()) << "(" << dn_stack.top()->element()->getSerializedSize()
                    << ")";
            cout << " type: " << dn_stack.top()->element()->getDataType() << endl;
//cout << " index: " << name_index << endl;
        }
        /* end debug output */

        /* if it has children, traverse into them */
        if (dn_stack.top()->hasAnother()) {
            dn_stack.push(dn_stack.top()->getNext());
            dn_stack.top()->rewind();
        } else {
            /* no more children, back out until we have children, then add next child to the top */
            while (!dn_stack.empty()) {
                if (!dn_stack.top()->hasAnother()) {
                    dn_stack.top()->rewind();
                    dn_stack.pop();
                } else
                    break;
            }

            if (!dn_stack.empty()) {
                dn_stack.push(dn_stack.top()->getNext());
                dn_stack.top()->rewind();
            }
        }
    }

    /* set the header for use in serialization */
    de_node_names.set(node_names);

    total_size += de_node_names.getSerializedSize();

    return total_size;
}

void DataNode::findAll(const char *name_in, vector<DataNode *> &node_list_out) {
    stack<DataNode *> dn_stack;

    /* start at the root */
    dn_stack.push(this);

    if (string(getName()) == string(name_in))
        node_list_out.push_back(this);

    while (!dn_stack.empty()) {
        while (dn_stack.top()->hasAnother(name_in)) {
            node_list_out.push_back(dn_stack.top()->getNext(name_in));
        }

        /* if it has children, traverse into them */
        if (dn_stack.top()->hasAnother()) {
            dn_stack.push(dn_stack.top()->getNext());
            dn_stack.top()->rewind();
        } else {
            /* no more children, back out until we have children, then add next child to the top */
            while (!dn_stack.empty()) {
                if (!dn_stack.top()->hasAnother()) {
                    dn_stack.top()->rewind();
                    dn_stack.pop();
                } else
                    break;
            }

            if (!dn_stack.empty()) {
                dn_stack.push(dn_stack.top()->getNext());
                dn_stack.top()->rewind();
            }
        }
    }

}

long DataTree::getSerialized(char **ser_str, bool debug) {
    long data_ptr = 0;
    long data_size = 0;

    stack<DataNode *> dn_stack;
    vector<string> node_names;
    map<string, int, string_less> node_name_index_map;

    /* header of node names, grabbed from getserializedsize to avoid having to memmove() or realloc() */
    DataElement de_node_names;

    data_size = getSerializedSize(de_node_names, debug);

    *ser_str = (char *) malloc(data_size);

    char *data_out = *ser_str;

    /* name list header */
    char *de_node_names_serialized;
    long de_node_names_serialized_size;

    de_node_names.getSerialized(&de_node_names_serialized);
    de_node_names_serialized_size = de_node_names.getSerializedSize();

    /* copy the header and increase the pointer */
    memcpy(data_out, de_node_names_serialized, de_node_names_serialized_size);
    data_ptr += de_node_names_serialized_size;

    /* start at the root */
    dn_stack.push(&dn_root);

    while (!dn_stack.empty()) {
        int name_index;
        int num_children;

        DataElement de_name_index;
        DataElement de_num_children;

        char *de_name_index_serialized;
        char *de_num_children_serialized;
        char *element_serialized;

        long de_name_index_serialized_size;
        long de_num_children_serialized_size;
        long element_serialized_size;

        /* build the name list */
        if (dn_stack.top()->getName().empty()) {
            name_index = 0; /* empty string */
        } else if (node_name_index_map[dn_stack.top()->getName().c_str()] == 0) {
            node_names.push_back(string(dn_stack.top()->getName()));
            name_index = node_names.size();
            node_name_index_map[dn_stack.top()->getName().c_str()] = name_index;
        } else {
            name_index = node_name_index_map[dn_stack.top()->getName().c_str()];
        }

        num_children = dn_stack.top()->numChildren();

        de_name_index.set(name_index);
        de_num_children.set(num_children);

        de_name_index_serialized_size = de_name_index.getSerializedSize();
        de_num_children_serialized_size = de_num_children.getSerializedSize();
        element_serialized_size = dn_stack.top()->element()->getSerializedSize();

        de_name_index.getSerialized(&de_name_index_serialized);
        de_num_children.getSerialized(&de_num_children_serialized);
        dn_stack.top()->element()->getSerialized(&element_serialized);

        /* add on the name index and number of children */
        memcpy(data_out + data_ptr, de_name_index_serialized, de_name_index_serialized_size);
        data_ptr += de_name_index_serialized_size;

        memcpy(data_out + data_ptr, de_num_children_serialized, de_num_children_serialized_size);
        data_ptr += de_num_children_serialized_size;

        /* add on the data element */
        memcpy(data_out + data_ptr, element_serialized, element_serialized_size);
        data_ptr += element_serialized_size;

        delete de_name_index_serialized;
        delete de_num_children_serialized;
        delete element_serialized;

        /* if it has children, traverse into them */
        if (dn_stack.top()->hasAnother()) {
            dn_stack.push(dn_stack.top()->getNext());
            dn_stack.top()->rewind();
        } else {
            /* no more children, back out until we have children, then add next child to the top */
            while (!dn_stack.empty()) {
                if (!dn_stack.top()->hasAnother()) {
                    dn_stack.top()->rewind();
                    dn_stack.pop();
                } else
                    break;
            }

            if (!dn_stack.empty()) {
                dn_stack.push(dn_stack.top()->getNext());
                dn_stack.top()->rewind();
            }
        }
    }

    return data_size;
}

void DataTree::setSerialized(char *ser_str, bool debug) {
    long data_ptr = 0;
//	long data_size = 0;

    stack<DataNode *> dn_stack;
    stack<int> dn_childcount_stack;
    vector<string> node_names;

    DataElement de_node_names;

    de_node_names.setSerialized(ser_str);
    data_ptr += de_node_names.getSerializedSize();
    de_node_names.get(node_names);

    DataElement de_name_index;
    DataElement de_num_children;
    DataElement de_element;

    dn_stack.push(&dn_root);
    dn_childcount_stack.push(0); /* root (parent null) has no siblings */

    /* unserialization is a little less straightforward since we have to do a countdown of remaining children */
    while (!dn_stack.empty()) {
        int name_index;
        int num_children;

        /* pull the index of the name of this node */
        de_name_index.setSerialized(ser_str + data_ptr);
        data_ptr += de_name_index.getSerializedSize();

        /* pull the number of children this node has */
        de_num_children.setSerialized(ser_str + data_ptr);
        data_ptr += de_num_children.getSerializedSize();

        /* get values from the temp dataelements */
        de_name_index.get(name_index);
        de_num_children.get(num_children);

        /* pull the node's element */
        dn_stack.top()->element()->setSerialized(ser_str + data_ptr);
        data_ptr += dn_stack.top()->element()->getSerializedSize();

        /* debug output */
        if (debug) {
            for (unsigned int i = 0; i < dn_stack.size() - 1; i++)
                cout << "--";
            cout << (name_index ? node_names[name_index - 1] : "NULL") << "(" << dn_stack.top()->element()->getSerializedSize() << ")";
            cout << " index: " << name_index << endl;
        }
        /* end debug output */

        /* name index >= 1 means it has a name */
        if (name_index) {
            dn_stack.top()->setName(node_names[name_index - 1].c_str());

        } else /* name is nil */
        {
            dn_stack.top()->setName("");
        }

        if (num_children) /* Has children, create first child and push it to the top */
        {
            dn_childcount_stack.push(num_children); /* push the child count onto the stack */

            de_name_index.setSerialized(ser_str + data_ptr); /* peek at the new child name but don't increment pointer */
            de_name_index.get(name_index);
            /* add this child onto the top of the stack */
            dn_stack.push(dn_stack.top()->newChild((name_index ? node_names[name_index - 1] : string("")).c_str()));
            dn_childcount_stack.top()--; /* decrement to count the new child */
            }
        else /* No children, move on to the next sibling */
        {
            if (dn_childcount_stack.top()) /* any siblings remaining? */
            {
                de_name_index.setSerialized(ser_str + data_ptr); /* peek at the new child name but don't increment pointer */
                de_name_index.get(name_index);

                dn_stack.pop();
                dn_stack.push(dn_stack.top()->newChild((name_index ? node_names[name_index - 1] : string("")).c_str())); /* create the next sibling and throw it on the stack */
                dn_childcount_stack.top()--; /* decrement to count the new sibling */
                }
            else /* This is the last sibling, move up the stack and find the next */
            {
                while (!dn_stack.empty()) /* move up the stack until we find the next sibling */
                {
                    if (dn_childcount_stack.top()) {
                        de_name_index.setSerialized(ser_str + data_ptr); /* peek at the new child name but don't increment pointer */
                        de_name_index.get(name_index);

                        dn_stack.pop();
                        dn_stack.push(dn_stack.top()->newChild((name_index ? node_names[name_index - 1] : string("")).c_str())); /* throw it on the stack */
                        dn_childcount_stack.top()--; /* count it */
                        break
;                    }
                    else
                    {
                        dn_childcount_stack.pop();
                        dn_stack.pop(); /* if no more siblings found the stack will empty naturally */
                    }
                }
            }
        }
    }
}

bool DataTree::LoadFromFileXML(const std::string& filename, DT_FloatingPointPolicy fpp) {
    TiXmlDocument doc(filename.c_str());
    bool loadOkay = doc.LoadFile();

    if (!loadOkay) {
        std::cout << "LoadFromFileXML[error loading]: " << filename << std::endl;
        return false;
    }

    TiXmlNode *xml_root_node = doc.RootElement();

    if (!xml_root_node) {
        std::cout << "LoadFromFileXML[error no root]: " << filename << std::endl;
        return false;
    }

    rootNode()->setName(xml_root_node->ToElement()->Value());

    setFromXML(rootNode(), xml_root_node, true, fpp);

    return true;
}

bool DataTree::SaveToFileXML(const std::string& filename) {
    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration("1.0", "", "");
    doc.LinkEndChild(decl);

    string rootName = rootNode()->getName();

    TiXmlElement *element = new TiXmlElement(rootName.empty() ? "root" : rootName.c_str());

    doc.LinkEndChild(element);

    nodeToXML(rootNode(), element);

    doc.SaveFile(filename.c_str());

    return true;
}

/*
 bool DataTree::SaveToFile(const std::string& filename)
 {
 char *serialized;

 long dataSize = getSerialized(&serialized);

 std::ofstream fout(filename.c_str(), ios::binary);

 fout.write(serialized, dataSize);

 fout << flush;
 fout.close();

 delete serialized;

 return true;
 }

 bool DataTree::LoadFromFile(const std::string& filename)
 {
 char *serialized;
 long dataSize;

 ifstream fin(filename.c_str(), ios::binary);

 fin.seekg (0, ios::end);
 dataSize = fin.tellg();
 fin.seekg (0, ios::beg);

 serialized = new char[dataSize];
 fin.read(serialized,dataSize);

 fin.close();

 setSerialized(serialized);

 delete serialized;

 return true;
 }
 */

bool DataTree::SaveToFile(const std::string& filename, bool compress, int compress_level) {
    long dataSize, compressedSize, headerSize;
    char *serialized, *hdr_serialized, *compressed;
    DataTree dtHeader;

    dataSize = getSerialized(&serialized);

#if USE_FASTLZ
    if (compress) {
        compressed = new char[(int) ceil(dataSize * 1.5)];

        compressedSize = fastlz_compress_level(compress_level, serialized, dataSize, compressed);

        compressed = (char *) realloc(compressed, compressedSize);

        delete serialized;
    }
#else
    if (compress) {
        std::cout << "Can't compress, FASTLZ disabled";
        compress = false;
    }
#endif
    DataNode *header = dtHeader.rootNode();

    *header->newChild("version") = 1.0f;
    *header->newChild("compression") = string(compress ? "FastLZ" : "none");
    *header->newChild("uncompressed_size") = dataSize;

    headerSize = dtHeader.getSerialized(&hdr_serialized);

    std::ofstream fout(filename.c_str(), ios::binary);

    fout.write((char *) &headerSize, sizeof(long));
    fout.write((char *) &(compress ? compressedSize : dataSize), sizeof(long));

    fout.write(hdr_serialized, headerSize);
    fout.write(compress ? compressed : serialized, compress ? compressedSize : dataSize);

    fout << flush;
    fout.close();

    delete hdr_serialized;

    if (!compress) {
        delete serialized;
    } else {
        delete compressed;
    }

    return true;
}

bool DataTree::LoadFromFile(const std::string& filename) {
    char *compressed, *serialized, *hdr_serialized;
    long dataSize, headerSize, compressedSize;

    ifstream fin(filename.c_str(), ios::binary);

    fin.read((char *) &headerSize, sizeof(long));
    fin.read((char *) &compressedSize, sizeof(long));

    hdr_serialized = new char[headerSize];
    fin.read(hdr_serialized, headerSize);

    DataTree dtHeader;
    dtHeader.setSerialized(hdr_serialized);
    DataNode *header = dtHeader.rootNode();

    string compressionType(*header->getNext("compression"));
    dataSize = *header->getNext("uncompressed_size");

    bool uncompress = false;
#if USE_FASTLZ
    if (compressionType == "FastLZ") {
        uncompress = true;
    }

    if (uncompress) {
        compressed = new char[compressedSize];
        fin.read(compressed, compressedSize);

        serialized = new char[dataSize];
        fastlz_decompress(compressed, compressedSize, serialized, dataSize);

        delete compressed;
    } else {
        serialized = new char[dataSize];
        fin.read(serialized, dataSize);
    }
#else
    if (compressionType == "FastLZ") {
        std::cout << "DataTree Unable to load FastLZ compressed file -- FastLZ is disabled";
        return false;
    }

    serialized = new char[dataSize];
    fin.read(serialized, dataSize);
#endif

    fin.close();

    setSerialized(serialized);

    delete serialized;
    delete hdr_serialized;

    return true;
}

