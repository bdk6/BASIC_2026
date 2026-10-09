//  BASIC_2026
// Simple BASIC interpreter
// Copyright (c) 2026 William R Cooke

#include <stdio.h>
#include <ctype.h>
#include <string.h>  // do we need this?
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>  // atoi


// Syntax
// PROG := {PLINE} .
// PLINE := [LABEL] ST {: ST} newline.
// LABEL := digit {digit} .
// ST := LET_ST |
//       PRINT_ST |
//       INPUT_ST |
//       IF_ST |
//       GOTO_ST |
//       GOSUB_ST |
//       RETURN_ST |
//       FOR_ST |
//       NEXT_ST |
//       DIM_ST |
//       END_ST .
//
// LET_ST := LET ref = exp .
// ref := varname .
// PRINT_ST := PRINT [exp {,exp}]
// INPUT_ST := INPUT [string] ref{,ref}
// IF_ST := IF exp THEN ST [ ELSE ST]
// GOTO_ST := GOTO label
// GOSUB_ST := GOSUB label
// RETURN_ST := RETURN
// FOR_ST := FOR ref = exp TO exp [STEP exp]
// NEXT_ST := NEXT [ref]
// DIM_ST := DIM varname (exp)
// varname := ident[ % | ! | # | $]
// ref := varname [ (exp) ]
//


// exp := OR_EXP .
// or_exp := and_exp { OR and_exp} .
// and_exp := bor_exp {AND bor_exp}
// bor_exp := bxor_exp { | bxor_exp} .
// bxor_exp := band_exp { ^ band_exp} .
// band_exp := eq_exp { & eq_exp} . 
// eq_exp := comp_exp { (= | <>) comp_exp} .
// comp_exp := sh_exp { comp_op sh_exp} .
//   comp_op := <, >, <=, >=, <>
// sh_exp := add_exp { ( << | >> ) add_exp} .
// add_exp := mul_exp { ( + | -) mul_exp} .
// mul_exp := factor { (* | / | %) factor} .
// factor := number | ref | function | ( exp ) | - factor .
// number := integer | float .
// ref var [ ( dimlist) ] .
// function := 


#define MAX_FILE 1000000

#define SYM_LEN  32
#define MAX_SYMS 1000


// BASIC Data types
typedef int32_t int_t;
#define TRUE      ((int_t)1)
#define FALSE     ((int_t)0)
typedef double  float_t;
typedef enum ERROR
{
	ERR_NONE = 0,
	ERR_NO_FILE,
	ERR_EOF,

	ERR_VALUE_STACK_OVERFLOW,
	ERR_VALUE_STACK_UNDERFLOW,

	ERR_UNDEFINED
} error_t;

typedef enum TOKEN_TYP
{
	TOK_PLUS = '+',
	TOK_MINUS = '-',
	TOK_STAR = '*',
	TOK_SLASH = '/',
	TOK_PERCENT = '%',
	TOK_CARET = '^',
	TOK_AMP = '&',
	TOK_LPAR = '(',
	TOK_RPAR = ')',
	TOK_EQUAL = '=',
	TOK_LESS = '<',
	TOK_GREATER = '>',
	TOK_COMMA = ',',
	TOK_SEMI = ';',
	TOK_COLON = ':',
	TOK_PIPE = '|',
	TOK_QUOTE = '\'',
	TOK_TILDE = '~',

	TOK_UNDEF = 256,
	TOK_NL,
	TOK_EOF,
	TOK_LET,
	TOK_PRINT,
	TOK_INPUT,
	TOK_IF,
	TOK_THEN,
	TOK_ELSE,
	TOK_GOTO,
	TOK_GOSUB,
	TOK_RETURN,
	TOK_FOR,
	TOK_TO,
	TOK_STEP,
	TOK_NEXT,
	TOK_IDENT,
	TOK_INT,
	TOK_FLOAT,
	TOK_STRING,
	TOK_DIM,
	TOK_END,
	TOK_REM,

	TOK_AND,
	TOK_OR,
	TOK_XOR,
	TOK_NOT,

	TOK_NOT_EQUAL,
	TOK_LESS_EQUAL,
	TOK_GREATER_EQUAL,
	TOK_SHL,
	TOK_SHR,

} token_typ_t;

typedef struct KEYWORD
{
	token_typ_t typ;
	char* str;
}keyword_t;

keyword_t keyword_list[] = 
{
	{TOK_IF, "IF"},
	{TOK_THEN, "THEN"},
	{TOK_ELSE, "ELSE"},
	{TOK_LET, "LET"},
	{TOK_GOTO, "GOTO"},
	{TOK_GOSUB, "GOSUB"},
	{TOK_RETURN, "RETURN"},
	{TOK_FOR, "FOR"},
	{TOK_TO, "TO"},
	{TOK_STEP, "STEP"},
	{TOK_NEXT, "NEXT"},
	{TOK_DIM, "DIM"},
	{TOK_PRINT, "PRINT"},
	{TOK_INPUT, "INPUT"},
	{TOK_END, "END"},
	{TOK_REM, "REM"},

	{TOK_AND, "AND"},
	{TOK_OR, "OR"},
	{TOK_NOT, "NOT"}
	


};

#define NUM_KW sizeof(keyword_list) / sizeof(keyword_t)



token_typ_t find_keyword(char* id)
{
	assert(id != NULL);
	token_typ_t rtn = TOK_IDENT;
	for(int i = 0; i < NUM_KW; i++)
	{
    if(strcasecmp(id, keyword_list[i].str) == 0)
		{
			//rtn = i;
			rtn = keyword_list[i].typ;
			break;
		}
	}
	//printf("Keyword at index %d\n", rtn);
	return rtn;
}



typedef struct TOKEN
{
	char str[SYM_LEN + 1];  // TODO fix this: unlimited for tokens / strings / names
	token_typ_t typ;
	uint32_t location;
} token_t;

typedef enum VALUE_TYPE
{
	TYPE_INTEGER = 0,
	TYPE_FLOAT,
	TYPE_STRING,
	TYPE_UNDEFINED
}value_type_t;

typedef struct VALUE
{
	value_type_t tag;
	union 
	{
		int32_t i;
		double f;
		char * s;
	};
	
} value_t;

#define MAX_VALUE_STACK  64
value_t value_stack[MAX_VALUE_STACK];
int val_sp = 0;

char code[MAX_FILE];
uint32_t code_ptr;
uint32_t code_end;
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
// Symbol table
////////////////////////////////////////////////////////
typedef struct SYMBOL
{
	char name[SYM_LEN + 1];
	value_t v;
} symbol_t;

symbol_t symtable[MAX_SYMS];

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

error_t string(token_t* tok);
error_t function(token_t* tok);
error_t ref(token_t* tok);
error_t number(token_t* tok);
error_t factor(token_t* tok);
error_t mul_exp(token_t* tok);
error_t add_exp(token_t* tok);
error_t sh_exp(token_t* tok);
error_t comp_exp(token_t* tok);
error_t eq_exp(token_t* tok);
error_t band_exp(token_t* tok);
error_t bxor_exp(token_t* tok);
error_t bor_exp(token_t* tok);
error_t and_exp(token_t* tok);
error_t or_exp(token_t* tok);
error_t expression(token_t* tok);

error_t scan(token_t* tok);

/////////////////////////////////////////////////////////////////
/// @fn value_push
/// @brief Push a value onto value stack.
/// @param[in] v The value to push
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t value_push(value_t v)
{
	error_t rtn = ERR_NONE;
	if(val_sp >= MAX_VALUE_STACK)
	{
		rtn = ERR_VALUE_STACK_OVERFLOW;
	}
	else
	{
		value_stack[val_sp++] = v;
	}
	return rtn;
}
/////////////////////////////////////////////////////////////////
/// @fn value_push_int
/// @brief Push an integer onto the value stack.
/// @param[in] i The integer to push.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t value_push_int(int_t i)
{ //printf("push int %d\n", i);
	error_t rtn = ERR_NONE;
	if(val_sp >= MAX_VALUE_STACK)
	{
		rtn = ERR_VALUE_STACK_OVERFLOW;
	}
	else
	{
		value_t v;
		v.tag = TYPE_INTEGER;
		v.i = i;
		value_stack[val_sp++] = v;
	}
	return rtn;
}
/////////////////////////////////////////////////////////////////
/// @fn value_push_float
/// @brief Push a float onto the value stack.
/// @para[in] f The float to push.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t value_push_float(float_t f)
{ //printf("push float %f\n", f);
	error_t rtn = ERR_NONE;
	if(val_sp >= MAX_VALUE_STACK)
	{
		rtn = ERR_VALUE_STACK_OVERFLOW;
	}
	else
	{
		value_t v;
		v.tag = TYPE_FLOAT;
		v.f = f;
		value_stack[val_sp++] = v;
	}
	return rtn;
}
/////////////////////////////////////////////////////////////////
/// @fn value_pop
/// @brief Pop a value from the value stack
/// @param[out] v Pointer to the value returned.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t value_pop(value_t* v)
{ //printf("pop\n");
	assert(v != NULL);
	error_t rtn = ERR_NONE;
	val_sp--;
	if(val_sp < 0)
	{
		rtn = ERR_VALUE_STACK_UNDERFLOW;
		v->tag = TYPE_UNDEFINED;
	}
	else
	{
    *v = value_stack[val_sp];
	}
	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn string
/// @brief Parse and eval a function call.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t match_types(value_t* lt, value_t* rt)
{
	error_t rtn = ERR_NONE;


	return rtn;
}
/////////////////////////////////////////////////////////////////
/// @fn string
/// @brief Parse and eval a function call.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t string(token_t* tok)
{
	// string := "{char}".
	error_t rtn = ERR_NONE;

	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn function
/// @brief Parse and eval a function call.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t function(token_t* tok)
{
	// function := 
	error_t rtn = ERR_NONE;

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn ref
/// @brief Parse and eval a reference.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t ref(token_t* tok)
{
	// ref var [ ( dimlist) ] .
	error_t rtn = ERR_NONE;

	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn number
/// @brief Parse and eval a number.
//// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t number(token_t* tok)
{ //printf("number\n");
	// number := integer | float .
	error_t rtn = ERR_NONE;
	if(tok->typ == TOK_INT)
	{
		int_t v = atoi(tok->str);
		value_push_int(v);
		//printf("number pushed %d\n", v);
	}
	else if(tok->typ == TOK_FLOAT)
	{
		float_t v = 0.0;
		value_push_float(v);
		//printf("number pushed float %f\n", v);
	}
	else
	{
		printf("number: invalid token %d\n", tok->typ);
		rtn = ERR_UNDEFINED;
	}
	scan(tok);

	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn factor
/// @brief Parse and eval a factor.
/// @param[in,out] tok The current toke.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t factor(token_t* tok)
{ //printf("factor\n");
	// factor := number | ref | function | ( exp ) | - factor .
	error_t rtn = ERR_NONE;
	switch(tok->typ)
	{
		case TOK_INT:
		case TOK_FLOAT:
		  rtn = number(tok);
			break;
		case TOK_LPAR:
		  scan(tok);
			rtn = expression(tok);
			if(tok->typ != TOK_RPAR)
			{
				//error
				printf("FACTOR: expected ')'\n");
				rtn = ERR_UNDEFINED;
			}
			else
			{
				scan(tok);
			}
			break;
		case TOK_MINUS:
		  scan(tok);
			factor(tok);
			value_t v;
			value_pop(&v);
			switch(v.tag)
			{
				case TYPE_INTEGER:
				  v.i = -v.i;
					break;
				case TYPE_FLOAT:
				  v.f = -v.f;
					break;
				case TYPE_STRING:
				  printf("FACTOR: error, can't take neg of string\n");
					rtn = ERR_UNDEFINED;
					break;
				default:
				  break;

			}
			value_push(v);
			break;
		default:
		  rtn = ERR_UNDEFINED;
	}

	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn mul_exp
/// @brief Parse and eval a multiplicative expression.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t mul_exp(token_t* tok)
{
	// mul_exp := factor { (* | / | %) factor} .
	error_t rtn = ERR_NONE;
	rtn = factor(tok);
	while(tok->typ == TOK_STAR || tok->typ == TOK_SLASH || tok->typ == TOK_PERCENT)
	{
		token_typ_t op = tok->typ;
		scan(tok);
		rtn = factor(tok);
		value_t rt;
		value_t lt;
		value_pop(&rt);
		value_pop(&lt);
		match_types(&lt, &rt);
		switch(lt.tag)
		{
			case TYPE_INTEGER:
			  if(op == TOK_STAR)
				{
					lt.i *= rt.i;
				}
				else if(op == TOK_SLASH)
				{
					lt.i /= rt.i;
				}
				else if(op == TOK_PERCENT)
				{
					lt.i %= rt.i;
				}
				break;
			case TYPE_FLOAT:
			  printf("MUL: float not yet supported\n");
				break;
			case TYPE_STRING:
			  printf("MUL: strings not supported (yet)\n");

			default:
			  printf("MUL, unknown types\n");
				break;
		}
		//lt.i = lt.i * rt.i;
		printf("mul = %d\n", lt.i);
		value_push(lt);

	}

	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn add_exp
/// @brief Parse and eval an additive expression.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t add_exp(token_t* tok)
{
	// add_exp := mul_exp { ( + | -) mul_exp} .
	error_t rtn = ERR_NONE;
	rtn = mul_exp(tok);
	while(tok->typ == TOK_PLUS || tok->typ == TOK_MINUS)
	{
		token_typ_t op = tok->typ;
		scan(tok);
		rtn = mul_exp(tok);
		value_t rt;
		value_t lt;
		value_pop(&rt);
		value_pop(&lt);
		match_types(&lt, &rt);
		switch(lt.tag)
		{
			case TYPE_INTEGER:
			  if(op == TOK_PLUS)
				{
					lt.i += rt.i;
				}
				else if(op == TOK_MINUS)
				{
					lt.i -= rt.i;
				}
				break;
			case TYPE_FLOAT:
			  printf("MUL: float not yet supported\n");
				break;
			case TYPE_STRING:
			  printf("ADD: strings not supported (yet)\n");

			default:
			  printf("ADD, unknown types\n");
				break;
		}
		//lt.i = lt.i * rt.i;
		printf("add = %d\n", lt.i);
		value_push(lt);

	}

	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn sh_exp
/// @brief Parse and eval a shift expression.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t sh_exp(token_t* tok)
{
	// sh_exp := add_exp { ( << | >> ) add_exp} .
	error_t rtn = ERR_NONE;
	rtn = add_exp(tok);
	while(tok->typ == TOK_SHL || tok->typ == TOK_SHR)
	{
		token_typ_t op = tok->typ;
		scan(tok);
		rtn = add_exp(tok);
		value_t rt;
		value_t lt;
		value_pop(&rt);
		value_pop(&lt);
		if(lt.tag != TYPE_INTEGER || rt.tag != TYPE_INTEGER)
		{
			printf("SHIFT: Error.  Both operands must be integer.\n");
			rtn = ERR_UNDEFINED;
		}
		else if(rt.i < 0)
		{
			printf("SHIFT: Error.  Shift amount cannot be negative.\n");
			rtn = ERR_UNDEFINED;
		}
		else if(rt.i >= sizeof(lt.i) * 8)
		{
			printf("SHIFT: Error.  Shift size larger than integer.\n");
			rtn = ERR_UNDEFINED;
		}
		else
		{
			// do shift
			// if rhs is neg, undefined
			// if rhs is >= number of bits, undefined
			// should we do arithmetic or logical shift?
			if(op == TOK_SHL)
			{
				lt.i <<= rt.i;
			}
			else
			{
				lt.i >>= rt.i;
			}

		}

		printf("shift = %d\n", lt.i);
		value_push(lt);
	}
	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn comp_exp
/// @brief Parse and eval a comparison expression.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t comp_exp(token_t* tok)
{
	// comp_exp := sh_exp { comp_op sh_exp} .
  //   comp_op := <, >, <=, >=
	error_t rtn = ERR_NONE;
	rtn = sh_exp(tok);

	while(tok->typ == TOK_LESS || tok->typ == TOK_LESS_EQUAL 
	     || tok->typ == TOK_GREATER || tok->typ == TOK_GREATER_EQUAL)
	{
		token_typ_t op = tok->typ;
		scan(tok);
		rtn = sh_exp(tok);
		value_t rt;
		value_t lt;
		value_pop(&rt);
		value_pop(&lt);
		match_types(&lt, &rt);
		int_t result = FALSE;
		switch(lt.tag)
		{
			case TYPE_INTEGER:
			  printf("compare: left: %d right: %d\n", lt.i, rt.i);
			  switch(op)
				{
					case TOK_LESS: 
					  if(lt.i < rt.i) result = TRUE;
						break;
					case TOK_LESS_EQUAL:
					  if(lt.i <= rt.i) result = TRUE;
						break;
					case TOK_GREATER:
					  if(lt.i > rt.i) result = TRUE;
						break;
					case TOK_GREATER_EQUAL:
					  if(lt.i >= rt.i) result = TRUE;
						break;
				}
				break;
			case TYPE_FLOAT:
			  switch(op)
				{
					case TOK_LESS: 
					  if(lt.f < rt.f) result = TRUE;
						break;
					case TOK_LESS_EQUAL:
					  if(lt.f <= rt.f) result = TRUE;
						break;
					case TOK_GREATER:
					  if(lt.f > rt.f) result = TRUE;
						break;
					case TOK_GREATER_EQUAL:
					  if(lt.f >= rt.f) result = TRUE;
						break;
				}
				break;
			case TYPE_STRING:
			  printf("Compare: strings not supported (yet)\n");
				break;

			default:
			  printf("Compare, unknown types\n");
				break;
		}
		//printf("comp = %d\n", result);
		value_push_int(result);
	}
	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn eq_exp
/// @brief Parse and eval an equality expression.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t eq_exp(token_t* tok)
{
	// eq_exp := comp_exp { (= | <>) comp_exp} .
	error_t rtn = ERR_NONE;
	rtn = comp_exp(tok);

	while(tok->typ == TOK_EQUAL || tok->typ == TOK_NOT_EQUAL)
	{
		token_typ_t op = tok->typ;
		scan(tok);
		rtn = comp_exp(tok);
		value_t rt;
		value_t lt;
		value_pop(&rt);
		value_pop(&lt);
		match_types(&lt, &rt);
		int_t result = FALSE;
		switch(lt.tag)
		{
			case TYPE_INTEGER:
			  printf("equal: left: %d right: %d\n", lt.i, rt.i);
			  switch(op)
				{
					case TOK_EQUAL: 
					  if(lt.i == rt.i) result = TRUE;
						break;
					case TOK_NOT_EQUAL:
					  if(lt.i != rt.i) result = TRUE;
						break;
				}
				break;
			case TYPE_FLOAT:
			// TODO add an epsilon here?
			  switch(op)
				{
					case TOK_EQUAL: 
					  if(lt.f == rt.f) result = TRUE;
						break;
					case TOK_NOT_EQUAL:
					  if(lt.f != rt.f) result = TRUE;
						break;
				}
				break;
			case TYPE_STRING:
			  printf("EQUAL: strings not supported (yet)\n");
				break;

			default:
			  printf("Equal, unknown types\n");
				break;
		}
		//printf("equal = %d\n", result);
		value_push_int(result);
	}
	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn band_exp
/// @brief Parse and eval a bitwise AND expression.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t band_exp(token_t* tok)
{
	// band_exp := eq_exp { & eq_exp} . 
	error_t rtn = ERR_NONE;
	rtn = eq_exp(tok);

	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn bxor_exp
/// @brief Parse and eval a bitwise XOR expression.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t bxor_exp(token_t* tok)
{
	// bxor_exp := band_exp { ^ band_exp} .
	error_t rtn = ERR_NONE;
	rtn = band_exp(tok);

	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn bor_exp
/// @brief Parse and eval a bitwise OR expression.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t bor_exp(token_t* tok)
{
	// bor_exp := bxor_exp { | bxor_exp} .
	error_t rtn = ERR_NONE;
	rtn = bxor_exp(tok);

	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn and_exp
/// @brief Parse and eval an and expression.
/// @param[in,out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t and_exp(token_t* tok)
{
	// and_exp := bor_exp {AND bor_exp}
	error_t rtn = ERR_NONE;
	rtn = bor_exp(tok);

	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn or_expression
/// @brief Parse and eval an or expression.
/// @param[in, out] tok The current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t or_expression(token_t* tok)
{
	// or_exp := and_exp { OR and_exp} 
	error_t rtn = ERR_NONE;
	rtn = and_exp(tok);

	return rtn;
}


/////////////////////////////////////////////////////////////////
/// @fn expression
/// @brief Parse and eval an expression.
/// @param[in, out] tok  Current token.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t expression(token_t* tok)
{ //printf("exp\n");
	// exp := OR_EXP .
	error_t rtn = ERR_NONE;
	rtn = or_expression(tok);

	return rtn;
}



/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
/// @fn get
/// @brief Gets next character of program.
/// @return The next character of -1 at end of program
/////////////////////////////////////////////////////////////////
int get(void)
{
	int rtn = -1;
	if(code_ptr < code_end)
	{
		//printf("getting char %d\n", code_ptr);
		rtn = code[code_ptr++];
	}
	else printf("code ptr: %d\n", code_ptr);
	if(rtn < 0 || rtn > 255)
	  printf("Get is returning %d\n", rtn);
	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn unget
/// @brief Put back the last character read from program.
/// @return None.
/////////////////////////////////////////////////////////////////
void unget(void)
{
	assert(code_ptr > 0);

	// Don't go back if at beginning or end of file
	if(code_ptr >= 0 && code_ptr < code_end)
	{
		code_ptr--;
	}
}
/////////////////////////////////////////////////////////////////
/// @fn scan
/// @brief Get the next code token.
/// @param[out] tok Pointer to the token to create.
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t scan(token_t* tok)
{
	if(tok == NULL) return -1;
	int rtn = 0;
	tok->str[0] = 0;
	tok->typ = TOK_UNDEF;
	tok->location = 0;

	int cnt = 0;    // token character counter

	int c;

	// skip whitespace
	do
	{
		c = get();
	} while(c == ' ' || c == '\t');

	tok->str[cnt++] = c;
	tok->location = code_ptr - 1;

	tok->typ = (token_typ_t) c;

	//printf("scan: c is %d, %02x\n", c, c & 0xff);
	if(c == '\n' || c == '\r')
	{ printf("newline\n");
		tok->typ = TOK_NL;
		do
		{
			c = get();//printf("scan got %d\n", c);
		} while (c == '\n' || c == '\r');
		//printf("ungetting %02x\n", c); unget();
	}
	else if( (c && 0xff) == 0xff) // BDK c == -1)
	{
		tok->typ == TOK_EOF;
	}
	else if(isalpha(c))
	{ //printf("Identifier ");
		// ident
		tok->typ = TOK_IDENT;
		
		while( isalnum(c=get()))
		{
			if(cnt <= SYM_LEN)
			{
				tok->str[cnt++] = c;
			}
		}
		if(c=='$' || c=='%' || c=='!' || c=='#')
		{
			if(cnt <= SYM_LEN)
			{
				tok->str[cnt++] = c;
			}
		}
		else
		{
			unget();
		}
		tok->str[cnt] = 0;
		printf("%s\n", tok->str);
		token_typ_t kw= find_keyword(tok->str);
		//printf("ident looked up as keyword %d\n", kw);
		if(kw > 0) tok->typ = kw;
		//printf("kw is now (712) %d\n", tok->typ);
	}

	// number
	else if(isdigit(c))
	{
		//printf("scanning an integer\n");
		tok->typ = TOK_INT;
		while(isdigit(c=get()))
		{
			if(cnt < SYM_LEN)
			{
				tok->str[cnt++] = c;
			}
			tok->str[cnt] = 0;
		}
		//printf("scanned integer %s\n", tok->str);
		unget();
		
	}
  
	// string
	else if(c == '"')
	{
		tok->typ = TOK_STRING;
		// TODO save the string
		while( (c = get()) != '"');
	}

	// <, <<, <=, <>
	else if(c == '<')
	{
		tok->typ = TOK_LESS;
		int c1 = get();
		if(c1 == '<')
		{
			tok->typ = TOK_SHL;
		}
		else if(c1 == '=')
		{
			tok->typ = TOK_LESS_EQUAL;
		}
		else if(c1=='>')
		{
			tok->typ = TOK_NOT_EQUAL;
		}
		else
		{
			unget();
		}

	}

	// >, >>, >=, ><
	else if(c == '>')
	{
		tok->typ = TOK_GREATER;
		int c1 = get();
		if(c1 == '>')
		{
			tok->typ = TOK_SHR;
		}
		else if(c1 == '=')
		{
			tok->typ = TOK_GREATER_EQUAL;
		}
		else if(c1 == '<')
		{
			tok->typ = TOK_NOT_EQUAL;
		}
		else
		{
			unget();
		}

	}
	// default
	else
	{
		// switch(c)
		// {
			// case ':':
			//   tok->typ = TOK_COLON;
			// 	break;
			// 	case ';':
			// 	tok->typ = TOK_SEMI;
			// 	break;
			// 	case ',':
			// 	tok->typ = TOK_COMMA;
			// 	break;
			// 	case '-':
			// 	tok->typ = TOK_MINUS;
			// 	break;
			// 	case '+':
			// 	tok->typ = TOK_PLUS;
			// 	break;
			// 	case '*':
			// 	tok->typ = TOK_STAR;
			// 	break;
			// 	case '/':
		// 		tok->typ = TOK_SLASH;
		// 		break;
		// 		case '%':
		// 		tok->typ = TOK_PERCENT;
		// 		break;
		// 		case '&':
		// 		tok->typ = TOK_AMP;
		// 		break;
		// 		case '|':
		// 		tok->typ = TOK_PIPE;
		// 		break;
		// 		case '^':
		// 		tok->typ = TOK_CARET;
		// 		break;
		// 		case '(':
		// 	  tok->typ = TOK_LPAR;
		// 		break;
		// 		case ')':
		// 		tok->typ = TOK_RPAR;
		// 		break;
		// 		default:
		// 		tok->typ = TOK_UNDEF;
		// 		printf("Invalid character %d\n", c);
		// 		break;
		// }
	
	}


	tok->str[cnt] = 0;
	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn label
/// @brief Process a scanned label
/// @param[in] tok The token holding the label.
/// @return error code
/////////////////////////////////////////////////////////////////
int label(token_t tok)
{
	int rtn = 0;
	// TODO:  store the label with its position
	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn let_st
/// @brief Parse and execute LET statement
/// @param[in,out] tok  Current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t let_st(token_t* tok)
{
	error_t rtn = ERR_NONE;

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn if_st
/// @brief Parse and execute IF statement
/// @param[in,out] tok  Current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t if_st(token_t* tok)
{
	error_t rtn = ERR_NONE;

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn goto_st
/// @brief Parse and execute GOTO statement
/// @param[in,out] tok  Current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t goto_st(token_t* tok)
{
	error_t rtn = ERR_NONE;

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn gosub_st
/// @brief Parse and execute GOSUB statement
/// @param[in,out] tok  Current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t gosub_st(token_t* tok)
{
	error_t rtn = ERR_NONE;

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn return_st
/// @brief Parse and execute RETURN statement
/// @param[in,out] tok  Current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t return_st(token_t* tok)
{
	error_t rtn = ERR_NONE;

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn for_st
/// @brief Parse and execute FOR statement
/// @param[in,out] tok  Current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t for_st(token_t* tok)
{
	error_t rtn = ERR_NONE;

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn next_st
/// @brief Parse and execute NEXT statement
/// @param[in,out] tok  Current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t next_st(token_t* tok)
{
	error_t rtn = ERR_NONE;

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn input_st
/// @brief Parse and execute INPUT statement
/// @param[in,out] tok  Current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t input_st(token_t* tok)
{
	error_t rtn = ERR_NONE;

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn print_st
/// @brief Parse and execute PRINT statement
/// @param[in,out] tok  Current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t print_st(token_t* tok)
{
	error_t rtn = ERR_NONE;

	//rtn = scan(tok);
	if(tok->typ != TOK_NL)
	{  //printf("print_st calling expression val_sp %d \n", val_sp);
		do
		{
			rtn = expression(tok);
			printf("print_st ready to print: stack %d\n", val_sp);
			if(val_sp != 0)  // Something on the stack to print
			{
				value_t v;
				printf("stack before %d\n", val_sp);
				error_t e = value_pop(&v);
				printf("stack after %d\n", val_sp);
				switch(v.tag)
				{
					case TYPE_INTEGER:
				    printf("%d\n", v.i);
						break;
					case TYPE_FLOAT:
					  printf("%f\n", v.f);
						break;
					case TYPE_STRING:
					  printf("%s\n",v.s);
						break;
					default:
					  // something went wrong
						break;
				}
			}
			if(tok->typ == TOK_COMMA)
			{
				printf("\t");
				scan(tok);

			}
			else if(tok->typ == TOK_SEMI)
			{
				// print nothing
				scan(tok);
			}
			else
			{
				// BDK scan(tok);
				break;   /// exit the loop
			}
		} while (1);
		
	}
	//printf("Exiting print_st\n");

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn dim_st
/// @brief Parse and execute DIM statement
/// @param[in,out] tok  Current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t dim_st(token_t* tok)
{
	error_t rtn = ERR_NONE;
	printf("DIM statement\n");
	rtn = scan(tok);

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn end_st
/// @brief Parse and execute END statement
/// @param[in,out] tok  Current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t end_st(token_t* tok)
{
	error_t rtn = ERR_NONE;
	
	// todo: stop execution

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn statement
/// @brief Parse and execute a statement.
/// @param[in,out] tok  Pointer to current token.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t statement(token_t* tok)
{
	error_t rtn = ERR_NONE;

	switch(tok->typ)
	{
		case TOK_LET:
		  scan(tok);
			rtn = let_st(tok);
		  break;
		case TOK_IF:
		  scan(tok);
			rtn = if_st(tok);
		  break;
		case TOK_GOTO:
		  scan(tok);
			rtn = goto_st(tok);
		  break;
		case TOK_GOSUB:
		  scan(tok);
			rtn = gosub_st(tok);
		  break;
		case TOK_RETURN:
		  scan(tok);
			rtn = return_st(tok);
		  break;
		case TOK_FOR:
		  scan(tok);
			rtn = for_st(tok);
		  break;
		case TOK_NEXT:
		  scan(tok);
			rtn = next_st(tok);
		  break;
		case TOK_INPUT:
		  scan(tok);
			rtn = input_st(tok);
		  break;
		case TOK_PRINT:
		  //printf("prog_line calling print_st\n");
		  scan(tok);
			rtn = print_st(tok);
		  break;
		case TOK_DIM:
		  scan(tok);
			rtn = dim_st(tok);
		  break;
		case TOK_END:
		  scan(tok);
			rtn = end_st(tok);
		  break;
		case TOK_REM:
		  printf("REM st, skipping to EOL\n");
		  do
			{
				scan(tok);
			} while (tok->typ != TOK_NL);
			break;
			
		default:
		  printf("statement: invalid token %d\n", tok->typ);
			rtn = ERR_UNDEFINED;
		  break;
	}

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn prog_line
/// @brief Read, parse, execute a single line
/// @return Error code
/////////////////////////////////////////////////////////////////
error_t prog_line(void)
{ printf("\nprog_line\n");
	error_t rtn = ERR_NONE;
	token_t tok;
	scan(&tok);
	//printf("token type: %d\n", tok.typ);

	if(tok.typ == TOK_INT)
	{
		// rtn = label();
		scan(&tok);
	}

	// TODO loop other statements to end of line here

	if(rtn == ERR_NONE)
	{
		do
		{
			rtn = statement(&tok);
			if(tok.typ == TOK_COLON)
			{
				scan(&tok);
			}
		} while (tok.typ != TOK_NL && rtn == ERR_NONE);
		if(tok.typ == TOK_NL)
		{
			scan(&tok);
		}
		printf("next prog line token %d\n", tok.typ);

		
	}


  printf("prog line rtn = %d\n", rtn);
	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn program
/// @brief Read, parse, execute a program.
/// @return Error code.
/////////////////////////////////////////////////////////////////
error_t program(void)
{ printf("program\n");
	error_t rtn = 0;
	do{
		rtn = prog_line();

	} while(!rtn);

	return rtn;
}

/////////////////////////////////////////////////////////////////
/// @fn load_file
/// @brief load a named file into code
/// @param[in] name Name of file to load
/// @return 0 if ok, error code if not (-1)
/////////////////////////////////////////////////////////////////
int load_file(char* name)
{
	int rtn = 0;
	printf("Loading %s\n", name);
  FILE* infile = fopen(name, "r");
	uint32_t fptr = 0;
  if(infile == NULL)
  {
		printf("%s Not found\n", name);
	  rtn = -1;
  }
	else
	{
		int c;
		while( (c = getc(infile) ) != -1 && fptr < MAX_FILE-1)
		{
			code[fptr] = (char)c;
			fptr++;
		}
		code[fptr] = 0;
	}
	printf("Loaded %d bytes\n", fptr);
	code_end = fptr;
  return rtn;

}

/////////////////////////////////////////////////////////////////
/// @fn init
/// @brief Get everything ready to run
/// @return 0 on success, error code otherwise
/////////////////////////////////////////////////////////////////
error_t init(void)
{
	error_t rtn = ERR_NONE;

	// clear symtable
	symbol_t empty;
	for(int i = 0; i < MAX_SYMS; i++)
	{
		symtable[i] = empty;

	}


	return rtn;
}
/////////////////////////////////////////////////////////////////
/// @fn main
/// @brief entry point of program
/// @param[in] argc Number of arguments passed to program
/// @param[in] argv String table of arguments
/// @return 0 on success, error code otherwise
/////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	printf("\nWelcome to BASIC 2026\n");
	printf("Copyright 2026 William R Cooke\n\n");

	init();
	int load_status = load_file("test.bas");
	if(! load_status)
	{
		program();
	}
	printf("\n all done!\n");
	int x = 5;
	x = x<<3<<2;
	printf("x=%d\n", x);
	return 0;
}