#ifndef rh_PARSER_TREE
    #define rh_PARSER_TREE
    #include <stdbool.h>
    #include <stddef.h>

    typedef struct _rh_parser_tree rh_ParserTree;

    #ifdef __cplusplus
    extern "C"{
    #endif

    /**
     * @brief Create a new ParserTree
     * 
     * @return - When it succeeds, it returns a pointer to a ParserTree handler.
     * @return - When it fails, it returns NULL and `rh_print_last_error` can tell what happened
     */
    rh_ParserTree* rh_ptree_init();


    /**
     * @brief Allocate the space for the partial_key and concatenate it to the older one.  
     * @brief It can be called multiple time if the key you want to store is separated in multiples chunks.  
     * @brief However, because of the realloc, it's quicker if you work with large chunks.
     * 
     * @param tree The handler returned by `rh_ptree_init`
     * @param partial_key a string that is the part of the key you are actually parsing
     * @param partial_key_len the strlen of the key (can be used to limit the size of the buffer)
     * @return - When it succeeds, it returns true.
     * @return - When it fails, it's a memory error, the operation is aborted and the previously allocated key is freed.
     */
    bool rh_ptree_update_key(rh_ParserTree* tree, const char* partial_key, size_t partial_key_len);


    /**
     * @brief Allocate the space for the partial_value and concatenate it to the older one.  
     * @brief It can be called multiple time if the value you want to store is separated in multiples chunks.  
     * @brief However, because of the realloc, it's quicker if you work with large chunks.
     * 
     * @param tree The handler returned by `rh_ptree_init`
     * @param partial_value a string that is the part of the value you are actually parsing
     * @param partial_value_len the strlen of the value (can be used to limit the size of the buffer)
     * @return - When it succeeds, it returns true.
     * @return - When it fails, it's a memory error, the operation is aborted and the previously allocated value is freed.
     */
    bool rh_ptree_update_value(rh_ParserTree* tree, const char* partial_value, size_t partial_value_len);


    /**
     * @brief Once you have added your key and your value,  
     * @brief you have to call rh_ptree_push to add this couple to the final tree and start saving something else.
     * 
     * @param tree The handler returned by `rh_ptree_init`
     * @param data_modifications a pointer to a function which can operate on the key and the value. For example, you can decode a base64 or an urlencoded string at this moment.
     * @return true when it succeed, otherwise, false.
     */
    bool rh_ptree_push(rh_ParserTree* tree, void (*data_modifications)(char*));


    /**
     * @brief Once the parsing is done, use this to get the value associated with KEY (KEY is case unsensitive).  
     * @brief If you have to modify this value, copy it before.
     * 
     * @param tree The handler returned by `rh_ptree_init`
     * @param key the key corresponding to the value registered
     * @return - when it succeeds, it returns the value registered
     * @return - when it fails, it returns NULL
     */
    const char* rh_ptree_get_value(rh_ParserTree* tree, const char* key);


    /**
     * @brief Free the tree and set the tree handler to NULL.
     * 
     * @param tree the address of the tree handler.
     */
    void rh_ptree_free(rh_ParserTree** tree);


    /**
     * @brief Abort the saving of current key/value.  
     * @brief This doesn't free the tree and you can continue to fill it after that.  
     * @brief You can use this in case you realize while parsing that the current partial key/values you are saving aren't wanted.
     * 
     * @param tree The handler returned by `rh_ptree_init`
     */
    void rh_ptree_abort(rh_ParserTree* tree);


    /**
     * @brief Display the tree.  
     * Used for debug purpose.
     * 
     * @param tree The handler returned by `rh_ptree_init`
     */
    void rh_ptree_display(rh_ParserTree* tree);

    #ifdef __cplusplus
    }
    #endif
#endif