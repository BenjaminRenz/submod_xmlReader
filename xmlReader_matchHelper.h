

//Phase 1
enum {
    MWMres_p1_XMLDecl_start=0,
    MWMres_p1_doctype_start=1,
    MWMres_p1_element_start=2,
    MWMres_p1_pi_start=3,
    MWMres_p1_comment_start=4,
    MWMres_p1_NonSpaceChar=5,
    MWMres_p1_IllegalChar=6
};
struct DynamicList* MWM_p1_start;

//Phase 2


struct DynamicList* WM_XMLDecl_start;
struct DynamicList* WM_XMLDecl_end;
struct DynamicList* WM_doctype_start;
struct DynamicList* WM_element_start;
struct DynamicList* WM_cdata_start;     //can only occur inside elements
struct DynamicList* WM_cdata_end;
struct DynamicList* WM_comment_start;
struct DynamicList* WM_comment_end;
struct DynamicList* WM_pi_start;
struct DynamicList* WM_pi_end;
struct DynamicList* WM_attlist_end;
struct DynamicList* CM_CharData_start;
struct DynamicList* CM_IllegalChar;
struct DynamicList* CM_NonSpaceChar;
struct DynamicList* CM_SpaceChar;

void init_matchlists(void){
    CM_CharData_start=createCharMatchList(2,'&','&');       //used for something like &amp; or &lt;
    CM_IllegalChar=createCharMatchList(0x0,0x8, 0xb,0xc, 0xe,0x1f, 0xd800,0xdfff, 0xfffe,0xffff);
    CM_NonSpaceChar=createCharMatchList(6,0x21,0xd7ff,0xe000,0xfffd,0x10000,0x10ffff);   //anything else except space
    CM_SpaceChar=createCharMatchList(12,0x09,0x09, 0x0a,0x0a, 0x0d,0x0d, 0x20,0xd7ff, 0xe000,0xfffd, 0x10000,0x10ffff);
    WM_IllegalChar=createWordMatchList(1,CM_IllegalChar);
    WM_NonSpaceChar=createWordMatchList(1,CM_NonSpaceChar);

    WM_XMLDecl_start=createWordMatchList(5,                      //xml declaration
        createCharMatchList(2,'<','<'),
        createCharMatchList(2,'?','?'),
        createCharMatchList(2,'x','x'),
        createCharMatchList(2,'m','m'),
        createCharMatchList(2,'l','l'),
        createCharMatchList(8,0x09,0x09,0x0a,0x0a,0x0d,0x0d,0x20,0x20) //whitespace must follow the XMLDecl
    );
    WM_XMLDecl_end=createWordMatchList(2,
        createCharMatchList(2,'?','?'),
        createCharMatchList(2,'>','>')
    );
    WM_doctype_start=createWordMatchList(10,
        createCharMatchList(2,'<','<'),
        createCharMatchList(2,'!','!'),
        createCharMatchList(2,'D','D'),
        createCharMatchList(2,'O','O'),
        createCharMatchList(2,'C','C'),
        createCharMatchList(2,'T','T'),
        createCharMatchList(2,'Y','Y'),
        createCharMatchList(2,'P','P'),
        createCharMatchList(2,'E','E'),
        createCharMatchList(8,0x09,0x09,0x0a,0x0a,0x0d,0x0d,0x20,0x20)  //whitespace must follow
    );
    WM_element_start=createWordMatchList(2,
        createCharMatchList(2,'<','<'),
        createCharMatchList(32,        //matches all allowed name start char's
            ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
            0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
            0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
            0x10000,0xeffff
        )
    );
    WM_cdata_start=createWordMatchList(9,
        createCharMatchList(2,'<','<'),
        createCharMatchList(2,'!','!'),
        createCharMatchList(2,'[','['),
        createCharMatchList(2,'C','C'),
        createCharMatchList(2,'D','D'),
        createCharMatchList(2,'A','A'),
        createCharMatchList(2,'T','T'),
        createCharMatchList(2,'A','A'),
        createCharMatchList(2,'[','[')
    );
    WM_cdata_end=createWordMatchList(3,
        createCharMatchList(2,']',']'),
        createCharMatchList(2,']',']'),
        createCharMatchList(2,'>','>')
    );
    WM_attlist_end=createWordMatchList(9,
        createCharMatchList(2,'<','<'),
        createCharMatchList(2,'!','!'),
        createCharMatchList(2,'A','A'),
        createCharMatchList(2,'T','T'),
        createCharMatchList(2,'T','T'),
        createCharMatchList(2,'L','L'),
        createCharMatchList(2,'I','I'),
        createCharMatchList(2,'S','S'),
        createCharMatchList(2,'T','T')
    );

    WM_comment_start=createWordMatchList(4,                      //Comment
        createCharMatchList(2,'<','<'),
        createCharMatchList(2,'!','!'),
        createCharMatchList(2,'-','-'),
        createCharMatchList(2,'-','-')
    );
    WM_comment_end=createWordMatchList(3,                      //Comment
        createCharMatchList(2,'-','-'),
        createCharMatchList(2,'-','-'),
        createCharMatchList(2,'>','>')
    );
    WM_pi_start=createWordMatchList(3,                      //Processing Instruction
        createCharMatchList(2,'<','<'),
        createCharMatchList(2,'?','?'),
        createCharMatchList(32,        //matches all allowed piTarget name start char's
            ':',':', 'A','Z', '_','_', 'a','z', 0xc0,0xd6,
            0xd8,0xf6, 0xf8,0x2ff, 0x370,0x37d, 0x37f,0x1fff, 0x200c,0x200d,
            0x2070,0x218f, 0x2c00,0x2fef, 0x3001,0xd7ff, 0xf900,0xfdcf, 0xfdf0,0xfffd,
            0x10000,0xeffff
        )
    );
    WM_pi_end=createWordMatchList(2,                      //Processing Instruction
        createCharMatchList(2,'?','?'),
        createCharMatchList(2,'>','>')
    );

    MWM_p1_start=createMultiWordMatchList(,
        WM_XMLDecl_start,
        WM_doctype_start,
        WM_element_start,
        WM_pi_start,
        WM_comment_start,
        WM_NonSpaceChar,
        WM_IllegalChar
    )


}

void deinit_matchlists(void){
    DlDelete(WM_XMLDecl_start);
    DlDelete(WM_XMLDecl_end);
    DlDelete(WM_doctype_start);
    DlDelete(WM_element_start);
    DlDelete(WM_cdata_start);
    DlDelete(WM_cdata_end);
}
