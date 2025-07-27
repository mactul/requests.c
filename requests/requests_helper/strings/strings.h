#ifndef RH_STRINGS_H
    #define RH_STRINGS_H
    #include <stdbool.h>
    #include <stdint.h>
    #include <stddef.h>

    /**
     * @brief returns whether or not a character is between 'a' and 'z'
     * 
     * @param c the character to test
     * 
     * @note Warning, this macro has a side effect, c should not be an expression that modify the state of the program.
     */
    #define RH_CHAR_IS_LOWERCASE(c) ((c) >= 'a' && (c) <= 'z')

    /**
     * @brief returns whether or not a character is between 'A' and 'Z'
     * 
     * @param c the character to test
     * 
     * @note Warning, this macro has a side effect, c should not be an expression that modify the state of the program.
     */
    #define RH_CHAR_IS_UPPERCASE(c) ((c) >= 'A' && (c) <= 'Z')

    /**
     * @brief returns whether or not a character is between '0' and '9'
     * 
     * @param c the character to test
     * 
     * @note Warning, this macro has a side effect, c should not be an expression that modify the state of the program.
     */
    #define RH_CHAR_IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

    /**
     * @brief returns whether or not a character is an hexadecimal character
     * 
     * @param c the character to test
     * 
     * @note Warning, this macro has a side effect, c should not be an expression that modify the state of the program.
     */
    #define RH_CHAR_IS_HEXDIGIT(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'F') || ((c) >= 'a' && (c) <= 'f'))

    #ifdef __cplusplus
    extern "C"{
    #endif

    /**
     * @brief Compare `str1` and `str2` with alphanumeric order.  
     * @brief This function is case independent, use rh_strcmp if you want to be case dependent  
     * 
     * @param str1
     * @param str2
     * 
     * @return - If str1 < str2, it will returns -1  
     * @return - If str1 > str2 it will returns 1  
     * @return - If str1 == str2, it will returns 0  
     */
    signed char rh_strcasecmp(const char* str1, const char* str2);

    /**
     * @brief Returns true if the first characters of `str` matches with `ref`.
     * 
     * @param str the string we want to check
     * @param ref the starting part we want to see in `str`
     * @return bool
     * 
     * @note This function is case sensitive.
     */
    bool rh_startswith(const char* str, const char* ref);


    /**
     * @brief Returns the position of the string `expr` in `str`  
     * @brief If `expr` is not in `str`, it returns -1
     * 
     * @param str 
     * @param expr 
     * @return int
     * 
     * @note This function is case unsensitive
     */
    int rh_str_search_case_unsensitive(const char* str, const char* expr);


    /**
     * @brief Reverse the string, for example "hello" will be transformed to "olleh".  
     * @brief If you have no idea about the len of the string, you can use strlen, like this:
     * @brief ```c
     * @brief reverse_string(str, strlen(str));
     * @brief ```
     * 
     * @param str the string to reverse in place.
     * @param len_str the length of the string like provided by strlen (often known when we want to reverse a string).
     */
    void rh_reverse_string(char* str, size_t len_str);

    /**
     * @brief Copy the content of `src` into `dest`  
     * @brief You can use it in cascade, like this:  
     * @brief ```c
     * @brief rh_strcpy(rh_strcpy(str, "hello"), " world");  // generate "hello world" in str
     * @brief ```
     * 
     * @param dest The destination buffer, should be at least the size of `src`
     * @param src The string we want to copy
     * @return A pointer to the end of the string, the `'\0'`
     */
    char* rh_strcpy(char* dest, const char* src);

    /**
     * @brief Copy the string `src` in `dest`.  
     * @brief If the size of `src` is bigger than `dest_buffer_size`, only the first bytes are copied.  
     * @brief In any case, `dest` is always terminated by '\0'.
     * 
     * @param dest The destination buffer
     * @param src The string to copy
     * @param dest_buffer_length The size of the buffer, should be `sizeof(dest)`
     * @return the number of characters copied, '\0' included
     */
    size_t rh_strncpy(char* dest, const char* src, size_t dest_buffer_size);

    /**
     * @brief Concatenate the string `add` to the end of `dest`.
     * @brief `dest` should be big enough to contains the two strings concatenated
     * 
     * @param dest the destination buffer containing de starting string
     * @param add the string to add at the end of dest
     */
    void rh_strcat(char* dest, const char* add);


    /**
     * @brief Modify the string `str` with whitespace stripped from the beginning and end of `str`  
     * @brief The characters removed are `' '`, `'\\t'`, `'\\b'`, `'\\n'`, `'\\r'` and `'\\v'`.
     * 
     * @param str the string to modify inplace.
     * @return the new beginning of the trimmed string.
     */
    char* rh_strtrim_inplace(char* str);


    /**
     * @brief Transform the unsigned integer n to a string and put it in result.  
     * @brief It also returns result this allows the function to be packed in another, like this:  
     * @brief ```c
     * @brief puts(rh_uint64_to_str(result, 57));
     * @brief ```
     * 
     * @param result the buffer to contain the number
     * @param n the number to convert
     * @return the pointer `result`
     */
    char* rh_uint64_to_str(char* result, uint64_t n);

    /**
     * @brief This transform a string to an uint64_t.  
     * @brief If the string is not the representation of an uint64_t, for example if it contains symbols like '-',
     * @brief the function will returns 0 and rh_get_last_error() will returns rh_ERROR_NAN.  
     * @brief Please make sure to check this if the function returns 0, unless you are sure that the string is well formatted.
     * 
     * @param str the string to convert
     * @return uint64_t 
     */
    uint64_t rh_str_to_uint64(const char* str);

    /**
     * @brief This transform a hexadecimal string to an uint64_t.
     * @brief If the string is not the hexadecimal representation of an uint64_t, for example if it contains symbols like '-',
     * @brief the function will returns 0 and rh_get_last_error() will returns rh_ERROR_NAN
     * @brief Please make sure to check this if the function returns 0, unless you are sure that the string is well formatted.
     * 
     * @param str the string to convert
     * @return uint64_t
     */
    uint64_t rh_hex_to_uint64(const char* str);

    #ifdef __cplusplus
    }
    #endif
#endif