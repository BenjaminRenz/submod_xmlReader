#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h> //for memcopy
#include <setjmp.h> //for setjmp
#include "xmlReader/xmlReader.h"
#include "xmlReader/xmlReader_matchHelper.h"
#include "debugPrint/debugPrint.h"


enum {  ////errorcodes
        error_unexpected_eof                =-1,
        error_unknown_filelength            =-2,
        error_unexpected_char               =-3,
        error_malformed_closing_tag         =-4,
        error_vinfo_not_in_xmldecl          =-5,
        internal_error_wrong_argument_type  =-6,
        error_comment_doubleminus           =-7,
        error_malformed_name                =-8,
        error_unexpected_docdecl            =-9,
        error_illegal_xmldecl_placement     =-10,
        error_not_implemented               =-11,
        error_zerolength_file               =-12
};

Dl_utf32Char* xmlDOMtoUTF32(xmlTreeElement* startingElement);
int parseXMLDecl(       Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP);
void parseDoctype(      Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP);
int parseNameAndAttrib( Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP,Dl_MWM* MWM_EndTagDlP);
int parseAttrib(        Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP,Dl_MWM* MWM_EndTagDlP);
void parseCdata(        Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP);
void parsePi(           Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP);
void parseComment(      Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP);
void match_loop(        Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP);
int match_misc(         Dl_utf32Char* xmlFileDyn,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP);
void match_element(     Dl_utf32Char* xmlFileDyn,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP);


enum{misc_ret_charLeft=0, misc_ret_eof=1};
/* Notes of some quirks present in xml:
-there can be text outside the tags which should not be interpreted
*/
jmp_buf env;    //contains SS ES and SP from the time setjmp was called

int readXML(FILE* xmlFile,xmlTreeElement** returnDocumentRootpp){
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
    uint32_t* utf32BufferP=(uint32_t*) malloc(sizeof(uint32_t)*numOfCharsUTF8);     //potentially to big if we have utf8 characters that span over more than one byte
    uint32_t numOfCharsUTF32=utf8ToUtf32(utf8Buffer,numOfCharsUTF8,utf32BufferP); //up until this point we didn't know the actual number of utf32 chars because utf8 chars can combine
    free(utf8Buffer);
    Dl_utf32Char* xmlFileDlP=Dl_utf32Char_alloc(numOfCharsUTF32,utf32BufferP);
    free(utf32BufferP);
    uint32_t offsetInXMLfile=0;
    //create document root
    xmlTreeElement* rootDocumentElementp=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
    //initialize root xmlElement
    rootDocumentElementp->parent=       0;
    rootDocumentElementp->name=         Dl_utf32Char_alloc(0,NULL);
    rootDocumentElementp->charData=     Dl_utf32Char_alloc(0,NULL);
    rootDocumentElementp->type=         xmltype_docRoot;
    rootDocumentElementp->children=     Dl_xmlP_alloc(0,NULL);
    rootDocumentElementp->attributes=   Dl_attP_alloc(0,NULL);
    init_matchlists();  //initialize matchHelper
    if(setjmp(env)){
        //Error occured, print exact location of offsetInXmlFile
        uint32_t lines=0;
        uint32_t charpos=0;
        uint32_t internaloffset=0;
        while(internaloffset<offsetInXMLfile){
            if(((uint32_t*)xmlFileDlP->items)[internaloffset++]=='\n'){
                lines++;
                charpos=0;
            }else{
                printf("%c",((uint32_t*)xmlFileDlP->items)[internaloffset]);
                charpos++;
            }
        }
        dprintf(DBGT_ERROR,"Error was in line %d, character %d.\n",lines+1,charpos);
        exit(1);
    }
    match_loop(xmlFileDlP,&offsetInXMLfile,rootDocumentElementp);
    (*returnDocumentRootpp)=rootDocumentElementp;
    dprintf(DBGT_INFO,"Starting element att itemcnt is %d",(int)(rootDocumentElementp->attributes->itemcnt));
    //printXMLsubelements(rootDocumentElementp);
    return 0;
}

void match_loop(Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP){
    int skipSpaces=0;
    while(1){
        enum {  MWMstart_res_XMLDecl_start =0,MWMstart_res_doctype_start =1,MWMstart_res_element_end   =2,
                MWMstart_res_element_start =3,MWMstart_res_pi_start      =4,MWMstart_res_cdata_start   =5,
                MWMstart_res_comment_start =6,MWMstart_res_IllegalChar   =7};
        switch(Dl_MWM_matchAndInc(xmlFileDlP,offsetInXMLfilep,MWM_start,0)){
            case MWMstart_res_XMLDecl_start:
                parseXMLDecl(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP);
                //struct DynamicList* XMLDeclAttribDlP=ObjectToAttachResultsP->attributes;
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
                uint32_t startElementName=(*offsetInXMLfilep)-1;
                if(Dl_WM_matchAndInc(xmlFileDlP,offsetInXMLfilep,WM_element_endNonEmpty,CM_NameChar)<0){
                    dprintf(DBGT_ERROR,"unexpected character in end tag");
                    longjmp(env,error_unexpected_char);
                }
                uint32_t endElementName=*offsetInXMLfilep-1;
                void* nameEndSrcP=xmlFileDlP->items+startElementName;
                Dl_utf32Char* closedNameDlP=Dl_utf32Char_alloc(endElementName-startElementName,nameEndSrcP);
                if(!Dl_utf32Char_equal_freeArg2(ObjectToAttachResultsP->name,closedNameDlP)){
                    dprintf(DBGT_ERROR,"closing tag does not match the one opened before");
                    longjmp(env,error_malformed_closing_tag);
                }
                ObjectToAttachResultsP=ObjectToAttachResultsP->parent;
            break;
            case MWMstart_res_element_start:;
            {
                xmlTreeElement* newXmlElementP=(xmlTreeElement*)malloc(sizeof(xmlTreeElement));
                newXmlElementP->attributes= Dl_attP_alloc(0,NULL);
                newXmlElementP->name=       Dl_utf32Char_alloc(0,NULL);
                newXmlElementP->charData=   Dl_utf32Char_alloc(0,NULL);
                newXmlElementP->parent=     ObjectToAttachResultsP;
                newXmlElementP->type=       xmltype_tag;
                newXmlElementP->children=   Dl_xmlP_alloc(0,NULL);

                Dl_xmlP_append(ObjectToAttachResultsP->children,1,&newXmlElementP);
                ObjectToAttachResultsP=newXmlElementP;
                enum{MWMres_el_etag_nonEmpty=0,MWMres_el_etag_empty=1};
                switch(parseNameAndAttrib(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP,MWM_element_end)){
                    case MWMres_el_etag_nonEmpty:
                        //dprintf(DBGT_INFO,"Now one layer deeper");
                    break;
                    case MWMres_el_etag_empty:
                        ObjectToAttachResultsP=ObjectToAttachResultsP->parent;
                    break;
                    default:
                        dprintf(DBGT_ERROR,"An error occured while parsing an element start or end tag");
                        exit(1);
                    break;
                }
            }
            break;
            case MWMstart_res_IllegalChar:
                dprintf(DBGT_ERROR,"Illegal character found in xml, offset=%d",(*offsetInXMLfilep));
                exit(1);
            break;
            default:
                //no tag found, so continue to process character data
                //TODO in case we ignore space, set
                if(skipSpaces){
                    Dl_CM_match(xmlFileDlP,offsetInXMLfilep,0,CM_SpaceChar);
                }
                uint32_t CharDataStartOffset=(*offsetInXMLfilep);
                //Move to the last character which is chardata
                int match_res=Dl_CM_match(xmlFileDlP,offsetInXMLfilep,CM_LessThanChar,CM_CharData);
                uint32_t CharDataEndOffset=(*offsetInXMLfilep);
                if(skipSpaces){ //cut trailing space characters
                    while(Dl_CM_match(xmlFileDlP,&CharDataEndOffset,CM_SpaceChar,0)>=0){
                        CharDataEndOffset--;
                    }
                }
                if(match_res<0){    //no new opening tag found
                    if((*offsetInXMLfilep)>=xmlFileDlP->itemcnt){
                        if(ObjectToAttachResultsP->parent!=0){
                            dprintf(DBGT_ERROR,"Unexpected EOF during maching");
                        }else{
                            dprintf(DBGT_INFO,"Finished parsing");
                            return;
                        }
                    }else{
                        dprintf(DBGT_ERROR,"Unexpected character during parsing");
                    }
                }
                if(CharDataStartOffset<CharDataEndOffset){
                    //there was valid chardata
                    xmlTreeElement* newCharDataElementP=(xmlTreeElement*)malloc(sizeof(xmlTreeElement));    //get first element of new list
                    //dprintf(DBGT_INFO,"Creating new xml element for chardata of length %d",CharDataEndOffset-CharDataStartOffset);
                    void* charDataScrP=xmlFileDlP->items+CharDataStartOffset;
                    newCharDataElementP->attributes=Dl_attP_alloc(0,NULL);
                    newCharDataElementP->name=      Dl_utf32Char_alloc(0,NULL);
                    newCharDataElementP->charData=  Dl_utf32Char_alloc(CharDataEndOffset-CharDataStartOffset,charDataScrP);
                    newCharDataElementP->parent=    ObjectToAttachResultsP;
                    newCharDataElementP->type=      xmltype_chardata;
                    newCharDataElementP->children=  Dl_xmlP_alloc(0,NULL);
                    //attach new chardata element to parrent
                    Dl_xmlP_append(ObjectToAttachResultsP->children,1,&newCharDataElementP);
                }else{
                    dprintf(DBGT_ERROR,"unexpected < in file");
                    exit(1);
                }
            break;
        }
    }
}

int parseXMLDecl(Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP){
    if(ObjectToAttachResultsP->parent!=0){//this is not the root element, the doctype tag is not allowed to appear here
        dprintf(DBGT_ERROR,"unexpected <?xml");
        longjmp(env,error_illegal_xmldecl_placement);
    }
    if(ObjectToAttachResultsP->attributes->itemcnt!=0){
        dprintf(DBGT_ERROR,"<?xml occured more than one or after Doctype");
        longjmp(env,error_illegal_xmldecl_placement);
    }
    ObjectToAttachResultsP->name=Dl_utf32Char_mergeDelete(ObjectToAttachResultsP->name,Dl_utf32Char_fromString("xml"));
    return parseAttrib(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP,MWM_XMLDecl_end);
}

void parseDoctype(Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP){
    dprintf(DBGT_ERROR,"Doctype not implemented!");
    exit(1);
}

//return indicates unexpected eof
int parseNameAndAttrib(Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP,Dl_MWM* MWM_EndTagDlP){
    uint32_t nameStartOffset=(*offsetInXMLfilep)-1; //the previous matcher also checked for a valid name start character
    //dprintf(DBGT_INFO,"nameStartOffset: %d",nameStartOffset);
    int matchResult=Dl_MWM_match(xmlFileDlP,offsetInXMLfilep,MWM_EndTagDlP,CM_NameChar);
    uint32_t nameEndOffset=(*offsetInXMLfilep);
    //dprintf(DBGT_INFO,"nameEndOffset: %d",nameEndOffset);
    if(matchResult<0){
        if(Dl_CM_matchAndInc(xmlFileDlP,offsetInXMLfilep,CM_SpaceChar,0)<0){        //there must be a space, after the element if attributes follow
            dprintf(DBGT_ERROR,"Invalid character in element name");
            exit(1);
        }
    }else{
        uint32_t lengthOfApplyingEndTag=MWM_EndTagDlP->items[matchResult]->itemcnt;
        (*offsetInXMLfilep)+=lengthOfApplyingEndTag;
    }
    void* nameSrcP=xmlFileDlP->items+nameStartOffset;
    Dl_utf32Char_append(ObjectToAttachResultsP->name,1,nameSrcP);
    if(matchResult>=0){
        return matchResult;
    }else{
        return parseAttrib(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResultsP,MWM_EndTagDlP);
    }
}

int parseAttrib(Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP,Dl_MWM* MWM_EndTagDlP){
    while(1){
        //check for key startCharacter
        uint32_t attKeyEndOffset;
        int matchRes=Dl_MWM_matchAndInc(xmlFileDlP,offsetInXMLfilep,MWM_EndTagDlP,CM_SpaceChar);
        uint32_t attKeyStartOffset=(*offsetInXMLfilep);
        if(matchRes<0){
            if(Dl_CM_matchAndInc(xmlFileDlP,offsetInXMLfilep,CM_NameStartChar,0)<0){
                dprintf(DBGT_ERROR,"unexpected character hex: %x as first attribute",((uint32_t*)xmlFileDlP->items)[*offsetInXMLfilep]);
                exit(1);
            }
        }else{
            return matchRes;
        }
        //jump to end of key
        enum {MCMres_pA_Space=0,MCMres_pA_Equals=1};
        switch(Dl_MCM_matchAndInc(xmlFileDlP,offsetInXMLfilep,MCM_AttNameEndOrEqual,CM_NameChar)){
            case MCMres_pA_Space:
                attKeyEndOffset=(*offsetInXMLfilep)-1;
                //move up to the = character
                if(Dl_CM_matchAndInc(xmlFileDlP,offsetInXMLfilep,CM_Equals,CM_SpaceChar)!=0){
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
        void* keySrcP=xmlFileDlP->items+attKeyStartOffset;
        Dl_utf32Char* keyDlP=Dl_utf32Char_alloc(attKeyEndOffset-attKeyStartOffset,keySrcP);

        //jump to start of value
        uint32_t attValStartOffset;
        uint32_t attValEndOffset;
        enum {MCMres_pA_QuoteSingle=0,MCMres_pA_QuoteDouble=1};
        switch(Dl_MCM_matchAndInc(xmlFileDlP,offsetInXMLfilep,MCM_Quotes,CM_SpaceChar)){
            case MCMres_pA_QuoteSingle:
                attValStartOffset=(*offsetInXMLfilep);
                if(Dl_CM_matchAndInc(xmlFileDlP,offsetInXMLfilep,CM_QuoteSingle,CM_PubidChar_withoutQuotes)<0){
                    dprintf(DBGT_ERROR,"unexpected character inside attribute key or eof");
                }
                attValEndOffset=(*offsetInXMLfilep)-1;
            break;
            case MCMres_pA_QuoteDouble:
                attValStartOffset=(*offsetInXMLfilep);
                if(Dl_CM_matchAndInc(xmlFileDlP,offsetInXMLfilep,CM_QuoteDouble,CM_PubidChar)<0){
                    dprintf(DBGT_ERROR,"unexpected character inside attribute key or eof");
                }
                attValEndOffset=(*offsetInXMLfilep)-1;
            break;
            default:
                dprintf(DBGT_ERROR,"invalid character before attribute key of eof");
            break;
        }
        //write value
        void* valSrcP=xmlFileDlP->items+attValStartOffset;
        Dl_utf32Char* valueDlP=Dl_utf32Char_alloc(attValEndOffset-attValStartOffset,valSrcP);
        //combine into key value pair and append to xmlTagAttributes
        struct key_val_pair* keyValPairP=(struct key_val_pair*)malloc(sizeof(struct key_val_pair));
        keyValPairP->key=keyDlP;
        keyValPairP->value=valueDlP;
        Dl_attP_append(ObjectToAttachResultsP->attributes,1,&keyValPairP);
        //dprintf(DBGT_INFO,"new itemcnt of attribute %d",ObjectToAttachResultsP->attributes->itemcnt);
    }
}

void parseComment(Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP){
    uint32_t commentStartOffset=(*offsetInXMLfilep);
    xmlTreeElement* CommentXmlElementP=(xmlTreeElement*)malloc(sizeof(xmlTreeElement));
    CommentXmlElementP->type=       xmltype_comment;
    CommentXmlElementP->children=   Dl_xmlP_alloc(0,NULL);
    CommentXmlElementP->attributes= Dl_attP_alloc(0,NULL);
    CommentXmlElementP->parent=     ObjectToAttachResultsP;
    CommentXmlElementP->name=       Dl_utf32Char_alloc(0,NULL);
    if(Dl_WM_matchAndInc(xmlFileDlP,offsetInXMLfilep,WM_comment_end,CM_AnyChar)){   //unexpected eof
        dprintf(DBGT_ERROR,"Unexpected end of file while scanning comment, start tag for comment on offset %d",commentStartOffset);
    }
    uint32_t commentEndOffset=(*offsetInXMLfilep)-WM_comment_end->itemcnt;
    void* commentSrcP=xmlFileDlP->items+commentStartOffset;
    CommentXmlElementP->charData=Dl_utf32Char_alloc(commentEndOffset-commentStartOffset,commentSrcP);
    //Append new xmlElement with type comment to parent
    Dl_xmlP_append(ObjectToAttachResultsP->children,1,&CommentXmlElementP);
}

void parsePi(Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,xmlTreeElement* ObjectToAttachResultsP){
    uint32_t piNameStartOffset=(*offsetInXMLfilep)-1; //the previous matcher also checked for a valid name start character
    int matchResult=Dl_WM_match(xmlFileDlP,offsetInXMLfilep,WM_pi_end,CM_NameChar);
    uint32_t piNameEndOffset=(*offsetInXMLfilep);
    if((matchResult<0) && (Dl_CM_matchAndInc(xmlFileDlP,offsetInXMLfilep,CM_SpaceChar,0)<0)){        //there must be a space, after the element if attributes follow
        dprintf(DBGT_ERROR,"Invalid character in pi name");
        exit(1);
    }
    void* NameSrcP=xmlFileDlP->items+piNameStartOffset;
    xmlTreeElement* PiXmlElementP=(xmlTreeElement*)malloc(sizeof(xmlTreeElement));
    PiXmlElementP->type=         xmltype_pi;
    PiXmlElementP->name=         Dl_utf32Char_alloc(piNameEndOffset-piNameStartOffset,NameSrcP);
    PiXmlElementP->attributes=   Dl_attP_alloc(0,NULL);
    PiXmlElementP->parent=       ObjectToAttachResultsP;
    PiXmlElementP->children=     Dl_xmlP_alloc(0,NULL);
    Dl_xmlP_append(ObjectToAttachResultsP->children,1,&PiXmlElementP);
    if(matchResult>=0){
        PiXmlElementP->charData=Dl_utf32Char_alloc(0,NULL);
        (*offsetInXMLfilep)+=WM_pi_end->itemcnt;
        return;
    }else{
        uint32_t piContentStartOffset=(*offsetInXMLfilep);
        if(Dl_WM_matchAndInc(xmlFileDlP,offsetInXMLfilep,WM_pi_end,CM_AnyChar)<0){
            dprintf(DBGT_ERROR,"Unexpected eof");
            exit(1);
        }else{
            uint32_t piContentEndOffset=(*offsetInXMLfilep)-WM_pi_end->itemcnt;
            void* PiSrcP=xmlFileDlP->items+piContentStartOffset;
            PiXmlElementP->charData=Dl_utf32Char_alloc(piContentEndOffset-piContentStartOffset,PiSrcP);
            return;
        }
    }
}

void parseCdata(Dl_utf32Char* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResultsP){
    if(ObjectToAttachResultsP->parent==0){
        dprintf(DBGT_ERROR,"CData is only allowed to occur inside tags");
        exit(1);
    }
    uint32_t cDataStartOffset=(*offsetInXMLfilep);
    struct xmlTreeElement* CDataXmlElementP=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
    CDataXmlElementP->type=         xmltype_cdata;
    CDataXmlElementP->name=         Dl_utf32Char_alloc(0,NULL);
    CDataXmlElementP->attributes=   Dl_attP_alloc(0,NULL);
    CDataXmlElementP->parent=       ObjectToAttachResultsP;
    CDataXmlElementP->children=     Dl_xmlP_alloc(0,NULL);
    if(Dl_WM_matchAndInc(xmlFileDlP,offsetInXMLfilep,WM_cdata_end,CM_AnyChar)){   //unexpected eof
        dprintf(DBGT_ERROR,"Unexpected end of file while scanning comment, start tag for cdata on offset %d",cDataStartOffset);
        exit(1);
    }
    uint32_t cDataEndOffset=(*offsetInXMLfilep)-WM_cdata_end->itemcnt;
    void* srcP=xmlFileDlP->items+cDataStartOffset;
    CDataXmlElementP->charData=Dl_utf32Char_alloc(cDataEndOffset-cDataStartOffset,srcP);
    //Append new xmlElement with type cdata to parent
    Dl_xmlP_append(ObjectToAttachResultsP->children,1,&CDataXmlElementP);
}



int writeXML(FILE* xmlOutFile,struct xmlTreeElement* inputDocumentRootP){
    Dl_utf32Char* utf32XmlDlP=xmlDOMtoUTF32(inputDocumentRootP);
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


Dl_utf32Char* xmlDOMtoUTF32(xmlTreeElement* startingElement){
    Dl_utf32Char* returnUtf32StringDlP=Dl_utf32Char_alloc(0,NULL);
    //TODO parse attributes of DocumentRoots, keep in mind newline is before start tag
    xmlTreeElement* LastXMLTreeElementP=startingElement->parent;
    xmlTreeElement* CurrentXMLTreeElementP=startingElement;
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
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("<?"));
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(CurrentXMLTreeElementP->name));
                    }
                    //print optional attributes
                    for(uint32_t attributenum=0; attributenum<CurrentXMLTreeElementP->attributes->itemcnt; attributenum++){
                        Dl_utf32Char* KeyDynlistp=CurrentXMLTreeElementP->attributes->items[attributenum]->key;
                        Dl_utf32Char* ValDynlistp=CurrentXMLTreeElementP->attributes->items[attributenum]->value;
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString(" "));
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(KeyDynlistp));
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("=\""));
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(ValDynlistp));
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("\""));
                    }
                    //print right closing tag "?>"
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("?>"));
                    //do not change depth, the document root acts like a processing instruction
                    subindex=subindex_needs_reinitialization;
                break;
                case xmltype_tag:
                    //print "<tagname"
                    {
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("<"));
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(CurrentXMLTreeElementP->name));
                    }
                    //print optional attributes
                    for(uint32_t attributenum=0; attributenum<CurrentXMLTreeElementP->attributes->itemcnt; attributenum++){
                        Dl_utf32Char* KeyString=CurrentXMLTreeElementP->attributes->items[attributenum]->key;
                        Dl_utf32Char* ValString=CurrentXMLTreeElementP->attributes->items[attributenum]->value;
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString(" "));
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(KeyString));
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("=\""));
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(ValString));
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("\""));
                    }
                    //print right closing tag (either > or />)
                    if(CurrentXMLTreeElementP->children->itemcnt){
                        //we have at least one valid subelement, so print ">"
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString(">"));
                        //Jump to the next subelement
                        subindex=subindex_needs_reinitialization;
                    }else{
                        //there are no subelements, so print "/>"
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("/>"));
                    }
                break;
                case xmltype_cdata:
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("<![CDATA["));
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(CurrentXMLTreeElementP->charData));
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("]]>"));
                break;
                case xmltype_chardata:
                    //test to confirm no < or & characters are inside
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(CurrentXMLTreeElementP->charData));
                break;
                case xmltype_comment:
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("<!--"));
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(CurrentXMLTreeElementP->charData));
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("-->"));
                break;
                case xmltype_pi:
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("<?"));
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(CurrentXMLTreeElementP->name));
                    if(CurrentXMLTreeElementP->charData->itemcnt){
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString(" "));
                        returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(CurrentXMLTreeElementP->charData));
                    }
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("?>"));
                break;
                default:
                    dprintf(DBGT_ERROR,"Tag of this type is not handled (type %x)",CurrentXMLTreeElementP->type);
                break;
            }
            if(subindex==subindex_needs_reinitialization){
                //reinitialize pointers to parse first subelement
                currentDepth++;
                subindex=0;
                CurrentXMLTreeElementP=CurrentXMLTreeElementP->children->items[subindex];
                LastXMLTreeElementP=CurrentXMLTreeElementP->parent;
            }else{
                //invert walking direction, because there are no further child elements
                LastXMLTreeElementP=CurrentXMLTreeElementP;
                CurrentXMLTreeElementP=CurrentXMLTreeElementP->parent;
            }
        }else{
            //go back upward
            //get subindex from the view of the parent
            uint32_t subindex_of_child_we_came_from=0;
            while(subindex_of_child_we_came_from<CurrentXMLTreeElementP->children->itemcnt && LastXMLTreeElementP!=CurrentXMLTreeElementP->children->items[subindex_of_child_we_came_from]){
                subindex_of_child_we_came_from++;
            }
            //is our object the last child element of our parent?
            if(subindex_of_child_we_came_from==CurrentXMLTreeElementP->children->itemcnt-1){
                //yes, so close our current tag and move one layer up
                currentDepth--;
                //write closing tag
                if(CurrentXMLTreeElementP->type!=xmltype_docRoot){//do not write a closing </xml> element on the end of our output
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString("</"));
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_shallowCopy(CurrentXMLTreeElementP->name));
                    returnUtf32StringDlP=Dl_utf32Char_mergeDelete(returnUtf32StringDlP,Dl_utf32Char_fromString(">"));
                }
                //Move one level upward
                LastXMLTreeElementP=CurrentXMLTreeElementP;
                CurrentXMLTreeElementP=CurrentXMLTreeElementP->parent;
            }else{
                //no, so setup to print it next
                LastXMLTreeElementP=CurrentXMLTreeElementP;
                CurrentXMLTreeElementP=CurrentXMLTreeElementP->children->items[subindex_of_child_we_came_from+1];
                subindex=subindex_of_child_we_came_from+1;
            }
        }
    }while(CurrentXMLTreeElementP!=startingElement->parent);     //Todo error with abort condition when starting from subelement
    return returnUtf32StringDlP;
}

//TODO does miss the upper layer of elements
void printXMLsubelements(xmlTreeElement* xmlElement){
    Dl_utf32Char* utf32XmlDlP=xmlDOMtoUTF32(xmlElement);
    char* utf8Xml=Dl_utf32Char_toStringAlloc_freeArg1(utf32XmlDlP);
    printf("%s\n",utf8Xml);
}

