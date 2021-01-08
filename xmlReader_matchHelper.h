//Do NOT include carelessly!
//before using any of the DynamicLists init_matchlists() has to be called
//also it should not be called twice

struct DynamicList* CM_CharData_start;
struct DynamicList* CM_IllegalChar;
struct DynamicList* CM_NonSpaceChar;
struct DynamicList* CM_AnyChar;
struct DynamicList* CM_SpaceChar;
struct DynamicList* CM_NameChar;
struct DynamicList* CM_NameStartChar;
struct DynamicList* CM_Equals;
struct DynamicList* CM_QuoteSingle;
struct DynamicList* CM_QuoteDouble;
struct DynamicList* CM_PubidChar_withoutQuotes;
struct DynamicList* CM_CharData;
struct DynamicList* CM_LessThanChar;
struct DynamicList* MCM_Quotes;
struct DynamicList* MCM_PubidChar;
struct DynamicList* WM_NameStartChar;
struct DynamicList* WM_element_endTag;
struct DynamicList* WM_element_endNonEmpty;
struct DynamicList* WM_element_endEmpty;
struct DynamicList* WM_SpaceChar;
struct DynamicList* WM_XMLDecl_start;
struct DynamicList* WM_XMLDecl_end;
struct DynamicList* WM_doctype_start;
struct DynamicList* WM_element_start;
struct DynamicList* WM_cdata_start;
struct DynamicList* WM_cdata_end;
struct DynamicList* WM_comment_start;
struct DynamicList* WM_comment_end;
struct DynamicList* WM_pi_start;
struct DynamicList* WM_pi_end;
struct DynamicList* WM_IllegalChar;
struct DynamicList* WM_NonSpaceChar;
struct DynamicList* WM_attlist_end;
struct DynamicList* MWM_NameStartChar;
struct DynamicList* MWM_element_end;
struct DynamicList* MWM_start;


void init_matchlists(void){
    CM_CharData_start=Dl_CMatch_create(2,'&','&');       //used for something like &amp; or &lt;
    CM_IllegalChar=Dl_CMatch_create(0x0,0x8, 0xb,0xc, 0xe,0x1f, 0xd800,0xdfff, 0xfffe,0xffff);
    CM_NonSpaceChar=Dl_CMatch_create(6,0x21,0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff);   //anything else except space
    CM_SpaceChar=Dl_CMatch_create(8,0x9,0x9, 0xd,0xd, 0xa,0xa, 0x20,0x20);
    CM_AnyChar=Dl_CMatch_create(12,0x09,0x09, 0x0a,0x0a, 0x0d,0x0d, 0x20,0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff);
    CM_CharData=Dl_CMatch_create(14,0x09,0x09, 0x0a,0x0a, 0x0d,0x0d, 0x20,'<', '<',0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff);
    CM_NameStartChar=Dl_CMatch_create(32,        //matches all allowed name start char's
        ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
        0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
        0x10000,0xeffff
    );
    CM_NameChar=Dl_CMatch_create(32,        //matches all allowed name start char's
        ':',':', 'A','Z', '_','_', 'a','z', '-','-', '.','.', '0','9', 0xb7,0xb7,
        0xc0,0xd6, 0xd8,0xf6, 0xf8,0x37d, 0x37f,0x1fff, 0x200c,0x200d, 0x203F, 0x2040,
        0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
        0x10000,0xeffff
    );
    //TODO some of the ranges below can be combined
    CM_PubidChar_withoutQuotes=Dl_CMatch_create(46,
        0x20,0x20, 0xd,0xd, 0xa,0xa, 'a','z', 'A','Z', '0','9', '-','-',
        '(',')', '+','+', ',',',', '.','.', '/','/', ':',':', '=','=', '?','?',
        ';',';', '!','!', '*','*', '#','#', '@','@', '$','$', '_','_', '%','%');
    CM_Equals=Dl_CMatch_create(2,'=','=');
    CM_QuoteSingle=Dl_CMatch_create(2,'\'','\'');
    CM_QuoteDouble=Dl_CMatch_create(2,'"','"');
    CM_LessThanChar=Dl_CMatch_create(2,'<','<');
    MCM_PubidChar=Dl_MCMatchP_create(2,CM_PubidChar_withoutQuotes,CM_QuoteSingle);
    MCM_Quotes=Dl_MCMatchP_create(2,CM_QuoteSingle,CM_QuoteDouble);
    WM_IllegalChar=Dl_WMatchP_create(1,DlDuplicate(sizeof(uint32_t),CM_IllegalChar));
    WM_NonSpaceChar=Dl_WMatchP_create(1,DlDuplicate(sizeof(uint32_t),CM_NonSpaceChar));
    WM_NameStartChar=Dl_WMatchP_create(1,CM_NameStartChar);
    WM_SpaceChar=Dl_WMatchP_create(1,CM_SpaceChar);
    WM_XMLDecl_start=Dl_WMatchP_create(5,                      //xml declaration
        Dl_CMatch_create(2,'<','<'),
        Dl_CMatch_create(2,'?','?'),
        Dl_CMatch_create(2,'x','x'),
        Dl_CMatch_create(2,'m','m'),
        Dl_CMatch_create(2,'l','l'),
        DlDuplicate(sizeof(uint32_t),CM_SpaceChar) //whitespace must follow the XMLDecl
    );
    WM_XMLDecl_end=Dl_WMatchP_create(2,
        Dl_CMatch_create(2,'?','?'),
        Dl_CMatch_create(2,'>','>')
    );
    WM_doctype_start=Dl_WMatchP_create(10,
        Dl_CMatch_create(2,'<','<'),
        Dl_CMatch_create(2,'!','!'),
        Dl_CMatch_create(2,'D','D'),
        Dl_CMatch_create(2,'O','O'),
        Dl_CMatch_create(2,'C','C'),
        Dl_CMatch_create(2,'T','T'),
        Dl_CMatch_create(2,'Y','Y'),
        Dl_CMatch_create(2,'P','P'),
        Dl_CMatch_create(2,'E','E'),
        DlDuplicate(sizeof(uint32_t),CM_SpaceChar)  //whitespace must follow
    );
    WM_element_start=Dl_WMatchP_create(2,
        DlDuplicate(sizeof(uint32_t),CM_LessThanChar),
        DlDuplicate(sizeof(uint32_t),CM_NameStartChar)
    );
    WM_element_endTag=Dl_WMatchP_create(3,
        Dl_CMatch_create(2,'<','<'),
        Dl_CMatch_create(2,'/','/'),
        DlDuplicate(sizeof(uint32_t),CM_NameStartChar)
    );
    WM_element_endNonEmpty=Dl_WMatchP_create(1,
        Dl_CMatch_create(2,'>','>')
    );
    WM_element_endEmpty=Dl_WMatchP_create(2,
        Dl_CMatch_create(2,'/','/'),
        Dl_CMatch_create(2,'>','>')
    );
    WM_cdata_start=Dl_WMatchP_create(9,
        Dl_CMatch_create(2,'<','<'),
        Dl_CMatch_create(2,'!','!'),
        Dl_CMatch_create(2,'[','['),
        Dl_CMatch_create(2,'C','C'),
        Dl_CMatch_create(2,'D','D'),
        Dl_CMatch_create(2,'A','A'),
        Dl_CMatch_create(2,'T','T'),
        Dl_CMatch_create(2,'A','A'),
        Dl_CMatch_create(2,'[','[')
    );
    WM_cdata_end=Dl_WMatchP_create(3,
        Dl_CMatch_create(2,']',']'),
        Dl_CMatch_create(2,']',']'),
        Dl_CMatch_create(2,'>','>')
    );
    WM_attlist_end=Dl_WMatchP_create(9,
        Dl_CMatch_create(2,'<','<'),
        Dl_CMatch_create(2,'!','!'),
        Dl_CMatch_create(2,'A','A'),
        Dl_CMatch_create(2,'T','T'),
        Dl_CMatch_create(2,'T','T'),
        Dl_CMatch_create(2,'L','L'),
        Dl_CMatch_create(2,'I','I'),
        Dl_CMatch_create(2,'S','S'),
        Dl_CMatch_create(2,'T','T')
    );
    WM_comment_start=Dl_WMatchP_create(4,                      //Comment
        Dl_CMatch_create(2,'<','<'),
        Dl_CMatch_create(2,'!','!'),
        Dl_CMatch_create(2,'-','-'),
        Dl_CMatch_create(2,'-','-')
    );
    WM_comment_end=Dl_WMatchP_create(3,                      //Comment
        Dl_CMatch_create(2,'-','-'),
        Dl_CMatch_create(2,'-','-'),
        Dl_CMatch_create(2,'>','>')
    );
    WM_pi_start=Dl_WMatchP_create(3,                      //Processing Instruction
        Dl_CMatch_create(2,'<','<'),
        Dl_CMatch_create(2,'?','?'),
        DlDuplicate(sizeof(uint32_t),CM_NameStartChar)
    );
    WM_pi_end=Dl_WMatchP_create(2,                      //Processing Instruction end
        Dl_CMatch_create(2,'?','?'),
        Dl_CMatch_create(2,'>','>')
    );
    MWM_element_end=Dl_MWMatchPP_create(2,
        WM_element_endNonEmpty,
        WM_element_endEmpty
    );
    MWM_start=Dl_MWMatchPP_create(8,
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
    DlDelete(WM_XMLDecl_start);
    DlDelete(WM_XMLDecl_end);
    DlDelete(WM_doctype_start);
    DlDelete(WM_element_start);
    DlDelete(WM_cdata_start);
    DlDelete(WM_cdata_end);
}
