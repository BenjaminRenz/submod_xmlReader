#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h> //for memcopy
#include "xmlReader.h"
#include "xmlReader_matchHelper.h"
#include "debug.h"
enum {  ////errorcodes
        error_unexpected_eof                =-1,
        error_unknown_filelength            =-2,
        error_zerolength_file               =-3,
        error_vinfo_not_in_xmldecl          =-4,
        internal_error_wrong_argument_type  =-5,
        error_comment_doubleminus           =-6,
        error_illegal_char                  =-7,
        error_malformed_name                =-8,
        error_unexpected_docdecl            =-9
};

int match_xmldecl(struct DynamicList* xmlFileDyn,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults);
int match_doctypedecl(struct DynamicList* xmlFileDyn,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults);
int parseAttrib(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults,struct DynamicList* MWM_EndTagDlP);

enum{misc_ret_charLeft=0, misc_ret_eof=1};
int match_misc(struct DynamicList* xmlFileDyn,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults);

void match_element(struct DynamicList* xmlFileDyn,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults);
/* Notes of some quirks present in xml:
-there can be text outside the tags which should not be interpreted
*/


int readXML(FILE* xmlFile,struct xmlTreeElement** returnDocumentRootpp){
    //Preprocess
    fseek(xmlFile,0,SEEK_END);
    int numOfCharsUTF8 = ftell(xmlFile);
    if(numOfCharsUTF8<0){
        printf("Error: Could not determine file size, abort.\n");
        return error_unknown_filelength;
    }else if(numOfCharsUTF8==0){
        printf("Error: File has length zero, abort.\n");
        return error_zerolength_file;
    }
    fseek(xmlFile,0,SEEK_SET);
    uint8_t* utf8Buffer=(uint8_t*) malloc(sizeof(uint8_t)*numOfCharsUTF8);
    fread(utf8Buffer,1,numOfCharsUTF8,xmlFile);
    //utf conversion
    uint32_t* utf32Buffer=(uint32_t*) malloc(sizeof(uint32_t)*numOfCharsUTF8);     //potentially to big if we have utf8 characters that span over more than one byte
    uint32_t numOfCharsUTF32=utf8ToUtf32(utf8Buffer,numOfCharsUTF8,utf32Buffer); //up until this point we didn't know the actual number of utf32 chars because utf8 chars can combine
    free(utf8Buffer);
    struct DynamicList* xmlFileDyn=DlCreate(sizeof(uint32_t),numOfCharsUTF32,dynlisttype_utf32chars);
    memcpy(xmlFileDyn->items,utf32Buffer,sizeof(uint32_t)*numOfCharsUTF32);
    free(utf32Buffer);
    uint32_t offsetInXMLfile=0;
    //create document root
    struct xmlTreeElement* rootDocumentElementp=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
    //very important because otherwise append_dynamiclist could realloc on uninitialized location
    rootDocumentElementp->parent=0;
    rootDocumentElementp->name=DlCreate(sizeof(uint32_t),0,dynlisttype_utf32chars);
    rootDocumentElementp->type=xmltype_tag;
    rootDocumentElementp->content=DlCreate(sizeof(uint32_t),0,dynlisttype_xmlELMNTCollectionp);
    rootDocumentElementp->attributes=DlCreate(sizeof(struct key_val_pair*),0,dynlisttype_keyValuePairsp);
    init_matchlists();  //initialize matchHelper
    match_loop(xmlFileDyn,&offsetInXMLfile,rootDocumentElementp);
    (*returnDocumentRootpp)=rootDocumentElementp;
    return 0;
}

int match_loop(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){
    int skipSpaces=0;
    while(1){
        enum {  MWMstart_res_XMLDecl_start =0,MWMstart_res_doctype_start =1,MWMstart_res_element_end   =2,
                MWMstart_res_element_start =3,MWMstart_res_pi_start      =4,MWMstart_res_comment_start =5,MWMstart_res_IllegalChar   =6};
        switch(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,MWM_start,0)){
            case MWMstart_res_XMLDecl_start:
                parseAttrib(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResults,WM_pi_end);
                struct DynamicList* XMLDeclAttribDlP=ObjectToAttachResults->attributes;
                //TODO check if the first attribute is a valid version information
            break;
            case MWMstart_res_doctype_start:
                parseDoctype(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResults);
            break;
            case MWMstart_res_pi_start:
                parsePi(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResults);
            break;
            case MWMstart_res_comment_start:
                parseComment(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResults);
            break;
            case MWMstart_res_element_end:
                //TODO check if closing element actually matches
                (*offsetInXMLfilep)+=ObjectToAttachResults->name->itemcnt+1;   //we have matched the first name character
                ObjectToAttachResults=ObjectToAttachResults->parent;
            break;
            case MWMstart_res_element_start:;
                struct xmlTreeElement* newXmlElementP=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
                newXmlElementP->attributes=DlCreate(sizeof(struct key_val_pair*),0,dynlisttype_keyValuePairsp);
                newXmlElementP->name=DlCreate(sizeof(uint32_t),0,dynlisttype_utf32chars);
                newXmlElementP->parent=ObjectToAttachResults;
                newXmlElementP->type=xmltype_tag;
                newXmlElementP->content=DlCreate(sizeof(struct xmlTreeElement*),0,dynlisttype_xmlELMNTCollectionp);
                DlAppend(sizeof(struct xmlTreeElement),&(ObjectToAttachResults->content),newXmlElementP,dynlisttype_xmlELMNTCollectionp);
                ObjectToAttachResults=newXmlElementP;
                enum{MWMres_el_etag_nonEmpty=0,MWMres_el_etag_empty=1};
                switch(parseNameAndAttrib(xmlFileDlP,offsetInXMLfilep,newXmlElementP,MWM_element_end)){
                    case MWMres_el_etag_nonEmpty:
                        dprintf(DBGT_INFO,"Now one layer deeper");
                    break;
                    case MWMres_el_etag_empty:
                        ObjectToAttachResults=ObjectToAttachResults->parent;
                    break;
                    default:
                        dprintf(DBGT_ERROR,"An error occured while parsing an element start or end tag");
                    break;
                }
            break;
            case MWMstart_res_IllegalChar:
                dprintf(DBGT_ERROR,"Illegal character found in xml, offset=%d",(*offsetInXMLfilep));
            break;
            default:
                //no tag found, so continue to process character data
                //TODO in case we ignore space, set
                if(skipSpaces){
                    Match(xmlFileDlP,offsetInXMLfilep,0,CM_SpaceChar);
                }
                uint32_t CharDataStartOffset=(*offsetInXMLfilep);
                //Move to the last character which is chardata
                int match_res=Match(xmlFileDlP,offsetInXMLfilep,CM_LessThanChar,CM_CharData);
                uint32_t CharDataEndOffset=(*offsetInXMLfilep);
                if(skipSpaces){ //cut trailing space characters
                    while(Match(xmlFileDlP,&CharDataEndOffset,CM_SpaceChar,0)>=0){
                        CharDataEndOffset--;
                    }
                }
                if(CharDataStartOffset<CharDataEndOffset){
                    //there was valid chardata
                    struct xmlTreeElement* newCharDataElementP=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
                    dprintf(DBGT_INFO,"Creating new xml element for chardata of length %d",CharDataEndOffset-CharDataStartOffset);
                    newCharDataElementP->attributes=DlCreate(sizeof(struct key_val_pair*),0,dynlisttype_keyValuePairsp);
                    newCharDataElementP->name=DlCreate(sizeof(uint32_t),0,dynlisttype_utf32chars);
                    newCharDataElementP->parent=ObjectToAttachResults;
                    newCharDataElementP->type=xmltype_chardata;
                    struct DynamicList* tempDynlistDlP=DlCreate(sizeof(uint32_t),CharDataEndOffset-CharDataStartOffset,dynlisttype_utf32chars);
                    newCharDataElementP->content=tempDynlistDlP;
                    memcpy(newCharDataElementP->content->items,(uint32_t*)(xmlFileDlP->items)+CharDataStartOffset,sizeof(uint32_t)*(CharDataEndOffset-CharDataStartOffset));
                    printUTF32Dynlist(newCharDataElementP->content);
                    DlAppend(sizeof(struct xmlTreeElement),&(ObjectToAttachResults->content),newCharDataElementP,dynlisttype_xmlELMNTCollectionp);
                }
                if(match_res<0){    //no new opening tag found
                    if((*offsetInXMLfilep)>=xmlFileDlP->itemcnt){
                        if(ObjectToAttachResults->parent!=0){
                            dprintf(DBGT_ERROR,"Unexpected EOF during maching");
                        }else{
                            dprintf(DBGT_INFO,"Finished parsing");
                            return 0;
                        }
                    }else{
                        dprintf(DBGT_ERROR,"Unexpected character during parsing");
                    }
                }
            break;
        }
    }
}

int parseXMLDecl(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){
    if(ObjectToAttachResults->parent!=0){//this is not the root element, the doctype tag is not allowed to appear here
        dprintf(DBGT_ERROR,"unexpected <?xml");
    }
    if(ObjectToAttachResults->attributes->itemcnt!=0){
        dprintf(DBGT_ERROR,"<?xml occured more than one or after Doctype");
    }
    ObjectToAttachResults->name=DlCombine_freeArg12(sizeof(uint32_t),ObjectToAttachResults->name,stringToUTF32Dynlist("xml"));
    parseAttrib(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResults,WM_XMLDecl_end);
}

int parseDoctype(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){

}

//return indicates unexpected eof
int parseNameAndAttrib(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults,struct DynamicList* MWM_EndTagDlP){
    uint32_t nameStartOffset=(*offsetInXMLfilep)-1; //the previous matcher also checked for a valid name start character
    dprintf(DBGT_INFO,"nameStartOffset: %d",nameStartOffset);
    int matchResult=Match(xmlFileDlP,offsetInXMLfilep,MWM_EndTagDlP,CM_NameChar);
    uint32_t nameEndOffset=(*offsetInXMLfilep);
    dprintf(DBGT_INFO,"nameEndOffset: %d",nameEndOffset);
    if((matchResult<0) && (MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,CM_SpaceChar,0)<0)){        //there must be a space, after the element if attributes follow
        dprintf(DBGT_ERROR,"Invalid character in element name");
        return -1;
    }else{
        uint32_t lengthOfApplyingEndTag=((struct DynamicList**)(MWM_EndTagDlP->items))[matchResult]->itemcnt;
        (*offsetInXMLfilep)+=lengthOfApplyingEndTag;
    }
    struct DynamicList* nameDlP=DlCreate(sizeof(uint32_t),nameEndOffset-nameStartOffset,dynlisttype_utf32chars);
    memcpy(nameDlP->items,(uint32_t*)(xmlFileDlP->items)+nameStartOffset,sizeof(uint32_t)*(nameEndOffset-nameStartOffset));
    printf("Found element with name: ");
    printUTF32Dynlist(nameDlP);
    dprintf(DBGT_INFO,"length: %d",nameDlP->itemcnt);
    printf("\n");
    ObjectToAttachResults->name=DlCombine_freeArg12(sizeof(uint32_t),ObjectToAttachResults->name,nameDlP);
    if(matchResult>=0){
        return matchResult;
    }else{
        return parseAttrib(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResults,MWM_EndTagDlP);
    }
}

int parseAttrib(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults,struct DynamicList* MWM_EndTagDlP){
    while(1){
        //check for key startCharacter
        uint32_t attKeyEndOffset;
        int matchRes=MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,MWM_EndTagDlP,CM_SpaceChar);
        uint32_t attKeyStartOffset=(*offsetInXMLfilep);
        if(matchRes<0){
            if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,CM_NameStartChar,0)<0){
                dprintf(DBGT_ERROR,"unexpected character as first attribute");
                return -1;
            }
        }else{
            dprintf(DBGT_INFO,"Attribute was closed");
            return matchRes;
        }
        dprintf(DBGT_INFO,"Start parsing next attribute");
        //jump to end of key
        enum {MCMres_pA_Space=0,MCMres_pA_Equals=1};
        struct DynamicList* MCM_AttNameEndOrEqual=createMultiCharMatchList(2,CM_SpaceChar,CM_Equals);
        switch(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,MCM_AttNameEndOrEqual,CM_NameChar)){
            case MCMres_pA_Space:
                attKeyEndOffset=(*offsetInXMLfilep)-1;
                //move up to the = character
                if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,CM_Equals,CM_SpaceChar)!=0){
                    dprintf(DBGT_ERROR,"Unexpected character after attribute name");
                }
            break;
            case MCMres_pA_Equals:
                attKeyEndOffset=(*offsetInXMLfilep)-1;
            break;
            default:
                dprintf(DBGT_ERROR,"unexpected character as first attribute");
            break;
        }
        //write key
        struct DynamicList* keyDlP=DlCreate(sizeof(uint32_t),attKeyEndOffset-attKeyStartOffset,dynlisttype_utf32chars);
        memcpy(keyDlP->items,(uint32_t*)(xmlFileDlP->items)+attKeyStartOffset,sizeof(uint32_t)*(attKeyEndOffset-attKeyStartOffset));

        //jump to start of value
        uint32_t attValStartOffset;
        uint32_t attValEndOffset;
        enum {MCMres_pA_QuoteSingle=0,MCMres_pA_QuoteDouble=1};
        switch(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,MCM_Quotes,CM_SpaceChar)){
            case MCMres_pA_QuoteSingle:
                attValStartOffset=(*offsetInXMLfilep);
                if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,CM_QuoteSingle,CM_PubidChar_withoutQuotes)<0){
                    dprintf(DBGT_ERROR,"unexpected character inside attribute key or eof");
                }
                attValEndOffset=(*offsetInXMLfilep)-1;
            break;
            case MCMres_pA_QuoteDouble:
                attValStartOffset=(*offsetInXMLfilep);
                dprintf(DBGT_INFO,"attValStartOffset %d",attValStartOffset);
                if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,CM_QuoteDouble,MCM_PubidChar)<0){
                    dprintf(DBGT_ERROR,"unexpected character inside attribute key or eof");
                }
                attValEndOffset=(*offsetInXMLfilep)-1;
                dprintf(DBGT_INFO,"attValEndOffset %d",attValEndOffset);
            break;
            default:
                dprintf(DBGT_ERROR,"invalid character before attribute key of eof");
            break;
        }
        //write value
        struct DynamicList* valueDlP=DlCreate(sizeof(uint32_t),attValEndOffset-attValStartOffset,dynlisttype_utf32chars);
        memcpy(valueDlP->items,(uint32_t*)(xmlFileDlP->items)+attValStartOffset,sizeof(uint32_t)*(attValEndOffset-attValStartOffset));
        //combine into key value pair and append to xmlTagAttributes
        struct key_val_pair* key_valP=(struct key_val_pair*)malloc(sizeof(struct key_val_pair));
        key_valP->key=keyDlP;
        key_valP->value=valueDlP;
        printf("Found element with key: ");
        printUTF32Dynlist(keyDlP);
        printf("%d",keyDlP->itemcnt);
        printf("\tvalue: ");
        printUTF32Dynlist(valueDlP);
        printf("\n");
        DlAppend(sizeof(struct key_val_pair*),&(ObjectToAttachResults->attributes),key_valP,dynlisttype_keyValuePairsp);
    }
}

int parseComment(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){
    uint32_t commentStartOffset=(*offsetInXMLfilep);
    struct xmlTreeElement* Comment=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
    Comment->type=xmltype_comment;
    Comment->name=DlCreate(sizeof(uint32_t*),0,dynlisttype_utf32chars);
    Comment->attributes=DlCreate(sizeof(struct key_val_pair*),0,dynlisttype_keyValuePairsp);
    Comment->parent=ObjectToAttachResults;
    if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,WM_comment_end,CM_AnyChar)){   //unexpected eof
        dprintf(DBGT_ERROR,"Unexpected end of file while scanning comment, start tag for comment on offset %d",commentStartOffset);
    }
    uint32_t commentEndOffset=(*offsetInXMLfilep)-WM_comment_end->itemcnt;
    struct DynamicList* CommentCDataDlP=DlCreate(sizeof(uint32_t),commentEndOffset-commentStartOffset,dynlisttype_utf32chars);
    memcpy(CommentCDataDlP->items,((uint32_t*)xmlFileDlP->items)+commentStartOffset,sizeof(uint32_t)*(commentEndOffset-commentStartOffset));
    Comment->content=CommentCDataDlP;
    DlAppend(sizeof(struct xmlTreeElement*),&(ObjectToAttachResults->content),Comment,dynlisttype_xmlELMNTCollectionp);
}

int parsePi(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){

}

int parseCdata(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){
    if(ObjectToAttachResults->parent==0){
        dprintf(DBGT_ERROR,"CData is only allowed to occur inside tags");
    }
    uint32_t cDataStartOffset=(*offsetInXMLfilep);
    struct xmlTreeElement* CData=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
    CData->type=xmltype_cdata;
    CData->name=DlCreate(sizeof(uint32_t*),0,dynlisttype_utf32chars);
    CData->attributes=DlCreate(sizeof(struct key_val_pair*),0,dynlisttype_keyValuePairsp);
    CData->parent=ObjectToAttachResults;
    if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,WM_cdata_end,CM_AnyChar)){   //unexpected eof
        dprintf(DBGT_ERROR,"Unexpected end of file while scanning comment, start tag for cdata on offset %d",cDataStartOffset);
    }
    uint32_t cDataEndOffset=(*offsetInXMLfilep)-WM_cdata_end->itemcnt;
    struct DynamicList* CDataDlP=DlCreate(sizeof(uint32_t),cDataEndOffset-cDataStartOffset,dynlisttype_utf32chars);
    memcpy(CDataDlP->items,((uint32_t*)xmlFileDlP->items)+cDataStartOffset,sizeof(uint32_t)*(cDataEndOffset-cDataStartOffset));
    CData->content=CDataDlP;
    DlAppend(sizeof(struct xmlTreeElement*),&(ObjectToAttachResults->content),CData,dynlisttype_xmlELMNTCollectionp);
}


//TODO still needed?
struct DynamicList* ClosingTagFromDynList(struct DynamicList* utf32DynListIn){
    struct DynamicList* w_match_list=DlCreate(sizeof(struct DynamicList*),utf32DynListIn->itemcnt+2,ListType_WordMatchp);
    ((struct DynamicList**)w_match_list->items)[0]=createCharMatchList(2,'<','<');
    ((struct DynamicList**)w_match_list->items)[1]=createCharMatchList(2,'/','/');
    for(unsigned int charInTag=0;charInTag<utf32DynListIn->itemcnt;charInTag++){
        ((struct DynamicList**)w_match_list->items)[charInTag+2]=createCharMatchList(2,((uint32_t*)utf32DynListIn->items)[charInTag],((uint32_t*)utf32DynListIn->items)[charInTag]);
    }
    return w_match_list;
}


int writeXML(FILE* xmlOutFile,struct xmlTreeElement* inputDocumentRootP){
    struct DynamicList* utf32XmlDlP=xmlDOMtoUTF32(inputDocumentRootP,4);
    uint8_t* datap=(uint8_t*)malloc(sizeof(uint8_t)*4*utf32XmlDlP->itemcnt);
    uint32_t numUTF8Chars=utf32ToUtf8(utf32XmlDlP->items,utf32XmlDlP->itemcnt,datap);
    #define blocksize 1
    if(fwrite(datap,numUTF8Chars*sizeof(uint8_t),blocksize,xmlOutFile)==blocksize){
        return 0;
    }else{
        printf("Error, could not write file\n");
        return -1;
    }
}


struct DynamicList* xmlDOMtoUTF32(struct xmlTreeElement* startingElement, int numSpacesIndent){
    struct DynamicList* returnUtf32StringDlP=DlCreate(sizeof(uint32_t),0,dynlisttype_utf32chars);
    //TODO parse attributes of DocumentRoots, keep in mind newline is before start tag
    struct xmlTreeElement* LastXMLTreeElement=startingElement->parent;
    struct xmlTreeElement* CurrentXMLTreeElement=startingElement;
    uint32_t currentDepth=0;        //for indentation
    uint32_t subindex=0;            //which child node is currently processed
    int writeOpeningTagToggle=1;
    do{
        uint32_t itemcnt=;
        if(CurrentXMLTreeElement->parent==LastXMLTreeElement){      //walk deeper
            if(writeOpeningTagToggle){//-------write start tag, attributes
                //write changes
                struct DynamicList* spacesDlP=DlCreate(sizeof(uint32_t),1+numSpacesIndent*currentDepth+1,dynlisttype_utf32chars);
                (uint32_t*)(spacesDlP->items)[0]='\n';
                for(uint32_t i=1;i<=numSpacesIndent*currentDepth;i++){
                    (uint32_t*)(spacesDlP->items)[i]=' ';
                }
                tempUTF32String[numSpacesIndent*currentDepth+1]='<';
                returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,spacesDlP);
                returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,CurrentXMLTreeElement->name);
                printf("name: %s\n",utf32dynlist_to_string(CurrentXMLTreeElement->name));
                for(uint32_t attributenum=0; attributenum<CurrentXMLTreeElement->attributes->itemcnt; attributenum++){
                    struct DynamicList* KeyDynlistp=(((struct key_val_pair*)CurrentXMLTreeElement->attributes->items)[attributenum]).key;     //Problematic segfault
                    struct DynamicList* ValDynlistp=(((struct key_val_pair*)CurrentXMLTreeElement->attributes->items)[attributenum]).value;
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist(" "));
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,KeyDynlistp);
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("=\""));
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,ValDynlistp);
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("\""));
                }
                if(CurrentXMLTreeElement->content->itemcnt){            //we have some subtags
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist(">"));
                }else{
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("/>"));
                }
            }//-------write start tag, attributes end
            writeOpeningTagToggle=1;
            #define maxDepthTMP 100
            for(;(subindex<CurrentXMLTreeElement->content->itemcnt)&&(currentDepth<maxDepthTMP);subindex++){        //go over all subelements that are not to deeply nested
                struct xmlTreeElement* xmlSubElement=(struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[subindex]
                switch(xmlSubElement->type){
                    case xmltype_tag:   //walk inside the new child element
                        LastXMLTreeElement=CurrentXMLTreeElement;
                        CurrentXMLTreeElement=((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[subindex];
                        subindex=0;
                        currentDepth++;
                    break;
                    case xmltype_cdata:
                        //test to confirm no < or & characters are inside
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,xmlSubElement->content);
                    break;
                    case xmltype_chardata:
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("<![CDATA["));
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,xmlSubElement->content);
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("]]>"));
                    break;
                    case xmltype_comment:
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("<!--"));
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,xmlSubElement->content);
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("-->"));
                    break;
                    case xmltype_pi:
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("<?"));
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,xmlSubElement->content);
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("?>"));
                    break;
                    default:
                        dprintf(DBGT_ERROR,"Tag of this type is not handeled");
                    break;
                }
            }
        }else{      //go back upward
            for(subindex=0;(subindex<CurrentXMLTreeElement->content->itemcnt)&&(LastXMLTreeElement!=((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[subindex]);subindex++){     //go over all subelements of our parent and search ourself
            }
            currentDepth--;
            if(++subindex<CurrentXMLTreeElement->content->itemcnt){  //Do we have more elements in the current Element, (assume one more element, if this is more than the elements in our parent then we have scanned the last element)
                LastXMLTreeElement=CurrentXMLTreeElement->parent;
            }else{
                {//-------write closing tag
                    struct DynamicList* spacesDlP=DlCreate(sizeof(uint32_t),1+numSpacesIndent*currentDepth+2,dynlisttype_utf32chars);
                    (uint32_t*)(spacesDlP->items)[0]='\n';
                    for(uint32_t i=1;i<=numSpacesIndent*currentDepth;i++){
                        (uint32_t*)(spacesDlP->items)[i]=' ';
                    }
                    (uint32_t*)(spacesDlP->items)[numSpacesIndent*currentDepth+1]='<';
                    (uint32_t*)(spacesDlP->items)[numSpacesIndent*currentDepth+2]='/';
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,CurrentXMLTreeElement->name);
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist(">"));
                }
                subindex=CurrentXMLTreeElement->content->itemcnt;    //a for loop would have set subindex to itemcnt in its last iteration -> further upward is true
            }
        }
        if(subindex==CurrentXMLTreeElement->content->itemcnt){//Turn around/further upward
            writeOpeningTagToggle=0;
            LastXMLTreeElement=CurrentXMLTreeElement;
            CurrentXMLTreeElement=CurrentXMLTreeElement->parent;
        }
    }while((CurrentXMLTreeElement!=0)&&(CurrentXMLTreeElement!=inputDocumentRoot));     //Todo error with abort condition when starting from subelement
}

//TODO does miss the upper layer of elements
void printXMLsubelements(struct xmlTreeElement* xmlElement){
    struct DynamicList* utf32XmlDlP=xmlDOMtoUTF32(xmlElement,4);
    uint8_t* utf8Xml=(uint8_t*)malloc(sizeof(uint8_t)*(4*utf32XmlDlP->itemcnt+1)); //for null term
    uint32_t numUTF8Chars=utf32ToUtf8(utf32XmlDlP->items,utf32XmlDlP->itemcnt,utf8Xml);
    #define blocksize 1
    utf8Xml[numUTF8Chars]='\0';
    printf("%s\n",utf8Xml);
}

*/
