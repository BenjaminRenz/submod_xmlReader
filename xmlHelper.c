#include "xmlReader/xmlHelper.h"
Dl_utf32Char* getValueFromKeyName(Dl_attP* attDlP,Dl_utf32Char* nameDlP){
    if(!attDlP){
        dprintf(DBGT_ERROR,"called getValueFromKeyName with nullptr");
        exit(1);
    }
    if(!attDlP->itemcnt){
        return 0;
    }
    for(size_t attIdx=0;attIdx<attDlP->itemcnt;attIdx++){
        if(Dl_utf32Char_equal(nameDlP,attDlP->items[attIdx]->key)){
            return attDlP->items[attIdx]->value;
        }
    }
    return 0;
}

Dl_utf32Char* getValueFromKeyName_freeArg2(Dl_attP* attDlP,Dl_utf32Char* nameDlP){
    Dl_utf32Char* utf32RetDlP=getValueFromKeyName(attDlP,nameDlP);
    Dl_utf32Char_delete(nameDlP);
    return utf32RetDlP;
}

xmlTreeElement* getNthChildWithType(xmlTreeElement* parentP, uint32_t n, uint32_t typeBitfield){
    if(!parentP){
        dprintf(DBGT_ERROR,"Null passed as parentP");
        return NULL;
    }
    if(!parentP->children->itemcnt){
        dprintf(DBGT_ERROR,"Element does not contain any subelements, it's empty");
        return NULL;
    }
    size_t validXmlIdx=0; //only accounts for the xml elements matched by typeBitfield
    for(size_t allXmlIdx=0;allXmlIdx<parentP->children->itemcnt;allXmlIdx++){
        xmlTreeElement* potentialXmlElementP=parentP->children->items[allXmlIdx];
        //check if at least on typeBit matches
        if(potentialXmlElementP->type&typeBitfield){
            if((validXmlIdx++)==n){
                return potentialXmlElementP;
            }
        }
    }
    return NULL;
}

//TODO max depth not working
Dl_xmlP* getAllSubelementsWith(xmlTreeElement* startingElementP,Dl_utf32Char* nameDlP,Dl_utf32Char* KeyDlP, Dl_utf32Char* ValDlP, uint32_t TypeBitfield, uint32_t maxDepth){
    if(!startingElementP){
        dprintf(DBGT_ERROR,"getFirstSubelement called with empty startingElemenP pointer");
        return NULL;
    }
    Dl_xmlP* returnDlP=Dl_xmlP_alloc(0,NULL);
    xmlTreeElement* LastXMLTreeElementP=startingElementP->parent;
    xmlTreeElement* CurrentXMLTreeElementP=startingElementP;
    uint32_t currentDepth=0;        //for indentation
    #define subindex_needs_reinitialization (-1)
    int32_t subindex=0;             //which child node is currently processed
    do{
        if(CurrentXMLTreeElementP->parent==LastXMLTreeElementP){      //walk direction is downward
            uint32_t objectMatches=1;
            switch(CurrentXMLTreeElementP->type){
                case xmltype_tag:
                case xmltype_docRoot:
                    if(CurrentXMLTreeElementP->children->itemcnt){
                        //Jump to the next subelement
                        currentDepth++;
                        subindex=subindex_needs_reinitialization;
                    }
                    __attribute__ ((fallthrough));
                case xmltype_cdata:
                case xmltype_chardata:
                case xmltype_comment:
                case xmltype_pi:
                    if(nameDlP){
                        if(!Dl_utf32Char_equal(CurrentXMLTreeElementP->name,nameDlP)){
                            objectMatches=0;
                        }
                    }
                    if(KeyDlP){
                        Dl_utf32Char* AttValueString=getValueFromKeyName(CurrentXMLTreeElementP->attributes,KeyDlP);
                        if(AttValueString && (AttValueString->itemcnt)){
                            if(ValDlP && (!Dl_utf32Char_equal(ValDlP,AttValueString))){
                                objectMatches=0;
                            }
                        }else{
                           objectMatches=0;
                        }
                    }
                    if(!(CurrentXMLTreeElementP->type&TypeBitfield)){
                        objectMatches=0;
                    }
                    if(objectMatches){
                        Dl_xmlP_append(returnDlP,1,&CurrentXMLTreeElementP);
                    }
                break;
                default:
                    dprintf(DBGT_ERROR,"Tag of this type is not handled (type %x)",CurrentXMLTreeElementP->type);
                break;
            }
            if(subindex!=subindex_needs_reinitialization){
                //successfully parsed last element, so increment subindex
                //are we the last child inside out parent
                if((uint32_t)(++subindex)==CurrentXMLTreeElementP->parent->children->itemcnt){
                    //yes, then one level upward
                    LastXMLTreeElementP=CurrentXMLTreeElementP;
                    CurrentXMLTreeElementP=CurrentXMLTreeElementP->parent;
                }else{
                    //
                    CurrentXMLTreeElementP=CurrentXMLTreeElementP->parent->children->items[subindex];
                }
            }else{
                //reinitialize pointers to parse first subelement
                subindex=0;
                CurrentXMLTreeElementP=CurrentXMLTreeElementP->children->items[0];
                LastXMLTreeElementP=CurrentXMLTreeElementP->parent;
            }
        }else{
            //go back upward
            //get subindex from the view of the parent
            uint32_t subindex_of_child_we_came_from=0;
            while(subindex_of_child_we_came_from<CurrentXMLTreeElementP->children->itemcnt &&
                  LastXMLTreeElementP!=CurrentXMLTreeElementP->children->items[subindex_of_child_we_came_from]){
                subindex_of_child_we_came_from++;
            }
            //is our object the last child element of our parent?
            if(subindex_of_child_we_came_from==CurrentXMLTreeElementP->children->itemcnt-1){
                //yes, so close our current tag and move one layer up
                currentDepth--;
                //Move one level upward
                LastXMLTreeElementP=CurrentXMLTreeElementP;
                CurrentXMLTreeElementP=CurrentXMLTreeElementP->parent;
            }else{
                //no, so setup to print it next
                LastXMLTreeElementP=CurrentXMLTreeElementP;
                CurrentXMLTreeElementP=CurrentXMLTreeElementP->children->items[subindex_of_child_we_came_from+1];
                subindex=(int)subindex_of_child_we_came_from+1;
            }
        }
    }while(CurrentXMLTreeElementP!=startingElementP->parent);     //TODO error with abort condition when starting from subelement
    return returnDlP;
};

//all Name/Key/Value/Type checks must be true for an element to be appended to the returnDlP, or the input to the check must be NULL
Dl_xmlP* getAllSubelementsWith_freeArg234(xmlTreeElement* startingElementP,Dl_utf32Char* nameDlP,Dl_utf32Char* KeyDlP, Dl_utf32Char* ValDlP, uint32_t TypeBitfield, uint32_t maxDepth){
    Dl_xmlP* returnDlP=getAllSubelementsWith(startingElementP,nameDlP,KeyDlP,ValDlP,TypeBitfield,maxDepth);
    //Delete Dynlists if they are valid pointers
    if(nameDlP){
        Dl_utf32Char_delete(nameDlP);
    }
    if(KeyDlP){
        Dl_utf32Char_delete(KeyDlP);
    }
    if(ValDlP){
        Dl_utf32Char_delete(ValDlP);
    }
    return returnDlP;
}

xmlTreeElement* getFirstSubelementWith_freeArg234(xmlTreeElement* startingElementP,Dl_utf32Char* nameDlP,Dl_utf32Char* KeyDlP, Dl_utf32Char* ValDlP, uint32_t TypeBitfield, uint32_t maxDepth){
    xmlTreeElement* returnXmlElmntP=getFirstSubelementWith(startingElementP,nameDlP,KeyDlP,ValDlP,TypeBitfield,maxDepth);
    //Delete Dynlists if they are valid pointers
    if(nameDlP){
        Dl_utf32Char_delete(nameDlP);
    }
    if(KeyDlP){
        Dl_utf32Char_delete(KeyDlP);
    }
    if(ValDlP){
        Dl_utf32Char_delete(ValDlP);
    }
    return returnXmlElmntP;
}

//maxDepth 0 means only search direct childs of current element
//TODO maxDepth is not working
//For Name, Key, Value and ElmntType a NULL-prt can be passed, which means that this property is ignored when matching
xmlTreeElement* getFirstSubelementWith(xmlTreeElement* startingElementP,Dl_utf32Char* nameDlP,Dl_utf32Char* KeyDlP, Dl_utf32Char* ValDlP, uint32_t TypeBitfield, uint32_t maxDepth){
    if(!startingElementP){
        dprintf(DBGT_ERROR,"getFirstSubelement called with empty startingElemenP pointer");
        return NULL;
    }
    xmlTreeElement* LastXMLTreeElementP=startingElementP->parent;
    xmlTreeElement* CurrentXMLTreeElementP=startingElementP;
    uint32_t currentDepth=0;        //for indentation
    #define subindex_needs_reinitialization (-1)
    int32_t subindex=0;             //which child node is currently processed
    do{
        if(CurrentXMLTreeElementP->parent==LastXMLTreeElementP){      //walk direction is downward
            uint32_t objectMatches=1;
            switch(CurrentXMLTreeElementP->type){
                case xmltype_tag:
                case xmltype_docRoot:
                    if(CurrentXMLTreeElementP->children->itemcnt){
                        //Jump to the next subelement
                        currentDepth++;
                        subindex=subindex_needs_reinitialization;
                    }
                    __attribute__ ((fallthrough));
                case xmltype_cdata:
                case xmltype_chardata:
                case xmltype_comment:
                case xmltype_pi:
                    if(nameDlP){
                        if(!Dl_utf32Char_equal(CurrentXMLTreeElementP->name,nameDlP)){
                            objectMatches=0;
                        }
                    }
                    if(KeyDlP){
                        Dl_utf32Char* AttValueString=getValueFromKeyName(CurrentXMLTreeElementP->attributes,KeyDlP);
                        if(AttValueString && (AttValueString->itemcnt)){
                            if(ValDlP && (!Dl_utf32Char_equal(ValDlP,AttValueString))){
                                objectMatches=0;
                            }
                        }else{
                           objectMatches=0;
                        }
                    }
                    if(!(CurrentXMLTreeElementP->type&TypeBitfield)){
                        objectMatches=0;
                    }
                    if(objectMatches){
                        return CurrentXMLTreeElementP;
                    }
                break;
                default:
                    dprintf(DBGT_ERROR,"Tag of this type is not handled (type %x)",CurrentXMLTreeElementP->type);
                break;
            }
            if(subindex!=subindex_needs_reinitialization){
                //successfully parsed last element, so increment subindex
                //are we the last child inside out parent
                if((uint32_t)(++subindex)==CurrentXMLTreeElementP->parent->children->itemcnt){
                    //yes, then one level upward
                    LastXMLTreeElementP=CurrentXMLTreeElementP;
                    CurrentXMLTreeElementP=CurrentXMLTreeElementP->parent;
                }else{
                    //
                    CurrentXMLTreeElementP=CurrentXMLTreeElementP->parent->children->items[subindex];
                }
            }else{
                //reinitialize pointers to parse first subelement
                subindex=0;
                CurrentXMLTreeElementP=CurrentXMLTreeElementP->children->items[0];
                LastXMLTreeElementP=CurrentXMLTreeElementP->parent;
            }
        }else{
            //go back upward
            //get subindex from the view of the parent
            uint32_t subindex_of_child_we_came_from=0;
            while(subindex_of_child_we_came_from<CurrentXMLTreeElementP->children->itemcnt &&
                  LastXMLTreeElementP!=CurrentXMLTreeElementP->children->items[subindex_of_child_we_came_from]){
                subindex_of_child_we_came_from++;
            }
            //is our object the last child element of our parent?
            if(subindex_of_child_we_came_from==CurrentXMLTreeElementP->children->itemcnt-1){
                //yes, so close our current tag and move one layer up
                currentDepth--;
                //Move one level upward
                LastXMLTreeElementP=CurrentXMLTreeElementP;
                CurrentXMLTreeElementP=CurrentXMLTreeElementP->parent;
            }else{
                //no, so setup to print it next
                LastXMLTreeElementP=CurrentXMLTreeElementP;
                CurrentXMLTreeElementP=CurrentXMLTreeElementP->children->items[subindex_of_child_we_came_from+1];
                subindex=(int)subindex_of_child_we_came_from+1;
            }
        }
    }while(CurrentXMLTreeElementP!=startingElementP->parent);
    return NULL;
};



