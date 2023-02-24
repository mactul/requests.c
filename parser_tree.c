#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "parser_tree.h"

typedef struct _tree_node {
    char* key;
    char* value;
    struct _tree_node* left_child;
    struct _tree_node* right_child;
} TreeNode;

struct _parser_tree {
    TreeNode* root;
    char* current_key;
    char* current_value;
};

ParserTree* ptree_init()
{
    ParserTree* tree = (ParserTree*) malloc(sizeof(ParserTree));
    if(tree == NULL)
        return NULL;
    
    tree->root = NULL;
    tree->current_key = NULL;
    tree->current_value = NULL;

    return tree;
}

char ptree_update_key(ParserTree* tree, const char* partial_key)
{
    char* writter;
    if(tree->current_key == NULL)
    {
        // New key
        tree->current_key = (char*) malloc((strlen(partial_key) + 1) * sizeof(char));
        writter = tree->current_key;
    }
    else
    {
        int len_old_key = strlen(tree->current_key);
        tree->current_key = (char*) realloc(tree->current_key, (len_old_key + strlen(partial_key) + 1) * sizeof(char));
        writter = tree->current_key + len_old_key;
    }
    if(tree->current_key == NULL)
        return 0;
    
    strcpy(writter, partial_key);

    return 1;
}

char ptree_update_value(ParserTree* tree, const char* partial_value)
{
    char* writter;
    if(tree->current_value == NULL)
    {
        // New key
        tree->current_value = (char*) malloc((strlen(partial_value) + 1) * sizeof(char));
        writter = tree->current_value;
    }
    else
    {
        int len_old_value = strlen(tree->current_value);
        tree->current_value = (char*) realloc(tree->current_value, (len_old_value + strlen(partial_value) + 1) * sizeof(char));
        writter = tree->current_value + len_old_value;
    }
    if(tree->current_value == NULL)
        return 0;
    
    strcpy(writter, partial_value);

    return 1;
}

void ptree_abort(ParserTree* tree)
{
    if(tree->current_key != NULL)
        free(tree->current_key);
    if(tree->current_value != NULL)
        free(tree->current_value);
    tree->current_key = NULL;
    tree->current_value = NULL;
}

char ptree_push(ParserTree* tree)
{
    TreeNode* node = (TreeNode*) malloc(sizeof(TreeNode));
    if(node == NULL)
        return 0;
    node->key = tree->current_key;
    node->value = tree->current_value;
    node->left_child = NULL;
    node->right_child = NULL;

    tree->current_key = NULL;
    tree->current_value = NULL;

    if(tree->root == NULL)
    {
        tree->root = node;
        return 1;
    }

    TreeNode* root = tree->root;

    while(root != NULL)
    {
        int cmp = stricmp(node->key, root->key);
        if(cmp == 0)
        {
            // The key already exists, just replace the value
            root->value = node->value;
            free(node);
            return 1;
        }
        if(cmp < 0)
        {
            if(root->left_child == NULL)
            {
                root->left_child = node;
                return 1;
            }
            root = root->left_child;
        }
        else
        {
            if(root->right_child == NULL)
            {
                root->right_child = node;
                return 1;
            }
            root = root->right_child;
        }
    }
    // we are not supposed to go here
    return 0;
}

const char* ptree_get_value(ParserTree* tree, const char* key)
{
    TreeNode* root = tree->root;
    while(root != NULL)
    {
        int cmp = stricmp(key, root->key);
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

void _ptree_free(TreeNode* root)
{
    if(root->left_child != NULL)
        _ptree_free(root->left_child);
    if(root->right_child != NULL)
        _ptree_free(root->right_child);
    
    if(root->key != NULL)
        free(root->key);
    if(root->value != NULL)
        free(root->value);
    
    free(root);
}

void ptree_free(ParserTree** tree)
{
    if(*tree != NULL)
    {
        _ptree_free((*tree)->root);
        if((*tree)->current_key != NULL)
            free((*tree)->current_key);
        if((*tree)->current_value != NULL)
            free((*tree)->current_value);
        free(*tree);
        *tree = NULL;
    }
}

void _ptree_display(TreeNode* root)
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

void ptree_display(ParserTree* tree)
{
    _ptree_display(tree->root);
}

