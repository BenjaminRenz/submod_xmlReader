#include "xmlReader/xmlReader.h"
#include "xmlReader/stringutils.h" //for Dl_utf32Str
//functions to work with xml tree

Dl_utf32Char* getValueFromKeyName         (Dl_attP* attDlP,Dl_utf32Char* nameDlP);
Dl_utf32Char* getValueFromKeyName_freeArg2(Dl_attP* attDlP,Dl_utf32Char* nameDlP);

xmlTreeElement* getNthChildWithType             (xmlTreeElement* parentP, uint32_t n, uint32_t typeBitfield);

xmlTreeElement* getFirstSubelementWith           (xmlTreeElement* startingElementP,Dl_utf32Char* nameDlP,Dl_utf32Char* KeyDlP, Dl_utf32Char* ValDlP, uint32_t TypeBitfield, uint32_t maxDepth);
xmlTreeElement* getFirstSubelementWith_freeArg234(xmlTreeElement* startingElementP,Dl_utf32Char* nameDlP,Dl_utf32Char* KeyDlP, Dl_utf32Char* ValDlP, uint32_t TypeBitfield, uint32_t maxDepth);

Dl_xmlP* getAllSubelementWith            (xmlTreeElement* startingElementP,Dl_utf32Char* nameDlP,Dl_utf32Char* KeyDlP, Dl_utf32Char* ValDlP, uint32_t TypeBitfield, uint32_t maxDepth);
Dl_xmlP* getAllSubelementsWith_freeArg234(xmlTreeElement* startingElementP,Dl_utf32Char* nameDlP,Dl_utf32Char* KeyDlP, Dl_utf32Char* ValDlP, uint32_t TypeBitfield, uint32_t maxDepth);



//This macro is used when getFirstSubelement is to be called with cStringArguments instead of utf32DlP
#define getFirstSubelementWithASCII(startElementp,nameOrContentString,keyString,valueDynlist,TypeBitfield,maxDepth)\
 getFirstSubelementWith_freeArg234(startElementp, \
                                   Dl_utf32Char_fromString(nameOrContentString),\
                                   Dl_utf32Char_fromString(keyString),\
                                   (valueDynlist),\
                                   (TypeBitfield),(maxDepth))

//This macro is used when getFirstSubelement is to be called with cStringArguments instead of utf32DlP
#define getAllSubelementsWithASCII(startElementp,nameOrContentString,keyString,valueDynlist,TypeBitfield,maxDepth)\
 getAllSubelementsWith_freeArg234(startElementp, \
                                   Dl_utf32Char_fromString(nameOrContentString),\
                                   Dl_utf32Char_fromString(keyString),\
                                   (valueDynlist),\
                                   (TypeBitfield),(maxDepth))

#define getValueFromKeyNameASCII(attlist,nameString) getValueFromKeyName_freeArg2((attlist),Dl_utf32Char_fromString(nameString))
