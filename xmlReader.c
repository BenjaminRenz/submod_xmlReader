#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h> //for memcopy
#include "xmlReader/xmlReader.h"
#include "xmlReader/xmlReader_matchHelper.h"
#include "xmlReader/debug.h"
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
    //initialize root xmlElement
    rootDocumentElementp->parent=0;
    rootDocumentElementp->name=DlCreate(sizeof(uint32_t),0,dynlisttype_utf32chars);
    rootDocumentElementp->type=xmltype_docRoot;
    rootDocumentElementp->content=DlCreate(sizeof(struct xmlTreeElement*),0,dynlisttype_xmlELMNTCollectionp);
    rootDocumentElementp->attributes=DlCreate(sizeof(struct key_val_pair),0,dynlisttype_keyValuePairsp);
    init_matchlists();  //initialize matchHelper
    if(match_loop(xmlFileDyn,&offsetInXMLfile,rootDocumentElementp)<0){
        //Error occured, print exact location of offsetInXmlFile
        uint32_t lines=0;
        uint32_t charpos=0;
        uint32_t internaloffset=0;
        while(internaloffset<offsetInXMLfile){
            if(((uint32_t*)xmlFileDyn->items)[internaloffset++]=='\n'){
                lines++;
                charpos=0;
            }else{
                printf("%c",((uint32_t*)xmlFileDyn->items)[internaloffset]);
                charpos++;
            }
        }
        dprintf(DBGT_ERROR,"Error was in line %d, character %d.\n",lines+1,charpos);
        exit(1);
    }
    (*returnDocumentRootpp)=rootDocumentElementp;
    dprintf(DBGT_INFO,"Starting element att itemcnt is %d",rootDocumentElementp->attributes->itemcnt);
    return 0;
}

int match_loop(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResultsP){
    int skipSpaces=0;
    while(1){
        enum {  MWMstart_res_XMLDecl_start =0,MWMstart_res_doctype_start =1,MWMstart_res_element_end   =2,
                MWMstart_res_element_start =3,MWMstart_res_pi_start      =4,MWMstart_res_cdata_start   =5,
                MWMstart_res_comment_start =6,MWMstart_res_IllegalChar   =7};
        switch(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,MWM_start,0)){
            case MWMstart_res_XMLDecl_start:
                parseXMLDecl(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP,WM_pi_end);
                struct DynamicList* XMLDeclAttribDlP=ObjectToAttachResultsP->attributes;
                //TODO check if the first attribute is a valid version information
            break;
            case MWMstart_res_doctype_start:
                parseDoctype(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP);
            break;
            case MWMstart_res_cdata_start:
                parseCdata(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP);
            break;
            case MWMstart_res_pi_start:
                parsePi(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP);
            break;
            case MWMstart_res_comment_start:
                parseComment(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP);
            break;
            case MWMstart_res_element_end:;
                //TODO check if closing element actually matches
                uint32_t startElementName=(*offsetInXMLfilep)-1;
                if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,WM_element_endNonEmpty,CM_NameChar)<0){
                    dprintf(DBGT_ERROR,"unexpected character in end tag");
                    return -1;
                }
                uint32_t endElementName=*offsetInXMLfilep-1;
                struct DynamicList* closedNameDlP=DlCreate(sizeof(uint32_t),endElementName-startElementName,dynlisttype_utf32chars);
                memcpy(closedNameDlP->items,((uint32_t*)xmlFileDlP->items)+startElementName,sizeof(uint32_t)*(endElementName-startElementName));
                if(!compareEqualDynamicUTF32List_freeArg2(ObjectToAttachResultsP->name,closedNameDlP)){
                    dprintf(DBGT_ERROR,"closing tag does not match the one opened before");
                    return -1;
                }
                ObjectToAttachResultsP=ObjectToAttachResultsP->parent;
            break;
            case MWMstart_res_element_start:;
                {   //these brackets are here, so one cannot access newXmlElementDlP after the corresponding memory block was freed
                    struct xmlTreeElement* newXmlElementP=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
                    newXmlElementP->attributes=DlCreate(sizeof(struct key_val_pair),0,dynlisttype_keyValuePairsp);
                    newXmlElementP->name=DlCreate(sizeof(uint32_t),0,dynlisttype_utf32chars);
                    newXmlElementP->parent=ObjectToAttachResultsP;
                    dprintf(DBGT_INFO,"Current object address %x",ObjectToAttachResultsP);
                    newXmlElementP->type=xmltype_tag;
                    newXmlElementP->content=DlCreate(sizeof(struct xmlTreeElement*),0,dynlisttype_xmlELMNTCollectionp);
                    ObjectToAttachResultsP->content=DlAppend(sizeof(struct xmlTreeElement*),ObjectToAttachResultsP->content,&newXmlElementP);
                }
                ObjectToAttachResultsP=((struct DynamicList**)ObjectToAttachResultsP->content->items)[ObjectToAttachResultsP->content->itemcnt-1];
                enum{MWMres_el_etag_nonEmpty=0,MWMres_el_etag_empty=1};
                switch(parseNameAndAttrib(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP,MWM_element_end)){  //TODO check if change from newXmlElementP to ObjectToAttachResults is ok
                    case MWMres_el_etag_nonEmpty:
                        dprintf(DBGT_INFO,"Now one layer deeper");
                    break;
                    case MWMres_el_etag_empty:
                        ObjectToAttachResultsP=ObjectToAttachResultsP->parent;
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
                    struct xmlTreeElement* newCharDataElementP=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));    //get first element of new list
                    dprintf(DBGT_INFO,"Creating new xml element for chardata of length %d",CharDataEndOffset-CharDataStartOffset);
                    newCharDataElementP->attributes=DlCreate(sizeof(struct key_val_pair),0,dynlisttype_keyValuePairsp);
                    newCharDataElementP->name=DlCreate(sizeof(uint32_t),0,dynlisttype_utf32chars);
                    newCharDataElementP->parent=ObjectToAttachResultsP;
                    newCharDataElementP->type=xmltype_chardata;
                    newCharDataElementP->content=DlCreate(sizeof(uint32_t),CharDataEndOffset-CharDataStartOffset,dynlisttype_utf32chars);
                    memcpy(newCharDataElementP->content->items,(uint32_t*)(xmlFileDlP->items)+CharDataStartOffset,sizeof(uint32_t)*(CharDataEndOffset-CharDataStartOffset));
                    //attach new chardata element to parrent
                    ObjectToAttachResultsP->content=DlAppend(sizeof(struct xmlTreeElement*),ObjectToAttachResultsP->content,&newCharDataElementP);
                }else{
                    dprintf(DBGT_ERROR,"unexpected < in file");
                    return -1;
                }
                if(match_res<0){    //no new opening tag found
                    if((*offsetInXMLfilep)>=xmlFileDlP->itemcnt){
                        if(ObjectToAttachResultsP->parent!=0){
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

int parseXMLDecl(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResultsP){
    if(ObjectToAttachResultsP->parent!=0){//this is not the root element, the doctype tag is not allowed to appear here
        dprintf(DBGT_ERROR,"unexpected <?xml");
    }
    if(ObjectToAttachResultsP->attributes->itemcnt!=0){
        dprintf(DBGT_ERROR,"<?xml occured more than one or after Doctype");
    }
    ObjectToAttachResultsP->name=DlCombine_freeArg12(sizeof(uint32_t),ObjectToAttachResultsP->name,stringToUTF32Dynlist("xml"));
    parseAttrib(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP,WM_XMLDecl_end);
}

int parseDoctype(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResultsP){
    dprintf(DBGT_ERROR,"Doctype not implemented!");
    exit(1);
}

//return indicates unexpected eof
int parseNameAndAttrib(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResultsP,struct DynamicList* MWM_EndTagDlP){
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
    dprintf(DBGT_INFO,"Found element with name: %s",utf32dynlist_to_string(nameDlP));
    ObjectToAttachResultsP->name=DlCombine_freeArg1(sizeof(uint32_t),ObjectToAttachResultsP->name,nameDlP);
    if(matchResult>=0){
        return matchResult;
    }else{
        return parseAttrib(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP,MWM_EndTagDlP);
    }
}

int parseAttrib(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResultsP,struct DynamicList* MWM_EndTagDlP){
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
            return matchRes;
        }
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
                if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,CM_QuoteDouble,MCM_PubidChar)<0){
                    dprintf(DBGT_ERROR,"unexpected character inside attribute key or eof");
                }
                attValEndOffset=(*offsetInXMLfilep)-1;
            break;
            default:
                dprintf(DBGT_ERROR,"invalid character before attribute key of eof");
            break;
        }
        //write value
        struct DynamicList* valueDlP=DlCreate(sizeof(uint32_t),attValEndOffset-attValStartOffset,dynlisttype_utf32chars);
        memcpy(valueDlP->items,(uint32_t*)(xmlFileDlP->items)+attValStartOffset,sizeof(uint32_t)*(attValEndOffset-attValStartOffset));
        //combine into key value pair and append to xmlTagAttributes
        struct DynamicList* newAttributeDlP=DlCreate(sizeof(struct key_val_pair),1,dynlisttype_keyValuePairsp);
        struct key_val_pair* key_valP=(struct key_val_pair*)newAttributeDlP->items;
        key_valP->key=keyDlP;
        key_valP->value=valueDlP;
        dprintf(DBGT_INFO,"new key:\t%s\nnew value:\t%s",utf32dynlist_to_string(keyDlP),utf32dynlist_to_string(valueDlP));
        ObjectToAttachResultsP->attributes=DlCombine_freeArg12(sizeof(struct key_val_pair),ObjectToAttachResultsP->attributes,newAttributeDlP);
        dprintf(DBGT_INFO,"new itemcnt of attribute %d",ObjectToAttachResultsP->attributes->itemcnt);
    }
}

int parseComment(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResultsP){
    uint32_t commentStartOffset=(*offsetInXMLfilep);
    struct xmlTreeElement* CommentXmlElementP=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
    CommentXmlElementP->type=xmltype_comment;
    CommentXmlElementP->name=DlCreate(sizeof(uint32_t),0,dynlisttype_utf32chars);
    CommentXmlElementP->attributes=DlCreate(sizeof(struct key_val_pair),0,dynlisttype_keyValuePairsp);
    CommentXmlElementP->parent=ObjectToAttachResultsP;
    if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,WM_comment_end,CM_AnyChar)){   //unexpected eof
        dprintf(DBGT_ERROR,"Unexpected end of file while scanning comment, start tag for comment on offset %d",commentStartOffset);
    }
    uint32_t commentEndOffset=(*offsetInXMLfilep)-WM_comment_end->itemcnt;
    struct DynamicList* CommentCDataDlP=DlCreate(sizeof(uint32_t),commentEndOffset-commentStartOffset,dynlisttype_utf32chars);
    memcpy(CommentCDataDlP->items,((uint32_t*)xmlFileDlP->items)+commentStartOffset,sizeof(uint32_t)*(commentEndOffset-commentStartOffset));
    CommentXmlElementP->content=CommentCDataDlP;
    //Append new xmlElement with type comment to parent
    ObjectToAttachResultsP->content=DlAppend(sizeof(struct xmlTreeElement*),ObjectToAttachResultsP->content,&CommentXmlElementP);
}

int parsePi(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResultsP){
    uint32_t piNameStartOffset=(*offsetInXMLfilep)-1; //the previous matcher also checked for a valid name start character
    int matchResult=Match(xmlFileDlP,offsetInXMLfilep,WM_pi_end,CM_NameChar);
    uint32_t piNameEndOffset=(*offsetInXMLfilep);
    if((matchResult<0) && (MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,CM_SpaceChar,0)<0)){        //there must be a space, after the element if attributes follow
        dprintf(DBGT_ERROR,"Invalid character in pi name");
        return -1;
    }
    struct DynamicList* nameDlP=DlCreate(sizeof(uint32_t),piNameEndOffset-piNameStartOffset,dynlisttype_utf32chars);
    memcpy(nameDlP->items,(uint32_t*)(xmlFileDlP->items)+piNameStartOffset,sizeof(uint32_t)*(piNameEndOffset-piNameStartOffset));
    struct xmlTreeElement* PiXmlElementP=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
    PiXmlElementP->type=xmltype_pi;
    PiXmlElementP->name=nameDlP;
    PiXmlElementP->attributes=DlCreate(sizeof(struct key_val_pair),0,dynlisttype_keyValuePairsp);
    PiXmlElementP->parent=ObjectToAttachResultsP;
    PiXmlElementP->content=DlCreate(sizeof(uint32_t),0,dynlisttype_utf32chars);
    ObjectToAttachResultsP->content=DlAppend(sizeof(struct xmlTreeElement*),ObjectToAttachResultsP->content,&PiXmlElementP);
    if(matchResult>=0){
        (*offsetInXMLfilep)+=WM_pi_end->itemcnt;
        return 0;
    }else{
        uint32_t piContentStartOffset=(*offsetInXMLfilep);
        if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,WM_pi_end,CM_AnyChar)<0){
            dprintf(DBGT_ERROR,"Unexpected eof");
            return -2;
        }else{
            uint32_t piContentEndOffset=(*offsetInXMLfilep)-WM_pi_end->itemcnt;
            struct DynamicList* PiContentDlP=DlCreate(sizeof(uint32_t),piContentEndOffset-piContentStartOffset,dynlisttype_utf32chars);
            memcpy(PiContentDlP->items,((uint32_t*)xmlFileDlP->items)+piContentStartOffset,sizeof(uint32_t)*(piContentEndOffset-piContentStartOffset));
            PiXmlElementP->content=DlCombine_freeArg1(sizeof(uint32_t),PiXmlElementP->content,PiContentDlP);
            return 0;
        }
    }
}

int parseCdata(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResultsP){
    if(ObjectToAttachResultsP->parent==0){
        dprintf(DBGT_ERROR,"CData is only allowed to occur inside tags");
    }
    uint32_t cDataStartOffset=(*offsetInXMLfilep);
    struct xmlTreeElement* CDataXmlElementP=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
    CDataXmlElementP->type=xmltype_cdata;
    CDataXmlElementP->name=DlCreate(sizeof(uint32_t*),0,dynlisttype_utf32chars);
    CDataXmlElementP->attributes=DlCreate(sizeof(struct key_val_pair),0,dynlisttype_keyValuePairsp);
    CDataXmlElementP->parent=ObjectToAttachResultsP;
    if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,WM_cdata_end,CM_AnyChar)){   //unexpected eof
        dprintf(DBGT_ERROR,"Unexpected end of file while scanning comment, start tag for cdata on offset %d",cDataStartOffset);
    }
    uint32_t cDataEndOffset=(*offsetInXMLfilep)-WM_cdata_end->itemcnt;
    struct DynamicList* CDataDlP=DlCreate(sizeof(uint32_t),cDataEndOffset-cDataStartOffset,dynlisttype_utf32chars);
    memcpy(CDataDlP->items,((uint32_t*)xmlFileDlP->items)+cDataStartOffset,sizeof(uint32_t)*(cDataEndOffset-cDataStartOffset));
    CDataXmlElementP->content=CDataDlP;
    //Append new xmlElement with type cdata to parent
    ObjectToAttachResultsP->content=DlAppend(sizeof(struct xmlTreeElement*),ObjectToAttachResultsP->content,&CDataXmlElementP);
}


struct DynamicList* xmlDOMtoUTF32(struct xmlTreeElement* startingElement, int formating,int numSpacesIndent);
int writeXML(FILE* xmlOutFile,struct xmlTreeElement* inputDocumentRootP){
    struct DynamicList* utf32XmlDlP=xmlDOMtoUTF32(inputDocumentRootP,1,4);
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


struct DynamicList* xmlDOMtoUTF32(struct xmlTreeElement* startingElement, int formating,int numSpacesIndent){
    struct DynamicList* returnUtf32StringDlP=DlCreate(sizeof(uint32_t),0,dynlisttype_utf32chars);
    //TODO parse attributes of DocumentRoots, keep in mind newline is before start tag
    struct xmlTreeElement* LastXMLTreeElementP=startingElement->parent;
    struct xmlTreeElement* CurrentXMLTreeElementP=startingElement;
    uint32_t currentDepth=0;        //for indentation
    #define subindex_needs_reinitialization (-1)
    int32_t subindex=0;             //which child node is currently processed
    do{
        //dprintf(DBGT_INFO,"Current content:\n%s",utf32dynlist_to_string(returnUtf32StringDlP));
        if(CurrentXMLTreeElementP->parent==LastXMLTreeElementP){      //walk direction is downward
            //-------write start tag, attributes
            switch(CurrentXMLTreeElementP->type){
                //TODO remove this type and just embedd the ?xml as processing instruction inside the document element
                case xmltype_docRoot:
                    //print "<xml"
                    {
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("<?"));
                        returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,CurrentXMLTreeElementP->name);
                    }
                    //print optional attributes
                    for(uint32_t attributenum=0; attributenum<CurrentXMLTreeElementP->attributes->itemcnt; attributenum++){
                        struct DynamicList* KeyDynlistp=(((struct key_val_pair*)CurrentXMLTreeElementP->attributes->items)[attributenum]).key;
                        struct DynamicList* ValDynlistp=(((struct key_val_pair*)CurrentXMLTreeElementP->attributes->items)[attributenum]).value;
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist(" "));
                        returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,KeyDynlistp);
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("=\""));
                        returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,ValDynlistp);
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("\""));
                    }
                    //print right closing tag "?>"
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("?>"));
                    //do not change depth, the document root acts like a processing instruction
                    subindex=subindex_needs_reinitialization;
                break;
                case xmltype_tag:
                    //print "<tagname"
                    {
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("<"));
                        returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,CurrentXMLTreeElementP->name);
                    }
                    //print optional attributes
                    for(uint32_t attributenum=0; attributenum<CurrentXMLTreeElementP->attributes->itemcnt; attributenum++){
                        struct DynamicList* KeyDynlistp=(((struct key_val_pair*)CurrentXMLTreeElementP->attributes->items)[attributenum]).key;
                        struct DynamicList* ValDynlistp=(((struct key_val_pair*)CurrentXMLTreeElementP->attributes->items)[attributenum]).value;
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist(" "));
                        returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,KeyDynlistp);
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("=\""));
                        returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,ValDynlistp);
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("\""));
                    }
                    //print right closing tag (either > or />)
                    if(CurrentXMLTreeElementP->content->itemcnt){
                        //we have at least one valid subelement, so print ">"
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist(">"));
                        //Jump to the next subelement
                        currentDepth++;
                        subindex=subindex_needs_reinitialization;
                    }else{
                        //there are no subelements, so print "/>"
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("/>"));
                    }
                break;
                case xmltype_cdata:
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("<![CDATA["));
                    returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,CurrentXMLTreeElementP->content);
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("]]>"));
                break;
                case xmltype_chardata:
                    //test to confirm no < or & characters are inside
                    returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,CurrentXMLTreeElementP->content);
                break;
                case xmltype_comment:
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("<!--"));
                    returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,CurrentXMLTreeElementP->content);
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("-->"));
                break;
                case xmltype_pi:
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("<?"));
                    returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,CurrentXMLTreeElementP->name);
                    if(CurrentXMLTreeElementP->content->itemcnt){
                        returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist(" "));
                        returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,CurrentXMLTreeElementP->content);
                    }
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("?>"));
                break;
                default:
                    dprintf(DBGT_ERROR,"Tag of this type is not handled (type %x)",CurrentXMLTreeElementP->type);
                break;
            }
            if(subindex!=subindex_needs_reinitialization){
                //successfully parsed last element, so increment subindex
                //are we the last child inside out parent
                if(subindex!=subindex_needs_reinitialization&&(++subindex)==CurrentXMLTreeElementP->parent->content->itemcnt){
                    //yes, then one level upward
                    LastXMLTreeElementP=CurrentXMLTreeElementP;
                    CurrentXMLTreeElementP=CurrentXMLTreeElementP->parent;
                }else{
                    //
                    CurrentXMLTreeElementP=(((struct xmlTreeElement**)CurrentXMLTreeElementP->parent->content->items)[subindex]);
                }
            }else{
                //reinitialize pointers to parse first subelement
                subindex=0;
                CurrentXMLTreeElementP=(((struct xmlTreeElement**)CurrentXMLTreeElementP->content->items)[subindex]);
                LastXMLTreeElementP=CurrentXMLTreeElementP->parent;
            }
        }else{
            //go back upward
            //get subindex from the view of the parent
            int subindex_of_child_we_came_from=0;
            while(subindex_of_child_we_came_from<CurrentXMLTreeElementP->content->itemcnt && LastXMLTreeElementP!=((struct xmlTreeElement**)CurrentXMLTreeElementP->content->items)[subindex_of_child_we_came_from]){
                subindex_of_child_we_came_from++;
            }
            //is our object the last child element of our parent?
            if(subindex_of_child_we_came_from==CurrentXMLTreeElementP->content->itemcnt-1){
                //yes, so close our current tag and move one layer up
                currentDepth--;
                //write closing tag
                if(CurrentXMLTreeElementP->type!=xmltype_docRoot){//do not write a closing </xml> element on the end of our output
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist("</"));
                    returnUtf32StringDlP=DlCombine_freeArg1(sizeof(uint32_t),returnUtf32StringDlP,CurrentXMLTreeElementP->name);
                    returnUtf32StringDlP=DlCombine_freeArg12(sizeof(uint32_t),returnUtf32StringDlP,stringToUTF32Dynlist(">"));
                }
                //Move one level upward
                LastXMLTreeElementP=CurrentXMLTreeElementP;
                CurrentXMLTreeElementP=CurrentXMLTreeElementP->parent;
            }else{
                //no, so setup to print it next
                LastXMLTreeElementP=CurrentXMLTreeElementP;
                CurrentXMLTreeElementP=(((struct xmlTreeElement**)CurrentXMLTreeElementP->content->items)[subindex_of_child_we_came_from+1]);
                subindex=subindex_of_child_we_came_from+1;
            }
        }
    }while(CurrentXMLTreeElementP!=startingElement->parent);     //Todo error with abort condition when starting from subelement
    return returnUtf32StringDlP;
}

//TODO does miss the upper layer of elements
void printXMLsubelements(struct xmlTreeElement* xmlElement){
    struct DynamicList* utf32XmlDlP=xmlDOMtoUTF32(xmlElement,0,4);
    uint8_t* utf8Xml=(uint8_t*)malloc(sizeof(uint8_t)*(4*utf32XmlDlP->itemcnt+1)); //for null term
    uint32_t numUTF8Chars=utf32ToUtf8(utf32XmlDlP->items,utf32XmlDlP->itemcnt,utf8Xml);
    #define blocksize 1
    utf8Xml[numUTF8Chars]='\0';
    printf("%s\n",utf8Xml);
}

