#ifndef STRINGUTILS_H_INCLUDED
#define STRINGUTILS_H_INCLUDED

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
struct xmlTreeElement;
struct key_val_pair;

enum {  DlType_utf32=0x30,          //items of type uint32_t (utf32 characters not null terminated)
        DlType_KeyValPairP=0x31,      //items of type struct key_val_pait*
        DlType_xmlElmntP=0x32, //items of type struct xmlTreeElement*

        DlType_CMatch,
        DlType_WMatchP,
        DlType_MWMatchPP,
        DlType_MCMatchP,

        DlType_int64,
        DlType_float,
        DlType_double,

        /*ListType_Collider,
        ListType_LayerCollection,
        ListType_TexturePointer*/
};

struct DynamicList{
    uint32_t type;
    uint32_t itemcnt;
    void* items; //Pointing to Sublist or Items
};


//Note on the function syntax, the functions are in camelCase, and a trailing _freeArgXYZ signifies that the function frees the memory for the element passed as argument with number X,Y and Z.

struct DynamicList* DlCreate            (size_t sizeofListElements,uint32_t NumOfNewElements,uint32_t typeId);
struct DynamicList* DlDuplicate         (size_t sizeofListElements,struct DynamicList* inDynlistP);
struct DynamicList* DlAppend            (size_t sizeofListElements,struct DynamicList* Dynlist1P,void* appendedElementP);
struct DynamicList* DlCombine           (size_t sizeofListElements,struct DynamicList* Dynlist1P,struct DynamicList* Dynlist2P);
struct DynamicList* DlCombine_freeArg2  (size_t sizeofListElements,struct DynamicList* Dynlist1P,struct DynamicList* Dynlist2P);
struct DynamicList* DlCombine_freeArg3  (size_t sizeofListElements,struct DynamicList* Dynlist1P,struct DynamicList* Dynlist2P);
struct DynamicList* DlCombine_freeArg23 (size_t sizeofListElements,struct DynamicList* Dynlist1P,struct DynamicList* Dynlist2P);
void                DlDelete            (struct DynamicList* DynListPtr);

uint32_t            Dl_utf32_compareEqual_freeArg2(struct DynamicList* List1UTF32,struct DynamicList* List2UTF32);
uint32_t            Dl_utf32_compareEqual(struct DynamicList* List1UTF32,struct DynamicList* List2UTF32);
char*               Dl_utf32_toString   (struct DynamicList* utf32dynlist);
struct DynamicList* Dl_utf32_fromString (char* inputString);
void                Dl_utf32_print      (struct DynamicList* inList);
struct DynamicList* Dl_utf32_StripSpaces_freeArg1(struct DynamicList* utf32StringInP);

struct DynamicList* Dl_utf32_to_Dl_int64            (struct DynamicList* NumberSeperatorP,struct DynamicList* utf32StringInP);
struct DynamicList* Dl_utf32_to_Dl_int64_freeArg1   (struct DynamicList* NumberSeperatorP,struct DynamicList* utf32StringInP);
struct DynamicList* Dl_utf32_to_Dl_double           (struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP);
struct DynamicList* Dl_utf32_to_Dl_double_freeArg123(struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP);
struct DynamicList* Dl_utf32_to_Dl_float            (struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP);
struct DynamicList* Dl_utf32_to_Dl_float_freeArg123 (struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP);

struct DynamicList* Dl_CMatch_create (uint32_t argumentCount,...);
struct DynamicList* Dl_WMatchP_create (uint32_t argumentCount,...);
struct DynamicList* Dl_MWMatchPP_create(uint32_t argumentCount,...);
struct DynamicList* Dl_MCMatchP_create(uint32_t argumentCount,...);

uint32_t utf8ToUtf32    (uint8_t* inputString,  uint32_t numberOfUTF8Chars, uint32_t* outputString);
size_t   utf16ToUtf32   (uint16_t* inputString, size_t numberOfUTF16Chars,  uint32_t* outputString);
uint32_t utf32ToUtf8    (uint32_t* inputString, uint32_t numberOfUTF32Chars,uint8_t* outputString);
size_t   utf32CutASCII  (uint32_t* inputString, uint32_t numberOfUTF32Chars,char* outputStringNullTer);

int MatchAndIncrement   (struct DynamicList* StringInUtf32DlP,uint32_t* InOutIndexP, struct DynamicList* breakIfMatchDlP, struct DynamicList* skipIfMatchDlP);
int Match               (struct DynamicList* StringInUtf32DlP,uint32_t* InOutIndexP, struct DynamicList* breakIfMatchDlP, struct DynamicList* skipIfMatchDlP);

//functions to work with xml tree
struct DynamicList*     getValueFromKeyName         (struct DynamicList* attlist,struct DynamicList* nameD2);
struct DynamicList*     getValueFromKeyName_freeArg2(struct DynamicList* attlist,struct DynamicList* nameD2);

struct xmlTreeElement*  getNthChildElmntOrChardata(struct xmlTreeElement* parentP, uint32_t n);

struct xmlTreeElement*  getFirstSubelementWith           (struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP, struct DynamicList* ValueDynlistP, uint32_t ElmntType, uint32_t maxDepth);
struct xmlTreeElement*  getFirstSubelementWith_freeArg234(struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP, struct DynamicList* ValueDynlistP, uint32_t ElmntType, uint32_t maxDepth);

struct DynamicList*     getAllSubelementsWith           (struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP, struct DynamicList* ValueDynlistP, uint32_t ElmntType, uint32_t maxDepth);
struct DynamicList*     getAllSubelementsWith_freeArg234(struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP, struct DynamicList* ValueDynlistP, uint32_t ElmntType, uint32_t maxDepth);
#endif // STRINGUTILS_H_INCLUDED
