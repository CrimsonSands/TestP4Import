/*
 * Copyright 1993, 2000 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

# include "jam.h"
# include "lists.h"
# include "parse.h"
# include "compile.h"
# include "rules.h"
# include "variable.h"
# include "regexp.h"
# include "headers.h"
# include "newstr.h"

/*
 * headers.c - handle #includes in source files
 *
 * Using regular expressions provided as the variable $(HDRSCAN), 
 * headers() searches a file for #include files and phonies up a
 * rule invocation:
 * 
 *	$(HDRRULE) <target> : <include files> ;
 *
 * External routines:
 *    headers() - scan a target for include files and call HDRRULE
 *
 * Internal routines:
 *    headers1() - using regexp, scan a file and build include LIST
 *
 * 04/13/94 (seiwald) - added shorthand L0 for null list pointer
 * 09/10/00 (seiwald) - replaced call to compile_rule with evaluate_rule,
 *		so that headers() doesn't have to mock up a parse structure
 *		just to invoke a rule.
 */

static LIST *headers1( LIST *l, const char *file, int rec, regexp *re[] );

/*
 * headers() - scan a target for include files and call HDRRULE
 */

# define MAXINC 10

void
headers( TARGET *t )
{
	LIST	*hdrscan;
	LIST	*hdrrule;
	LIST	*headlist = 0;
	LOL	lol;
	regexp	*re[ MAXINC ];
	int	rec = 0;

	if( !( hdrscan = var_get( "HDRSCAN" ) ) || 
	    !( hdrrule = var_get( "HDRRULE" ) ) )
	        return;

	if( DEBUG_HEADER )
	    printf( "header scan %s\n", t->name );

	/* Compile all regular expressions in HDRSCAN */

	while( rec < MAXINC && hdrscan )
	{
	    re[rec++] = regcomp( hdrscan->string );
	    hdrscan = list_next( hdrscan );
	}

	/* Doctor up call to HDRRULE rule */
	/* Call headers1() to get LIST of included files. */

	lol_init( &lol );
	lol_add( &lol, list_new( L0, t->name, 1 ) );
	lol_add( &lol, headers1( headlist, t->boundname, rec, re ) );

	if( lol_get( &lol, 1 ) )
	    list_free( evaluate_rule( hdrrule->string, &lol, L0 ) );

	/* Clean up */

	lol_free( &lol );

	while( rec )
	    free( (char *)re[--rec] );
}

/*
 * headers1() - using regexp, scan a file and build include LIST
 */

static LIST *
headers1( 
	LIST	*result,
	const char *file,
	int	rec,
	regexp	*re[] )
{
	FILE	*f;
	char	buf[ 1024 ];
	int	i;

	if( !( f = fopen( file, "r" ) ) )
	    return result;

	while( fgets( buf, sizeof( buf ), f ) )
	{
	    for( i = 0; i < rec; i++ )
		if( regexec( re[i], buf ) && re[i]->startp[1] )
	    {
		/* Copy and terminate extracted string. */

		char buf2[ MAXSYM ];
		int l = re[i]->endp[1] - re[i]->startp[1];
		memcpy( buf2, re[i]->startp[1], l );
		buf2[ l ] = 0;
		result = list_new( result, buf2, 0 );

		if( DEBUG_HEADER )
		    printf( "header found: %s\n", buf2 );
	    }
	}

	fclose( f );

	return result;
}
