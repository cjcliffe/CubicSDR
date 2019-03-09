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
#include <iomanip>
#include <locale>
#include <stdlib.h>
#include <algorithm>


/* DataElement class */

using namespace std;

#define STRINGIFY(A)  #A

#define MAX_STR_SIZE  (1024)

DataElement::DataElement() : data_type(DATA_NULL) {
    //
}

DataElement::DataElement(DataElement &cloneFrom) : data_type(cloneFrom.getDataType()), data_val(cloneFrom.data_val), data_val_vector(cloneFrom.data_val_vector) {
    //
}

DataElement::~DataElement() {
    //nothing
}



char * DataElement::getDataPointer() {

    if (!data_val.empty()) {
        return (char*)&data_val[0];
    }

    return nullptr;
}

DataElement::DataElementTypeEnum DataElement::getDataType() {
    return data_type;
}

size_t DataElement::getDataSize() {
    return data_val.size();
}


void DataElement::set(const char *data_in, long size_in) {
    data_type = DATA_VOID;

    data_val.assign(data_in, data_in + size_in);
}

void DataElement::set(const char *data_in) {
    data_type = DATA_STRING;

    size_t clamped_size = ::strnlen(data_in, MAX_STR_SIZE);

    data_val.assign(data_in, data_in + clamped_size);
}


void DataElement::set(const std::set<string> &strset_in) {
   
    vector<string> tmp_vect;

    for (auto single_string : strset_in) {
        tmp_vect.push_back(single_string);
    }

    set(tmp_vect);
}


void DataElement::get(DataElement::DataElementBuffer& data_in) {

    if (data_type != DATA_VOID) {
        throw(DataTypeMismatchException("Type mismatch, not a VOID*"));
    }

    data_in = data_val;
}


void DataElement::get(std::set<string> &strset_out) {
  
    if (data_type != DATA_STR_VECTOR)
        throw(DataTypeMismatchException("Type mismatch, not a STRING VECTOR/SET"));

    std::vector<string> tmp_vect;
   
    get(tmp_vect);

    strset_out.clear();

    for (auto single_string : tmp_vect) {
        strset_out.insert(single_string);
    }
}

std::string DataElement::toString() {
    int dataType = getDataType();
    std::string strValue = "";
    
    try {
        if (dataType == DATA_STRING) {
            get(strValue);
        } else if (dataType == DATA_INT || dataType == DATA_LONG || dataType == DATA_LONGLONG) {
            long long intSettingValue;
            get(intSettingValue);
            strValue = std::to_string(intSettingValue);
        } else if (dataType == DATA_FLOAT || dataType == DATA_DOUBLE) {
            double floatSettingValue;
            get(floatSettingValue);
            strValue = std::to_string(floatSettingValue);
        } else if (dataType == DATA_NULL) {
            strValue = "";
        } else if (dataType == DATA_WSTRING) {
            std::wstring wstr;
            get(wstr);
            //TODO: code below returns a forced cast in (char*) beware...
            strValue = *wstr.c_str();
        }
        else {
            std::cout << "Unhandled DataElement toString for type: " << dataType  << std::endl;
        }
    } catch (DataTypeMismatchException e) {
        std::cout << "toString() DataTypeMismatch: " << dataType  << std::endl;
    }
    
    return strValue;
}

/* DataNode class */

DataNode::DataNode(): parentNode(NULL), ptr(0) {
    data_elem = new DataElement();
}

DataNode::DataNode(const char *name_in): parentNode(NULL), ptr(0) {
    node_name = name_in;
    data_elem = new DataElement();
}

DataNode::DataNode(const char *name_in, DataNode &cloneFrom): parentNode(NULL), ptr(0) {
    node_name = name_in;
    data_elem = new DataElement(*cloneFrom.element());
    
    // TODO: stack recursion optimization
    while (cloneFrom.hasAnother()) {
        DataNode *cNode = cloneFrom.getNext();
        newChildCloneFrom(cNode->getName().c_str(), cNode);
    }
}

DataNode::DataNode(const char *name_in, DataElement &cloneFrom): parentNode(NULL), ptr(0) {
    node_name = name_in;
    data_elem = new DataElement(cloneFrom);
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

DataNode *DataNode::newChild(const char *name_in, DataNode *otherNode) {
    children.push_back(otherNode);
    childmap[name_in].push_back(children.back());
    
    children.back()->setParentNode(*this);
    
    return children.back();
}

DataNode *DataNode::newChildCloneFrom(const char *name_in, DataNode *cloneFrom) {
    DataNode *cloneNode = new DataNode(name_in, *cloneFrom->element());
    
    children.push_back(cloneNode);
    childmap[name_in].push_back(children.back());
    children.back()->setParentNode(*this);
    
    // TODO: stack recursion optimization
    while (cloneFrom->hasAnother()) {
        DataNode *cNode = cloneFrom->getNext();
        cloneNode->newChildCloneFrom(cNode->getName().c_str(), cNode);
    }
    
    cloneFrom->rewind();
    
    return children.back();
}


DataNode *DataNode::child(const char *name_in, int index) {
    DataNode *child_ret;

    child_ret = childmap[name_in][index];

    if (!child_ret) {
        stringstream error_str;
        error_str << "no child '" << index << "' in DataNode '" << node_name << "'";
        throw(DataInvalidChildException(error_str.str().c_str()));
    }

    return child_ret;
}

DataNode *DataNode::child(int index) {

    DataNode *child_ret;

    child_ret = children[index];

    if (!child_ret) {
        stringstream error_str;
        error_str << "no child '" << index << "' in DataNode '" << node_name << "'";
        throw(DataInvalidChildException(error_str.str().c_str()));
    }

    return child_ret;
}

size_t DataNode::numChildren() {
    return children.size();
}

size_t DataNode::numChildren(const char *name_in) {
    return childmap[name_in].size();
}

bool DataNode::hasAnother() {
    return children.size() != ptr;
}

bool DataNode::hasAnother(const char *name_in) {
    return childmap[name_in].size() != childmap_ptr[name_in];
}

DataNode *DataNode::getNext() {
    return child(ptr++);
}

DataNode *DataNode::getNext(const char *name_in) {
    return child(name_in, childmap_ptr[name_in]++);
}

void DataNode::rewind() {
    ptr = 0;
    childmap_ptr.erase(childmap_ptr.begin(),childmap_ptr.end());
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

string DataTree::wsEncode(const wstring& wstr) {
    stringstream encStream;
    
    //wchar_t is typically 16 bits on windows, and 32 bits on Unix, so use sizeof(wchar_t) everywhere.
    size_t bufSizeBytes = (wstr.length()+1) * sizeof(wchar_t);

    char *data_str = (char *)calloc(bufSizeBytes, sizeof(char));
    
    wcstombs(data_str, wstr.c_str(), bufSizeBytes - sizeof(wchar_t));
    
    std::string byte_str(data_str);

    free(data_str);
    
    encStream << std::hex;

    for(auto i = byte_str.begin(); i != byte_str.end(); i++) {
        encStream << '%' << setfill('0') << (unsigned int)((unsigned char)(*i));
    }
    
    return encStream.str();
}

wstring DataTree::wsDecode(const string& str) {
    
    std::stringstream decStream;
    std::stringstream mbstr;
    unsigned int x;
    
    string decStr = str;
    std::replace( decStr.begin(), decStr.end(), '%', ' ');
    decStream << trim(decStr);
  
    string sResult;

    //this actually assume we will get as many char as wchar_t from the decodes string,
    //who cares ?
    size_t maxLen = decStr.length();

    //wchar_t is typically 16 bits on windows, and 32 bits on Unix, so use sizeof(wchar_t) everywhere.
    wchar_t *wc_str = (wchar_t *) ::calloc(maxLen  + 1, sizeof(wchar_t));

    while (!decStream.eof()) {
        decStream >> std::hex >> x;
        //extract actually 2 hex-chars by 2 hex-chars to form a char value.
        mbstr << (unsigned char) x;
    }

    ::mbstowcs(wc_str, mbstr.str().c_str(), maxLen);
    
    wstring result(wc_str);

    ::free(wc_str);
    
    return result;
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
    
    vector<double> tmp_doublevect;
   
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

            elem->element()->set(tmp_float);
        } else {
            tmp_stream >> tmp_double;

            elem->element()->set(tmp_double);
        }
    } else if (in_text.find_first_not_of("0123456789- ") == string::npos) {
        tmp_stream << in_text;

        vChars = true;
        vInts = true;
        vLongs = true;

        while (!tmp_stream.eof()) {
            tmp_stream >> tmp_llong;
            tmp_char = (char)tmp_llong;
            tmp_int = (int)tmp_llong;
            tmp_long = (long)tmp_llong;

            if ((long long)tmp_char != tmp_llong) {
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
            for (auto single_long : tmp_llongvect) {
                tmp_charvect.push_back((char)single_long);
            }
            tmp_llongvect.clear();
            elem->element()->set(tmp_charvect);
            tmp_charvect.clear();

        } else if (vInts) {
            for (auto single_long : tmp_llongvect) {
                tmp_intvect.push_back((int)single_long);
            }
            tmp_llongvect.clear();
            elem->element()->set(tmp_intvect);
            tmp_intvect.clear();
        } else if (vLongs) {
            for (auto single_long : tmp_llongvect) {
                tmp_longvect.push_back(single_long);
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
    } else if (in_text.find_first_not_of("0123456789abcdef%") == string::npos) {
        elem->element()->set(wsDecode(src_text));
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
    case TiXmlNode::TINYXML_DOCUMENT:
        //				printf( "Document" );
        break;

    case TiXmlNode::TINYXML_ELEMENT:
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

    case TiXmlNode::TINYXML_COMMENT:
//				printf( "Comment: \"%s\"", elxml->Value());
        break;

    case TiXmlNode::TINYXML_UNKNOWN:
//				printf( "Unknown" );
        break;

    case TiXmlNode::TINYXML_TEXT:
        pText = elxml->ToText();

        decodeXMLText(elem, pText->Value(), fpp);

//				pText = elxml->ToText();
//				printf( "Text: [%s]", pText->Value() );
        break;

    case TiXmlNode::TINYXML_DECLARATION:
//				printf( "Declaration" );
        break;
    default:
        break;
    }

//	printf( "\n" );

    TiXmlNode * pChild;

    if (!elxml->NoChildren()) {
        if (elxml->FirstChild()->Type() == TiXmlNode::TINYXML_ELEMENT) {
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
        std::wstring wtmp;
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
       
        std::vector<string> tmp_stringvect;
       
        TiXmlElement *tmp_node;
        std::string tmp_pstr_as_string;
        double tmp_double;
        
        float tmp_float;
        char tmp_char;
        unsigned char tmp_uchar;
        int tmp_int;
        unsigned int tmp_uint;
        long tmp_long;
        unsigned long tmp_ulong;
        long long tmp_llong;

        switch (child->element()->getDataType()) {
        case DataElement::DATA_NULL:
            break;
        case DataElement::DATA_VOID:
            child->element()->get(tmp_pstr_as_string); // returned VOID as string
// following badgerfish xml->json and xml->ruby convention for attributes..
            if (nodeName.substr(0, 1) == string("@")) {
                elxml->SetAttribute(nodeName.substr(1).c_str(), tmp_pstr_as_string.c_str()); //the c_str take care of adding a null erminated character...
                delete element;
                element = NULL;
            } else {
                text = new TiXmlText(tmp_pstr_as_string.c_str());
                element->LinkEndChild(text);
            }
            break;
        case DataElement::DATA_CHAR:
            child->element()->get(tmp_char);

            tmp_stream.str("");

            tmp_stream << tmp_char;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DataElement::DATA_UCHAR:
            child->element()->get(tmp_uchar);

            tmp_stream.str("");

            tmp_stream << tmp_uchar;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DataElement::DATA_INT:
            child->element()->get(tmp_int);

            tmp_stream.str("");

            tmp_stream << tmp_int;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DataElement::DATA_UINT:
            child->element()->get(tmp_uint);

            tmp_stream.str("");

            tmp_stream << tmp_uint;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DataElement::DATA_LONG:
            child->element()->get(tmp_long);

            tmp_stream.str("");

            tmp_stream << tmp_long;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DataElement::DATA_ULONG:
            child->element()->get(tmp_ulong);

            tmp_stream.str("");

            tmp_stream << tmp_ulong;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DataElement::DATA_LONGLONG:
            child->element()->get(tmp_llong);

            tmp_stream.str("");

            tmp_stream << tmp_llong;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DataElement::DATA_FLOAT:
            child->element()->get(tmp_float);

            tmp_stream.str("");

            tmp_stream << tmp_float;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DataElement::DATA_DOUBLE:
            child->element()->get(tmp_double);

            tmp_stream.str("");

            tmp_stream << tmp_double;

            text = new TiXmlText(tmp_stream.str().c_str());
            element->LinkEndChild(text);
            break;
        case DataElement::DATA_STRING:
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
        case DataElement::DATA_WSTRING:
            child->element()->get(wtmp);
            tmp = wsEncode(wtmp);
            if (nodeName.substr(0, 1) == string("@")) {
                elxml->SetAttribute(nodeName.substr(1).c_str(), tmp.c_str());
                delete element;
                element = NULL;
            } else {
                text = new TiXmlText(tmp.c_str());
                element->LinkEndChild(text);
            }
            break;
        case DataElement::DATA_STR_VECTOR:
            child->element()->get(tmp_stringvect);

            tmp_stream.str("");

            for (auto single_string : tmp_stringvect) {
                tmp_node = new TiXmlElement("str");
                text = new TiXmlText(single_string.c_str());
                tmp_node->LinkEndChild(text);
                element->LinkEndChild(tmp_node);
            }

            tmp_stringvect.clear();
            break;
        case DataElement::DATA_CHAR_VECTOR:
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
        case DataElement::DATA_UCHAR_VECTOR:
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
        case DataElement::DATA_INT_VECTOR:
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
        case DataElement::DATA_UINT_VECTOR:
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
        case DataElement::DATA_LONG_VECTOR:
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
        case DataElement::DATA_ULONG_VECTOR:
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
        case DataElement::DATA_LONGLONG_VECTOR:
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
        case DataElement::DATA_FLOAT_VECTOR:
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
        case DataElement::DATA_DOUBLE_VECTOR:
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
        } //en switch


        if (element) {
            elxml->LinkEndChild(element);

            if (child->numChildren()) {
                nodeToXML(child, element);
            }
        }
    } // end while

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

void DataNode::rewindAll() {
    stack<DataNode *> dn_stack;
    
    /* start at the root */
    dn_stack.push(this);
    
    while (!dn_stack.empty()) {
        dn_stack.top()->rewind();
        
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



