//Do NOT include carelessly!
//before using any of the DynamicLists init_matchlists() has to be called
//also it should not be called twice

Dl_CM*  CM_CharData_start;
Dl_CM*  CM_IllegalChar;
Dl_CM*  CM_NonSpaceChar;
Dl_CM*  CM_AnyChar;
Dl_CM*  CM_SpaceChar;
Dl_CM*  CM_NameChar;
Dl_CM*  CM_NameStartChar;
Dl_CM*  CM_Equals;
Dl_CM*  CM_QuoteSingle;
Dl_CM*  CM_QuoteDouble;
Dl_CM*  CM_PubidChar_withoutQuotes;
Dl_CM*  CM_CharData;
Dl_CM*  CM_LessThanChar;
Dl_CM*  CM_PubidChar;
Dl_MCM* MCM_Quotes;
Dl_MCM* MCM_AttNameEndOrEqual;
Dl_WM*  WM_NameStartChar;
Dl_WM*  WM_element_endTag;
Dl_WM*  WM_element_endNonEmpty;
Dl_WM*  WM_element_endEmpty;
Dl_WM*  WM_SpaceChar;
Dl_WM*  WM_XMLDecl_start;
//Dl_WM*  WM_XMLDecl_end;
Dl_WM*  WM_doctype_start;
Dl_WM*  WM_element_start;
Dl_WM*  WM_cdata_start;
Dl_WM*  WM_cdata_end;
Dl_WM*  WM_comment_start;
Dl_WM*  WM_comment_end;
Dl_WM*  WM_pi_start;
Dl_WM*  WM_pi_end;
Dl_WM*  WM_IllegalChar;
Dl_WM*  WM_NonSpaceChar;
Dl_WM*  WM_attlist_end;
Dl_MWM* MWM_XMLDecl_end;
Dl_MWM* MWM_NameStartChar;
Dl_MWM* MWM_element_end;
Dl_MWM* MWM_start;


void init_matchlists(void){

    CM_CharData_start=  Dl_CM_initFromList('&','&');       //used for something like &amp; or &lt;
    CM_IllegalChar=     Dl_CM_initFromList(0x0,0x8, 0xb,0xc, 0xe,0x1f, 0xd800,0xdfff, 0xfffe,0xffff);
    CM_NonSpaceChar=    Dl_CM_initFromList(0x21,0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff);   //anything else except space
    CM_SpaceChar=       Dl_CM_initFromList(0x9,0x9, 0xd,0xd, 0xa,0xa, 0x20,0x20);
    CM_AnyChar=         Dl_CM_initFromList(0x09,0x09, 0x0a,0x0a, 0x0d,0x0d, 0x20,0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff);
    CM_CharData=        Dl_CM_initFromList(0x09,0x09, 0x0a,0x0a, 0x0d,0x0d, 0x20,'<', '<',0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff);
    CM_NameStartChar=   Dl_CM_initFromList(        //matches all allowed name start char's
        ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
        0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
        0x10000,0xeffff
    );
    CM_NameChar=        Dl_CM_initFromList(        //matches all allowed name start char's
        ':',':', 'A','Z', '_','_', 'a','z', '-','-', '.','.', '0','9', 0xb7,0xb7,
        0xc0,0xd6, 0xd8,0xf6, 0xf8,0x37d, 0x37f,0x1fff, 0x200c,0x200d, 0x203F, 0x2040,
        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
        0x10000,0xeffff
    );
    //TODO some of the ranges below can be combined
    CM_PubidChar_withoutQuotes=Dl_CM_initFromList(
        0x20,0x20, 0xd,0xd, 0xa,0xa, 'a','z', 'A','Z', '0','9', '-','-',
        '(',')', '+','+', ',',',', '.','.', '/','/', ':',':', '=','=', '?','?',
        ';',';', '!','!', '*','*', '#','#', '@','@', '$','$', '_','_', '%','%');
    CM_Equals=          Dl_CM_initFromList('=','=');
    CM_QuoteSingle=     Dl_CM_initFromList('\'','\'');
    CM_QuoteDouble=     Dl_CM_initFromList('"','"');
    CM_LessThanChar=    Dl_CM_initFromList('<','<');
    CM_PubidChar=       Dl_CM_initFromList(
        0x20,0x20, 0xd,0xd, 0xa,0xa, 'a','z', 'A','Z', '0','9', '-','-',
        '(',')', '+','+', ',',',', '.','.', '/','/', ':',':', '=','=', '?','?',
        ';',';', '!','!', '*','*', '#','#', '@','@', '$','$', '_','_', '%','%',
        '\'','\'');
    MCM_Quotes=         Dl_MCM_initFromList(CM_QuoteSingle,CM_QuoteDouble);
    MCM_AttNameEndOrEqual=Dl_MCM_initFromList(CM_SpaceChar,CM_Equals);

    WM_IllegalChar=     Dl_WM_initFromList(Dl_CM_shallowCopy(CM_IllegalChar));
    WM_NonSpaceChar=    Dl_WM_initFromList(Dl_CM_shallowCopy(CM_NonSpaceChar));
    WM_NameStartChar=   Dl_WM_initFromList(CM_NameStartChar);
    WM_SpaceChar=       Dl_WM_initFromList(CM_SpaceChar);
    WM_XMLDecl_start=   Dl_WM_initFromList(                      //xml declaration
        Dl_CM_initFromList('<','<'),
        Dl_CM_initFromList('?','?'),
        Dl_CM_initFromList('x','x'),
        Dl_CM_initFromList('m','m'),
        Dl_CM_initFromList('l','l'),
        Dl_CM_shallowCopy(CM_SpaceChar) //whitespace must follow the XMLDecl
    );
    WM_doctype_start=Dl_WM_initFromList(
        Dl_CM_initFromList('<','<'),
        Dl_CM_initFromList('!','!'),
        Dl_CM_initFromList('D','D'),
        Dl_CM_initFromList('O','O'),
        Dl_CM_initFromList('C','C'),
        Dl_CM_initFromList('T','T'),
        Dl_CM_initFromList('Y','Y'),
        Dl_CM_initFromList('P','P'),
        Dl_CM_initFromList('E','E'),
        Dl_CM_shallowCopy(CM_SpaceChar)  //whitespace must follow
    );
    WM_element_start=Dl_WM_initFromList(
        Dl_CM_shallowCopy(CM_LessThanChar),
        Dl_CM_shallowCopy(CM_NameStartChar)
    );
    WM_element_endTag=Dl_WM_initFromList(
        Dl_CM_initFromList('<','<'),
        Dl_CM_initFromList('/','/'),
        Dl_CM_shallowCopy(CM_NameStartChar)
    );
    WM_element_endNonEmpty=Dl_WM_initFromList(
        Dl_CM_initFromList('>','>')
    );
    WM_element_endEmpty=Dl_WM_initFromList(
        Dl_CM_initFromList('/','/'),
        Dl_CM_initFromList('>','>')
    );
    WM_cdata_start=Dl_WM_initFromList(
        Dl_CM_initFromList('<','<'),
        Dl_CM_initFromList('!','!'),
        Dl_CM_initFromList('[','['),
        Dl_CM_initFromList('C','C'),
        Dl_CM_initFromList('D','D'),
        Dl_CM_initFromList('A','A'),
        Dl_CM_initFromList('T','T'),
        Dl_CM_initFromList('A','A'),
        Dl_CM_initFromList('[','[')
    );
    WM_cdata_end=Dl_WM_initFromList(
        Dl_CM_initFromList(']',']'),
        Dl_CM_initFromList(']',']'),
        Dl_CM_initFromList('>','>')
    );
    WM_attlist_end=Dl_WM_initFromList(
        Dl_CM_initFromList('<','<'),
        Dl_CM_initFromList('!','!'),
        Dl_CM_initFromList('A','A'),
        Dl_CM_initFromList('T','T'),
        Dl_CM_initFromList('T','T'),
        Dl_CM_initFromList('L','L'),
        Dl_CM_initFromList('I','I'),
        Dl_CM_initFromList('S','S'),
        Dl_CM_initFromList('T','T')
    );
    WM_comment_start=Dl_WM_initFromList(                      //Comment
        Dl_CM_initFromList('<','<'),
        Dl_CM_initFromList('!','!'),
        Dl_CM_initFromList('-','-'),
        Dl_CM_initFromList('-','-')
    );
    WM_comment_end=Dl_WM_initFromList(                      //Comment
        Dl_CM_initFromList('-','-'),
        Dl_CM_initFromList('-','-'),
        Dl_CM_initFromList('>','>')
    );
    WM_pi_start=Dl_WM_initFromList(                      //Processing Instruction
        Dl_CM_initFromList('<','<'),
        Dl_CM_initFromList('?','?'),
        Dl_CM_shallowCopy(CM_NameStartChar)
    );
    WM_pi_end=Dl_WM_initFromList(                     //Processing Instruction end
        Dl_CM_initFromList('?','?'),
        Dl_CM_initFromList('>','>')
    );
    MWM_XMLDecl_end=Dl_MWM_initFromList(
        Dl_WM_initFromList(
            Dl_CM_initFromList('?','?'),
            Dl_CM_initFromList('>','>')
        )
    );

    MWM_element_end=Dl_MWM_initFromList(
        WM_element_endNonEmpty,
        WM_element_endEmpty
    );
    MWM_start=Dl_MWM_initFromList(
        WM_XMLDecl_start,
        WM_doctype_start,
        WM_element_endTag,      //must be before WM_element_start
        WM_element_start,
        WM_pi_start,
        WM_cdata_start,
        WM_comment_start,
        WM_IllegalChar
    );

}

void deinit_matchlists(void){
    //TODO add all lists
    Dl_WM_delete(WM_XMLDecl_start);
    Dl_WM_delete(WM_doctype_start);
    Dl_WM_delete(WM_element_start);
    Dl_WM_delete(WM_cdata_start);
    Dl_WM_delete(WM_cdata_end);
    Dl_MWM_delete(MWM_XMLDecl_end);
}
