#define relu(x) x < 0 ? 0 : x

void int_to_string(int n, char s[]);
void bytescpy(char* dest, const char* src, int n);
int stristr(const char* string, const char* exp);
char starts_with(char* str, const char* ref);
char* strtrim(char* str);
void retrieve_absolute_url(char* url, const char* reference_url);