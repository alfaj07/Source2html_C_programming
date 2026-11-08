
#include <stdio.h>
#include <string.h>
#include "s2html_event.h"

#define SIZE_OF_SYMBOLS (sizeof(symbols))
#define SIZE_OF_OPERATORS (sizeof(operators))
#define WORD_BUFF_SIZE	100

/********** Internal states and event of parser **********/
typedef enum
{
	PSTATE_IDLE,
	PSTATE_PREPROCESSOR_DIRECTIVE,
	PSTATE_SUB_PREPROCESSOR_MAIN,
	PSTATE_SUB_PREPROCESSOR_RESERVE_KEYWORD,
	PSTATE_SUB_PREPROCESSOR_ASCII_CHAR,
	PSTATE_HEADER_FILE,
	PSTATE_RESERVE_KEYWORD,
	PSTATE_NUMERIC_CONSTANT,
	PSTATE_STRING,
	PSTATE_SINGLE_LINE_COMMENT,
	PSTATE_MULTI_LINE_COMMENT,
	PSTATE_ASCII_CHAR
}pstate_e;

/********** global variables **********/

/* parser state variable */
static pstate_e state = PSTATE_IDLE;

/* sub state is used only in preprocessor state */
static pstate_e state_sub = PSTATE_SUB_PREPROCESSOR_MAIN;

/* event variable to store event and related properties */
static pevent_t pevent_data;
static int event_data_idx=0;

/* word[] and word_idx reserved for future use */
/* static char word[WORD_BUFF_SIZE]; */
/* static int word_idx=0; */


static char* res_kwords_data[] = {"const", "volatile", "extern", "auto", "register",
   						   "static", "signed", "unsigned", "short", "long", 
						   "double", "char", "int", "float", "struct", 
						   "union", "enum", "void", "typedef", ""
						  };

static char* res_kwords_non_data[] = {"goto", "return", "continue", "break", 
							   "if", "else", "for", "while", "do", 
							   "switch", "case", "default","sizeof", ""
							  };

static char operators[] = {'/', '+', '*', '-', '%', '=', '<', '>', '~', '&', ',', '!', '^', '|'};
static char symbols[] = {'(', ')', '{', '[', ':'};

/********** state handlers **********/
pevent_t * pstate_idle_handler(FILE *fd, int ch);
pevent_t * pstate_single_line_comment_handler(FILE *fd, int ch);
pevent_t * pstate_multi_line_comment_handler(FILE *fd, int ch);
pevent_t * pstate_numeric_constant_handler(FILE *fd, int ch);
pevent_t * pstate_string_handler(FILE *fd, int ch);
pevent_t * pstate_header_file_handler(FILE *fd, int ch);
pevent_t * pstate_ascii_char_handler(FILE *fd, int ch);
pevent_t * pstate_reserve_keyword_handler(FILE *fd, int ch);
pevent_t * pstate_preprocessor_directive_handler(FILE *fd, int ch);
pevent_t * pstate_sub_preprocessor_main_handler(FILE *fd, int ch);

/********** Utility functions **********/

/* function to check if given word is reserved key word */
static int is_reserved_keyword(char *word)
{
	int idx = 0;

	/* search for data type reserved keyword */
	while(*res_kwords_data[idx])
	{
		if(strcmp(res_kwords_data[idx++], word) == 0)
			return RES_KEYWORD_DATA;
	}

	idx = 0; // reset index
	/* search for non data type reserved key word */
	while(*res_kwords_non_data[idx])
	{
		if(strcmp(res_kwords_non_data[idx++], word) == 0)
			return RES_KEYWORD_NON_DATA;
	}

	return 0; // word did not match, return false
}

/* function to check symbols */
static int is_symbol(char c)
{
	size_t idx;
	for(idx = 0; idx < SIZE_OF_SYMBOLS; idx++)
	{
		if(symbols[idx] == c)
			return 1;
	}

	return 0;
}

/* function to check operator */
static int is_operator(char c)
{
	size_t idx;
	for(idx = 0; idx < SIZE_OF_OPERATORS; idx++)
	{
		if(operators[idx] == c)
			return 1;
	}

	return 0;
}

/* suppress unused-function warnings – these will be used in future */
static void _unused_refs(void) __attribute__((unused));
static void _unused_refs(void) { (void)is_symbol(0); (void)is_operator(0); }

/* to set parser event */
static void set_parser_event(pstate_e s, pevent_e e)
{
	pevent_data.data[event_data_idx] = '\0';
	pevent_data.length = event_data_idx;
	event_data_idx = 0;
	state = s;
	pevent_data.type = e;
}


/************ Event functions **********/

/* This function parses the source file and generate 
 * event based on parsed characters and string
 */
pevent_t *get_parser_event(FILE *fd)
{
	int ch;
	pevent_t *evptr = NULL;
	/* Read char by char */
	while((ch = fgetc(fd)) != EOF)
	{
#ifdef DEBUG
	//	putchar(ch);
#endif
		switch(state)
		{
			case PSTATE_IDLE :
				if((evptr = pstate_idle_handler(fd, ch)) != NULL)
					return evptr;
				break;
			case PSTATE_SINGLE_LINE_COMMENT :
				if((evptr = pstate_single_line_comment_handler(fd, ch)) != NULL)
					return evptr;
				break;
			case PSTATE_MULTI_LINE_COMMENT :
				if((evptr = pstate_multi_line_comment_handler(fd, ch)) != NULL)
					return evptr;
				break;
			case PSTATE_PREPROCESSOR_DIRECTIVE :
				if((evptr = pstate_preprocessor_directive_handler(fd, ch)) != NULL)
					return evptr;
				break;
			case PSTATE_RESERVE_KEYWORD :
				if((evptr = pstate_reserve_keyword_handler(fd, ch)) != NULL)
					return evptr;
				break;
			case PSTATE_NUMERIC_CONSTANT :
				if((evptr = pstate_numeric_constant_handler(fd, ch)) != NULL)
					return evptr;
				break;
			case PSTATE_STRING :
				if((evptr = pstate_string_handler(fd, ch)) != NULL)
					return evptr;
				break;
			case PSTATE_HEADER_FILE :
				if((evptr = pstate_header_file_handler(fd, ch)) != NULL)
					return evptr;
				break;
			case PSTATE_ASCII_CHAR :
				if((evptr = pstate_ascii_char_handler(fd, ch)) != NULL)
					return evptr;
				break;
			default : 
				printf("unknown state\n");
				state = PSTATE_IDLE;
				break;
		}
	}

	/* end of file is reached, move back to idle state and set EOF event */
	set_parser_event(PSTATE_IDLE, PEVENT_EOF);

	return &pevent_data; // return final event
}


/********** IDLE state Handler **********
 * Idle state handler identifies
 ****************************************/

pevent_t * pstate_idle_handler(FILE *fd, int ch)
{
	int pre_ch;
	switch(ch)
	{
		case '\'' : // begining of ASCII char 
			break;

		case '/' :
			pre_ch = ch;
			if((ch = fgetc(fd)) == '*') // multi line comment
			{
				if(event_data_idx) // we have regular exp in buffer first process that
				{
					fseek(fd, -2L, SEEK_CUR); // unget chars
					set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
					return &pevent_data;
				}
				else //	multi line comment begin 
				{
#ifdef DEBUG	
					printf("Multi line comment Begin : /*\n");
#endif
					state = PSTATE_MULTI_LINE_COMMENT;
					pevent_data.data[event_data_idx++] = pre_ch;
					pevent_data.data[event_data_idx++] = ch;
				}
			}
			else if(ch == '/') // single line comment
			{
				if(event_data_idx) // we have regular exp in buffer first process that
				{
					fseek(fd, -2L, SEEK_CUR); // unget chars
					set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
					return &pevent_data;
				}
				else //	single line comment begin
				{
#ifdef DEBUG	
					printf("Single line comment Begin : //\n");
#endif
					state = PSTATE_SINGLE_LINE_COMMENT;
					pevent_data.data[event_data_idx++] = pre_ch;
					pevent_data.data[event_data_idx++] = ch;
				}
			}
			else // it is regular exp
			{
				pevent_data.data[event_data_idx++] = pre_ch;
				pevent_data.data[event_data_idx++] = ch;
			}
			break;
		case '#' :
			if (event_data_idx)
			{
				fseek(fd, -1L, SEEK_CUR); // unget chars
				set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
				return &pevent_data;
			}
			else
			{
				state = PSTATE_PREPROCESSOR_DIRECTIVE;
				pevent_data.data[event_data_idx++] = ch;
			}
			break;
		case '\"' :
			if (event_data_idx) // we have regular exp in buffer first process that
			{
				fseek(fd, -1L, SEEK_CUR); // unget chars
				set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
				return &pevent_data;
			}
			else //		LINE LITERAL IS BEGGINING
			{
	#ifdef DEBUG
				printf("string literal Begin : /*\n");
	#endif
				state = PSTATE_STRING;
				pevent_data.data[event_data_idx++] = ch;
			}
			break;
		case '0' ... '9' : // detect numeric constant
			if (event_data_idx) // we have regular exp in buffer first process that
			{
				fseek(fd, -1L, SEEK_CUR); // unget chars
				set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
				return &pevent_data;
			}
			else //		LINE LITERAL IS BEGGINING
			{
	#ifdef DEBUG
				printf("numeric scanning Begin : /*\n");
	#endif
				state = PSTATE_NUMERIC_CONSTANT;
				pevent_data.data[event_data_idx++] = ch;
			}
			break;

		case 'a' ... 'z' : // could be reserved key word
			if (event_data_idx) // we have regular exp in buffer first process that
			{
				fseek(fd, -1L, SEEK_CUR); // unget chars
				set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
				return &pevent_data;
			}
			else //		LINE LITERAL IS BEGGINING
			{
	#ifdef DEBUG
				printf("alpahbits scanning Begin : /*\n");
	#endif
				state = PSTATE_RESERVE_KEYWORD;
				pevent_data.data[event_data_idx++] = ch;
			}
			break;
		default : // Assuming common text starts by default.
			pevent_data.data[event_data_idx++] = ch;
			break;
	}

	return NULL;
}
pevent_t * pstate_preprocessor_directive_handler(FILE *fd, int ch)
{
	switch(state_sub)
	{
		case PSTATE_SUB_PREPROCESSOR_MAIN :
			return pstate_sub_preprocessor_main_handler(fd, ch);
		case PSTATE_SUB_PREPROCESSOR_RESERVE_KEYWORD :
			return pstate_reserve_keyword_handler(fd, ch);
		case PSTATE_SUB_PREPROCESSOR_ASCII_CHAR :
			return pstate_ascii_char_handler(fd, ch);
		default :
				printf("unknown state\n");
				state = PSTATE_IDLE;
	}

	return NULL;
}
pevent_t * pstate_sub_preprocessor_main_handler(FILE *fd, int ch)
{
	/* write a switch case here to detect several events here
	 * This state is similar to Idle state with slight difference
	 * in state transition.
	 * return event data at the end of event
	 * else return NULL
	 */
	static int space = 1;
	switch (ch)
	{
	case ' ': // checks for macro
		space = 0;
		pevent_data.data[event_data_idx++] = ch;
		break;
	case '<': // checks for header file
		space = 1;
		set_parser_event(PSTATE_HEADER_FILE, PEVENT_PREPROCESSOR_DIRECTIVE);
		return &pevent_data;
		break;
	case '"': // checks for user header
		space = 1;
		fseek(fd, -1L, SEEK_CUR);
		set_parser_event(PSTATE_HEADER_FILE, PEVENT_PREPROCESSOR_DIRECTIVE);
		pevent_data.property = USER_HEADER_FILE;
		return &pevent_data;
		break;
	case '\n': // if the is only preprocessor ex.#endif
		set_parser_event(PSTATE_IDLE, PEVENT_PREPROCESSOR_DIRECTIVE);
		pevent_data.property = USER_HEADER_FILE;
		return &pevent_data;
		break;
	case 'a' ... 'z': // if is is a macro
	case 'A' ... 'Z':
		if (space)
		{
			pevent_data.data[event_data_idx++] = ch;
			return NULL;
		}
		space = 1;
		fseek(fd, -1L, SEEK_CUR);
		set_parser_event(PSTATE_IDLE, PEVENT_PREPROCESSOR_DIRECTIVE);
		return &pevent_data;
		break;
	default:
		pevent_data.data[event_data_idx++] = ch;
		break;
	}
	return NULL;
}

pevent_t * pstate_header_file_handler(FILE *fd, int ch)
{
    /* write a switch case here to store header file name
	 * return event data at the end of event
	 * else return NULL
	 */
	switch (ch)
	{
	case '>':
		set_parser_event(PSTATE_IDLE, PEVENT_HEADER_FILE);
		return &pevent_data;
		break;
	case '"': /* single line comment ends here */
		fseek(fd, -1, SEEK_CUR);
		set_parser_event(PSTATE_IDLE, PEVENT_HEADER_FILE);
		return &pevent_data;
		break;
	default: // collect single line comment chars
#ifdef DEBUG
		printf("\nheader file end\n");
#endif

		pevent_data.data[event_data_idx++] = ch;
	}
	return NULL;
}
pevent_t * pstate_reserve_keyword_handler(FILE *fd, int ch)
{
    /*
     * Switch‑case based parser for a reserved keyword.
     * It collects characters that can be part of an identifier and stops
     * when a character that cannot belong to an identifier is seen.
     */
    while (1) {
        switch (ch) {
            /* alphanumeric and underscore are allowed in a word */
            case 'a' ... 'z':
    		case 'A' ... 'Z':
    		case '0' ... '9':
    		case '_':
                pevent_data.data[event_data_idx++] = (char)ch;
                if (event_data_idx >= PEVENT_DATA_SIZE - 1) {
                    /* Buffer full – treat what we have as regular text */
                    set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
                    return &pevent_data;
                }
                ch = fgetc(fd);
                continue;   /* stay in the loop */
            default:
                /* End of the identifier – null-terminate before lookup */
                pevent_data.data[event_data_idx] = '\0';
                {
                    int kw_type = is_reserved_keyword(pevent_data.data);
                    if (kw_type) {
                        set_parser_event(PSTATE_IDLE, PEVENT_RESERVE_KEYWORD);
                        pevent_data.property = kw_type;   /* DATA or NON_DATA */
                        if (ch != EOF) {
                            ungetc(ch, fd);
                        }
                        return &pevent_data;
                    }
                    /* Not a keyword – emit regular expression event */
                    set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
                    if (ch != EOF) {
                        ungetc(ch, fd);
                    }
                    return &pevent_data;
                }
        }
    }
}
pevent_t * pstate_numeric_constant_handler(FILE *fd, int ch)
{
    /*
     * Switch‑case based parser for a numeric constant.
     * It collects characters that can be part of a numeric literal
     * (digits, hex prefix, decimal point, underscores, and hex letters).
     */
    while (1) {
        switch (ch) {
            case '0' ... '9':
    		case 'a' ... 'f':
    		case 'A' ... 'F':
    		case 'x':
    		case 'X':
    		case '.':
    		case '_':
                pevent_data.data[event_data_idx++] = (char)ch;
                if (event_data_idx >= PEVENT_DATA_SIZE - 1) {
                    /* Buffer full – emit what we have as a numeric constant */
                    set_parser_event(PSTATE_IDLE, PEVENT_NUMERIC_CONSTANT);
                    return &pevent_data;
                }
                ch = fgetc(fd);
                continue;   /* stay in this loop */
            default:
                /* End of numeric literal */
                set_parser_event(PSTATE_IDLE, PEVENT_NUMERIC_CONSTANT);
                if (ch != EOF) {
                    ungetc(ch, fd);   /* push back the terminator for the next state */
                }
                return &pevent_data;
        }
    }
}
pevent_t * pstate_string_handler(FILE *fd, int ch)
{
	/*
	 * Switch-case based parser for a string literal.
	 * Collects all characters between opening and closing double-quotes.
	 * Handles escaped quotes (\") so they do not end the string.
	 */
	int prev_ch = 0;

	/* Store the opening double-quote */
	pevent_data.data[event_data_idx++] = (char)ch;

	while ((ch = fgetc(fd)) != EOF) {
		switch (ch) {
			case '\"' :
				/* Closing double-quote found */
				pevent_data.data[event_data_idx++] = (char)ch;
				if (prev_ch == '\\') {
					/* It was escaped – continue collecting */
					prev_ch = ch;
					continue;
				}
				/* Unescaped closing quote – string ends here */
				set_parser_event(PSTATE_IDLE, PEVENT_STRING);
				return &pevent_data;
			default :
				/* Any other character – collect it */
				pevent_data.data[event_data_idx++] = (char)ch;
				if (event_data_idx >= PEVENT_DATA_SIZE - 1) {
					/* Buffer full – emit what we have */
					set_parser_event(PSTATE_IDLE, PEVENT_STRING);
					return &pevent_data;
				}
				break;
		}
		prev_ch = ch;
	}

	/* EOF reached before closing quote – emit what we have */
	set_parser_event(PSTATE_IDLE, PEVENT_STRING);
	return &pevent_data;
}

pevent_t * pstate_single_line_comment_handler(FILE *fd, int ch)
{
	(void)fd;   /* fd not used in this handler – suppress warning */
	switch(ch)
	{
		case '\n' : /* single line comment ends here */
#ifdef DEBUG	
			printf("\nSingle line comment end\n");
#endif
			pevent_data.data[event_data_idx++] = ch;
			set_parser_event(PSTATE_IDLE, PEVENT_SINGLE_LINE_COMMENT);
			return &pevent_data;
		default :  // collect single line comment chars
			pevent_data.data[event_data_idx++] = ch;
			break;
	}

	return NULL;
}
pevent_t * pstate_multi_line_comment_handler(FILE *fd, int ch)
{
	int pre_ch;
	switch(ch)
	{
		case '*' : /* comment might end here */
			pre_ch = ch;
			pevent_data.data[event_data_idx++] = ch;
			if((ch = fgetc(fd)) == '/')
			{
#ifdef DEBUG	
				printf("\nMulti line comment End : */\n");
#endif
				pre_ch = ch;
				pevent_data.data[event_data_idx++] = ch;
				set_parser_event(PSTATE_IDLE, PEVENT_MULTI_LINE_COMMENT);
				return &pevent_data;
			}
			else // multi line comment string still continued
			{
				pevent_data.data[event_data_idx++] = ch;
			}
			break;
		case '/' :
			/* go back by two steps and read previous char */
			fseek(fd, -2L, SEEK_CUR); // move two steps back
			pre_ch = fgetc(fd); // read a char
			fgetc(fd); // to come back to current offset

			pevent_data.data[event_data_idx++] = ch;
			if(pre_ch == '*')
			{
				set_parser_event(PSTATE_IDLE, PEVENT_MULTI_LINE_COMMENT);
				return &pevent_data;
			}
			break;
		default :  // collect multi-line comment chars
			pevent_data.data[event_data_idx++] = ch;
			break;
	}

	return NULL;
}
pevent_t * pstate_ascii_char_handler(FILE *fd, int ch)
{
	/*
	 * Switch-case based parser for ASCII character literals  e.g. 'a', '\n'.
	 * Structure of a char literal:  ' <char> '
	 *   – opening single-quote  (already in ch when we enter)
	 *   – the character itself  (may be '\' followed by an escape code)
	 *   – closing single-quote
	 */
	int escape = 0;

	/* Store the opening single-quote */
	pevent_data.data[event_data_idx++] = (char)ch;

	while ((ch = fgetc(fd)) != EOF) {
		switch (ch) {
			case '\\' :
				/* Backslash – next character is an escape code */
				pevent_data.data[event_data_idx++] = (char)ch;
				escape = 1;
				break;
			case '\'' :
				/* Single-quote found */
				pevent_data.data[event_data_idx++] = (char)ch;
				if (escape) {
					/* It was part of an escape sequence – keep going */
					escape = 0;
					break;
				}
				/* Closing single-quote – character literal ends here */
				set_parser_event(PSTATE_IDLE, PEVENT_ASCII_CHAR);
				return &pevent_data;
			default :
				/* Any other character (the char itself or escape code) */
				pevent_data.data[event_data_idx++] = (char)ch;
				escape = 0;
				if (event_data_idx >= PEVENT_DATA_SIZE - 1) {
					/* Buffer full – emit what we have */
					set_parser_event(PSTATE_IDLE, PEVENT_ASCII_CHAR);
					return &pevent_data;
				}
				break;
		}
	}

	/* EOF reached before closing quote – emit what we have */
	set_parser_event(PSTATE_IDLE, PEVENT_ASCII_CHAR);
	return &pevent_data;
}
/**** End of file ****/
