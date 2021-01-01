#ifndef STRINGUTILS_H_INCLUDED
#define STRINGUTILS_H_INCLUDED

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
struct xmlTreeElement;
struct key_val_pair;

enum {  dynlisttype_utf32chars=0x30,          //items of type uint32_t (utf32 characters not null terminated)
        dynlisttype_keyValuePairsp=0x31,      //items of type struct key_val_pait*
        dynlisttype_xmlELMNTCollectionp=0x32, //items of type struct xmlTreeElement*

        ListType_CharMatch,
        ListType_WordMatchp,
        ListType_MultiWordMatchp,
        ListType_MultiCharMatchp,

        ListType_int64,
        ListType_float,
        ListType_double,

        ListType_Collider,
        ListType_LayerCollection,
        ListType_TexturePointer
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

uint32_t compareEqualDynamicUTF32List(struct DynamicList* List1UTF32,struct DynamicList* List2UTF32);

char* utf32dynlist_to_string(struct DynamicList* utf32dynlist);
struct DynamicList* stringToUTF32Dynlist(char* inputString);

void Dl_utf32_print(struct DynamicList* inList);

struct DynamicList* createCharMatchList(uint32_t argumentCount,...);
struct DynamicList* createWordMatchList(uint32_t argumentCount,...);
struct DynamicList* createMultiWordMatchList(uint32_t argumentCount,...);
struct DynamicList* createMultiCharMatchList(uint32_t argumentCount,...);


//internal xml reader functions
uint32_t utf8ToUtf32    (uint8_t* inputString, uint32_t numberOfUTF8Chars, uint32_t* outputString);
size_t   utf16ToUtf32   (uint16_t* inputString, size_t numberOfUTF16Chars, uint32_t* outputString);
size_t   utf32CutASCII  (uint32_t* inputString, uint32_t numberOfUTF32Chars, char* outputStringNullTer);
uint32_t utf32ToUtf8    (uint32_t* inputString, uint32_t numberOfUTF32Chars,uint8_t* outputString);


int MatchAndIncrement(struct DynamicList* StringInUtf32DlP,uint32_t* InOutIndexP, struct DynamicList* breakIfMatchDlP, struct DynamicList* skipIfMatchDlP);
int Match(struct DynamicList* StringInUtf32DlP,uint32_t* InOutIndexP, struct DynamicList* breakIfMatchDlP, struct DynamicList* skipIfMatchDlP);


//xml functions
struct xmlTreeElement* getNthSubelement(struct xmlTreeElement* parentP, uint32_t n);

struct DynamicList* getValueFromKeyName         (struct DynamicList* attlist,struct DynamicList* nameD2);
struct DynamicList* getValueFromKeyName_freeArg2(struct DynamicList* attlist,struct DynamicList* nameD2);

struct DynamicList* getAllSubelementsWith           (struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP, struct DynamicList* ValueDynlistP, uint32_t ElmntType, uint32_t maxDepth);
struct DynamicList* getAllSubelementsWith_freeArg234(struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP, struct DynamicList* ValueDynlistP, uint32_t ElmntType, uint32_t maxDepth);

struct xmlTreeElement* getFirstSubelementWith           (struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP, struct DynamicList* ValueDynlistP, uint32_t ElmntType, uint32_t maxDepth);
struct xmlTreeElement* getFirstSubelementWith_freeArg234(struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP, struct DynamicList* ValueDynlistP, uint32_t ElmntType, uint32_t maxDepth);

struct DynamicList* utf32dynlistStripSpaceChars(struct DynamicList* utf32StringInP);

//xml content functions
struct DynamicList* utf32dynlistToInts64         (struct DynamicList* NumberSeperatorP,struct DynamicList* utf32StringInP);
struct DynamicList* utf32dynlistToInts64_freeArg1(struct DynamicList* NumberSeperatorP,struct DynamicList* utf32StringInP);
struct DynamicList* utf32dynlistToDoubles           (struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP);
struct DynamicList* utf32dynlistToDoubles_freeArg123(struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP);
struct DynamicList* utf32dynlistToFloats            (struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP);
struct DynamicList* utf32dynlistToFloats_freeArg123 (struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP);

#endif // STRINGUTILS_H_INCLUDED
