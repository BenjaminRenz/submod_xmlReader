#define DONT_DEFINE_PARSERS
#include "xmlReader.h"
#include <stdlib.h>     //for malloc
#include <string.h>     //for memcpy
#include <math.h>       //for pow
#include <debugPrint/debugPrint.h>
#include <mathHelper/mathHelper.h>
#include <stdarg.h>     //for va_args

Dl_CM* _internal_Dl_CM_initFromList(uint32_t argumentCount,...){
    va_list argp;
    va_start(argp, argumentCount);
    Dl_CM* CmDlP=Dl_CM_alloc(argumentCount/2,NULL);
    for(size_t tuple=0;tuple<argumentCount/2;tuple++){
        CmDlP->items[tuple].startChar=va_arg(argp, char32_t);
        CmDlP->items[tuple].endChar=va_arg(argp, char32_t);
    }
    va_end(argp);
    return CmDlP;
}

Dl_MCM* _internal_Dl_MCM_initFromList(uint32_t argumentCount,...){
    va_list argp;
    va_start(argp, argumentCount);
    Dl_MCM* McmDlP=Dl_MCM_alloc(argumentCount,NULL);
    for(size_t McmIdx=0;McmIdx<argumentCount;McmIdx++){
        McmDlP->items[McmIdx]=va_arg(argp,Dl_CM*);
    }
    va_end(argp);
    return McmDlP;
}

Dl_WM* _internal_Dl_WM_initFromList(uint32_t argumentCount,...){
    va_list argp;
    va_start(argp, argumentCount);
    Dl_WM* WmDlP=Dl_WM_alloc(argumentCount,NULL);
    for(size_t WmIdx=0;WmIdx<argumentCount;WmIdx++){
        WmDlP->items[WmIdx]=va_arg(argp,Dl_CM*);
    }
    va_end(argp);
    return WmDlP;
}

Dl_MWM* _internal_Dl_MWM_initFromList(uint32_t argumentCount,...){
    va_list argp;
    va_start(argp, argumentCount);
    Dl_MWM* MwmDlP=Dl_MWM_alloc(argumentCount,NULL);
    for(size_t WmIdx=0;WmIdx<argumentCount;WmIdx++){
        MwmDlP->items[WmIdx]=va_arg(argp,Dl_WM*);
    }
    va_end(argp);
    return MwmDlP;
}

Dl_utf32Char* Dl_utf32Char_fromString(char* inputString){
    if(!inputString){
        return NULL;
    }
    uint32_t stringlength=0;
    while(inputString[stringlength++]){}            //calculate length of string until null termination character
    stringlength-=1;                                //allocate sufficient memory minus termination char
    Dl_utf32Char* outputString=Dl_utf32Char_alloc(stringlength,NULL);
    for(uint32_t index=0;index<stringlength;index++){
        outputString->items[index]=inputString[index];
    }
    return outputString;
}

char* Dl_utf32Char_toStringAlloc(Dl_utf32Char* utf32Input){
    if(!utf32Input){
        dprintf(DBGT_ERROR,"list ptr empty");
        return 0;
    }
    char* outstring=(char*)malloc(sizeof(char)*(utf32Input->itemcnt+1));
    utf32CutASCII(utf32Input->items,utf32Input->itemcnt,outstring);
    return outstring;
}

char* Dl_utf32Char_toStringAlloc_freeArg1(Dl_utf32Char* utf32Input){
    char* returnString=Dl_utf32Char_toStringAlloc(utf32Input);
    Dl_utf32Char_delete(utf32Input);
    return returnString;
}

void Dl_utf32Char_print(Dl_utf32Char* utf32Input){
    if(!utf32Input){
        printf("Error: NullPtr was passed to print UTF32Dynlist\n");
    }
    char* asciiString=Dl_utf32Char_toStringAlloc(utf32Input);
    printf("%s\n",asciiString);
    free(asciiString);
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

int Dl_CM_MatchSingleCharacter(Dl_utf32Char* StringInUtf32DlP,uint32_t* offsetInStringP,Dl_CM* CmDlP){
    for(size_t CmIdx=0;CmIdx<CmDlP->itemcnt;CmIdx++){
        if(CmDlP->items[CmIdx].startChar<=StringInUtf32DlP->items[*offsetInStringP] &&
           CmDlP->items[CmIdx].endChar  >=StringInUtf32DlP->items[*offsetInStringP]){
            return CmIdx;
        }
    }
    return -1;
}

int Dl_CM_matchAndInc(Dl_utf32Char* StringInUtf32DlP, uint32_t* offsetInStringP, Dl_CM* breakIfMatchDlP, Dl_CM* skipIfMatchDlP){
    int returnVal;
    if((returnVal=Dl_CM_match(StringInUtf32DlP, offsetInStringP, breakIfMatchDlP, skipIfMatchDlP))<0){
        return returnVal;
    }
    (*offsetInStringP)++;
    return returnVal;
}

int Dl_CM_match(Dl_utf32Char* StringInUtf32DlP,uint32_t* offsetInStringP, Dl_CM* breakIfMatchDlP, Dl_CM* skipIfMatchDlP){
    while((*offsetInStringP)<StringInUtf32DlP->itemcnt){
        if(breakIfMatchDlP){
            int ret;
            if((ret=Dl_CM_MatchSingleCharacter(StringInUtf32DlP,offsetInStringP,breakIfMatchDlP))>-1){
                return ret;
            }
        }
        if(skipIfMatchDlP){  //last character was not inside breakIfMatchDlP, check if we are allowed to skip over characters
            if(Dl_CM_MatchSingleCharacter(StringInUtf32DlP,offsetInStringP,skipIfMatchDlP)>-1){ //check if the character matches out skip characters
                (*offsetInStringP)++; //shift offset to prepare to read next character
            }else{
                break;   //character not skippable
            }
        }else{
            break;      //no skipping allowed -> missmatch
        }
    }
    return -1; //String ended or missmatch
}


int Dl_MCM_matchAndInc(Dl_utf32Char* StringInUtf32DlP, uint32_t* offsetInStringP, Dl_MCM* breakIfMatchDlP, Dl_CM* skipIfMatchDlP){
    int returnVal;
    if((returnVal=Dl_MCM_match(StringInUtf32DlP, offsetInStringP, breakIfMatchDlP, skipIfMatchDlP))<0){
        return returnVal;
    }
    (*offsetInStringP)++;
    return returnVal;
}

int Dl_MCM_match(Dl_utf32Char* StringInUtf32DlP,uint32_t* offsetInStringP, Dl_MCM* breakIfMatchDlP, Dl_CM* skipIfMatchDlP){
    while((*offsetInStringP)<StringInUtf32DlP->itemcnt){
        if(breakIfMatchDlP){
            for(size_t McmIdx=0;McmIdx<breakIfMatchDlP->itemcnt;McmIdx++){        //iterate over sub char match lists
                if(Dl_CM_MatchSingleCharacter(StringInUtf32DlP,offsetInStringP,breakIfMatchDlP->items[McmIdx])>-1){
                    return McmIdx;
                }
            }
        }
        if(skipIfMatchDlP){  //last character was not inside breakIfMatchDlP, check if we are allowed to skip over characters
            if(Dl_CM_MatchSingleCharacter(StringInUtf32DlP,offsetInStringP,skipIfMatchDlP)>-1){ //check if the character matches out skip characters
                (*offsetInStringP)++; //shift offset to prepare to read next character
            }else{
                break;   //character not skippable
            }
        }else{
            break;      //no skipping allowed -> missmatch
        }
    }
    return -1; //String ended or missmatch
}


int Dl_WM_matchAndInc(Dl_utf32Char* StringInUtf32DlP, uint32_t* offsetInStringP, Dl_WM* breakIfMatchDlP, Dl_CM* skipIfMatchDlP){
    int returnVal;
    if((returnVal=Dl_WM_match(StringInUtf32DlP, offsetInStringP, breakIfMatchDlP, skipIfMatchDlP))<0){
        return returnVal;
    }
    (*offsetInStringP)+=breakIfMatchDlP->items[returnVal]->itemcnt;
    return returnVal;
}

int Dl_WM_match(Dl_utf32Char* StringInUtf32DlP,uint32_t* offsetInStringP, Dl_WM* breakIfMatchDlP, Dl_CM* skipIfMatchDlP){
    while((*offsetInStringP)<StringInUtf32DlP->itemcnt){
        if(breakIfMatchDlP){
            for(size_t WmIdx=0;WmIdx<breakIfMatchDlP->itemcnt;WmIdx++){ //iterate over sub char match lists in each word
                uint32_t offsetCopy=(*offsetInStringP)+WmIdx;
                if(offsetCopy>=StringInUtf32DlP->itemcnt){
                    break;
                }
                if(Dl_CM_MatchSingleCharacter(StringInUtf32DlP,&offsetCopy,breakIfMatchDlP->items[WmIdx])>-1){
                    if(WmIdx == (breakIfMatchDlP->itemcnt-1)){ //check if this is the last letter in word
                        return 0;
                    }
                }else{
                    break;
                }
            }
        }
        if(skipIfMatchDlP){  //last character was not inside breakIfMatchDlP, check if we are allowed to skip over characters
            if(Dl_CM_MatchSingleCharacter(StringInUtf32DlP,offsetInStringP,skipIfMatchDlP)>-1){ //check if the character matches out skip characters
                (*offsetInStringP)++; //shift offset to prepare to read next character
            }else{
                break;   //character not skippable
            }
        }else{
            break;      //no skipping allowed -> missmatch
        }
    }
    return -1; //String ended or missmatch
}

int Dl_MWM_matchAndInc(Dl_utf32Char* StringInUtf32DlP, uint32_t* offsetInStringP, Dl_MWM* breakIfMatchDlP, Dl_CM* skipIfMatchDlP){
    int returnVal;
    if((returnVal=Dl_MWM_match(StringInUtf32DlP, offsetInStringP, breakIfMatchDlP, skipIfMatchDlP))<0){
        return returnVal;
    }
    (*offsetInStringP)+=breakIfMatchDlP->items[returnVal]->itemcnt;
    return returnVal;
}

int Dl_MWM_match(Dl_utf32Char* StringInUtf32DlP,uint32_t* offsetInStringP, Dl_MWM* breakIfMatchDlP, Dl_CM* skipIfMatchDlP){
    while((*offsetInStringP)<StringInUtf32DlP->itemcnt){
        if(breakIfMatchDlP){
            for(size_t MwmIdx=0;MwmIdx<breakIfMatchDlP->itemcnt;MwmIdx++){
                Dl_WM* breakIfWmDlP=breakIfMatchDlP->items[MwmIdx];
                for(size_t WmIdx=0;WmIdx<breakIfWmDlP->itemcnt;WmIdx++){ //iterate over sub char match lists in each word
                    uint32_t offsetCopy=(*offsetInStringP)+WmIdx;
                    if(offsetCopy>=StringInUtf32DlP->itemcnt){
                        break;
                    }
                    if(Dl_CM_MatchSingleCharacter(StringInUtf32DlP,&offsetCopy,breakIfWmDlP->items[WmIdx])>-1){
                        if(WmIdx == (breakIfWmDlP->itemcnt-1)){ //check if this is the last letter in word
                            return MwmIdx;
                        }
                    }else{
                        break;
                    }
                }
            }
        }
        if(skipIfMatchDlP){  //last character was not inside breakIfMatchDlP, check if we are allowed to skip over characters
            if(Dl_CM_MatchSingleCharacter(StringInUtf32DlP,offsetInStringP,skipIfMatchDlP)>-1){ //check if the character matches out skip characters
                (*offsetInStringP)++; //shift offset to prepare to read next character
            }else{
                break;   //character not skippable
            }
        }else{
            break;      //no skipping allowed -> missmatch
        }
    }
    return -1; //String ended or missmatch
}

Dl_utf32Char* Dl_utf32Char_stripOuterSpaces(Dl_utf32Char* utf32StringDlP){
    Dl_CM* temp_CM_NonSpaceChar=Dl_CM_initFromList(0x21,0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff);
    Dl_CM* temp_CM_SpaceChar=Dl_CM_initFromList(0x9,0x9, 0xd,0xd, 0xa,0xa, 0x20,0x20);
    uint32_t firstNonSpaceChar=0;
    Dl_CM_match(utf32StringDlP,&firstNonSpaceChar,temp_CM_NonSpaceChar,temp_CM_SpaceChar);
    uint32_t lastNonSpaceChar;
    for(lastNonSpaceChar=utf32StringDlP->itemcnt-1;firstNonSpaceChar<=lastNonSpaceChar;lastNonSpaceChar--){
        if((-1)==Dl_CM_MatchSingleCharacter(utf32StringDlP,&lastNonSpaceChar,temp_CM_SpaceChar)){
            lastNonSpaceChar+=1;
            break;  //if something different from a space character is found this is our new last character
        }
    }
    Dl_CM_delete(temp_CM_NonSpaceChar);
    Dl_CM_delete(temp_CM_SpaceChar);
    char32_t* srcP=&(utf32StringDlP->items[firstNonSpaceChar]);
    Dl_utf32Char* outputString=Dl_utf32Char_alloc(lastNonSpaceChar-firstNonSpaceChar,srcP);
    return outputString;
}

Dl_utf32Char* Dl_utf32Char_stripOuterSpaces_freeArg1(Dl_utf32Char* utf32StringDlP){
    Dl_utf32Char* outputString=Dl_utf32Char_stripOuterSpaces(utf32StringDlP);
    Dl_utf32Char_delete(utf32StringDlP);
    return outputString;
}


#define CreateIntegerTypeTo_Dl_utf32Char_Parser(name,type)                                                                  \
Dl_##name* Dl_utf32Char_to_##name##_freeArg1(Dl_CM* NumSepP,Dl_utf32Char* InputString){                                     \
    Dl_##name* returnDlP=Dl_##name##_alloc(0,NULL);                                                                         \
    uint32_t offsetInString=0;                                                                                              \
    double mantisVal=0;                                                                                                     \
    enum {                                                                                                                  \
         flag_in_digits=        (1<<0),                                                                                     \
         flag_minus_mantis=     (1<<5)                                                                                      \
    };                                                                                                                      \
    int32_t flagreg=0;                                                                                                      \
    int32_t exponVal=0;                                                                                                     \
    enum {                                                                                                                  \
        match_res_nummatch_sign=        10,                                                                                 \
        match_res_nummatch_nseparator=  11,                                                                                 \
        match_res_nummatch_illegal=     12};                                                                                \
                                                                                                                            \
                                                                                                                            \
    Dl_MCM* nummatch=Dl_MCM_initFromList(                                                                                   \
        Dl_CM_initFromList('0','0'),                                                                                        \
        Dl_CM_initFromList('1','1'),                                                                                        \
        Dl_CM_initFromList('2','2'),                                                                                        \
        Dl_CM_initFromList('3','3'),                                                                                        \
        Dl_CM_initFromList('4','4'),                                                                                        \
        Dl_CM_initFromList('5','5'),                                                                                        \
        Dl_CM_initFromList('6','6'),                                                                                        \
        Dl_CM_initFromList('7','7'),                                                                                        \
        Dl_CM_initFromList('8','8'),                                                                                        \
        Dl_CM_initFromList('9','9'),                                                                                        \
        Dl_CM_initFromList('-','-'),                                                                                        \
        NumSepP,                                                                                                            \
        Dl_CM_initFromList(0x00,0xffff)                                                                                     \
    );                                                                                                                      \
    while(offsetInString<InputString->itemcnt){                                                                             \
        int matchIndex=Dl_MCM_matchAndInc(InputString,&offsetInString,nummatch,0);                                          \
        switch(matchIndex){                                                                                                 \
            case match_res_nummatch_illegal:                                                                                \
                dprintf(DBGT_ERROR,"Illegal Character in inputstring");                                                     \
                return 0;                                                                                                   \
            break;                                                                                                          \
            case match_res_nummatch_sign:                                                                                   \
                if(flagreg==0){                                                                                             \
                    flagreg|=flag_minus_mantis;                                                                             \
                }else{                                                                                                      \
                    dprintf(DBGT_ERROR,"Unexpected Sign Char");                                                             \
                    return 0;                                                                                               \
                }                                                                                                           \
            break;                                                                                                          \
            case match_res_nummatch_nseparator:                                                                             \
                if(flagreg&flag_in_digits){                                                                                 \
                    if(flagreg&flag_minus_mantis){                                                                          \
                        mantisVal*=(-1);                                                                                    \
                    }                                                                                                       \
                    mantisVal*=pow(10,exponVal);                                                                            \
                    type retVal=(type)mantisVal;                                                                            \
                    Dl_##name##_append(returnDlP,1,&retVal);                                                                \
                    flagreg=0;                                                                                              \
                    mantisVal=0;                                                                                            \
                }                                                                                                           \
            break;                                                                                                          \
            default:                                                                                                        \
                flagreg|=flag_in_digits;                                                                                    \
                mantisVal*=10;                                                                                              \
                mantisVal+=matchIndex;                                                                                      \
            break;                                                                                                          \
        }                                                                                                                   \
    }                                                                                                                       \
    if(flagreg&flag_in_digits){                                                                                             \
        if(flagreg&flag_minus_mantis){                                                                                      \
            mantisVal*=(-1);                                                                                                \
        }                                                                                                                   \
        type retVal=(type)mantisVal;                                                                                        \
        Dl_##name##_append(returnDlP,1,&retVal);                                                                            \
    }                                                                                                                       \
    Dl_MCM_delete(nummatch);                                                                                                \
    return returnDlP;                                                                                                       \
}                                                                                                                           \
Dl_##name* Dl_utf32Char_to_##name(Dl_CM* NumSepP,Dl_utf32Char* InputString){                                                \
    return Dl_utf32Char_to_##name##_freeArg1(Dl_CM_shallowCopy(NumSepP),InputString);                                       \
}




#define CreateFloatingPointTypeTo_Dl_utf32Char_Parser(name,type)                                                            \
Dl_##name* Dl_utf32Char_to_##name##_freeArg123(Dl_CM* NumSepP,Dl_CM* OrdOfMagP, Dl_CM* DecSepP,Dl_utf32Char* InputString){  \
    Dl_##name* returnDlP=Dl_##name##_alloc(0,NULL);                                                                         \
    uint32_t offsetInString=0;                                                                                              \
    double mantisVal=0;                                                                                                     \
    enum{flag_in_digits=        (1<<0),                                                                                     \
         flag_decplc=           (1<<1),                                                                                     \
         flag_in_decplcdigits=  (1<<2),                                                                                     \
         flag_orOfMag=          (1<<3),                                                                                     \
         flag_in_exp_digits=    (1<<4),                                                                                     \
         flag_minus_mantis=     (1<<5),                                                                                     \
         flag_minus_exp=        (1<<6)                                                                                      \
         };                                                                                                                 \
    int32_t flagreg=0;                                                                                                      \
    int32_t NumOfDecplcDig=0;                                                                                               \
    int32_t exponVal=0;                                                                                                     \
    enum {                                                                                                                  \
        match_res_nummatch_sign=        10,                                                                                 \
        match_res_nummatch_nseparator=  11,                                                                                 \
        match_res_nummatch_dseperator=  12,                                                                                 \
        match_res_nummatch_OrderOfMag=  13,                                                                                 \
        match_res_nummatch_illegal=     14                                                                                  \
    };                                                                                                                      \
    Dl_MCM* nummatch=Dl_MCM_initFromList(                                                                                   \
        Dl_CM_initFromList('0','0'),                                                                                        \
        Dl_CM_initFromList('1','1'),                                                                                        \
        Dl_CM_initFromList('2','2'),                                                                                        \
        Dl_CM_initFromList('3','3'),                                                                                        \
        Dl_CM_initFromList('4','4'),                                                                                        \
        Dl_CM_initFromList('5','5'),                                                                                        \
        Dl_CM_initFromList('6','6'),                                                                                        \
        Dl_CM_initFromList('7','7'),                                                                                        \
        Dl_CM_initFromList('8','8'),                                                                                        \
        Dl_CM_initFromList('9','9'),                                                                                        \
        Dl_CM_initFromList('-','-'),                                                                                        \
        NumSepP,                                                                                                            \
        DecSepP,                                                                                                            \
        OrdOfMagP,                                                                                                          \
        Dl_CM_initFromList(0x00,0xffff)                                                                                     \
    );                                                                                                                      \
    while(offsetInString<InputString->itemcnt){                                                                             \
        int matchIndex=Dl_MCM_matchAndInc(InputString,&offsetInString,nummatch,0);                                          \
        switch(matchIndex){                                                                                                 \
            case match_res_nummatch_illegal:                                                                                \
                dprintf(DBGT_ERROR,"Illegal Character in inputstring");                                                     \
                return 0;                                                                                                   \
            break;                                                                                                          \
            case match_res_nummatch_sign:                                                                                   \
                if(flagreg&flag_orOfMag){                                                                                   \
                    flagreg|=flag_minus_exp;                                                                                \
                }else if(flagreg==0){                                                                                       \
                    flagreg|=flag_minus_mantis;                                                                             \
                }else{                                                                                                      \
                    dprintf(DBGT_ERROR,"Unexpected Sign Char");                                                             \
                    return 0;                                                                                               \
                }                                                                                                           \
            break;                                                                                                          \
            case match_res_nummatch_nseparator:                                                                             \
                if(flagreg&flag_orOfMag){                                                                                   \
                    dprintf(DBGT_ERROR,"Number ended appruptly after exponent");                                            \
                    return 0;                                                                                               \
                }                                                                                                           \
                if(flagreg&flag_decplc){                                                                                    \
                    dprintf(DBGT_ERROR,"Number ended appruptly after DecimalSeperator Char");                               \
                    return 0;                                                                                               \
                }                                                                                                           \
                if(flagreg&(flag_in_digits|flag_in_decplcdigits|flag_in_exp_digits)){                                       \
                    if(flagreg&flag_minus_mantis){                                                                          \
                        mantisVal*=(-1.0);                                                                                  \
                    }                                                                                                       \
                    if(flagreg&flag_minus_exp){                                                                             \
                        exponVal*=(-1);                                                                                     \
                    }                                                                                                       \
                    exponVal-=NumOfDecplcDig;                                                                               \
                    mantisVal*=pow(10,exponVal);                                                                            \
                    type retVal=(type)mantisVal;                                                                            \
                    Dl_##name##_append(returnDlP,1,&retVal);                                                                \
                    flagreg=0;                                                                                              \
                    NumOfDecplcDig=0;                                                                                       \
                    mantisVal=0;                                                                                            \
                    exponVal=0;                                                                                             \
                }                                                                                                           \
            break;                                                                                                          \
            case match_res_nummatch_dseperator:                                                                             \
                if(flagreg&flag_in_digits){                                                                                 \
                    flagreg|=flag_in_decplcdigits;                                                                          \
                    flagreg&= ~(flag_in_digits);                                                                            \
                }else if(flagreg==0){                                                                                       \
                    flagreg|=flag_in_decplcdigits;                                                                          \
                    mantisVal=0;                                                                                            \
                }else{                                                                                                      \
                    if(flagreg&flag_in_exp_digits){                                                                         \
                        dprintf(DBGT_ERROR,"unexpected decDigSeparator in exponent");                                       \
                        exit(1);                                                                                            \
                    }else if(flagreg&flag_in_decplcdigits){                                                                 \
                        dprintf(DBGT_ERROR,"more than one decDigSeperator in mantisse");                                    \
                        exit(1);                                                                                            \
                    }                                                                                                       \
                }                                                                                                           \
            break;                                                                                                          \
            case match_res_nummatch_OrderOfMag:                                                                             \
                if(flagreg&(flag_in_digits|flag_in_decplcdigits)){                                                          \
                    flagreg|=flag_orOfMag;                                                                                  \
                    flagreg&=~(flag_in_digits|flag_in_decplcdigits);                                                        \
                }else{                                                                                                      \
                    dprintf(DBGT_ERROR,"Unexpected Order of Magnitude Char");                                               \
                    exit(1);                                                                                                \
                }                                                                                                           \
            break;                                                                                                          \
            default:                                                                                                        \
                if(flagreg&flag_in_decplcdigits){                                                                           \
                    NumOfDecplcDig++;                                                                                       \
                    mantisVal*=10;                                                                                          \
                    mantisVal+=matchIndex;                                                                                  \
                }else if(flagreg&flag_decplc){                                                                              \
                    NumOfDecplcDig++;                                                                                       \
                    flagreg|=flag_in_decplcdigits;                                                                          \
                    flagreg&=~(flag_decplc);                                                                                \
                    mantisVal*=10;                                                                                          \
                    mantisVal+=matchIndex;                                                                                  \
                }else if(flagreg&flag_in_digits){                                                                           \
                    mantisVal*=10;                                                                                          \
                    mantisVal+=matchIndex;                                                                                  \
                }else if(flagreg&flag_orOfMag){                                                                             \
                    exponVal=matchIndex;                                                                                    \
                    flagreg|=flag_in_exp_digits;                                                                            \
                    flagreg&=~(flag_orOfMag);                                                                               \
                }else if(flagreg&flag_in_exp_digits){                                                                       \
                    exponVal*=10;                                                                                           \
                    exponVal+=matchIndex;                                                                                   \
                }else{                                                                                                      \
                    flagreg|=flag_in_digits;                                                                                \
                    mantisVal=matchIndex;                                                                                   \
                }                                                                                                           \
            break;                                                                                                          \
        }                                                                                                                   \
    }                                                                                                                       \
    if(flagreg&flag_orOfMag){                                                                                               \
        dprintf(DBGT_ERROR,"Number ended appruptly after exponent");                                                        \
        exit(1);                                                                                                            \
    }                                                                                                                       \
    if(flagreg&flag_decplc){                                                                                                \
        dprintf(DBGT_ERROR,"Number ended appruptly after DecimalSeperator Char");                                           \
        exit(1);                                                                                                            \
    }                                                                                                                       \
    if(flagreg&(flag_in_digits|flag_in_decplcdigits|flag_in_exp_digits)){                                                   \
        if(flagreg&flag_minus_mantis){                                                                                      \
            mantisVal*=(-1.0);                                                                                              \
        }                                                                                                                   \
        if(flagreg&flag_minus_exp){                                                                                         \
            exponVal*=(-1);                                                                                                 \
        }                                                                                                                   \
        exponVal-=NumOfDecplcDig;                                                                                           \
        mantisVal*=pow(10,exponVal);                                                                                        \
        type retVal=(type)mantisVal;                                                                                        \
        Dl_##name##_append(returnDlP,1,&retVal);                                                                            \
    }                                                                                                                       \
    Dl_MCM_delete(nummatch);                                                                                                \
    return returnDlP;                                                                                                       \
}                                                                                                                           \
Dl_##name* Dl_utf32Char_to_##name(Dl_CM* NumSepP,Dl_CM* OrdOfMagP, Dl_CM* DecSepP,Dl_utf32Char* InputString){               \
    return Dl_utf32Char_to_##name##_freeArg123(Dl_CM_shallowCopy(NumSepP), Dl_CM_shallowCopy(OrdOfMagP), Dl_CM_shallowCopy(DecSepP), InputString); \
}


CreateIntegerTypeTo_Dl_utf32Char_Parser(int32,int32_t);
CreateIntegerTypeTo_Dl_utf32Char_Parser(int64,int64_t);
CreateFloatingPointTypeTo_Dl_utf32Char_Parser(float,float);
CreateFloatingPointTypeTo_Dl_utf32Char_Parser(double,double);
