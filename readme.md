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

### xmlTreeElement
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

