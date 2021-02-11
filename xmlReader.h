#ifndef XMLREADER_H_INCLUDED
#define XMLREADER_H_INCLUDED
#include <stdio.h>
#include <xmlReader/stringutils.h>
#include <dynList/dynList.h>

typedef struct key_val_pair{
    Dl_utf32Char* key;        //points to a DynamicList of type dynlisttype_utf32chars
    Dl_utf32Char* value;      //points to a DynamicList of type dynlisttype_utf32chars
}key_val_pair;

DlTypedef_plain(attP,key_val_pair*);
typedef struct xmlTreeElement xmlTreeElement;
DlTypedef_plain(xmlP,xmlTreeElement*);

typedef struct xmlTreeElement{
    uint32_t type;                      //can be comment, pi, normal_tag or cdata
    Dl_utf32Char* name;                 //tagname for xmltype_tag or xmltype_pi
    Dl_utf32Char* charData;             //character content of chardata elements
    struct xmlTreeElement* parent;
    Dl_xmlP* children;                  //if type!=xmltype_tag this list has length 0, else #child xml elements
    Dl_attP* attributes;                //list of key,value pairs
}xmlTreeElement;

enum {
    xmltype_docRoot = (1<<0), //return of the parsing function
    xmltype_comment = (1<<1), //comment
    xmltype_pi      = (1<<2), //processing instruction
    xmltype_tag     = (1<<3), //subtag
    xmltype_cdata   = (1<<4), //text not processed by parser
    xmltype_chardata= (1<<5)  //text processed by parser inside elements
};

int readXML(FILE* xmlFile,xmlTreeElement** returnDocumentRoot);
int writeXML(FILE* xmlOutFile,xmlTreeElement* inputDocumentRoot);
void printXMLsubelements(xmlTreeElement* xmlElement);


#endif // XMLREADER_H_INCLUDED
