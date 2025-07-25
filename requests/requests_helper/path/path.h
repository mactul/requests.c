#ifndef rh_PATH
    #define rh_PATH
    #include <stdbool.h>

    #ifdef __cplusplus
    extern "C"{
    #endif
    
    /**
     * @brief Remove unwanted ../ and ./ in a path.  
     * @brief For example, "abc/fgh/../ijk" => "abc/ijk"
     * 
     * @param dest the final path. should be at least the size of `src`
     * @param src the path to simplify
     */
    void rh_simplify_path(char* dest, const char* src);


    /**
     * @brief Concatenate each variadic element in `path_dest` with '/' between each.
     * 
     * @param path_dest The final path concatenated
     * @param dest_buffer_size the size of the buffer for the final path.
     * @param nb_elements The number of variadic parameters.
     * @param ... strings to concatenate
     */
    void rh_path_join(char* path_dest, int dest_buffer_size, int nb_elements, ...);
    
    #ifdef __cplusplus
    }
    #endif
#endif