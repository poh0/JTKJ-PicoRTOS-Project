#include <string.h>

typedef struct {
    const char *code;
    char letter;
} MorseMap;

// (AI-generated table, prompt "create a morse table that maps string of morse to letters")
// this goes into progmem
static const MorseMap MORSE_TABLE[] = {
    // Letters (A-Z)
    { ".-",     'A' }, { "-...",   'B' }, { "-.-.",   'C' }, { "-..",    'D' },
    { ".",      'E' }, { "..-.",   'F' }, { "--.",    'G' }, { "....",   'H' },
    { "..",     'I' }, { ".---",   'J' }, { "-.-",    'K' }, { ".-..",   'L' },
    { "--",     'M' }, { "-.",     'N' }, { "---",    'O' }, { ".--.",   'P' },
    { "--.-",   'Q' }, { ".-.",    'R' }, { "...",    'S' }, { "-",      'T' },
    { "..-",    'U' }, { "...-",   'V' }, { ".--",    'W' }, { "-..-",   'X' },
    { "-.--",   'Y' }, { "--..",   'Z' },

    // Digits (0-9)
    { "-----",  '0' }, { ".----",  '1' }, { "..---",  '2' }, { "...--",  '3' },
    { "....-",  '4' }, { ".....",  '5' }, { "-....",  '6' }, { "--...",  '7' },
    { "---..",  '8' }, { "----.",  '9' },

    // Nordic / Extended Letters (Ä, Ö, Å)
    { ".-.-",   '\xC4' }, // 'Ä' (Latin-1 / Windows-1252: 0xC4)
    { "---.",   '\xD6' }, // 'Ö' (Latin-1 / Windows-1252: 0xD6)
    { ".--.-",  '\xC5' }, // 'Å' (Latin-1 / Windows-1252: 0xC5)

    // Punctuation & Special Characters
    { ".-.-.-", '.' }, { "--..--", ',' }, { "..--..", '?' }, { ".----.", '\'' },
    { "-.-.--", '!' }, { "-..-.",  '/' }, { "-.--.",  '(' }, { "-.--.-", ')' },
    { ".-...",  '&' }, { "---...", ':' }, { "-.-.-.", ';' }, { "-...-",  '=' },
    { ".-.-.",  '+' }, { "-....-", '-' }, { "..--.-", '_' }, { ".-..-.", '"' },
    { "...-..-",'$' }, { ".--.-.", '@' }
};

char lookup_morse(const char *code) {

    if (code == NULL || *code == '\0') {
        return '\0';
    }

    // Calculate table size at compile-time/runtime safely
    size_t table_size = sizeof(MORSE_TABLE) / sizeof(MORSE_TABLE[0]);

    for (size_t i = 0; i < table_size; i++) {
        if (strcmp(MORSE_TABLE[i].code, code) == 0) {
            return MORSE_TABLE[i].letter;
        }
    }

    // isn't a valid morse character
    return '?';
}

int morse_to_text(const char *in_buf, char *out_buf, size_t out_size) {
    if (out_size == 0) return -1;
    
    size_t out_idx = 0;
    char token[8];
    size_t token_idx = 0;

    for (size_t i = 0; ; i++) {
        char c = in_buf[i];

        // If we hit a space or the end of the string, we have a complete Morse token
        if (c == ' ' || c == '\0') {
            if (token_idx > 0) {
                token[token_idx] = '\0'; // Null-terminate the token
                
                // Check buffer space for the decoded character
                if (out_idx + 1 >= out_size) {
                    out_buf[0] = '\0';
                    return -1; // Overflow prevented
                }

                out_buf[out_idx++] = lookup_morse(token);
                token_idx = 0; // Reset for the next token
            }
            
            // Handle multiple spaces (e.g., word separators if you want to add ' ')
            if (c == '\0') break;
        } else {
            // Accumulate '.' or '-' into the token
            if (token_idx < sizeof(token) - 1) {
                token[token_idx++] = c;
            }
        }
    }

    out_buf[out_idx] = '\0';
    return 0;
}