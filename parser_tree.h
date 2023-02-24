
typedef struct _parser_tree ParserTree;

ParserTree* ptree_init();
char ptree_update_key(ParserTree* tree, const char* partial_key);
char ptree_update_value(ParserTree* tree, const char* partial_value);
char ptree_push(ParserTree* tree);
const char* ptree_get_value(ParserTree* tree, const char* key);
void ptree_free(ParserTree** tree);
void ptree_display(ParserTree* tree);
void ptree_abort(ParserTree* tree);