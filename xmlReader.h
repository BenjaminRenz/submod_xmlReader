#ifndef XMLREADER_H_INCLUDED
#define XMLREADER_H_INCLUDED
#include <stdio.h>
#include <xmlReader/stringutils.h>
struct xmlTreeElement{
    uint32_t type;                  //can be comment, pi, normal_tag or cdata
    struct DynamicList* name;       //is name of tag or piTarget
    struct xmlTreeElement* parent;
    struct DynamicList* content;    //NULL or content in list form like utf32,or multiple elements
    struct DynamicList* attributes; //NULL or list of key,value pairs
};

struct key_val_pair{
    struct DynamicList* key;        //points to a DynamicList of type dynlisttype_utf32chars
    struct DynamicList* value;      //points to a DynamicList of type dynlisttype_utf32chars
};

enum {
    xmltype_comment = 0x42, //comment
    xmltype_pi      = 0x43, //processing instruction
    xmltype_tag     = 0x44, //subtag
    xmltype_cdata   = 0x45, //text not processed by parser
    xmltype_chardata= 0x46 //text processed by parser inside elements
};



int readXML(FILE* xmlFile,struct xmlTreeElement** returnDocumentRoot);
int writeXML(FILE* xmlOutFile,struct xmlTreeElement* inputDocumentRoot);
void printXMLsubelements(struct xmlTreeElement* xmlElement);
uint32_t compareEqualDynamicUTF32List(struct DynamicList* List1UTF32,struct DynamicList* List2UTF32);


#endif // XMLREADER_H_INCLUDED
