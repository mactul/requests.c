#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "requests_helper/strings/strings.h"
#include "requests_helper/parsing/parser_tree.h"

typedef struct _tree_node {
    char* key;
    char* value;
    struct _tree_node* left_child;
    struct _tree_node* right_child;
} TreeNode;

struct _rh_parser_tree {
    TreeNode* root;
    char* current_key;
    char* current_value;
};


/*
Create a new ParserTree.
If it fails, it returns NULL.
*/
rh_ParserTree* rh_ptree_init()
{
    rh_ParserTree* tree = (rh_ParserTree*) malloc(sizeof(rh_ParserTree));
    if(tree == NULL)
    {
        return NULL;
    }

    tree->root = NULL;
    tree->current_key = NULL;
    tree->current_value = NULL;

    return tree;
}

/*
Allocate the space for the partial_key and concatenate it to the older one.
It can be called multiple time if the key you want to store is separated in multiples chunks.
However, because of the realloc, it's quicker if you work with large chunks.
If it succeeded, it returns true.
If it fails, it's a memory error, the operation is aborted and the previously allocated key is freed.
*/
bool rh_ptree_update_key(rh_ParserTree* tree, const char* partial_key, size_t partial_key_len)
{
    char* writer;
    if(tree->current_key == NULL)
    {
        // New key
        tree->current_key = (char*) malloc((partial_key_len + 1) * sizeof(char));
        writer = tree->current_key;
    }
    else
    {
        size_t len_old_key = strlen(tree->current_key);
        char* temp = (char*) realloc(tree->current_key, (len_old_key + partial_key_len + 1) * sizeof(char));
        if(temp == NULL)
        {
            free(tree->current_key);
            tree->current_key = NULL;
        }
        tree->current_key = temp;
        writer = tree->current_key + len_old_key;
    }
    if(tree->current_key == NULL)
    {
        return false;
    }

    rh_strncpy(writer, partial_key, partial_key_len);

    return true;
}

/*
Allocate the space for the partial_value and concatenate it to the older one.
It can be called multiple time if the value you want to store is separated in multiples chunks.
However, because of the realloc, it's quicker if you work with large chunks.
If it succeeded, it returns true.
If it fails, it's a memory error, the operation is aborted and the previously allocated value is freed.
*/
bool rh_ptree_update_value(rh_ParserTree* tree, const char* partial_value, size_t partial_value_len)
{
    char* writer;
    if(tree->current_value == NULL)
    {
        // New key
        tree->current_value = (char*) malloc((partial_value_len + 1) * sizeof(char));
        writer = tree->current_value;
    }
    else
    {
        size_t len_old_value = strlen(tree->current_value);
        char* temp = (char*) realloc(tree->current_value, (len_old_value + partial_value_len + 1) * sizeof(char));
        if(temp == NULL)
        {
            free(tree->current_value);
            tree->current_value = NULL;
            return false;
        }
        tree->current_value = temp;
        writer = tree->current_value + len_old_value;
    }
    if(tree->current_value == NULL)
    {
        return false;
    }

    rh_strncpy(writer, partial_value, partial_value_len);

    return true;
}

/*
Abort the saving of current key/value.
This doesn't free the tree and you can continue to fill it after that.
You can use this in case you realize while parsing that the current partial key/values you are saving aren't wanted.
*/
void rh_ptree_abort(rh_ParserTree* tree)
{
    free(tree->current_key);
    tree->current_key = NULL;
    free(tree->current_value);
    tree->current_value = NULL;
}

/*
Once you have added your key and your value,
you have to call rh_ptree_push to add this couple to the final tree and start saving something else.

DATA_MODIFICATIONS is a pointer to a function which can operate on the key and the value.
For example, you can decode a base64 or an urlencoded string at this moment.
*/
bool rh_ptree_push(rh_ParserTree* tree, void (*data_modifications)(char*))
{
    TreeNode* node = (TreeNode*) malloc(sizeof(TreeNode));
    if(node == NULL)
        return false;
    node->key = tree->current_key;
    if(data_modifications != NULL && node->key != NULL)
        (*data_modifications)(node->key);
    node->value = tree->current_value;
    if(data_modifications != NULL && node->value != NULL)
        (*data_modifications)(node->value);
    node->left_child = NULL;
    node->right_child = NULL;

    tree->current_key = NULL;
    tree->current_value = NULL;

    if(tree->root == NULL)
    {
        tree->root = node;
        return true;
    }

    TreeNode* root = tree->root;

    while(root != NULL)
    {
        signed char cmp = rh_strcasecmp(node->key, root->key);
        if(cmp == 0)
        {
            // The key already exists, just replace the value
            free(root->value);
            root->value = node->value;
            free(node->key);
            free(node);
            return true;
        }
        if(cmp < 0)
        {
            if(root->left_child == NULL)
            {
                root->left_child = node;
                return true;
            }
            root = root->left_child;
        }
        else
        {
            if(root->right_child == NULL)
            {
                root->right_child = node;
                return true;
            }
            root = root->right_child;
        }
    }
    // we are not supposed to go here
    return false;
}

/*
Once the parsing is done, use this to get the value associated with KEY (KEY is case unsensitive).
If you have to modify this value, copy it before.
*/
const char* rh_ptree_get_value(rh_ParserTree* tree, const char* key)
{
    TreeNode* root = tree->root;
    while(root != NULL)
    {
        signed char cmp = rh_strcasecmp(key, root->key);
        if(cmp == 0)
        {
            return root->value;
        }
        if(cmp < 0)
        {
            root = root->left_child;
        }
        else
        {
            root = root->right_child;
        }
    }
    return NULL;
}

static void _ptree_free(TreeNode* root)
{
    if(root == NULL)
        return;

    if(root->left_child != NULL)
        _ptree_free(root->left_child);
    if(root->right_child != NULL)
        _ptree_free(root->right_child);

    free(root->key);
    free(root->value);

    free(root);
}

/*
Take the address of the tree handler.
Free the tree and set the tree handler to NULL.
*/
void rh_ptree_free(rh_ParserTree** tree)
{
    if(*tree != NULL)
    {
        _ptree_free((*tree)->root);
        free((*tree)->current_key);
        free((*tree)->current_value);
        free(*tree);
        *tree = NULL;
    }
}

static void _ptree_display(TreeNode* root)
{
    if(root->left_child != NULL)
        _ptree_display(root->left_child);
    if(root->right_child != NULL)
        _ptree_display(root->right_child);

    if(root->key != NULL)
        printf("KEY: %s\n", root->key);
    if(root->value != NULL)
        printf("VALUE: %s\n\n", root->value);
}

/*
Display the tree.
Used for debug purpose.
*/
void rh_ptree_display(rh_ParserTree* tree)
{
    if(tree == NULL || tree->root == NULL)
    {
        return;
    }
    _ptree_display(tree->root);
}
