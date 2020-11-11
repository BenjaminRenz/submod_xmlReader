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

enum{misc_ret_charLeft=0, misc_ret_eof=1};
int match_misc(struct DynamicList* xmlFileDyn,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults);

void match_element(struct DynamicList* xmlFileDyn,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults);
/* Notes of some quirks present in xml:
-there can be text outside the tags which should not be interpreted

*/

/*matchList copy paste space
whitespace / s          createCharMatchList(8,0x09,0x09,0x0a,0x0a,0x0d,0x0d,0x20,0x20)
any char                createCharMatchList(12,0x09,0x09, 0x0a,0x0a, 0x0d,0x0d, 0x20,0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff)
non whitespace char     createCharMatchList(6,0x21,0xd7ff,0xe000,0xfffd,0x10000,0x10ffff)



createWordMatchList(2,                      //STag
    createCharMatchList(2,'<','<'),
    createCharMatchList(32,':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6, 0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d, 0x2070,0x218F, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd, 0x10000,0xeffff)
),
*/


void xmlMatchAndMoveOffset(struct DynamicList* xmlFileDyn,uint32_t* offsetInXmlFilep,struct DynamicList* matchObj,uint32_t* matchIndexp){
    (*offsetInXmlFilep)+=getOffsetUntil(((uint32_t*)xmlFileDyn->items)+(*offsetInXmlFilep),xmlFileDyn->itemcnt-(*offsetInXmlFilep),matchObj,matchIndexp);
    if((*offsetInXmlFilep)==xmlFileDyn->itemcnt){
        printf("ERROR: Unexpected end of file!\n");
        exit(EXIT_FAILURE);
    }
}


//TODO
/*
-first check for XMLDecl, DoctypeDoctypedecl, Element or Misc (Pi,Comment...)
-depending on whitespace check what the offset in the document was when XMLDecl was found, if it is >0 error, because XMLDecl can not be preceeded by any Misc characters
-if Element is found skip next step
-check for DoctypeDoctypedecl, Element or Misc
-if if Misc is found repeat last step
-check for Element or Misc
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
    match_phase1(xmlFile,&offsetInXMLfile,rootDocumentElementp);
    (*returnDocumentRootpp)=rootDocumentElementp;
    return 0;
}

int match_phase1(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){
    //if XMLDecl exists overwrite the root element with this
    switch(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,MWM_p1_start,CM_SpaceChar)){
        case MWMres_p1_XMLDecl_start:
            parseAttrib(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResults,WM_pi_end);
        break;
        case MWMres_p1_doctype_start:
            parseDoctype(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResults);
        break;
        case MWMres_p1_pi_start:
        break;
        case MWMres_p1_comment_start:
            parseComment(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResults);
        break;
        case: MWMres_p1_element_start:
        break;
        case: MWMres_p1_NonSpaceChar:
            dprintf(DBGT_ERROR,"unexpected character found in xml, offset=%d",(*offsetInXMLfilep));
        break;
        case: MWMres_p1_IllegalChar:
            dprintf(DBGT_ERROR,"Illegal character found in xml, offset=%d",(*offsetInXMLfilep));
        break;
        //TODO what about CharData????
        default:
            //error -1
            dprintf(DBGT_ERROR,"Unexpected EOF during phase 1");
        break;
    }
}

int parseXMLDecl(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){
    if(ObjectToAttachResults->parent!=0){//this is not the root element, the doctype tag is not allowed to appear here
        dprintf(DBGT_ERROR,"unexpected <?xml");
    }
    if(ObjectToAttachResults->attributes->itemcnt!=0){
        dprintf(DBGT_ERROR,"<?xml occured more than one or after Doctype");
    }
    ObjectToAttachResults->name=DlCombine_freeArg12(ObjectToAttachResults->name,stringToUTF32Dynlist("xml"));
    parseAttrib(xmlFileDlP,offsetInXMLfilep,ObjectToAttachResults,WM_XMLDecl_end);
}

int parseDoctype(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){

}

//return indicates unexpected eof
int parseNameAndAttrib(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults,struct DynamicList* EndTag){

}//return indicates unexpected eo

int parseAttrib(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults,struct DynamicList* WM_EndTagDlP){
    //check for name startCharacter
    uint32_t attNameStartOffset=(*offsetInXMLfilep);
    uint32_t attNameEndOffset;
    enum {MWMres_pA_EndTag=0,MWMres_pA_NameStart};
    struct DynamicList* MWM_AttNameStartOrEndTag=createMultiWordMatchList(2,WM_EndTagDlP,WM_NameStartChar);
    switch(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,MWM_AttNameStartOrEndTag,0)){
        case MWMres_pA_EndTag:
            return 0;
        break;
        case MWMres_pA_NameStart:
            break;
        default:
            dprintf(DBGT_ERROR,"unexpected character as first attribute");
        break;
    }
    //jump to end of name
    enum {MCMres_pA_Space=0,MCMres_pA_Equals=1};
    struct DynamicList* MCM_AttNameEndOrEqual=createMultiWordMatchList(2,CM_SpaceChar,CM_Equals);
    switch(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,MCM_AttNameEndOrEqual,CM_NameChar)){
        case MWMres_pA_Space:
            attNameEndOffset=(*offsetInXMLfilep);
            //move up to the = character
            if(MatchAndIncrement(xmlFileDlP,offsetInXMLfilep,CM_Equals,CM_SpaceChar)!=0){
                dprintf(DBGT_ERROR,"Unexpected character after attribute name");
            }
        break;
        case MCMres_pA_Equals:
            attNameEndOffset=(*offsetInXMLfilep);
        default:
            dprintf(DBGT_ERROR,"unexpected character as first attribute");
        break;
    }
    //write name
    struct DynamicList* keyDlP=DlCreate(sizeof(struct key_val_pair*),attNameEndOffset-attNameStartOffset,dynlisttype_utf32chars);
    memcpy(keyDlP->items,((uint32_t*)xmlFileDlP->items)+attNameStartOffset,attNameEndOffset-attNameStartOffset);
    //jump to start of AttValue
    enum {};


    //combine into key value pair and append to xmlTagAttributes
    struct key_val_pair* key_valP=(struct key_val_pair*)malloc(sizeof(struct key_val_pair));
    key_valP->key=keyDlP;
    DlAppend(&(ObjectToAttachResults->attributes),key_valP,sizeof(key_val_pair*),dynlisttype_keyValuePairsp);





}

int parseComment(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){
    uint32_t commentStartOffset=(*offsetInXMLfilep);
    struct xmlTreeElement* Comment=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
    Comment->type=xmltype_comment;
    Comment->name=DlCreate(sizeof(uint32_t*),0,dynlisttype_utf32chars);
    Comment->attributes=DlCreate(sizeof(struct key_val_pair*),0,dynlisttype_keyValuePairsp);
    Comment->parent=ObjectToAttachResults;
    if(Match(xmlFileDlP,offsetInXMLfilep,WM_comment_end,CM_AnyChar)){   //unexpected eof
        dprintf(DBGT_ERROR,"Unexpected end of file while scanning comment, start tag for comment on offset %d",commentStartOffset);
    }
    uint32_t commentEndOffset=(*offsetInXMLfilep);
    struct DynamicList* CommentCDataDlP=DlCreate(sizeof(uint32_t),commentEndOffset-commentStartOffset,dynlisttype_utf32chars);
    memcpy(CommentCDataDlP->items,((uint32_t*)xmlFileDlP->items)+commentStartOffset,commentEndOffset-commentStartOffset);
    Comment->content=CommentCDataDlP;
    DlAppend(&(ObjectToAttachResults->content),Comment,sizeof(struct xmlTreeElement*),dynlisttype_xmlELMNTCollectionp);
}

int parsePi(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){

}

int parseCdata(struct DynamicList* xmlFileDlP,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){
    uint32_t cDataStartOffset=(*offsetInXMLfilep);
    struct xmlTreeElement* CData=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
    CData->type=xmltype_cdata;
    CData->name=DlCreate(sizeof(uint32_t*),0,dynlisttype_utf32chars);
    CData->attributes=DlCreate(sizeof(struct key_val_pair*),0,dynlisttype_keyValuePairsp);
    CData->parent=ObjectToAttachResults;
    if(Match(xmlFileDlP,offsetInXMLfilep,WM_cdata_end,CM_AnyChar)){   //unexpected eof
        dprintf(DBGT_ERROR,"Unexpected end of file while scanning comment, start tag for cdata on offset %d",commentStartOffset);
    }
    uint32_t cDataEndOffset=(*offsetInXMLfilep);
    struct DynamicList* CDataDlP=DlCreate(sizeof(uint32_t),cDataEndOffset-cDataStartOffset,dynlisttype_utf32chars);
    memcpy(CDataDlP->items,((uint32_t*)xmlFileDlP->items)+cDataStartOffset,cDataEndOffset-cDataStartOffset);
    CData->content=CDataDlP;
    DlAppend(&(ObjectToAttachResults->content),CData,sizeof(struct xmlTreeElement*),dynlisttype_xmlELMNTCollectionp);
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











//TODO remove OLD CODE below
void match_element(struct DynamicList* xmlFileDyn,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){
    //Stay in this function until we exit the last element
    uint32_t ignorePrecedingWhitespace=1;
    uint32_t matchIndex=0;
    struct xmlTreeElement* MostRecentElement=ObjectToAttachResults;
    uint32_t nameStartPosition=0;
    uint32_t nameEndPosition=0;

    //Find the start of the tag name for the outer most element
    enum{match_res_element1_start=0,match_res_element1_illegal=1};
    struct DynamicList* mw_match_element1=createMultiWordMatchList(2,       //For outermost element
        createWordMatchList(2,
            createCharMatchList(2,'<','<'),
            createCharMatchList(32,        //matches all allowed name start char's
                ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
                0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
                0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
                0x10000,0xeffff
            )
        ),
        createWordMatchList(1,              //no elements in file
            createCharMatchList(6,0x21,0xd7ff,0xe000,0xfffd,0x10000,0x10ffff)
        )
    );



    //Find the end of the tag name
    enum {match_res_element2_emptyElement=0,match_res_elemement2_closetag=1,match_res_element2_whitespace=2,match_res_element2_illegal=3};
    struct DynamicList* mw_match_element2=createMultiWordMatchList(4,
        createWordMatchList(2,
            createCharMatchList(2,'/','/'),
            createCharMatchList(2,'>','>')
        ),
        createWordMatchList(1,
            createCharMatchList(2,'>','>')
        ),
        createWordMatchList(1,
            createCharMatchList(8, 0x09,0x09, 0x0a,0x0a, 0x0d,0x0d, 0x20,0x20)
        ),
        createWordMatchList(1,
            createCharMatchList(34,                                         //List of characters that are not allowed to occur in name
                0x21,0x2c, 0x2f,0x2f, 0x3b,0x40, 0x5b,0x5e, 0x60,0x60,
                0x7b,0xb6, 0xb8,0xbf, 0xd7,0xd7, 0xf7,0xf7, 0x37e,0x37e,
                0x2000,0x200b, 0x200e,0x203e, 0x2041,0x2069, 0x2190,0x2bff, 0x2ff0,0x3000,
                0xe000,0xf8ff, 0xfdd0,0xfdef
            )
        )
    );

    //Find the end of an end tag
    enum{match_res_element4_end=0,match_res_element4_illegal=1};
    struct DynamicList* mc_match_element4=createMultiCharMatchList(2,
        createCharMatchList(2,'>','>'),
        createCharMatchList(6,0x21,0xd7ff,0xe000,0xfffd,0x10000,0x10ffff)   //anything else except space
    );

    enum{match_res_element5_emptyElement=0,match_res_element5_end=1};

    //only for the outermost element
    xmlMatchAndMoveOffset(xmlFileDyn,offsetInXMLfilep,mw_match_element1,&matchIndex);
    DlDelete(mw_match_element1);
    if(matchIndex==match_res_element1_illegal){
        printf("Error: Could not find first element\n");
        exit(1);
    }
    printf("Info: Found at least one element\n");


    while(1){   //While loop over elements
        //The following lists are dynamically generate because they contain the mathing closing tag name
        struct DynamicList* mw_match_element3_ignorespace;
        struct DynamicList* mw_match_element3_withspace;
        enum{   match_res_element3_start=0,match_res_element3_closing=1,
                match_res_element3_comment=2,match_res_element3_pi=3,
                match_res_element3_cdatasec=4,match_res_element3_ref=5,
                match_res_element3_chardata=6
            };
        struct DynamicList* mw_match_element6;
        enum{match_res_element6_start=0,match_res_element6_closing=1};

        if(MostRecentElement->name==0){
            mw_match_element3_ignorespace=createMultiWordMatchList(7,       //to match content
                createWordMatchList(2,
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(32,        //matches all allowed piTarget name start char's
                        ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
                        0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
                        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
                        0x10000,0xeffff
                    )
                ),
                createWordMatchList(1,          //Dummy, there is no corresponding closing tag
                    createCharMatchList(2,0,0)
                ),
                createWordMatchList(1,                      //Comment
                    createCharMatchList(2,0,0)
                ),
                createWordMatchList(1,                      //Processing Instruction
                    createCharMatchList(2,0,0)
                ),
                createWordMatchList(1,          //Dummy, don't search for any special tags when searching the first element
                    createCharMatchList(2,0,0)
                ),
                createWordMatchList(1,
                    createCharMatchList(2,0,0)
                ),
                createWordMatchList(1,
                    createCharMatchList(6,0x21,0xd7ff,0xe000,0xfffd,0x10000,0x10ffff)
                )
            );
            mw_match_element3_withspace=createMultiWordMatchList(7,       //to match content
                createWordMatchList(2,
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(32,        //matches all allowed piTarget name start char's
                        ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
                        0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
                        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
                        0x10000,0xeffff
                    )
                ),
                createWordMatchList(1,          //Dummy, there is no corresponding closing tag
                    createCharMatchList(2,0,0)
                ),
                createWordMatchList(1,          //Dummy, don't search for any special tags when searching the first element
                    createCharMatchList(2,0,0)
                ),
                createWordMatchList(1,          //Dummy, don't search for any special tags when searching the first element
                    createCharMatchList(2,0,0)
                ),
                createWordMatchList(1,          //Dummy, don't search for any special tags when searching the first element
                    createCharMatchList(2,0,0)
                ),
                createWordMatchList(1,          //Dummy, don't search for any special tags when searching the first element
                    createCharMatchList(2,0,0)
                ),
                createWordMatchList(1,
                    createCharMatchList(12,0x09,0x09, 0x0a,0x0a, 0x0d,0x0d, 0x20,0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff)
                )
            );
            mw_match_element6=createMultiWordMatchList(2,       //to match content
                createWordMatchList(1,
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(32,        //matches all allowed piTarget name start char's
                        ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
                        0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
                        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
                        0x10000,0xeffff
                    )
                ),
                createWordMatchList(1,
                    createCharMatchList(2,0,0)
                )
            );
        }else{
            printf("Generated new closing tag match for: ");

            //To find new Start tag, endtag or chardata
            mw_match_element3_ignorespace=createMultiWordMatchList(7,       //to match content
                createWordMatchList(2,
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(32,        //matches all allowed piTarget name start char's
                        ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
                        0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
                        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
                        0x10000,0xeffff
                    )
                ),
                ClosingTagFromDynList(MostRecentElement->name),
                createWordMatchList(4,                      //Comment
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(2,'!','!'),
                    createCharMatchList(2,'-','-'),
                    createCharMatchList(2,'-','-')
                ),
                createWordMatchList(3,                      //Processing Instruction
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(2,'?','?'),
                    createCharMatchList(32,        //matches all allowed piTarget name start char's
                        ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
                        0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
                        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
                        0x10000,0xeffff
                    )
                ),
                createWordMatchList(9,
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(2,'!','!'),
                    createCharMatchList(2,'[','['),
                    createCharMatchList(2,'C','C'),
                    createCharMatchList(2,'D','D'),
                    createCharMatchList(2,'A','A'),
                    createCharMatchList(2,'T','T'),
                    createCharMatchList(2,'A','A'),
                    createCharMatchList(2,'[','[')
                ),
                createWordMatchList(1,      //Entity or char reference
                    createCharMatchList(2,'&','&')
                ),
                createWordMatchList(1,
                    createCharMatchList(6,0x21,0xd7ff,0xe000,0xfffd,0x10000,0x10ffff)
                )
            );
            mw_match_element3_withspace=createMultiWordMatchList(7,       //to match content
                createWordMatchList(2,
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(32,        //matches all allowed piTarget name start char's
                        ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
                        0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
                        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
                        0x10000,0xeffff
                    )
                ),
                ClosingTagFromDynList(MostRecentElement->name),
                createWordMatchList(4,                      //Comment
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(2,'!','!'),
                    createCharMatchList(2,'-','-'),
                    createCharMatchList(2,'-','-')
                ),
                createWordMatchList(3,                      //Processing Instruction
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(2,'?','?'),
                    createCharMatchList(32,        //matches all allowed piTarget name start char's
                        ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
                        0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
                        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
                        0x10000,0xeffff
                    )
                ),
                createWordMatchList(9,
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(2,'!','!'),
                    createCharMatchList(2,'[','['),
                    createCharMatchList(2,'C','C'),
                    createCharMatchList(2,'D','D'),
                    createCharMatchList(2,'A','A'),
                    createCharMatchList(2,'T','T'),
                    createCharMatchList(2,'A','A'),
                    createCharMatchList(2,'[','[')
                ),
                createWordMatchList(1,      //Entity or char reference
                    createCharMatchList(2,'&','&')
                ),
                createWordMatchList(1,
                    createCharMatchList(12,0x09,0x09, 0x0a,0x0a, 0x0d,0x0d, 0x20,0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff)
                )
            );
            printUTF32Dynlist(MostRecentElement->name);

            //Used to scan end of chardata
            mw_match_element6=createMultiWordMatchList(2,       //to match content
                createWordMatchList(2,
                    createCharMatchList(2,'<','<'),
                    createCharMatchList(32,        //matches all allowed piTarget name start char's
                        ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
                        0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
                        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
                        0x10000,0xeffff
                    )
                ),
                ClosingTagFromDynList(MostRecentElement->name)
            );
        }

        //TODO maybe ignore whitespace and newlines
        xmlMatchAndMoveOffset(xmlFileDyn,offsetInXMLfilep,mw_match_element3_ignorespace,&matchIndex);
        DlDelete(mw_match_element3_ignorespace);  //we regenerate this so that we can find the corresponding closing tag
        if(matchIndex==match_res_element3_start){
            (*offsetInXMLfilep)+=1;//Skip over "<"
            printf("Opening Tag\n");
            nameStartPosition=(*offsetInXMLfilep);
            (*offsetInXMLfilep)+=1;//Skip over first character in name
            xmlMatchAndMoveOffset(xmlFileDyn,offsetInXMLfilep,mw_match_element2,&matchIndex);
            if(matchIndex==match_res_element2_illegal){
                printf("ERROR: Illegal Char in Name\n");
                exit(1);
            }
            //if whitespace,emptyElement or closetag, we know the name of the tag
            nameEndPosition=(*offsetInXMLfilep);
            {
                struct xmlTreeElement* newElementp=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
                newElementp->parent=MostRecentElement;
                newElementp->name=DlCreate(sizeof(uint32_t),nameEndPosition-nameStartPosition,dynlisttype_utf32chars);
                memcpy(newElementp->name->items,(((uint32_t*)xmlFileDyn->items)+nameStartPosition),sizeof(uint32_t)*(nameEndPosition-nameStartPosition));   //works
                newElementp->type=xmltype_tag;
                newElementp->attributes=0;  //not determined yet
                newElementp->content=0;     //not determined yet
                DlAppend(&(MostRecentElement->content),&newElementp,sizeof(struct xmlTreeElement*),dynlisttype_xmlELMNTCollectionp);
                MostRecentElement->content->type=dynlisttype_xmlELMNTCollectionp;
                MostRecentElement=newElementp;

            }
            printf("Found New Opened Tag with Name: ");
            printUTF32Dynlist(MostRecentElement->name);
            if(matchIndex==match_res_element2_whitespace){
                matchIndex=parseAttributesAndMatchEndOfTag(xmlFileDyn,offsetInXMLfilep,MostRecentElement,
                    createMultiWordMatchList(2,
                        createWordMatchList(2,
                            createCharMatchList(2,'/','/'),
                            createCharMatchList(2,'>','>')
                        ),
                        createWordMatchList(1,
                            createCharMatchList(2,'>','>')
                        )
                    )
                );        //Warning this function deletes mw_match_element
                if(matchIndex==match_res_element5_emptyElement){
                    (*offsetInXMLfilep)+=2; //Move past "/>"
                    MostRecentElement=MostRecentElement->parent;        //tag has been closed so move out one layer
                    printf("Closed Empty Tag with attributes\n");
                }else if(matchIndex==match_res_element5_end){
                    (*offsetInXMLfilep)+=1; //Move past ">"
                    printf("Closed Starting Tag with attributes\n");
                }

                //match_res_element3_attribute=0,match_res_element3_closetag=1,match_res_element3_emptyElement=2
            }else if(matchIndex==match_res_element2_emptyElement){
                MostRecentElement=MostRecentElement->parent;        //tag has been closed so move out one layer
                (*offsetInXMLfilep)+=2; //Skip over "/>"
                printf("Found Empty Tag\n");
            }else{ //matchIndex==match_res_element2_close
                (*offsetInXMLfilep)+=1; //Skip over ">"
                printf("Closed Starting Tag without attributes\n");
            }
        }else if(matchIndex==match_res_element3_closing){
            printf("Corresponding End Tag found\n");
            (*offsetInXMLfilep)+=2+(MostRecentElement->name->itemcnt);    //Skip over "</" and name
            xmlMatchAndMoveOffset(xmlFileDyn,offsetInXMLfilep,mc_match_element4,&matchIndex);
            if(matchIndex==match_res_element4_illegal){
                printf("ERROR: Illegal char in closing tag\n");
                exit(1);
            }
            (*offsetInXMLfilep)+=1; //move past ">"
            MostRecentElement=MostRecentElement->parent;
            if(MostRecentElement==ObjectToAttachResults){
                printf("Info: Outermost Element closed\n");
                printf("TODO: Free SearchLists\n");
                return;
            }
        }else if(matchIndex==match_res_element3_comment){
            //TODO
        }else if(matchIndex==match_res_element3_pi){
            //TODO
        }else if(matchIndex==match_res_element3_cdatasec){
            //TODO
        }else if(matchIndex==match_res_element3_ref){
            //TODO
        }else{      //matchIndex==match_res_element3_chardata
            printf("Found new cdata\n");
            uint32_t cdatastart=(*offsetInXMLfilep);
            xmlMatchAndMoveOffset(xmlFileDyn,offsetInXMLfilep,mw_match_element6,&matchIndex);
            DlDelete(mw_match_element6);
            uint32_t cdataend=(*offsetInXMLfilep);
            printf("cdatas: %d, cdatae: %d",cdatastart,cdataend);
            //TODO include whitespaces
            //Copy cdata somewhere to store it
            struct xmlTreeElement* newCdataElement=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
            newCdataElement->parent=MostRecentElement;
            newCdataElement->name=0;
            newCdataElement->type=xmltype_cdata;
            newCdataElement->attributes=0;
            newCdataElement->content=DlCreate(sizeof(uint32_t),cdataend-cdatastart,dynlisttype_utf32chars);
            memcpy(newCdataElement->content->items,(((uint32_t*)xmlFileDyn->items)+cdatastart),sizeof(uint32_t)*(cdataend-cdatastart));
            DlAppend(&(MostRecentElement->content),&newCdataElement,sizeof(struct xmlFileDyn*),dynlisttype_xmlELMNTCollectionp);

            printf("Info: Found cdata: ");
            printUTF32Dynlist(newCdataElement->content);

            if(matchIndex==match_res_element6_closing){
                (*offsetInXMLfilep)+=2+(MostRecentElement->name->itemcnt);
                xmlMatchAndMoveOffset(xmlFileDyn,offsetInXMLfilep,mc_match_element4,&matchIndex);
                if(matchIndex==match_res_element4_illegal){
                    printf("ERROR: Illegal char in closing tag\n");
                    exit(1);
                }
                (*offsetInXMLfilep)+=1; //move past ">"
                printf("Closing Tag after cdata\n");
                MostRecentElement=MostRecentElement->parent;
                printf("New element to search for:");
                printUTF32Dynlist(MostRecentElement->name);
            }
        }
    }
}



int match_xmldecl(struct DynamicList* xmlFileDyn,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){
    uint32_t matchIndex;
    if(xmlFileDyn->type!=dynlisttype_utf32chars){
        return internal_error_wrong_argument_type;
    }
    enum {match_res_xmldecl1_xmldecl=0,match_res_xmldecl1_nodecl=1};
    struct DynamicList* mw_match_xmldecl1=createMultiWordMatchList(2,
        createWordMatchList(5,                      //xml declaration
            createCharMatchList(2,'<','<'),
            createCharMatchList(2,'?','?'),
            createCharMatchList(2,'x','x'),
            createCharMatchList(2,'m','m'),
            createCharMatchList(2,'l','l'),
            createCharMatchList(8,0x09,0x09,0x0a,0x0a,0x0d,0x0d,0x20,0x20) //whitespace must follow the XMLDecl
        ),
        createWordMatchList(1,                      //no xml declaration is this file, so go ahead
            createCharMatchList(12,0x09,0x09,0x0a,0x0a,0x0d,0x0d,0x20,0xd7ff,0xe000,0xfffd,0x10000,0x10ffff)
        )
    );
    xmlMatchAndMoveOffset(xmlFileDyn,offsetInXMLfilep,mw_match_xmldecl1,&matchIndex);
    DlDelete(mw_match_xmldecl1);
    if(matchIndex==match_res_xmldecl1_nodecl){
        printf("Info: Document has no XMLDecl\n");
        ObjectToAttachResults->attributes=0;      //No docdeclaration, therefore we don't add documents attributes
        return 0;
    }
    (*offsetInXMLfilep)+=6;     //move current position after "<?xml "
    printf("Info: Matching xmldecl start.\n");
    //Parse xmldecl
    parseAttributesAndMatchEndOfTag(xmlFileDyn,offsetInXMLfilep,ObjectToAttachResults,createMultiWordMatchList(1,
        createWordMatchList(2,
            createCharMatchList(2,'?','?'),
            createCharMatchList(2,'>','>')
        )
    ));
    (*offsetInXMLfilep)+=2; //Move over "?>"
    struct DynamicList* w_match_xmldecl2=createWordMatchList(7,
        createCharMatchList(2,'v','v'),
        createCharMatchList(2,'e','e'),
        createCharMatchList(2,'r','r'),
        createCharMatchList(2,'s','s'),
        createCharMatchList(2,'i','i'),
        createCharMatchList(2,'o','o'),
        createCharMatchList(2,'n','n')
    );
    struct DynamicList* w_match_xmldecl3=createWordMatchList(3,
        createCharMatchList(2,'1','1'),
        createCharMatchList(2,'.','.'),
        createCharMatchList(2,'0','9')
    );
    if(ObjectToAttachResults->attributes->itemcnt){
        if(getOffsetUntil(((struct key_val_pair*)ObjectToAttachResults->attributes->items)[0].key->items,7,w_match_xmldecl2,&matchIndex)){
            printf("ERROR: XMLdecl missing version key in attributs\n");
            exit(1);
        }
        DlDelete(w_match_xmldecl2);
        if(getOffsetUntil(((struct key_val_pair*)ObjectToAttachResults->attributes->items)[0].value->items,7,w_match_xmldecl3,&matchIndex)){
            printf("ERROR: XMLdecl has illegal version value in attributes\n");
        }
        printf("TODO check last part of xmldecl version val\n");
        return 1;
    }else{
        printf("Error: xmldecl did not contain any attributes\n");
        exit(1);
    }
}

//Returns negative values for error, 0 for file has ended or something else found, and 1 for at least one misc matched
int match_misc(struct DynamicList* xmlFileDyn,uint32_t* offsetInXMLfilep,struct xmlTreeElement* ObjectToAttachResults){
    uint32_t didMatchSomethingFLAG=0;
    if(xmlFileDyn->type!=dynlisttype_utf32chars){
        return internal_error_wrong_argument_type;
    }
    do{
        uint32_t matchIndex;
        enum {match_res_misc1_comment=0,match_res_misc1_pi=1,match_res_misc1_nonspace=2};
        struct DynamicList* mw_match_misc1=createMultiWordMatchList(3,
            createWordMatchList(4,                      //Comment
                createCharMatchList(2,'<','<'),
                createCharMatchList(2,'!','!'),
                createCharMatchList(2,'-','-'),
                createCharMatchList(2,'-','-')
            ),
            createWordMatchList(3,                      //Processing Instruction
                createCharMatchList(2,'<','<'),
                createCharMatchList(2,'?','?'),
                createCharMatchList(32,        //matches all allowed piTarget name start char's
                    ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
                    0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
                    0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
                    0x10000,0xeffff
                )
            ),
            createWordMatchList(1,                      //something else than misc
                createCharMatchList(6, 0x21,0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff) //match any other character except space
            )
        );
        //TODO account for the case that the xml file realy ends with a space character
        (*offsetInXMLfilep)+=getOffsetUntil(((uint32_t*)xmlFileDyn->items)+(*offsetInXMLfilep),xmlFileDyn->itemcnt-(*offsetInXMLfilep),mw_match_misc1,&matchIndex);
        if((*offsetInXMLfilep)==xmlFileDyn->itemcnt){
            DlDelete(mw_match_misc1);
            printf("Info: EOF\n");
            return misc_ret_eof;
        }
        DlDelete(mw_match_misc1);
        if(matchIndex==match_res_misc1_comment){
            didMatchSomethingFLAG=1;
            (*offsetInXMLfilep)+=4;  //Skip past "<!--"
            uint32_t innerStartOfComment=(*offsetInXMLfilep);
            //Find -->
            enum {match_res_comment1_commentend=0,match_res_comment1_illegalminus=1,match_res_comment1_illegalchar=2};
            struct DynamicList* mw_match_comment1=createMultiWordMatchList(3,
                createWordMatchList(3,
                    createCharMatchList(2,'-','-'),
                    createCharMatchList(2,'-','-'),
                    createCharMatchList(2,'>','>')
                ),
                createWordMatchList(2,          //any occurrence of "--" in the comment is illegal
                    createCharMatchList(2,'-','-'),
                    createCharMatchList(2,'-','-')
                )
            );
            xmlMatchAndMoveOffset(xmlFileDyn,offsetInXMLfilep,mw_match_comment1,&matchIndex);
            DlDelete(mw_match_comment1);
            if(matchIndex==match_res_comment1_illegalminus){
                return error_comment_doubleminus;
            }
            uint32_t innerEndOfComment=(*offsetInXMLfilep);     //index is one character after the actual end index
            (*offsetInXMLfilep)+=3;     //Skip past "-->"

            //Copy tagcontent to object

            struct DynamicList* commentText=DlCreate(sizeof(uint32_t),innerEndOfComment-innerStartOfComment,dynlisttype_utf32chars);
            memcpy(commentText->items,(((uint32_t*)xmlFileDyn->items)+innerStartOfComment),sizeof(uint32_t)*(innerEndOfComment-innerStartOfComment));


            struct xmlTreeElement* commentElement=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
            commentElement->type=xmltype_comment;
            commentElement->attributes=0;
            commentElement->name=0;
            commentElement->parent=ObjectToAttachResults;
            commentElement->content=commentText;

            DlAppend(&(ObjectToAttachResults->content),&commentElement,sizeof(struct DynamicList*),dynlisttype_xmlELMNTCollectionp);

        }else if(matchIndex==match_res_misc1_pi){
            didMatchSomethingFLAG=1;
            (*offsetInXMLfilep)+=2;  //Skip past "<?"
            uint32_t innerStartOfPiTarget=(*offsetInXMLfilep);
            //find PITarget
            //scan for name
            //Make sure that "xml is not following"
            struct DynamicList* w_match_name1=createWordMatchList(3,
                createCharMatchList(4,'x','x','X','X'),
                createCharMatchList(4,'m','m','M','M'),
                createCharMatchList(4,'l','l','L','L')
            );
            if(0==getOffsetUntil(((uint32_t*)xmlFileDyn->items)+(*offsetInXMLfilep),3,w_match_name1,NULL)){ //We have xml following
                printf("Error: Unexpected document declaration <?xml !\n");
                return error_unexpected_docdecl;
            }
            DlDelete(w_match_name1);
            (*offsetInXMLfilep)+=1; //Skip over name start character, (was matched with pi start)
            //get the rest of the name
            enum{match_res_pi2_whitespace=0,match_res_pi2_end=1,match_res_pi2_illegal=2};
            struct DynamicList* mw_match_pi2=createMultiWordMatchList(3,
                createWordMatchList(1,
                    createCharMatchList(8,0x09,0x09,0x0a,0x0a,0x0d,0x0d,0x20,0x20) //whitespace
                ),
                createWordMatchList(2,
                    createCharMatchList(2,'?','?'),                                 //end of pi
                    createCharMatchList(2,'>','>')
                ),
                createWordMatchList(1,
                    createCharMatchList(34,                                         //List of characters that are not allowed to occur in name
                        0x21,0x2c, 0x2f,0x2f, 0x3b,0x40, 0x5b,0x5e, 0x60,0x60,
                        0x7b,0xb6, 0xb8,0xbf, 0xd7,0xd7, 0xf7,0xf7, 0x37e,0x37e,
                        0x2000,0x200b, 0x200e,0x203e, 0x2041,0x2069, 0x2190,0x2bff, 0x2ff0,0x3000,
                        0xe000,0xf8ff, 0xfdd0,0xfdef
                    )
                )
            );
            xmlMatchAndMoveOffset(xmlFileDyn,offsetInXMLfilep,mw_match_pi2,&matchIndex);
            DlDelete(mw_match_pi2);
            //Check if name has any illegal characters in it
            if(matchIndex==match_res_pi2_illegal){
                printf("ERROR: Illegal char in Name\n");
                return error_illegal_char;
            }
            //Copy name to new xml Object
            uint32_t innerEndOfPTarget=(*offsetInXMLfilep); //point to the character following piTarget
            struct DynamicList* pitarget=DlCreate(sizeof(uint32_t),innerEndOfPTarget-innerStartOfPiTarget,dynlisttype_utf32chars);
            memcpy(pitarget->items,(((uint32_t*)xmlFileDyn->items)+innerStartOfPiTarget),sizeof(uint32_t)*(innerEndOfPTarget-innerStartOfPiTarget));

            struct xmlTreeElement* PIElement=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
            PIElement->type=xmltype_pi;
            PIElement->name=pitarget;
            PIElement->parent=ObjectToAttachResults;
            PIElement->content=0;
            PIElement->attributes=0;
            DlAppend(&(ObjectToAttachResults->content),&PIElement,sizeof(struct DynamicList*),dynlisttype_xmlELMNTCollectionp);


            if(matchIndex==match_res_pi2_whitespace){ //we have some text following
                //Search for "!>"
                (*offsetInXMLfilep)+=1; //move past whitespace
                uint32_t innerStartOfPiContent=(*offsetInXMLfilep);
                enum{match_res_pi3_end=0};
                struct DynamicList* mw_match_pi3=createMultiWordMatchList(1,
                    createWordMatchList(2,
                        createCharMatchList(2,'?','?'),                                 //end of pi
                        createCharMatchList(2,'>','>')
                    )
                );
                xmlMatchAndMoveOffset(xmlFileDyn,offsetInXMLfilep,mw_match_pi3,&matchIndex);
                uint32_t innerEndOfPiContent=(*offsetInXMLfilep);
                (*offsetInXMLfilep)+=2; //move past "?>"
                struct DynamicList* picontent=DlCreate(sizeof(uint32_t),innerEndOfPiContent-innerStartOfPiContent,dynlisttype_utf32chars);
                memcpy(picontent->items,(((uint32_t*)xmlFileDyn->items)+innerStartOfPiContent),sizeof(uint32_t)*(innerEndOfPiContent-innerStartOfPiContent));
                PIElement->content=picontent;
            }
        }else{      //matchIndex==match_res_misc1_nonspace
            didMatchSomethingFLAG=0;
            //No more misc to process
        }
    }while(didMatchSomethingFLAG);
    return misc_ret_charLeft;
}



int writeXML(FILE* xmlOutFile,struct xmlTreeElement* inputDocumentRoot){
    struct DynamicList* utf32XmlDlP=xmlDOMtoUTF32(inputDocumentRoot,4);
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
    uint32_t subindex=0;
    int writeOpeningTagToggle=1;
    do{
        uint32_t itemcnt;
        if(CurrentXMLTreeElement->content==0){
            itemcnt=0;
        }else{
            itemcnt=CurrentXMLTreeElement->content->itemcnt;
        }
        if(CurrentXMLTreeElement->parent==LastXMLTreeElement){      //walk deeper
            if(writeOpeningTagToggle){//-------write start tag, attributes
                //write start of start tag
                //calculate space requirements
                uint32_t requieredChars=1+numSpacesIndent*currentDepth+1+(CurrentXMLTreeElement->name->itemcnt);//for 1*"\n", 1*"<", name
                //allocate storage
                tempUTF32String=realloc(tempUTF32String,sizeof(uint32_t)*(numUTF32Chars+requieredChars));
                //write changes
                tempUTF32String[numUTF32Chars++]='\n';
                for(uint32_t i=0;i<numSpacesIndent*currentDepth;i++){
                    tempUTF32String[numUTF32Chars++]=' ';
                }
                tempUTF32String[numUTF32Chars++]='<';
                memcpy(tempUTF32String+numUTF32Chars,CurrentXMLTreeElement->name->items,sizeof(uint32_t)*CurrentXMLTreeElement->name->itemcnt);
                printf("name: %s\n",utf32dynlist_to_string(CurrentXMLTreeElement->name));
                numUTF32Chars+=CurrentXMLTreeElement->name->itemcnt;
                if(CurrentXMLTreeElement->attributes){      //check if element even has attributes
                    for(uint32_t attributenum=0; attributenum<CurrentXMLTreeElement->attributes->itemcnt; attributenum++){
                        printf("test");
                        struct DynamicList* KeyDynlistp=(((struct key_val_pair*)CurrentXMLTreeElement->attributes->items)[attributenum]).key;     //Problematic segfault
                        struct DynamicList* ValDynlistp=(((struct key_val_pair*)CurrentXMLTreeElement->attributes->items)[attributenum]).value;
                        //calculate space requirements
                        printf("key: %s\n",utf32dynlist_to_string(KeyDynlistp));
                        printf("val: %s\n",utf32dynlist_to_string(ValDynlistp));
                        requieredChars=1+KeyDynlistp->itemcnt+1+1+ValDynlistp->itemcnt+1;        //for key, 1x" ", 1x"=", 1x"\"", value, 1x"\""
                        //allocate storage
                        tempUTF32String=realloc(tempUTF32String,sizeof(uint32_t)*(numUTF32Chars+requieredChars));
                        tempUTF32String[numUTF32Chars++]=' ';
                        //Write key
                        memcpy(tempUTF32String+numUTF32Chars,KeyDynlistp->items,sizeof(uint32_t)*KeyDynlistp->itemcnt);
                        numUTF32Chars+=KeyDynlistp->itemcnt;
                        tempUTF32String[numUTF32Chars++]='=';
                        tempUTF32String[numUTF32Chars++]='"';
                        //Write value
                        memcpy(tempUTF32String+numUTF32Chars,ValDynlistp->items,sizeof(uint32_t)*ValDynlistp->itemcnt);
                        numUTF32Chars+=ValDynlistp->itemcnt;
                        tempUTF32String[numUTF32Chars++]='"';
                    }
                }
                if(itemcnt){            //we have some subtags
                    tempUTF32String=realloc(tempUTF32String,sizeof(uint32_t)*(numUTF32Chars+1));
                    tempUTF32String[numUTF32Chars++]='>';
                }else{
                    tempUTF32String=realloc(tempUTF32String,sizeof(uint32_t)*(numUTF32Chars+2));
                    tempUTF32String[numUTF32Chars++]='/';
                    tempUTF32String[numUTF32Chars++]='>';
                }
            }//-------write start tag, attributes end
            writeOpeningTagToggle=1;
            #define maxDepthTMP 100
            for(;(subindex<itemcnt)&&(currentDepth<maxDepthTMP);subindex++){        //go over all subelements that are not to deeply nested
                if(((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[subindex]->type==xmltype_tag){  //One valid subelement found
                    LastXMLTreeElement=CurrentXMLTreeElement;
                    CurrentXMLTreeElement=((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[subindex];
                    subindex=0;
                    currentDepth++;
                    //TODO? This tests only elements of xmltype_tag
                    break;
                }else{
                    //-------write Comments, Pi
                    dprintf(DBGT_ERROR,"Non standard tag"); //like comment or other chardata
                }
            }
        }else{      //go back upward
            for(subindex=0;(subindex<itemcnt)&&(LastXMLTreeElement!=((struct xmlTreeElement**)CurrentXMLTreeElement->content->items)[subindex]);subindex++){     //go over all subelements of our parent and search ourself
            }
            currentDepth--;
            if(++subindex<itemcnt){  //Do we have more elements in the current Element, (assume one more element, if this is more than the elements in our parent then we have scanned the last element)
                LastXMLTreeElement=CurrentXMLTreeElement->parent;
            }else{
                {//-------write closing tag
                    //calculate space requirements
                    uint32_t requieredChars=1+numSpacesIndent*currentDepth+1+1+(CurrentXMLTreeElement->name->itemcnt)+1;//for 1*"\n", 1*"<", 1*"/", name 1*">"
                    //allocate storage
                    tempUTF32String=realloc(tempUTF32String,sizeof(uint32_t)*(numUTF32Chars+requieredChars));
                    tempUTF32String[numUTF32Chars++]='\n';
                    for(uint32_t i=0;i<numSpacesIndent*currentDepth;i++){
                        tempUTF32String[numUTF32Chars++]=' ';
                    }
                    tempUTF32String[numUTF32Chars++]='<';
                    tempUTF32String[numUTF32Chars++]='/';
                    memcpy(tempUTF32String+numUTF32Chars,CurrentXMLTreeElement->name->items,sizeof(uint32_t)*CurrentXMLTreeElement->name->itemcnt);
                    numUTF32Chars+=CurrentXMLTreeElement->name->itemcnt;
                    tempUTF32String[numUTF32Chars++]='>';
                }
                subindex=itemcnt;    //a for loop would have set subindex to itemcnt in its last iteration -> further upward is true
            }
        }
        if(subindex==itemcnt){//Turn around/further upward
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

