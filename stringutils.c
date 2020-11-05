#include "stringutils.h"
#include "xmlReader.h"
#include <stdlib.h>     //for malloc
#include <string.h>     //for memcpy
#include <math.h>       //for pow
#include <debug.h>

void DlAppend(struct DynamicList** ListOrNullpp,void* newElementp,size_t sizeofListElements,uint32_t typeId){      //Destroys old pointer and returns new pointer, user needs to update all pointers referring to this object
    if(!ListOrNullpp){
        dprintf(DBGT_ERROR,"argument cant be nullptr");
        exit(-1);
    }
    struct DynamicList* oldDynList=(*ListOrNullpp);
    struct DynamicList* newDynList;
    if(oldDynList){//Does already exist, so increase storage space
        if(oldDynList->type!=typeId){
            dprintf(DBGT_ERROR,"List type missmatch");
            dprintf(DBGT_ERROR,"was %x,new %x",oldDynList->type,typeId);
            exit(-1);
        }
        newDynList=realloc(oldDynList,sizeof(struct DynamicList)+sizeofListElements*(1+oldDynList->itemcnt));
        //DO NOT USE oldDynList or (*ListOrNullpp) variable after this statement, is is invalid
        newDynList->itemcnt++;
    }else{
        newDynList=(struct DynamicList*)malloc(sizeof(struct DynamicList)+sizeofListElements);
        newDynList->itemcnt=1;
        newDynList->type=typeId;
    }
    //Fix dangling pointers
    newDynList->items=(&(newDynList[1]));
    //copy new element in list
    void** lastElementInListp=(void**)((char*)newDynList->items+(sizeofListElements*(newDynList->itemcnt-1)));
    memcpy(lastElementInListp,newElementp,sizeofListElements);
    //(*lastElementInListp)=newElementp;
    *ListOrNullpp=newDynList;
    return;
}

struct DynamicList* DlCreate(size_t sizeofListElements,uint32_t NumOfNewElements,uint32_t typeId){
    struct DynamicList* newDynList;
    newDynList=(struct DynamicList*) malloc(sizeof(struct DynamicList)+sizeofListElements*NumOfNewElements);
    newDynList->itemcnt=NumOfNewElements;
    newDynList->items=(&(newDynList[1]));
    newDynList->type=typeId;
    return newDynList;
}

struct DynamicList* createCharMatchList(uint32_t argumentCount,...){ //only pass uint32_t to this function!!, Matches from even character to next odd character
    va_list argp;
    va_start(argp, argumentCount);
    struct DynamicList* DynListPtr=DlCreate(sizeof(uint32_t),argumentCount,ListType_CharMatch);
    for(uint32_t item=0;item<argumentCount;item++){
       ((uint32_t*)DynListPtr->items)[item]=va_arg(argp, uint32_t);
    }
    va_end(argp);
    return DynListPtr;
}

struct DynamicList* createWordMatchList(uint32_t argumentCount,...){ //in case you want to match a word with characters defined by the CharMatchLists like (abc acb bca bac cab cba)
    va_list argp;
    va_start(argp, argumentCount);
    struct DynamicList* DynListPtr=DlCreate(sizeof(struct DynamicList*),argumentCount,ListType_WordMatchp);
    for(uint32_t item=0;item<argumentCount;item++){
       ((struct DynamicList**)DynListPtr->items)[item]=va_arg(argp, struct DynamicList*);
    }
    va_end(argp);
    return DynListPtr;
}

struct DynamicList* createMultiWordMatchList(uint32_t argumentCount,...){ //in case you want to match xml and XML but not xMl or XmL
    va_list argp;
    va_start(argp, argumentCount);
    struct DynamicList* DynListPtr=DlCreate(sizeof(struct DynamicList*),argumentCount,ListType_MultiWordMatchp);
    for(uint32_t item=0;item<argumentCount;item++){
       ((struct DynamicList**)DynListPtr->items)[item]=va_arg(argp, struct DynamicList*);
    }
    va_end(argp);
    return DynListPtr;
}

struct DynamicList* createMultiCharMatchList(uint32_t argumentCount,...){
    va_list argp;
    va_start(argp, argumentCount);
    struct DynamicList* DynListPtr=DlCreate(sizeof(struct DynamicList*),argumentCount,ListType_MultiCharMatchp);
    for(uint32_t item=0;item<argumentCount;item++){
       ((struct DynamicList**)DynListPtr->items)[item]=va_arg(argp, struct DynamicList*);
    }
    va_end(argp);
    return DynListPtr;
}

struct DynamicList* stringToUTF32Dynlist(char* inputString){
    uint32_t stringlength=0;
    while(inputString[stringlength++]){}
    struct DynamicList* outputString=DlCreate(sizeof(uint32_t),--stringlength,dynlisttype_utf32chars);
    for(uint32_t index=0;index<stringlength;index++){
        ((uint32_t*)outputString->items)[index]=inputString[index];
    }
    return outputString;
}

char* utf32dynlist_to_string(struct DynamicList* utf32dynlist){
    if(utf32dynlist->type!=dynlisttype_utf32chars){
        dprintf(DBGT_ERROR,"incorrect list type");
    }
    dprintf(DBGT_INFO,"LenOfStrg %d",utf32dynlist->itemcnt);
    char* outstring=(char*)malloc(sizeof(char*)*(utf32dynlist->itemcnt+1));
    utf32CutASCII(utf32dynlist->items,utf32dynlist->itemcnt,outstring);
    return outstring;
}

void DlDelete(struct DynamicList* DynListPtr){
    if(!DynListPtr){
        dprintf(DBGT_ERROR,"tried to free non existing DynList or subDynList");
        return;
    }
    switch(DynListPtr->type){
        case ListType_WordMatchp:
        case ListType_MultiWordMatchp:
        case ListType_MultiCharMatchp:
            for(uint32_t SubList=0; SubList<(DynListPtr->itemcnt); SubList++){
                DlDelete(((struct DynamicList**)DynListPtr->items)[SubList]);
            }
        default:
            free(DynListPtr);
            break;
    }
}

uint32_t compareEqualDynamicUTF32List(struct DynamicList* List1UTF32,struct DynamicList* List2UTF32){
    if(!List1UTF32||!List2UTF32){
        dprintf(DBGT_ERROR,"Unexpected Nullptr");
        return 0;
    }
    if(List1UTF32->itemcnt!=List2UTF32->itemcnt){
        //dprintf(DBGT_INFO,"Compared strings do not have the same length\n"); TODO uncomment
        return 0;
    }
    int matchFlag=1;
    for(uint32_t index=0;index<List1UTF32->itemcnt;index++){
        if(((uint32_t*)List1UTF32->items)[index]!=((uint32_t*)List2UTF32->items)[index]){   //Missmatch
            matchFlag=0;
        }
    }
    return matchFlag;
}

void printUTF32Dynlist(struct DynamicList* inList){
    if(!inList){
        printf("Error: NullPtr was passed to print UTF32Dynlist\n");
    }
    if(inList->type!=dynlisttype_utf32chars){
        printf("Error: Can't Print List, is of wrong type!\n");
    }else{
        char* AsciiStringp=(char*)malloc(sizeof(char)*((inList->itemcnt)+1));
        utf32CutASCII(inList->items,inList->itemcnt,AsciiStringp);
        printf("%.*s\n",inList->itemcnt,AsciiStringp);
    }
}

uint32_t utf8ToUtf32(uint8_t* inputString, uint32_t numberOfUTF8Chars, uint32_t* outputString){
    uint32_t input_array_index=0;
    uint32_t output_array_index=0;
    dprintf(DBGT_INFO,"Info: Starting conversion of %d utf8 to unknown utf32 chars\n",numberOfUTF8Chars);
    while(input_array_index<numberOfUTF8Chars){
        if(inputString[input_array_index]>=0xF0){
            outputString[output_array_index++]=
                (((uint32_t)(inputString[input_array_index]  &0x07))<<(6*3))||
                (((uint32_t)(inputString[input_array_index+1]&0x3F))<<(6*2))||
                (((uint32_t)(inputString[input_array_index+2]&0x3F))<<(6*1))||
                (((uint32_t)(inputString[input_array_index+3]&0x3F))       );
                input_array_index+=4;
        }else if(inputString[input_array_index]>=0xE0){
            outputString[output_array_index++]=
                (((uint32_t)(inputString[input_array_index]  &0x0F))<<(6*2))||
                (((uint32_t)(inputString[input_array_index+1]&0x3F))<<(6*1))||
                (((uint32_t)(inputString[input_array_index+2]&0x3F))       );
                input_array_index+=3;
        }else if(inputString[input_array_index]>=0xC0){
            outputString[output_array_index++]=
                (((uint32_t)(inputString[input_array_index]  &0x1F))<<6)||
                (((uint32_t)(inputString[input_array_index+1]&0x3F))    );
            input_array_index+=2;
        }else{
            outputString[output_array_index++]=(uint32_t)(inputString[input_array_index++]&0x7F);
        }
    }
    printf("Info: Converted %d utf8 to %d utf32 chars\n",(unsigned int)input_array_index,(unsigned int)output_array_index);
    return output_array_index;
}



uint32_t utf32ToUtf8(uint32_t* inputString, uint32_t numberOfUTF32Chars,uint8_t* outputString){
    uint32_t output_array_index=0;
    for(uint32_t input_array_index=0;input_array_index<numberOfUTF32Chars;input_array_index++){
        if      (inputString[input_array_index]>=0x00010000){
            outputString[output_array_index]=  (((uint8_t)inputString[input_array_index]>>(3*6))&0x07)|0xf0;
            outputString[output_array_index+1]=(((uint8_t)inputString[input_array_index]>>(2*6))&0x3f)|0x80;
            outputString[output_array_index+2]=(((uint8_t)inputString[input_array_index]>>(1*6))&0x3f)|0x80;
            outputString[output_array_index+3]=(((uint8_t)inputString[input_array_index]>>(0*6))&0x3f)|0x80;
            output_array_index+=4;
        }else if(inputString[input_array_index]>=0x00000800){
            outputString[output_array_index]=(((uint8_t)inputString[input_array_index]  >>(2*6))&0x0f)|0xe0;
            outputString[output_array_index+1]=(((uint8_t)inputString[input_array_index]>>(1*6))&0x3f)|0x80;
            outputString[output_array_index+2]=(((uint8_t)inputString[input_array_index]>>(0*6))&0x3f)|0x80;
            output_array_index+=3;
        }else if(inputString[input_array_index]>=0x00000080){
            outputString[output_array_index]=(((uint8_t)inputString[input_array_index]  >>(1*6))&0x1f)|0xc0;
            outputString[output_array_index+1]=(((uint8_t)inputString[input_array_index]>>(0*6))&0x3f)|0x80;
            output_array_index+=2;
        }else{
            outputString[output_array_index]=(((uint8_t)inputString[input_array_index])         &0x7f);
            output_array_index+=1;
        }
    }
    printf("Info: Converted %d utf32 to %d utf8 chars\n",(unsigned int)numberOfUTF32Chars,(unsigned int)output_array_index);
    return output_array_index;
}

size_t utf16ToUtf32(uint16_t* inputString, size_t numberOfUTF16Chars, uint32_t* outputString){ //Make sure the supplied output buffer is able to hold at least
    size_t input_array_index=0;
    size_t output_array_index=0;
    while(input_array_index<numberOfUTF16Chars){
        if(inputString[input_array_index]>=0xD800){//detect double characters
            outputString[output_array_index++]=(((uint32_t)(inputString[input_array_index]&0x3FF))<<10)||(inputString[++input_array_index]);
        }else{
            outputString[output_array_index++]=(uint32_t)inputString[input_array_index++];
        }
    }
    printf("Info: Converted %d utf16 to %d utf32 chars\n",(unsigned int)input_array_index,(unsigned int)output_array_index);
    return output_array_index;
}



size_t utf32CutASCII(uint32_t* inputString, uint32_t numberOfUTF32Chars, char* outputStringNullTer){
    uint32_t output_array_index=0;
    for(size_t input_array_index=0;input_array_index<numberOfUTF32Chars;input_array_index++){
        if(inputString[input_array_index]>0x7F){        //TODO check if 0x7f or 0x7f000000
            outputStringNullTer[output_array_index++]='?';
        }else{
            outputStringNullTer[output_array_index++]=inputString[input_array_index]&0x7F;
        }
    }
    outputStringNullTer[numberOfUTF32Chars]=0;
    return output_array_index;
}




/** \brief finds the offset to the first occurence of word(s) or character(s)
 *
 * \param inputfile pointer to file in utf32
 * \param how many characters the function is allowed to scan into the inputfile
 * \param Pointer to word/character list to match againt
 * \param if not null will return the offset in the struct DynamicList for wordS and characterS to know which one matched
 * \return offset of the first position of the word or offset to the character from the start of the inputfile in utf32 characters (4bytes)
 *
 */


uint32_t getOffsetUntil_freeArg3(uint32_t* fileInUtf32, uint32_t maxScanLength, struct DynamicList* MatchAgainst, uint32_t* optional_matchIndex){
    uint32_t offset=getOffsetUntil(fileInUtf32,maxScanLength,MatchAgainst,optional_matchIndex);
    DlDelete(MatchAgainst);
    return offset;
}

//TODO urgent: avoid that word match scans so far into the file, that the word offset itself goes past the file
uint32_t getOffsetUntil(uint32_t* fileInUtf32, uint32_t maxScanLength, struct DynamicList* MatchAgainst, uint32_t* optional_matchIndex){
    switch(MatchAgainst->type){
        case ListType_MultiCharMatchp:
            for(uint32_t sPosOffset=0;sPosOffset<maxScanLength;sPosOffset++){
                for(uint32_t matchMultiCharArrayPos=0;matchMultiCharArrayPos<(MatchAgainst->itemcnt);matchMultiCharArrayPos++){
                    struct DynamicList* CharMatch=(((struct DynamicList**)MatchAgainst->items)[matchMultiCharArrayPos]);
                    for(uint32_t matchCharArrayPos=0;matchCharArrayPos<(CharMatch->itemcnt);matchCharArrayPos+=2){
                        if((((uint32_t*)CharMatch->items)[matchCharArrayPos]<=fileInUtf32[sPosOffset]) && (((uint32_t*)CharMatch->items)[matchCharArrayPos+1]>=fileInUtf32[sPosOffset])){
                            if(optional_matchIndex){
                                (*optional_matchIndex)=matchMultiCharArrayPos;
                            }
                            return sPosOffset;
                        }
                    }
                }
            }
            return maxScanLength;
        break;
        case ListType_CharMatch:
            for(uint32_t sPosOffset=0;sPosOffset<maxScanLength;sPosOffset++){
                for(uint32_t matchCharArrayPos=0;matchCharArrayPos<(MatchAgainst->itemcnt);matchCharArrayPos+=2){
                    if((((uint32_t*)MatchAgainst->items)[matchCharArrayPos]<=fileInUtf32[sPosOffset]) && (((uint32_t*)MatchAgainst->items)[matchCharArrayPos+1]>=fileInUtf32[sPosOffset])){
                        return sPosOffset;
                    }
                }
            }
            return maxScanLength; //no match found
        break;
        case ListType_WordMatchp: //multiple CharMatch lists
            for(uint32_t sPosOffset=0;sPosOffset<=(maxScanLength-(MatchAgainst->itemcnt));sPosOffset++){
                uint32_t wordPosOffset=0;
                for(;wordPosOffset<MatchAgainst->itemcnt;wordPosOffset++){
                    struct DynamicList* CharMatch=(((struct DynamicList**)MatchAgainst->items)[wordPosOffset]);
                    if(CharMatch->type!=ListType_CharMatch){
                        return UINT32_MAX;//type error
                    }
                    uint32_t MatchingRangeFlag=0;//TODO use KMP-algorithm instead (faster if multiple close matches)
                    for(uint32_t matchCharArrayPos=0;matchCharArrayPos<(CharMatch->itemcnt);matchCharArrayPos+=2){
                        if((((uint32_t*)CharMatch->items)[matchCharArrayPos]<=fileInUtf32[sPosOffset+wordPosOffset]) && (((uint32_t*)CharMatch->items)[matchCharArrayPos+1]>=fileInUtf32[sPosOffset+wordPosOffset])){
                            MatchingRangeFlag=1; //if this character range did match set matchflag
                        }
                    }
                    if(!MatchingRangeFlag){
                        break; //as soon as we get a mismatch ignore the rest of the word and restart from the next position
                    }
                }
                if(wordPosOffset==(MatchAgainst->itemcnt)){ //all characters matched, we have found the word at the position
                    return sPosOffset; //return the index of the first character of the matching word
                }
            }
            return maxScanLength; //no match found
            break;
        case ListType_MultiWordMatchp:{
            uint32_t sPosOffset=0;
            for(;sPosOffset<maxScanLength;sPosOffset++){
                uint32_t wordIndex=0;
                for(;wordIndex<MatchAgainst->itemcnt;wordIndex++){
                    struct DynamicList* WordMatch=((struct DynamicList**)MatchAgainst->items)[wordIndex];
                    uint32_t wordPosOffset=0;
                    for(;wordPosOffset<WordMatch->itemcnt;wordPosOffset++){
                        if(wordPosOffset+sPosOffset>maxScanLength){
                            break; //word is extending beyond the scan area
                        }
                        struct DynamicList* CharMatch=((struct DynamicList**)WordMatch->items)[wordPosOffset];
                        uint32_t MatchingRangeFlag=0;
                        for(uint32_t matchCharArrayPos=0;matchCharArrayPos<(CharMatch->itemcnt);matchCharArrayPos+=2){
                            if((((uint32_t*)CharMatch->items)[matchCharArrayPos]<=fileInUtf32[sPosOffset+wordPosOffset]) && (((uint32_t*)CharMatch->items)[matchCharArrayPos+1]>=fileInUtf32[sPosOffset+wordPosOffset])){
                                MatchingRangeFlag=1;
                            }
                        }
                        if(!MatchingRangeFlag){ //abort further search on one mismatched char
                            break;
                        }
                    }
                    if(wordPosOffset==WordMatch->itemcnt){ //the whole word matches (the inner loop did not terminate prematurely, full word scanned)
                        break;
                    }
                }
                if(wordIndex!=MatchAgainst->itemcnt){ //we got a match from the inner loop (the inner loop did terminate prematurely, word at wordIndex matched)
                    if(optional_matchIndex){
                        (*optional_matchIndex)=wordIndex;
                    }
                    return sPosOffset;
                }
            }
            if(sPosOffset==maxScanLength){ //nothing found, reached end of assigned scan range
                    return maxScanLength;
                }
        }
        break;

        default:
            printf("ERROR: Wrong list type for getOffsetUntil!\n");
            return UINT32_MAX;//type error
            break;
    }
    return 0;
}

struct xmlTreeElement* getNthSubelement(struct xmlTreeElement* parentP, uint32_t n){
    if(parentP->content==0){
        return 0;
    }
    if(parentP->content->type!=dynlisttype_xmlELMNTCollectionp){
        return 0;
    }
    if(parentP->content->itemcnt<=n){
        return 0;   //not enough subelements
    }
    return ((struct xmlTreeElement**)(parentP->content->items))[n];
}

struct xmlTreeElement* getFirstSubelementWith_freeArg2345(struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP,struct DynamicList* ValueDynlistP,struct DynamicList* ContentDynlistP, uint32_t maxDepth){
    struct xmlTreeElement* returnXmlElmntP=getFirstSubelementWith(startElementp,NameDynlistP,KeyDynlistP,ValueDynlistP,ContentDynlistP,maxDepth);
    //Delete Dynlists if they are valid pointers
    if(NameDynlistP)    {DlDelete(NameDynlistP);}
    if(KeyDynlistP)     {DlDelete(KeyDynlistP);}
    if(ValueDynlistP)   {DlDelete(ValueDynlistP);}
    if(ContentDynlistP) {DlDelete(ContentDynlistP);}
    return returnXmlElmntP;
}

//maxDepth 0 means only search direct childs of current element
//For Name, Key, Value and Content a NULL-prt can be passed, which means that this property is ignored when matching
struct xmlTreeElement* getFirstSubelementWith(struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP,struct DynamicList* ValueDynlistP,struct DynamicList* ContentDynlistP, uint32_t maxDepth){
    struct xmlTreeElement* LastXMLTreeElement=startElementp->parent;
    struct xmlTreeElement* CurrentXMLTreeElement=startElementp;
    uint32_t currentDepth=0;
    uint32_t index=0;
    do{
        uint32_t itemcnt;
        if(CurrentXMLTreeElement->content==0){
            itemcnt=0;
        }else{
            itemcnt=CurrentXMLTreeElement->content->itemcnt;
        }
        if(CurrentXMLTreeElement->parent==LastXMLTreeElement){      //walk deeper
            for(;index<itemcnt;index++){
                //Check if properties are all fullfilled
                struct xmlTreeElement* xmlToCheckElementP=((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index];
                //first check if we should match this property or ignore it (with !NullPtr ?) the second part behind || will only be executed when it is a non NullPtr
                struct DynamicList* ValUtf32P=NULL;
                if((!NameDynlistP   ||compareEqualDynamicUTF32List(xmlToCheckElementP->name,NameDynlistP))&&
                   (!ContentDynlistP||xmlToCheckElementP->content->type==dynlisttype_utf32chars||compareEqualDynamicUTF32List(xmlToCheckElementP->content,ContentDynlistP))&&
                   (!KeyDynlistP    ||(ValUtf32P=getValueFromKeyName(xmlToCheckElementP->attributes,KeyDynlistP)))){
                   //check we are not searching for key or we are not having a matching key && we are not searching for a value and the value is valid
                   if(!KeyDynlistP||(ValueDynlistP&&ValUtf32P&&compareEqualDynamicUTF32List(ValUtf32P,ValueDynlistP))){
                        return xmlToCheckElementP;
                   }
                   if(ValUtf32P){DlDelete(ValUtf32P);}
                }
                if(currentDepth<maxDepth){ //make sure our programm stops scanning for new elements if the maxdepth is reached
                    if(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]->type==xmltype_tag){  //One valid subelement found
                        LastXMLTreeElement=CurrentXMLTreeElement;
                        CurrentXMLTreeElement=((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index];
                        index=0;
                        currentDepth++;
                        //TODO? This tests only elements of xmltype_tag
                        break;
                    }else{
                        dprintf(DBGT_ERROR,"Non standrad tag");
                    }
                }else{
                    dprintf(DBGT_INFO,"MaxDepthLimit reached");
                }
            }
        }else{      //go back upward
            for(index=0;index<itemcnt;index++){     //Try to find the index of the element we were in from it's parent
                if(LastXMLTreeElement==((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]){
                    if(itemcnt>1+index){  //Do we have more elements in the current Element
                        index++;
                        LastXMLTreeElement=CurrentXMLTreeElement->parent;
                        currentDepth--;
                        break;
                    }
                }
            }
        }
        if(index==itemcnt){
            //Turn around
            LastXMLTreeElement=CurrentXMLTreeElement;
            CurrentXMLTreeElement=CurrentXMLTreeElement->parent;
        }else{
            continue;
        }
    }while((CurrentXMLTreeElement!=0)&&(CurrentXMLTreeElement!=startElementp->parent));     //Todo error with abort condition when starting from subelement
    //did not find anyhing, return NULL
    return NULL;
};

struct DynamicList* DlDuplicate(struct DynamicList* inDynlistP){
    if(!inDynlistP){
        dprintf(DBGT_ERROR,"Called with Nullptr");
        return 0;
    }
    if((inDynlistP->type!=dynlisttype_utf32chars)&&(inDynlistP->type!=ListType_CharMatch)){
        dprintf(DBGT_ERROR,"currently unsupported list type");
        return 0;
    }
    struct DynamicList* newDynlistP=(struct DynamicList*)malloc(sizeof(struct DynamicList)+sizeof(void*)*(inDynlistP->itemcnt));
    memcpy(newDynlistP,inDynlistP,sizeof(struct DynamicList)+sizeof(void*)*(inDynlistP->itemcnt));
    newDynlistP->items=(&(newDynlistP[1]));
    return newDynlistP;
}

struct DynamicList* DlCombine(struct DynamicList* Dynlist1P,struct DynamicList* Dynlist2P){
    //check if of same type
    if(!Dynlist1P||!Dynlist2P){
        dprintf(DBGT_INFO,"Called with one Nullptr");
        return 0;
    }
    if(Dynlist1P->type!=Dynlist2P->type){
        dprintf(DBGT_INFO,"Attempted to concatenate lists of different types");
        return 0;
    }
    size_t newDataSize=Dynlist1P->itemcnt+Dynlist2P->itemcnt;
    struct DynamicList* DynlistRP=(struct DynamicList*)malloc(sizeof(struct DynamicList*)+sizeof(void*)*(newDataSize));
    memcpy(DynlistRP,Dynlist1P,sizeof(struct DynamicList*)+sizeof(void*)*(Dynlist1P->itemcnt));
    DynlistRP->items=(&(DynlistRP[1]));
    memcpy(DynlistRP->items+DynlistRP->itemcnt,Dynlist2P->items,Dynlist2P->itemcnt*sizeof(void*));
    return(DynlistRP);
}

struct DynamicList* DlCombine_freeArg2(struct DynamicList* Dynlist1P,struct DynamicList* Dynlist2P){
    struct DynamicList* DynlistRP=DlCombine(Dynlist1P,Dynlist2P);
    DlDelete(Dynlist2P);
    return(DynlistRP);
}

struct DynamicList* DlCombine_freeArg12(struct DynamicList* Dynlist1P,struct DynamicList* Dynlist2P){
    struct DynamicList* DynlistRP=DlCombine(Dynlist1P,Dynlist2P);
    DlDelete(Dynlist1P);
    DlDelete(Dynlist2P);
    return(DynlistRP);
}

struct DynamicList* getValueFromKeyName_freeArg2(struct DynamicList* attlist,struct DynamicList* nameD2){
    struct DynamicList* DynlistRP=getValueFromKeyName(attlist,nameD2);
    DlDelete(nameD2);
    return DynlistRP;
}

struct DynamicList* getValueFromKeyName(struct DynamicList* attlist,struct DynamicList* nameD2){
  if(!attlist){
        dprintf(DBGT_ERROR,"Attlist was nullptr");
        return 0;
    }
    if(!attlist->itemcnt){
        dprintf(DBGT_ERROR,"Attlist has no attributes");
        return 0;
    }
    for(uint32_t index=0;index<attlist->itemcnt;index++){
        if(compareEqualDynamicUTF32List(((struct key_val_pair*)attlist->items)[index].key,nameD2)){
            return ((struct key_val_pair*)attlist->items)[index].value;
        }
    }
    return 0;
}



struct DynamicList* getSubelementsWith_freeArg2345(struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP,struct DynamicList* ValueDynlistP,struct DynamicList* ContentDynlistP, uint32_t maxDepth){
    struct DynamicList*result=getSubelementsWith(startElementp,NameDynlistP,KeyDynlistP,ValueDynlistP,ContentDynlistP,maxDepth);
    DlDelete(NameDynlistP);
    DlDelete(KeyDynlistP);
    DlDelete(ValueDynlistP);
    DlDelete(ContentDynlistP);
    return result;
}

//maxDepth 0 means only search direct childs of current element
//For Name, Key, Value and Content a NULL-prt can be passed, which means that this property is ignored when matching
//TODO version that delete input dynamic lists, so it can be used with createUTF32dynlist from string.
struct DynamicList* getSubelementsWith(struct xmlTreeElement* startElementp,struct DynamicList* NameDynlistP,struct DynamicList* KeyDynlistP,struct DynamicList* ValueDynlistP,struct DynamicList* ContentDynlistP, uint32_t maxDepth){
    struct DynamicList* returnDynList=0;
    struct xmlTreeElement* LastXMLTreeElement=startElementp->parent;
    struct xmlTreeElement* CurrentXMLTreeElement=startElementp;
    uint32_t currentDepth=0;
    uint32_t index=0;
    do{
        uint32_t itemcnt;
        if(CurrentXMLTreeElement->content==0){
            itemcnt=0;
        }else{
            itemcnt=CurrentXMLTreeElement->content->itemcnt;
        }
        if(CurrentXMLTreeElement->parent==LastXMLTreeElement){      //walk deeper
            for(;index<itemcnt;index++){
                //Check if properties are all fullfilled
                struct xmlTreeElement* xmlToCheckElementP=((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index];
                //first check if we should match this property or ignore it (with !NullPtr ?) the second part behind || will only be executed when it is a non NullPtr
                struct DynamicList* ValUtf32P=NULL;
                if((!NameDynlistP   ||compareEqualDynamicUTF32List(xmlToCheckElementP->name,NameDynlistP))&&
                   (!ContentDynlistP||xmlToCheckElementP->content->type==dynlisttype_utf32chars||compareEqualDynamicUTF32List(xmlToCheckElementP->content,ContentDynlistP))&&
                   (!KeyDynlistP    ||(ValUtf32P=getValueFromKeyName(xmlToCheckElementP->attributes,KeyDynlistP)))){
                   //check in case we searched for a key, that if there is a matching value specified, it is also ok
                   if(!ValUtf32P||!ValueDynlistP||compareEqualDynamicUTF32List(ValUtf32P,ValueDynlistP)){
                        DlAppend(&returnDynList,(void**)&xmlToCheckElementP,sizeof(struct xmlTreeElement*),dynlisttype_xmlELMNTCollectionp);
                   }
                }
                if(currentDepth<maxDepth){ //make sure our programm stops scanning for new elements if the maxdepth is reached
                    if(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]->type==xmltype_tag){  //One valid subelement found
                        LastXMLTreeElement=CurrentXMLTreeElement;
                        CurrentXMLTreeElement=((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index];
                        index=0;
                        currentDepth++;
                        //TODO? This tests only elements of xmltype_tag
                        break;
                    }else{
                        dprintf(DBGT_ERROR,"Non standrad tag");
                    }
                }else{
                    dprintf(DBGT_INFO,"MaxDepthLimit reached");
                }
            }
        }else{      //go back upward
            for(index=0;index<itemcnt;index++){     //Try to find the index of the element we were in from it's parent
                if(LastXMLTreeElement==((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]){
                    if(itemcnt>1+index){  //Do we have more elements in the current Element
                        index++;
                        LastXMLTreeElement=CurrentXMLTreeElement->parent;
                        currentDepth--;
                        break;
                    }
                }
            }
        }
        if(index==itemcnt){
            //Turn around
            LastXMLTreeElement=CurrentXMLTreeElement;
            CurrentXMLTreeElement=CurrentXMLTreeElement->parent;
        }else{
            continue;
        }
    }while((CurrentXMLTreeElement!=0)&&(CurrentXMLTreeElement!=startElementp->parent));     //Todo error with abort condition when starting from subelement
    if(returnDynList){
        dprintf(DBGT_INFO,"Finished Search with %d matches\n",returnDynList->itemcnt);
    }else{
        dprintf(DBGT_INFO,"Did not found any match");
    }
    return returnDynList;
};

//0 for depth means search only childs of current element
struct DynamicList* getSubelementsWithCheckFunc(uint32_t (*checkfkt)(struct xmlTreeElement*),struct xmlTreeElement* startElementp,uint32_t maxDepth){
    struct DynamicList* returnDynList=0;
    struct xmlTreeElement* LastXMLTreeElement=startElementp->parent;
    struct xmlTreeElement* CurrentXMLTreeElement=startElementp;
    uint32_t currentDepth=0;
    uint32_t index=0;
    do{
        uint32_t itemcnt;
        if(CurrentXMLTreeElement->content==0){
            itemcnt=0;
        }else{
            itemcnt=CurrentXMLTreeElement->content->itemcnt;
        }
        if(CurrentXMLTreeElement->parent==LastXMLTreeElement){      //walk deeper
            for(;index<itemcnt;index++){
                if((*checkfkt)(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index])){
                    DlAppend(&returnDynList,(void**)&(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]),sizeof(struct xmlTreeElement*),dynlisttype_xmlELMNTCollectionp);
                }
                if(currentDepth<maxDepth){ //make sure our programm stops scanning for new elements if the maxdepth is reached
                    if(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]->type==xmltype_tag){  //One valid subelement found
                        LastXMLTreeElement=CurrentXMLTreeElement;
                        CurrentXMLTreeElement=((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index];
                        index=0;
                        currentDepth++;
                        //TODO? This tests only elements of xmltype_tag
                        break;
                    }else{
                        dprintf(DBGT_ERROR,"Non standrad tag");
                    }
                }else{
                    dprintf(DBGT_INFO,"MaxDepthLimit reached");
                }
            }
        }else{      //go back upward
            for(index=0;index<itemcnt;index++){     //Try to find the index of the element we were in from it's parent
                if(LastXMLTreeElement==((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[index]){
                    if(itemcnt>1+index){  //Do we have more elements in the current Element
                        index++;
                        LastXMLTreeElement=CurrentXMLTreeElement->parent;
                        currentDepth--;
                        break;
                    }
                }
            }
        }
        if(index==itemcnt){
            //Turn around
            LastXMLTreeElement=CurrentXMLTreeElement;
            CurrentXMLTreeElement=CurrentXMLTreeElement->parent;
        }else{
            continue;
        }
    }while((CurrentXMLTreeElement!=0)&&(CurrentXMLTreeElement!=startElementp->parent));     //Todo error with abort condition when starting from subelement
    if(returnDynList){
        dprintf(DBGT_INFO,"Finished Search with %d matches\n",returnDynList->itemcnt);
    }else{
        dprintf(DBGT_INFO,"Did not found any match");
    }
    return returnDynList;
}

uint32_t nameCheckFkt(struct xmlTreeElement* nameInit_or_xmlElement){
    static struct DynamicList* nameDynlistUFT32=0;        //0 if nor initialized
    if(nameInit_or_xmlElement){
        if(!nameDynlistUFT32){//if function was not initialized with a name yet
            nameDynlistUFT32=(struct DynamicList*) nameInit_or_xmlElement;
            if(nameDynlistUFT32->type!=dynlisttype_utf32chars){
                return 43;
            }
            return 42;  //initialized name succesfully
        }else{
            if(compareEqualDynamicUTF32List(nameInit_or_xmlElement->name,nameDynlistUFT32)){
                return 1;
            }else{
                return 0;
            }
        }
    }else{
        nameDynlistUFT32=0; //Deinit
        return 42;
    }
}


struct DynamicList* utf32dynlistToInts(struct DynamicList* NumberSeperatorP,struct DynamicList* utf32StringInP){
    return utf32dynlistToInts_freeArg1(DlDuplicate(NumberSeperatorP),utf32StringInP);
}

struct DynamicList* utf32dynlistToInts_freeArg1(struct DynamicList* NumberSeperatorP,struct DynamicList* utf32StringInP){
    if(NumberSeperatorP->type!=ListType_CharMatch||utf32StringInP->type!=dynlisttype_utf32chars){
        dprintf(DBGT_ERROR,"Invalid parameter passed to function");
        return 0;
    }
    struct DynamicList* returnDynlistp=0;
    uint32_t offsetInString=0;
    enum{flag_in_digits=(1<<0),flag_minus_mantis=(1<<1)};
    int32_t flagreg=0;
    int64_t mantisVal=0;
    enum {
        match_res_nummatch_0=0,
        match_res_nummatch_1=1,
        match_res_nummatch_2=2,
        match_res_nummatch_3=3,
        match_res_nummatch_4=4,
        match_res_nummatch_5=5,
        match_res_nummatch_6=6,
        match_res_nummatch_7=7,
        match_res_nummatch_8=8,
        match_res_nummatch_9=9,
        match_res_nummatch_sign=10,
        match_res_nummatch_nsperator=11,
        match_res_nummatch_illegal=12
    };
    struct DynamicList* nummatch=createMultiCharMatchList(13,
        createCharMatchList(2,'0','0'),
        createCharMatchList(2,'1','1'),
        createCharMatchList(2,'2','2'),
        createCharMatchList(2,'3','3'),
        createCharMatchList(2,'4','4'),
        createCharMatchList(2,'5','5'),
        createCharMatchList(2,'6','6'),
        createCharMatchList(2,'7','7'),
        createCharMatchList(2,'8','8'),
        createCharMatchList(2,'9','9'),
        createCharMatchList(2,'-','-'),
        NumberSeperatorP,
        createCharMatchList(2,0x00,0xffff)
    );
    uint32_t matchIndex=0;
    while(offsetInString<utf32StringInP->itemcnt){
        //no offset because will match all possible chars
        getOffsetUntil(((uint32_t*)utf32StringInP->items)+offsetInString,1,nummatch,&matchIndex);
        if(matchIndex==match_res_nummatch_illegal){
            dprintf(DBGT_ERROR,"Illegal Character in inputstring");
            return 0;
        }else if(matchIndex==match_res_nummatch_sign){
            if(flagreg==0){
                flagreg|=flag_minus_mantis;
            }else{
                dprintf(DBGT_ERROR,"Unexpected Sign Char");
                return 0;
            }
        }else if(matchIndex==match_res_nummatch_nsperator){
            if(flagreg&flag_in_digits){
                if(flagreg&flag_minus_mantis){
                    mantisVal*=(-1);
                }
                DlAppend(&returnDynlistp,&mantisVal,sizeof(double),ListType_double);
                flagreg=0;
                mantisVal=0;
            }
        }else{
            flagreg|=flag_in_digits;
            mantisVal*=10;
            mantisVal+=matchIndex;
        }
        offsetInString++;
    }

    if(flagreg&flag_in_digits){
        if(flagreg&flag_minus_mantis){
            mantisVal*=(-1);
        }
        DlAppend(&returnDynlistp,&mantisVal,sizeof(double),ListType_double);
    }
    DlDelete(nummatch);
    return returnDynlistp;
}


struct DynamicList* utf32dynlistToDoubles(struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP){
    return utf32dynlistToDoubles_freeArg123(DlDuplicate(NumberSeperatorP),DlDuplicate(OrderOfMagP),DlDuplicate(DecimalSeperatorP),utf32StringInP);
}

struct DynamicList* utf32dynlistToDoubles_freeArg123(struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP){
    struct DynamicList* returnDynlistp=0;
    uint32_t offsetInString=0;
    double mantisVal=0;
    enum{flag_in_digits=(1<<0),flag_decplc=(1<<1),flag_in_decplcdigits=(1<<2),flag_orOfMag=(1<<3),flag_in_exp_digits=(1<<4),flag_minus_mantis=(1<<5),flag_minus_exp=(1<<6)};
    int32_t flagreg=0;
    int32_t NumOfDecplcDig=0;
    int32_t exponVal=0;
    enum {
        match_res_nummatch_0=0,
        match_res_nummatch_1=1,
        match_res_nummatch_2=2,
        match_res_nummatch_3=3,
        match_res_nummatch_4=4,
        match_res_nummatch_5=5,
        match_res_nummatch_6=6,
        match_res_nummatch_7=7,
        match_res_nummatch_8=8,
        match_res_nummatch_9=9,
        match_res_nummatch_sign=10,
        match_res_nummatch_nseperator=11,
        match_res_nummatch_dseperator=12,
        match_res_nummatch_OrderOfMag=13,
        match_res_nummatch_illegal=14
    };
    struct DynamicList* nummatch=createMultiCharMatchList(15,
        createCharMatchList(2,'0','0'),
        createCharMatchList(2,'1','1'),
        createCharMatchList(2,'2','2'),
        createCharMatchList(2,'3','3'),
        createCharMatchList(2,'4','4'),
        createCharMatchList(2,'5','5'),
        createCharMatchList(2,'6','6'),
        createCharMatchList(2,'7','7'),
        createCharMatchList(2,'8','8'),
        createCharMatchList(2,'9','9'),
        createCharMatchList(2,'-','-'),
        NumberSeperatorP,
        DecimalSeperatorP,
        OrderOfMagP,
        createCharMatchList(2,0x00,0xffff)
    );
    uint32_t matchIndex=0;
    while(offsetInString<utf32StringInP->itemcnt){
        //no offset because will match all possible chars
        getOffsetUntil(((uint32_t*)(utf32StringInP->items))+offsetInString,1,nummatch,&matchIndex);
        if(matchIndex==match_res_nummatch_illegal){
            dprintf(DBGT_ERROR,"Illegal Character in inputstring");
            return 0;
        }else if(matchIndex==match_res_nummatch_sign){
            if(flagreg&flag_orOfMag){
                flagreg|=flag_minus_exp;
            }else if(flagreg==0){
                flagreg|=flag_minus_mantis;
            }else{
                dprintf(DBGT_ERROR,"Unexpected Sign Char");
                return 0;
            }
        }else if(matchIndex==match_res_nummatch_nseperator){
            if(flagreg&flag_orOfMag){
                dprintf(DBGT_ERROR,"Number ended appruptly after exponent");
                return 0;
            }
            if(flagreg&flag_decplc){
                dprintf(DBGT_ERROR,"Number ended appruptly after DecimalSeperator Char");
                return 0;
            }
            if(flagreg&(flag_in_digits|flag_in_decplcdigits|flag_in_exp_digits)){
                if(flagreg&flag_minus_mantis){
                    mantisVal*=(-1.0);
                }
                if(flagreg&flag_minus_exp){
                    exponVal*=(-1);
                }
                exponVal-=NumOfDecplcDig;
                mantisVal*=pow(10,exponVal);
                DlAppend(&returnDynlistp,&mantisVal,sizeof(double),ListType_double);
                flagreg=0;
                NumOfDecplcDig=0;
                mantisVal=0;
                exponVal=0;
            }
        }else if(matchIndex==match_res_nummatch_dseperator){
            if(flagreg&flag_in_digits){
                flagreg|=flag_in_decplcdigits;         //enable in_decplcdigits flag
                flagreg&= ~(flag_in_digits);    //disable in_digits flag
            }else if(flagreg==0){               //to handle ".2"
                flagreg|=flag_in_decplcdigits;
                mantisVal=0;
            }else{
                if(flagreg&flag_in_exp_digits){
                    dprintf(DBGT_ERROR,"unexpected decDigSeparator in exponent");
                }else if(flagreg&flag_in_decplcdigits){
                    dprintf(DBGT_ERROR,"more than one decDigSeperator in mantisse");
                }
            }
        }else if(matchIndex==match_res_nummatch_OrderOfMag){
            if(flagreg&(flag_in_digits|flag_in_decplcdigits)){
                flagreg|=flag_orOfMag;
                flagreg&=~(flag_in_digits|flag_in_decplcdigits);    //clear flags
            }else{
                dprintf(DBGT_ERROR,"Unexpected Order of Magnitude Char");
                return 0;
            }
        }else{      //Digit from 0-9
            if(flagreg&flag_in_decplcdigits){
                NumOfDecplcDig++;
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_decplc){
                NumOfDecplcDig++;
                flagreg|=flag_in_decplcdigits;
                flagreg&=~(flag_decplc);
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_in_digits){
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_orOfMag){
                exponVal=matchIndex;
                flagreg|=flag_in_exp_digits;
                flagreg&=~(flag_orOfMag);
            }else if(flagreg&flag_in_exp_digits){
                exponVal*=10;
                exponVal+=matchIndex;
            }else{
                flagreg|=flag_in_digits;
                mantisVal=matchIndex;
            }
        }
        offsetInString++;
    }

    //finish parsing last double
    if(flagreg&flag_orOfMag){
        dprintf(DBGT_ERROR,"Number ended appruptly after exponent");
        return 0;
    }
    if(flagreg&flag_decplc){
        dprintf(DBGT_ERROR,"Number ended appruptly after DecimalSeperator Char");
        return 0;
    }
    if(flagreg&(flag_in_digits|flag_in_decplcdigits|flag_in_exp_digits)){
        if(flagreg&flag_minus_mantis){
            mantisVal*=(-1.0);
        }
        if(flagreg&flag_minus_exp){
            exponVal*=(-1);
        }
        exponVal-=NumOfDecplcDig;
        mantisVal*=pow(10,exponVal);
        DlAppend(&returnDynlistp,&mantisVal,sizeof(double),ListType_double);
    }
    DlDelete(nummatch);       //TODO fix for all return types
    return returnDynlistp;
}

struct DynamicList* utf32dynlistToFloats(struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP){
    return utf32dynlistToFloats_freeArg123(DlDuplicate(NumberSeperatorP),DlDuplicate(OrderOfMagP),DlDuplicate(DecimalSeperatorP),utf32StringInP);
}


struct DynamicList* utf32dynlistToFloats_freeArg123(struct DynamicList* NumberSeperatorP,struct DynamicList* OrderOfMagP, struct DynamicList* DecimalSeperatorP,struct DynamicList* utf32StringInP){
    struct DynamicList* returnDynlistp=0;
    uint32_t offsetInString=0;
    float mantisVal=0;
    enum{flag_in_digits=(1<<0),flag_decplc=(1<<1),flag_in_decplcdigits=(1<<2),flag_orOfMag=(1<<3),flag_in_exp_digits=(1<<4),flag_minus_mantis=(1<<5),flag_minus_exp=(1<<6)};
    int32_t flagreg=0;
    int32_t NumOfDecplcDig=0;
    int32_t exponVal=0;
    enum {
        match_res_nummatch_0=0,
        match_res_nummatch_1=1,
        match_res_nummatch_2=2,
        match_res_nummatch_3=3,
        match_res_nummatch_4=4,
        match_res_nummatch_5=5,
        match_res_nummatch_6=6,
        match_res_nummatch_7=7,
        match_res_nummatch_8=8,
        match_res_nummatch_9=9,
        match_res_nummatch_sign=10,
        match_res_nummatch_nseperator=11,
        match_res_nummatch_dseperator=12,
        match_res_nummatch_OrderOfMag=13,
        match_res_nummatch_illegal=14
    };
    struct DynamicList* nummatch=createMultiCharMatchList(15,
        createCharMatchList(2,'0','0'),
        createCharMatchList(2,'1','1'),
        createCharMatchList(2,'2','2'),
        createCharMatchList(2,'3','3'),
        createCharMatchList(2,'4','4'),
        createCharMatchList(2,'5','5'),
        createCharMatchList(2,'6','6'),
        createCharMatchList(2,'7','7'),
        createCharMatchList(2,'8','8'),
        createCharMatchList(2,'9','9'),
        createCharMatchList(2,'-','-'),
        NumberSeperatorP,
        DecimalSeperatorP,
        OrderOfMagP,
        createCharMatchList(2,0x00,0xffff)
    );
    uint32_t matchIndex=0;
    while(offsetInString<utf32StringInP->itemcnt){
        //no offset because will match all possible chars
        getOffsetUntil(((uint32_t*)(utf32StringInP->items))+offsetInString,1,nummatch,&matchIndex);
        if(matchIndex==match_res_nummatch_illegal){
            dprintf(DBGT_ERROR,"Illegal Character in inputstring");
            return 0;
        }else if(matchIndex==match_res_nummatch_sign){
            if(flagreg&flag_orOfMag){
                flagreg|=flag_minus_exp;
            }else if(flagreg==0){
                flagreg|=flag_minus_mantis;
            }else{
                dprintf(DBGT_ERROR,"Unexpected Sign Char");
                return 0;
            }
        }else if(matchIndex==match_res_nummatch_nseperator){
            if(flagreg&flag_orOfMag){
                dprintf(DBGT_ERROR,"Number ended appruptly after exponent");
                return 0;
            }
            if(flagreg&flag_decplc){
                dprintf(DBGT_ERROR,"Number ended appruptly after DecimalSeperator Char");
                return 0;
            }
            if(flagreg&(flag_in_digits|flag_in_decplcdigits|flag_in_exp_digits)){
                if(flagreg&flag_minus_mantis){
                    mantisVal*=(-1.0);
                }
                if(flagreg&flag_minus_exp){
                    exponVal*=(-1);
                }
                exponVal-=NumOfDecplcDig;
                mantisVal*=pow(10,exponVal);
                DlAppend(&returnDynlistp,&mantisVal,sizeof(float),ListType_float);
                flagreg=0;
                NumOfDecplcDig=0;
                mantisVal=0;
                exponVal=0;
            }
        }else if(matchIndex==match_res_nummatch_dseperator){
            if(flagreg&flag_in_digits){
                flagreg|=flag_in_decplcdigits;         //enable in_decplcdigits flag
                flagreg&= ~(flag_in_digits);    //disable in_digits flag
            }else if(flagreg==0){               //to handle ".2"
                flagreg|=flag_in_decplcdigits;
                mantisVal=0;
            }else{
                if(flagreg&flag_in_exp_digits){
                    dprintf(DBGT_ERROR,"unexpected decDigSeparator in exponent");
                }else if(flagreg&flag_in_decplcdigits){
                    dprintf(DBGT_ERROR,"more than one decDigSeperator in mantisse");
                }
            }
        }else if(matchIndex==match_res_nummatch_OrderOfMag){
            if(flagreg&(flag_in_digits|flag_in_decplcdigits)){
                flagreg|=flag_orOfMag;
                flagreg&=~(flag_in_digits|flag_in_decplcdigits);    //clear flags
            }else{
                dprintf(DBGT_ERROR,"Unexpected Order of Magnitude Char");
                return 0;
            }
        }else{      //Digit from 0-9
            if(flagreg&flag_in_decplcdigits){
                NumOfDecplcDig++;
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_decplc){
                NumOfDecplcDig++;
                flagreg|=flag_in_decplcdigits;
                flagreg&=~(flag_decplc);
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_in_digits){
                mantisVal*=10;
                mantisVal+=matchIndex;
            }else if(flagreg&flag_orOfMag){
                exponVal=matchIndex;
                flagreg|=flag_in_exp_digits;
                flagreg&=~(flag_orOfMag);
            }else if(flagreg&flag_in_exp_digits){
                exponVal*=10;
                exponVal+=matchIndex;
            }else{
                flagreg|=flag_in_digits;
                mantisVal=matchIndex;
            }
        }
        offsetInString++;
    }

    //finish parsing last float
    if(flagreg&flag_orOfMag){
        dprintf(DBGT_ERROR,"Number ended appruptly after exponent");
        return 0;
    }
    if(flagreg&flag_decplc){
        dprintf(DBGT_ERROR,"Number ended appruptly after DecimalSeperator Char");
        return 0;
    }
    if(flagreg&(flag_in_digits|flag_in_decplcdigits|flag_in_exp_digits)){
        if(flagreg&flag_minus_mantis){
            mantisVal*=(-1.0);
        }
        if(flagreg&flag_minus_exp){
            exponVal*=(-1);
        }
        exponVal-=NumOfDecplcDig;
        mantisVal*=pow(10,exponVal);
        DlAppend(&returnDynlistp,&mantisVal,sizeof(float),ListType_float);
    }
    DlDelete(nummatch);       //TODO fix for all return types
    return returnDynlistp;
}
