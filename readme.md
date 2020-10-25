# XML to dom tree library #
## usage examples ##
### parse xml file to dom ###
```
FILE* xmlFileP=fopen("./path/to/file.xml","wb");
struct xmlTreeElement* xmlRootP=0;
readXML(xmlFileP,&xmlRootP);
fclose(xmlFileP);
```

### get attributes of one xml element ###

### parse attributes as list of floats ###

### delete one attribute of an xml element ###

### initialize and add a new xml element as a child to an existing parent element ###
```
struct xmlTreeElement* parent = alreadyExistingParentElement;
struct xmlTreeElement* newChildElement = (struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
newChildElement->type        = xmltype_tag;
newChildElement->name        = stringToUTF32Dynlist("tagname");
newChildElement->parent      = parent;
newChildElement->content     = 0;
newChildElement->attributes  = 0;
append_DynamicList(&(parent->content),&newChildElement,sizeof(struct xmlTreeElement*), ); 
```

### initialize and add a new attribute ###
```
struct DynamicList* listToBeAppended = 0; //or an already existing attributes list
struct key_val_pair newkeyvalpair;
newkeyvalpair->key    = stringToUTF32Dynlist("keyname");  // will malloc internally
newkeyvalpair->value  = stringToUTF32Dynlist("true");     // will malloc internally
append_DynamicList(&listToBeAppended,&newkeyvalpair,sizeof(struct key_val_pair),dynlisttype_keyValuePairsp);
```

### append one character to an utf32dynlist ###
```
struct DynamicList* listToBeAppended  = 0;  //or an already existing utf32 list
uint32_t characterToBeAppended        = 'a';
append_DynamicList(&listToBeAppended,&characterToBeAppended,sizeof(uint32_t),dynlisttype_utf32chars);
```

### create new dynamic list ###

### add new element to any element of type dynamic list ###
use the function
`void append_DynamicList(struct DynamicList** ListOrNullpp,void* newElementp,size_t sizeofListElements,uint32_t typeId)`,
which attaches the initialized newElementp at the end of ListOrNullpp. The typeId is for safety checks.




### search for subelements with specific properties ###

## examples of structs in the returned dom tree ##
### root element ###
```
struct xmlTreeElement
|__uint32_t type                  = xmltype_tag;
|__struct DynamicList* name       = 0;      
|__struct xmlTreeElement* parent  = 0;
|__struct DynamicList* content    = contentOfRootDocument;   
|__struct DynamicList* attributes = XmlVersion;
```

### attributes Dynamic List ###
```
struct DynamicList attributes
|__uint32_t type          = dynlisttype_keyValuePairsp;
|__uint32_t itemcnt       = 9;
|__void* items            = (referes to memory address of (struct first_key_valp*) inside this struct)
|__struct key_val_pair*   = first_key_valp;
|__struct key_val_pair*   = second_key_valp;
|__struct key_val_pair*   = ...
```
```
first_key_valp==((struct key_val_pair**)attributes->items)[0]
```

### key_value_pair struct, stores one key-val-pair ###
```
struct key_val_pair first_key_valp
|__struct DynamicList* key    = UTF32string1;
|__struct DynamicList* value  = UTF32string2;
```

### utf32string ###
used as:
* key/val inside key_value_pair struct
* the raw file when parsing completeXmlDocument
```
struct DynamicList UTF32string1
|__uint32_t type    = dynlisttype_utf32chars
|__uint32_t itemcnt = 8                       //arbitrary example
|__void* items      = &first_UTF32_char       //(refers to memory address below, which stores first UTF32char)
|__uint32_t         = first_UTF32_char
|__uint32_t         = second_UTF32_char
...
|__uint32_t         = last_UTF32_char
```

### xmlTreeElement ###
```
struct DynamicList xmlElementCollection
|__uint32_t type          = dynlisttype_xmlELMNTCollectionp;
|__uint32_t itemcnt       = 5;                                //arbitrary example
|__void* items            = &firstElementInXmlFile            //(refers to memory address below, which stores first struct xmlTreeElement*);
|__struct xmlTreeElement* = firstElementInXmlFile;
|__struct xmlTreeElement* = secondElementInXmlFile;
...
|__struct xmlTreeElement* = lastElementInXmlFile;
```

## internal functionality ##
### match lists ###
#### WARNING ####
DO NOT USE ANY DYNAMIC LIST MORE THAN ONCE INSIDE OTHERS (MultiChar-, Word- and MultiWord-MatchLists).

It should be noted that if you nest lists like multiple CharMatchLists into e.g. WordMatchLists, you should NOT get pointers to the CharMatchLists. The reason for this is that it might be tempting to reuse a CharMatchList inside another WordMatchList. But when you delete one of the WordMatchLists all child lists are delted, so the second WordMatchList would have a dangling pointer. If you free both WordMatchLists you would also execute a double free on the duplicated CharMatchList which is undefined behavior. Please directly nest the creation of these lists and don't copy any pointers, if you are not absolutely sure what you are doing.

#### allowed nesting hirarchy the lists ####
```
CharMatchList --(multiple can be nested in)--> MultiCharMatchList
              |
              --(multiple can be nested in)--> WordMatchList --(multiple can be nested in)--> MultiWordMatchList
```
#### enum for match identification ####
because functions like `uint32_t getOffsetUntil(uint32_t* xmlInUtf32, uint32_t maxScanLength, struct DynamicList* MatchAgainst, uint32_t* optional_matchIndex);` will return you an index which corresponds to the sublist which has matched, depending on the order in which they have been initialized. Therefore it is useful to track the order of initialization of MultiChar-, Word- and MultiWord-MatchLists. For the MultiCharMatchList one example would be the enum
```
enum CML1_res_idx{res_letter=0, res_number=1};
```
#### create a char match list ####
to create such a list which matches all characters between 'a' and 'b', aswell as 'd' and 'z' you have to:
```
struct DynamicList* ExampleCharMatchList1=createCharMatchList(4,'a','b','d','z');
```
to only match one character, for example 'a':
```
struct DynamicList* ExampleCharMatchList2=createCharMatchList(2,'a','a');
```

#### multi char match lists ####
to destinguish different groups of characters, like numbers and letters, you can create MultiCharMatchLists:
```
struct DynamicList* ExampleCharMatchList1=createMultiCharMatchList(2,
    createCharMatchList(4,'a','z','A','Z'),
    createCharMatchList(2,'0','9')
);
```
#### word match lists ####
multiple char match lists can be combined into word match lists, each letter of the word will then be compared to the corresponding char match list at this index.
The list below will match (ok, Ok, oK and OK).
```
struct DynamicList* ExampleWordMatchList=createWordMatchList(2,
    createCharMatchList(4,'o','o','O','O'),
    createCharMatchList(4,'k','k','K','K')
);
```

#### multi word match lists ####
If you either want to match against multiple words simultaneously or you have specific requirements for the capitalization (only ok and OK) one use the following example:
```
struct DynamicList* ExampleMultiWordMatchList1=
    createMultiWordMatchList(2,
        createWordMatchList(2,
            createCharMatchList(2,'o','o'),
            createCharMatchList(2,'k','k')
        ),
        createWordMatchList(2,
            createCharMatchList(2,'O','O'),
            createCharMatchList(2,'K','K')
        )
);
```
For each position in the document it starts with the first entry in the MultiWordMatchList and checks if that one matches, if not it tries the second word and so on.
If there is not match for a given position it will increment the position by one and start again with the first WordMatchList.

#### deleting the abovementioned lists ####
When a dynamicList of the char,multichar,word or multiword type is no longer needed the (uppermost list in case of nested) list is passed to
`deleteDynList(uppermostListPointer);` which will delte the dynlist (and all sublists in case of a nested list).

#### getOffsetUntil ####
To scan through a string of type utf32dynlist and match against the aforementioned lists the following function can be used:
```
uint32_t getOffsetUntil(uint32_t* xmlInUtf32, uint32_t maxScanLength, struct DynamicList* MatchAgainst, uint32_t* optional_matchIndex);
```
As argument it takes maxScanLength, which decides how long
It will return the offset of a match or if there is not the maxScanLength. The optional_matchIndex is useful for MultiWordMatchLists and MultiCharMatchLists to find out which one of the entries caused the match.
