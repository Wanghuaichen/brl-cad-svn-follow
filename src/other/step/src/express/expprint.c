/** @file expprint.c
 *
 * Routines for examining express parser state.
 *
 */

#include "express/expbasic.h"
#include "express/dict.h"
#include "express/symbol.h"

#include "express/expprint.h"
#include "expparse.h"

static void
printStart(const char *structName)
{
    printf("%s {\n", structName);
}

static void
printEnd()
{
    printf("}\n");
}

static void
printSymbol(Symbol *structp)
{
    printStart("Symbol");

    printf("name = %s\n", structp->name);
    printf("filename = %s\n", structp->name);
    printf("line = %hd\n", structp->line);
    printf("resolved = %c\n", structp->resolved);

    printEnd();
}

static void
printElement(Element structp)
{
    printStart("Element");

    printf("key = %s\n", structp->key);;
    printf("data = %s\n", structp->data);;

    if (structp->next != NULL) {
	printElement(structp->next);
    }

    printSymbol(structp->symbol);
    printf("type = %c\n", structp->type);

    printEnd();
}

static void
printDictionary(Dictionary structp)
{
    int i, j, numSegs, numKeys;
    Segment seg;

    printStart("Dictionary");

    printf("p = %hd\n", structp->p);
    printf("maxp = %hd\n", structp->maxp);
    printf("KeyCount = %ld\n", structp->KeyCount);
    printf("SegmentCount = %hd\n", structp->SegmentCount);
    printf("MinLoadFactor = %hd\n", structp->MinLoadFactor);
    printf("MaxLoadFactor = %hd\n", structp->MaxLoadFactor);

    numSegs = structp->SegmentCount;
    numKeys = structp->KeyCount;

    for (i = 0; i < numSegs; i++) {
	seg = structp->Directory[i];

	for (j = 0; j < SEGMENT_SIZE; j++) {
	    if (seg[j] != NULL) {
		printElement(seg[j]);
	    }
	}
    }

    printEnd();
}

void
expprintExpress(Express structp)
{
    printStart("Express");

    printSymbol(&structp->symbol);
    printf("type = %c\n", structp->type);
    printf("search_id = %d\n", structp->search_id);
    printDictionary(structp->symbol_table);

    if (structp->superscope != NULL) {
	expprintExpress(structp->superscope);
    }

#if 0
    switch(structp->type) {
	case OBJ_PROCEDURE:
	    printProcedure();
	case OBJ_FUNCTION:
	case OBJ_RULE:
	case OBJ_ENTITY:
	case OBJ_SCHEMA:
	case OBJ_EXPRESS:
	case OBJ_INCREMENT:
	case OBJ_TYPE:
    }
#endif

    printEnd();
}

#if 0
/* struct definitions temporarily included for reference purposes */

struct Scope_ {
    Symbol symbol;
    char type;		   /* see above */
    ClientData clientData; /* user may use this for any purpose */
    int search_id;	   /* key to avoid searching this scope twice */
    Dictionary symbol_table;
    struct Scope_ *superscope;
    union {
	struct Procedure_ *proc;
	struct Function_ *func;
	struct Rule_ *rule;
	struct Entity_ *entity;
	struct Schema_ *schema;
	struct Express_ *express;
	struct Increment_ *incr;
	struct TypeHead_ *type;
    } u;
    Linked_List where; /* optional where clause */
};

typedef void *ClientData;

typedef struct Hash_Table_ {
	short	p;		/* Next bucket to be split	*/
	short	maxp;		/* upper bound on p during expansion	*/
	long	KeyCount;	/* current # keys	*/
	short	SegmentCount;	/* current # segments	*/
	short	MinLoadFactor;
	short	MaxLoadFactor;
	Segment	Directory[DIRECTORY_SIZE];
} *Hash_Table;
typedef struct Hash_Table_	*Dictionary;

struct Procedure_ {
	int pcount;	/* # of parameters */
	int tag_count;	/* # of different parameter tags */
	Linked_List parameters;
	Linked_List body;
	struct FullText text;
	int builtin;	/* builtin if true */
};

struct Function_ {
	int pcount;	/* # of parameters */
	int tag_count;	/* # of different parameter/return value tags */
	Linked_List parameters;
	Linked_List body;
	Type return_type;
	struct FullText text;
	int builtin;	/* builtin if true */
};

struct Rule_ {
	Linked_List parameters;
	Linked_List body;
	struct FullText text;
};

struct Entity_ {
	Linked_List	supertype_symbols; /* linked list of original symbols*/
				/* as generated by parser */
	Linked_List	supertypes;	/* linked list of supertypes (as entities) */
	Linked_List	subtypes;	/* simple list of subtypes */
			/* useful for simple lookups */
	Expression	subtype_expression;	/* DAG of subtypes, with complete */
			/* information including, OR, AND, and ONEOF */
	Linked_List	attributes;	/* explicit attributes */
	int		inheritance;	/* total number of attributes */
					/* inherited from supertypes */
	int		attribute_count;
	Linked_List	unique;	/* list of identifiers that are unique */
	Linked_List	instances;	/* hook for applications */
	int		mark;	/* usual hack - prevent traversing sub/super */
				/* graph twice */
	Boolean		abstract;/* is this an abstract supertype? */
	Type		type;	/* type pointing back to ourself */
				/* Useful to have when evaluating */
				/* expressions involving entities */
};

struct Schema_ {
	Linked_List rules;
	Linked_List reflist;
	Linked_List uselist;
	/* dictionarys into which are entered renames for each specific */
	/* object specified in a rename clause (even if it uses the same */
	/* name */
	Dictionary refdict;
	Dictionary usedict;
	/* lists of schemas that are fully ref/use'd */
	/* entries can be 0 if schemas weren't found during RENAMEresolve */
	Linked_List use_schemas;
	Linked_List ref_schemas;
};

struct Express_ {
	FILE *file;
	char *filename;
	char *basename;	/* name of file but without directory or .exp suffix */
};

/* this is an element in the optional Loop scope */
struct Increment_ {
    Expression init;
    Expression end;
    Expression increment;
};

struct TypeHead_ {
	Type head;			/* if we are a defined type */
					/* this is who we point to */
	struct TypeBody_ *body;		/* true type, ignoring defined types */
};

typedef struct Linked_List_ *Linked_List;

typedef struct Link_ {
    struct Link_ *	next;
    struct Link_ *	prev;
    Generic		data;
} *Link;

struct Linked_List_ {
    Link mark;
};
#endif

void
expprintToken(int tokenID)
{
    switch (tokenID) {
	case TOK_EQUAL: puts("TOK_EQUAL"); break;
	case TOK_GREATER_EQUAL: puts("TOK_GREATER_EQUAL"); break;
	case TOK_GREATER_THAN: puts("TOK_GREATER_THAN"); break;
	case TOK_IN: puts("TOK_IN"); break;
	case TOK_INST_EQUAL: puts("TOK_INST_EQUAL"); break;
	case TOK_INST_NOT_EQUAL: puts("TOK_INST_NOT_EQUAL"); break;
	case TOK_LESS_EQUAL: puts("TOK_LESS_EQUAL"); break;
	case TOK_LESS_THAN: puts("TOK_LESS_THAN"); break;
	case TOK_LIKE: puts("TOK_LIKE"); break;
	case TOK_NOT_EQUAL: puts("TOK_NOT_EQUAL"); break;
	case TOK_MINUS: puts("TOK_MINUS"); break;
	case TOK_PLUS: puts("TOK_PLUS"); break;
	case TOK_OR: puts("TOK_OR"); break;
	case TOK_XOR: puts("TOK_XOR"); break;
	case TOK_DIV: puts("TOK_DIV"); break;
	case TOK_MOD: puts("TOK_MOD"); break;
	case TOK_REAL_DIV: puts("TOK_REAL_DIV"); break;
	case TOK_TIMES: puts("TOK_TIMES"); break;
	case TOK_AND: puts("TOK_AND"); break;
	case TOK_ANDOR: puts("TOK_ANDOR"); break;
	case TOK_CONCAT_OP: puts("TOK_CONCAT_OP"); break;
	case TOK_EXP: puts("TOK_EXP"); break;
	case TOK_NOT: puts("TOK_NOT"); break;
	case TOK_DOT: puts("TOK_DOT"); break;
	case TOK_BACKSLASH: puts("TOK_BACKSLASH"); break;
	case TOK_LEFT_BRACKET: puts("TOK_LEFT_BRACKET"); break;
	case TOK_LEFT_PAREN: puts("TOK_LEFT_PAREN"); break;
	case TOK_RIGHT_PAREN: puts("TOK_RIGHT_PAREN"); break;
	case TOK_RIGHT_BRACKET: puts("TOK_RIGHT_BRACKET"); break;
	case TOK_COLON: puts("TOK_COLON"); break;
	case TOK_COMMA: puts("TOK_COMMA"); break;
	case TOK_AGGREGATE: puts("TOK_AGGREGATE"); break;
	case TOK_OF: puts("TOK_OF"); break;
	case TOK_IDENTIFIER: puts("TOK_IDENTIFIER"); break;
	case TOK_ALIAS: puts("TOK_ALIAS"); break;
	case TOK_FOR: puts("TOK_FOR"); break;
	case TOK_END_ALIAS: puts("TOK_END_ALIAS"); break;
	case TOK_ARRAY: puts("TOK_ARRAY"); break;
	case TOK_ASSIGNMENT: puts("TOK_ASSIGNMENT"); break;
	case TOK_BAG: puts("TOK_BAG"); break;
	case TOK_BOOLEAN: puts("TOK_BOOLEAN"); break;
	case TOK_INTEGER: puts("TOK_INTEGER"); break;
	case TOK_REAL: puts("TOK_REAL"); break;
	case TOK_NUMBER: puts("TOK_NUMBER"); break;
	case TOK_LOGICAL: puts("TOK_LOGICAL"); break;
	case TOK_BINARY: puts("TOK_BINARY"); break;
	case TOK_STRING: puts("TOK_STRING"); break;
	case TOK_BY: puts("TOK_BY"); break;
	case TOK_LEFT_CURL: puts("TOK_LEFT_CURL"); break;
	case TOK_RIGHT_CURL: puts("TOK_RIGHT_CURL"); break;
	case TOK_OTHERWISE: puts("TOK_OTHERWISE"); break;
	case TOK_CASE: puts("TOK_CASE"); break;
	case TOK_END_CASE: puts("TOK_END_CASE"); break;
	case TOK_BEGIN: puts("TOK_BEGIN"); break;
	case TOK_END: puts("TOK_END"); break;
	case TOK_PI: puts("TOK_PI"); break;
	case TOK_E: puts("TOK_E"); break;
	case TOK_CONSTANT: puts("TOK_CONSTANT"); break;
	case TOK_END_CONSTANT: puts("TOK_END_CONSTANT"); break;
	case TOK_DERIVE: puts("TOK_DERIVE"); break;
	case TOK_END_ENTITY: puts("TOK_END_ENTITY"); break;
	case TOK_ENTITY: puts("TOK_ENTITY"); break;
	case TOK_ENUMERATION: puts("TOK_ENUMERATION"); break;
	case TOK_ESCAPE: puts("TOK_ESCAPE"); break;
	case TOK_SELF: puts("TOK_SELF"); break;
	case TOK_OPTIONAL: puts("TOK_OPTIONAL"); break;
	case TOK_VAR: puts("TOK_VAR"); break;
	case TOK_END_FUNCTION: puts("TOK_END_FUNCTION"); break;
	case TOK_FUNCTION: puts("TOK_FUNCTION"); break;
	case TOK_BUILTIN_FUNCTION: puts("TOK_BUILTIN_FUNCTION"); break;
	case TOK_LIST: puts("TOK_LIST"); break;
	case TOK_SET: puts("TOK_SET"); break;
	case TOK_GENERIC: puts("TOK_GENERIC"); break;
	case TOK_QUESTION_MARK: puts("TOK_QUESTION_MARK"); break;
	case TOK_IF: puts("TOK_IF"); break;
	case TOK_THEN: puts("TOK_THEN"); break;
	case TOK_END_IF: puts("TOK_END_IF"); break;
	case TOK_ELSE: puts("TOK_ELSE"); break;
	case TOK_INCLUDE: puts("TOK_INCLUDE"); break;
	case TOK_STRING_LITERAL: puts("TOK_STRING_LITERAL"); break;
	case TOK_TO: puts("TOK_TO"); break;
	case TOK_AS: puts("TOK_AS"); break;
	case TOK_REFERENCE: puts("TOK_REFERENCE"); break;
	case TOK_FROM: puts("TOK_FROM"); break;
	case TOK_USE: puts("TOK_USE"); break;
	case TOK_INVERSE: puts("TOK_INVERSE"); break;
	case TOK_INTEGER_LITERAL: puts("TOK_INTEGER_LITERAL"); break;
	case TOK_REAL_LITERAL: puts("TOK_REAL_LITERAL"); break;
	case TOK_STRING_LITERAL_ENCODED: puts("TOK_STRING_LITERAL_ENCODED"); break;
	case TOK_LOGICAL_LITERAL: puts("TOK_LOGICAL_LITERAL"); break;
	case TOK_BINARY_LITERAL: puts("TOK_BINARY_LITERAL"); break;
	case TOK_LOCAL: puts("TOK_LOCAL"); break;
	case TOK_END_LOCAL: puts("TOK_END_LOCAL"); break;
	case TOK_ONEOF: puts("TOK_ONEOF"); break;
	case TOK_UNIQUE: puts("TOK_UNIQUE"); break;
	case TOK_FIXED: puts("TOK_FIXED"); break;
	case TOK_END_PROCEDURE: puts("TOK_END_PROCEDURE"); break;
	case TOK_PROCEDURE: puts("TOK_PROCEDURE"); break;
	case TOK_BUILTIN_PROCEDURE: puts("TOK_BUILTIN_PROCEDURE"); break;
	case TOK_QUERY: puts("TOK_QUERY"); break;
	case TOK_ALL_IN: puts("TOK_ALL_IN"); break;
	case TOK_SUCH_THAT: puts("TOK_SUCH_THAT"); break;
	case TOK_REPEAT: puts("TOK_REPEAT"); break;
	case TOK_END_REPEAT: puts("TOK_END_REPEAT"); break;
	case TOK_RETURN: puts("TOK_RETURN"); break;
	case TOK_END_RULE: puts("TOK_END_RULE"); break;
	case TOK_RULE: puts("TOK_RULE"); break;
	case TOK_END_SCHEMA: puts("TOK_END_SCHEMA"); break;
	case TOK_SCHEMA: puts("TOK_SCHEMA"); break;
	case TOK_SELECT: puts("TOK_SELECT"); break;
	case TOK_SEMICOLON: puts("TOK_SEMICOLON"); break;
	case TOK_SKIP: puts("TOK_SKIP"); break;
	case TOK_SUBTYPE: puts("TOK_SUBTYPE"); break;
	case TOK_ABSTRACT: puts("TOK_ABSTRACT"); break;
	case TOK_SUPERTYPE: puts("TOK_SUPERTYPE"); break;
	case TOK_END_TYPE: puts("TOK_END_TYPE"); break;
	case TOK_TYPE: puts("TOK_TYPE"); break;
	case TOK_UNTIL: puts("TOK_UNTIL"); break;
	case TOK_WHERE: puts("TOK_WHERE"); break;
	case TOK_WHILE: puts("TOK_WHILE"); break;
	default: printf("UNRECOGNIZED (%d)\n", tokenID);
    }
}
