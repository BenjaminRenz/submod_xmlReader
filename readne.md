Example xmlTree for an average document:
struct xmlTreeElement
|__uint32_t type=xmltype_tag
|__struct DynamicList* name=0;      
|__struct xmlTreeElement* parent=0
|__struct DynamicList* content=contentOfRootDocument;   
|__struct DynamicList* attributes=XmlVersion;

struct DynamicList attributes
|__uint32_t type=dynlisttype_keyValuePairsp;
|__uint32_t itemcnt=2;
|__void* items=(referes to memory address below, which stores first_key_valp)
|__struct key_val_pair* =first_key_valp;
|__struct key_val_pair* =second_key_valp;

first_key_valp==((struct key_val_pair**)attributes->items)[0]

sturct key_val_pair first_key_valp
|__struct DynamicList* key;
|__struct DynamicList* value;

used for key, value, completeXmlDocument while parsing:
struct DynamicList UTF32string
|__uint32_t type=dynlisttype_utf32chars
|__uint32_t itemcnt=8
|__void* items=(refers to memory address below, which stores first UTR32char)
|__uint32_t =first_UTF32_char
|__uint32_t =second_UTF32_char
...
|__uint32_t =last_UTF32_char

xmlTreeElement->content==struct DynamicList* contentOfRootDocument
|__uint32_t type=dynlisttype_xmlELMNTCollectionp;
|__uint32_t itemcnt=5;
|__void* items=(refers to memory address below, which stores first struct xmlTreeElement*)
|__struct xmlTreeElement* =firstElementInXmlFile;
|__struct xmlTreeElement* =secondElementInXmlFile;
...
|__struct xmlTreeElement* =secondElementInXmlFile;

Example use of:
function uses data at *newElementp and attaches it at the end of the List *ListOrNullpp
void append_DynamicList(struct DynamicList** ListOrNullpp,void* newElementp,size_t sizeofListElements,uint32_t typeId)

to append one character to an dynlisttype_utf32chars
struct DynamicList* listToBeAppended=0; //or non empty list
uint32_t characterToBeAppended='a'
append_DynamicList(&listToBeAppended,&characterToBeAppended,sizeof(uint32_t),dynlisttype_utf32chars);

to append one key_val_pairp to an dynlisttype_keyValuePairsp
struct DynamicList* listToBeAppended=0; //or non empty list
struct key_val_pair newkeyvalpair;
newkeyvalpair->key=stringToUTF32Dynlist("keyname");//malloc
newkeyvalpair->value=stringToUTF32Dynlist("true");//malloc
append_DynamicList(&listToBeAppended,&newkeyvalpair,sizeof(struct key_val_pair),dynlisttype_keyValuePairsp);

to append one struct xmlTreeElement* to an dynlisttype_xmlELMNTCollectionp
struct xmlTreeElement* parent=...;
struct DynamicList* listToBeAppended=parent->content;
struct xmlTreeElement* newElement=(struct xmlTreeElement*)malloc(sizeof(struct xmlTreeElement));
newElement->type=xmltype_tag;
newElement->name=stringToUTF32Dynlist("tagname");
newElement->parent=parent;
newElement->content=0;
newElement->attributes=0;
append_DynamicList(&listToBeAppended,&newElement,sizeof(struct xmlTreeElement*),dynlisttype_keyValuePairsp);
//because listToBeAppended is a list of pointers
